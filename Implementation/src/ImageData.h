#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <random>
#include <gui/Image.h>
#include <mu/IAppSettings.h>
#include <mu/mu.h>

// ============================================================
// ImageData: grayscale float image in [0,1]
// SRP: owns the raw float pixel buffer and its dimensions.
//      Provides load/save and conversion helpers only.
//      No solver, no GUI logic.
// ============================================================
class ImageData
{
public:
    int width  = 0;
    int height = 0;
    std::vector<float> pixels; // row-major, [0,1]

    ImageData() = default;
    ImageData(int w, int h) : width(w), height(h), pixels(w * h, 0.0f) {}

    int size() const { return width * height; }

    float& at(int row, int col)       { return pixels[row * width + col]; }
    float  at(int row, int col) const { return pixels[row * width + col]; }

    // ------------------------------------------------------------------
    // Load grayscale from a file path (cross-platform, uses NatID gui::Image)
    // ------------------------------------------------------------------
    bool loadFromFile(const std::string& filePath)
    {
        gui::Image img;
        if (!img.load(filePath.c_str()))
            return false;

        gui::Image::Data d;
        if (!img.getData(d))
            return false;

        width  = static_cast<int>(d.width);
        height = static_cast<int>(d.height);
        pixels.resize(width * height);

        int bpp = d.bitsPerPixel / 8;

        for (int i = 0; i < height; ++i)
        {
            for (int j = 0; j < width; ++j)
            {
                int idx = (i * width + j) * bpp;
                float r = 0.0f, g = 0.0f, b = 0.0f;

                if (d.format == gui::Image::Format::RGBA || d.format == gui::Image::Format::RGB)
                {
                    r = d.data[idx]     / 255.0f;
                    g = d.data[idx + 1] / 255.0f;
                    b = d.data[idx + 2] / 255.0f;
                }
                else
                {
                    r = g = b = d.data[idx] / 255.0f;
                }

                pixels[i * width + j] = 0.299f * r + 0.587f * g + 0.114f * b;
            }
        }
        return true;
    }

