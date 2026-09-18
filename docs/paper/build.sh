#!/bin/sh
# Rebuilds the paper.
#   1. experiments (optional, needs the natID SDK):
#        cmake -S experiments -B <build dir> && cmake --build <build dir> --config Release
#        TVPaperExperiments ../../Implementation data
#   2. figures:  python scripts/make_figures.py
#   3. PDF:      sh build.sh   (pdflatex + bibtex, copies the PDF to docs/)
set -e
cd "$(dirname "$0")"
name=tv-denoising
final="../TV Image Denoising - Paper.pdf"
mkdir -p build
pdflatex -interaction=nonstopmode -output-directory=build "$name.tex" > build/pass1.log || true
cp references.bib build/
(cd build && bibtex "$name" > bibtex.log || true)
# IEEEtran.bst turns page ranges into en dashes; keep plain hyphens
sed 's/--/-/g' "build/$name.bbl" > "build/$name.bbl.tmp" && mv "build/$name.bbl.tmp" "build/$name.bbl"
pdflatex -interaction=nonstopmode -output-directory=build "$name.tex" > build/pass2.log || true
pdflatex -interaction=nonstopmode -output-directory=build "$name.tex" > build/pass3.log || true
grep -E "^!|Overfull|undefined" "build/$name.log" || echo "no errors or overfull boxes"
cp "build/$name.pdf" "$final"
echo "wrote $final"
