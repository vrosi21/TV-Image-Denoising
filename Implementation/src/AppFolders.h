#pragma once
#include <mu/mu.h>
#include <mu/IAppSettings.h>
#include <fo/FileOperations.h>
#include <syst/Shell.h>
#include <td/Date.h>
#include <td/Time.h>
#include <td/String.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>
#include <system_error>

// ============================================================
// AppFolders: where the application reads and writes files.
//
//   <Documents>/TV Image Denoising/Images                 user images (PNG/JPEG)
//   <Documents>/TV Image Denoising/Exports/<date_time_…>  one folder per result
//
// <Documents> is <home>/Documents when it exists, otherwise the home
// folder.  The home folder comes from natID (mu::IAppSettings), paths
// use natID's fo::fs (std::filesystem) and folders are opened with
// natID's syst::Shell, so the code is the same on Windows, macOS and
// Linux.  No file dialogs are used: the natID file dialogs corrupt the
// heap on Windows (SDK issue).
// ============================================================
namespace appfs
{

constexpr const char* kAppFolder = "TV Image Denoising";

// ---- UTF-8 conversions ----------------------------------------
inline fo::fs::path toPath(const char* utf8)
{
    return fo::fs::path(std::u8string(reinterpret_cast<const char8_t*>(utf8)));
}

inline std::string toUtf8(const fo::fs::path& p)
{
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

// ---- folders ------------------------------------------------------
inline fo::fs::path baseFolder()
{
    std::error_code ec;
    const fo::fs::path home = toPath(mu::getAppSettings()->getHomeFolder().c_str());
    const fo::fs::path docs = home / "Documents";
    const fo::fs::path root = (fo::fs::is_directory(docs, ec) ? docs : home) / kAppFolder;
    fo::fs::create_directories(root, ec);
    return root;
}

inline fo::fs::path subFolder(const char* name)
{
    std::error_code ec;
    const fo::fs::path p = baseFolder() / name;
    fo::fs::create_directories(p, ec);
    return p;
}

inline fo::fs::path imagesFolder()  { return subFolder("Images"); }
inline fo::fs::path exportsFolder() { return subFolder("Exports"); }

// ---- names ----------------------------------------------------------
// "2026-09-17_14-03-22"
inline std::string timestamp()
{
    td::Date d(true);
    td::Time t(true);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d_%02d-%02d-%02d",
                  d.getYear(), d.getMonth(), d.getDay(), t.getHour(), t.getMinute(), t.getSecond());
    return buf;
}

// Replaces characters that are not portable in file names.
inline std::string safeName(std::string s)
{
    for (char& c : s)
        if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            c = '_';
    return s;
}

// New folder <Exports>/<timestamp>_<label>; a suffix keeps it unique.
inline fo::fs::path createExportFolder(const std::string& label)
{
    std::error_code ec;
    const std::string base = timestamp() + "_" + safeName(label);
    fo::fs::path dir = exportsFolder() / toPath(base.c_str());
    for (int i = 2; fo::fs::exists(dir, ec); ++i)
        dir = exportsFolder() / toPath((base + "_" + std::to_string(i)).c_str());
    fo::fs::create_directories(dir, ec);
    return dir;
}

// <folder>/<name><ext>, or <name>_2<ext>, <name>_3<ext>, ... if taken.
inline fo::fs::path uniqueFile(const fo::fs::path& folder, const std::string& name, const char* ext)
{
    std::error_code ec;
    fo::fs::path p = folder / toPath((name + ext).c_str());
    for (int i = 2; fo::fs::exists(p, ec); ++i)
        p = folder / toPath((name + "_" + std::to_string(i) + ext).c_str());
    return p;
}

// PNG / JPEG files in the Images folder, sorted by name.
inline std::vector<fo::fs::path> listImages()
{
    std::vector<fo::fs::path> out;
    std::error_code ec;
    for (fo::fs::directory_iterator it(imagesFolder(), ec), end; !ec && it != end; it.increment(ec))
    {
        if (!it->is_regular_file(ec)) continue;
        std::string ext = toUtf8(it->path().extension());
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return (char) std::tolower(ch); });
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") out.push_back(it->path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

// ---- opening a folder in the system file manager ---------------------
// file:// URL with percent-encoding; works for Explorer, Finder and xdg-open.
inline td::String fileUrl(const fo::fs::path& p)
{
    const std::u8string u = fo::fs::absolute(p).generic_u8string();
    std::string url = "file://";
    if (u.empty() || u[0] != u8'/') url += '/';
    static const char* hex = "0123456789ABCDEF";
    for (char8_t ch : u)
    {
        const unsigned char c = (unsigned char) ch;
        if (std::isalnum(c) || c == '/' || c == ':' || c == '-' || c == '_' || c == '.' || c == '~')
            url += (char) c;
        else
        {
            url += '%';
            url += hex[c >> 4];
            url += hex[c & 15];
        }
    }
    return td::String(url.c_str());
}

inline bool openFolder(const fo::fs::path& p)
{
    std::error_code ec;
    fo::fs::create_directories(p, ec);
    return syst::Shell::openUrl(fileUrl(p));
}

} // namespace appfs