    // ------------------------------------------------------------------
    // Write as an uncompressed (stored-block deflate) PNG.
    //
    // WHY PNG instead of BMP:
    //   The natID GTK distribution ships libpng16.dll but NO BMP pixbuf
    //   loader plugin, so gui::Image::load() silently fails on any BMP
    //   file.  PNG is loaded natively by both GTK/libpng on Windows and
    //   CoreGraphics on macOS.
    //
    // WHY uncompressed (BTYPE=00 stored) deflate:
    //   Pure std C++ only, no external zlib required.  The deflate
    //   "stored" block type passes raw bytes verbatim, still wrapped in a
    //   valid zlib header + Adler-32 trailer so libpng accepts it.
    //
    // Uses only <cstdint>, <cstdio>, <vector>, no platform-specific code.
    // ------------------------------------------------------------------
    bool saveToPng(const std::string& filePath) const
    {
        if (width <= 0 || height <= 0) return false;

        // ---- CRC-32 (IEEE 802.3 reflected polynomial) ----
        static uint32_t crcT[256];
        static bool     crcReady = false;
        if (!crcReady)
        {
            for (uint32_t n = 0; n < 256; ++n)
            {
                uint32_t c = n;
                for (int k = 0; k < 8; ++k)
                    c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                crcT[n] = c;
            }
            crcReady = true;
        }

        // Compute CRC-32 over a byte range (static crcT is reachable without capture)
        auto crc32 = [](const uint8_t* d, size_t n) -> uint32_t {
            uint32_t c = 0xFFFFFFFFu;
            for (size_t i = 0; i < n; ++i)
                c = crcT[(c ^ d[i]) & 0xFFu] ^ (c >> 8);
            return c ^ 0xFFFFFFFFu;
        };

        // Append a big-endian uint32 to a vector
        auto push32 = [](std::vector<uint8_t>& v, uint32_t x) {
            v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
            v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
            v.push_back(static_cast<uint8_t>((x >>  8) & 0xFF));
            v.push_back(static_cast<uint8_t>( x        & 0xFF));
        };

        // Write a PNG chunk: length(4) + type(4) + data(n) + crc32(type+data)(4)
        auto pushChunk = [&](std::vector<uint8_t>& out,
                             const char* type,
                             const uint8_t* data, uint32_t len)
        {
            push32(out, len);
            const size_t crcOff = out.size();
            out.push_back(static_cast<uint8_t>(type[0]));
            out.push_back(static_cast<uint8_t>(type[1]));
            out.push_back(static_cast<uint8_t>(type[2]));
            out.push_back(static_cast<uint8_t>(type[3]));
            if (len > 0) out.insert(out.end(), data, data + len);
            push32(out, crc32(out.data() + crcOff, 4u + len));
        };

        // ---- Build raw PNG scanline data: [filter=0][gray...] per row ----
        const int rowStride = 1 + width;
        std::vector<uint8_t> raw(static_cast<size_t>(rowStride) * height);
        uint32_t a1 = 1u, a2 = 0u; // Adler-32 accumulators
        for (int y = 0; y < height; ++y)
        {
            uint8_t* row = raw.data() + y * rowStride;
            row[0] = 0u; // filter method None
            a1 = (a1 + 0u) % 65521u; a2 = (a2 + a1) % 65521u;
            for (int x = 0; x < width; ++x)
            {
                float v = pixels[y * width + x];
                if (v < 0.f) v = 0.f;
                if (v > 1.f) v = 1.f;
                const uint8_t b = static_cast<uint8_t>(v * 255.f + 0.5f);
                row[1 + x] = b;
                a1 = (a1 + b) % 65521u;
                a2 = (a2 + a1) % 65521u;
            }
        }

        // ---- Build zlib stream: header + stored deflate blocks + Adler-32 ----
        // CMF=0x78 (deflate, 32 K window), FLG=0x01 → (0x7801 % 31 == 0) ✓
        std::vector<uint8_t> zlib;
        zlib.reserve(raw.size() + 10 + (raw.size() / 65535 + 1) * 5);
        zlib.push_back(0x78u);
        zlib.push_back(0x01u);

        const uint8_t* src  = raw.data();
        size_t          left = raw.size();
        while (left > 0u)
        {
            const uint16_t blen = (left > 65535u) ? 65535u : static_cast<uint16_t>(left);
            const bool      last = (static_cast<size_t>(blen) == left);
            zlib.push_back(last ? 0x01u : 0x00u); // BFINAL | (BTYPE=00)
            zlib.push_back( blen        & 0xFFu);
            zlib.push_back((blen >> 8)  & 0xFFu);
            const uint16_t nlen = static_cast<uint16_t>(~blen);
            zlib.push_back( nlen        & 0xFFu);
            zlib.push_back((nlen >> 8)  & 0xFFu);
            zlib.insert(zlib.end(), src, src + blen);
            src  += blen;
            left -= blen;
        }
        push32(zlib, (a2 << 16) | a1); // Adler-32 big-endian

        // ---- Assemble PNG file ----
        std::vector<uint8_t> png;
        png.reserve(zlib.size() + 100u);

        // PNG signature
        static const uint8_t kSig[8] = {137,80,78,71,13,10,26,10};
        png.insert(png.end(), kSig, kSig + 8);

        // IHDR (width, height, bit_depth=8, color_type=0 grayscale,
        //        compression=0, filter=0, interlace=0)
        uint8_t ihdr[13] = {};
        ihdr[0] = static_cast<uint8_t>((width  >> 24) & 0xFF);
        ihdr[1] = static_cast<uint8_t>((width  >> 16) & 0xFF);
        ihdr[2] = static_cast<uint8_t>((width  >>  8) & 0xFF);
        ihdr[3] = static_cast<uint8_t>( width         & 0xFF);
        ihdr[4] = static_cast<uint8_t>((height >> 24) & 0xFF);
        ihdr[5] = static_cast<uint8_t>((height >> 16) & 0xFF);
        ihdr[6] = static_cast<uint8_t>((height >>  8) & 0xFF);
        ihdr[7] = static_cast<uint8_t>( height        & 0xFF);
        ihdr[8] = 8u; // bit depth
        ihdr[9] = 0u; // color type 0 = grayscale
        pushChunk(png, "IHDR", ihdr, 13u);

        // IDAT
        pushChunk(png, "IDAT", zlib.data(), static_cast<uint32_t>(zlib.size()));

        // IEND
        pushChunk(png, "IEND", nullptr, 0u);

        // Write to disk
        FILE* f = std::fopen(filePath.c_str(), "wb");
        if (!f) return false;
        const bool ok = (std::fwrite(png.data(), 1u, png.size(), f) == png.size());
        std::fclose(f);
        return ok;
    }

    // ------------------------------------------------------------------
    // Convert to gui::Image via a uniquely-named temp PNG.
    // Unique filenames prevent gui::Image filename-based caching.
    //
    // Uses mu::getAppSettings()->getTmpFolder(), the NatID cross-
    // platform temp-directory accessor, instead of getenv / #ifdef.
    // Forward slash works on all platforms (Windows C-runtime accepts it).
    // ------------------------------------------------------------------
    bool toImage(gui::Image& img, const char* baseTag = "tvden") const
    {
        static int seq = 0;
        ++seq;

        const td::String& tmpDir = mu::getAppSettings()->getTmpFolder();

        char fname[64];
        std::snprintf(fname, sizeof(fname), "%s_%04d.png", baseTag, seq);

        std::string path = tmpDir.c_str();
        path += '/';   // forward slash is valid on Windows, macOS, and Linux
        path += fname;

        if (!saveToPng(path)) return false;
        return img.load(path.c_str());
    }

    // ------------------------------------------------------------------
    // Add Gaussian noise in place (sigma in [0,1]), clipped to [0,1].
    //
    // The same seed gives the same noise on every platform: mt19937 is
    // fully specified, and Box-Muller is done by hand because
    // std::normal_distribution differs between standard libraries.
    // ------------------------------------------------------------------
    void addGaussianNoise(float sigma, unsigned seed)
    {
        std::mt19937 rng(seed);
        auto uniform = [&rng]() { return (static_cast<double>(rng()) + 0.5) / 4294967296.0; };

        for (float& p : pixels)
        {
            const double z = std::sqrt(-2.0 * std::log(uniform())) * std::cos(6.283185307179586 * uniform());
            p += sigma * static_cast<float>(z);
            if (p < 0.f) p = 0.f;
            if (p > 1.f) p = 1.f;
        }
    }
};
