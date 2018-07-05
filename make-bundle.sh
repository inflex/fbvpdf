#!/bin/sh

echo Making macOS bundle
set -x

D=fbvpdf.app
PL=$D/Info.plist
mkdir $D
mkdir $D/Contents
mkdir $D/Contents/MacOS
mkdir $D/Contents/Resources
cp -rv resources/Frameworks $D/Contents/
cp -v Info.plist $D/Contents
cp -v build/flexbv-macos $D/Contents/MacOS/fbvpdf
cp -v fbvpdfIcon.png $D/Contents/Resources/fbvpdf.icns


