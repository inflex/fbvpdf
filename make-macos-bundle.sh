#!/bin/sh

echo Making macOS bundle
set -x

export FBVPDF_BUILD=`git rev-list HEAD --count`
export PP="fbvpdf-R${FBVPDF_BUILD}-macos.zip"

mkdir tmp

D=fbvpdf.app
PL=$D/Info.plist
mkdir $D
if [ -d tmp ]; then
	rm -rf tmp
fi

if [ -f fbvpdfInstall.dmg ]; then
	rm fbvpdfInstall.dmg
fi

mkdir tmp
mkdir $D/Contents
mkdir $D/Contents/MacOS
mkdir $D/Contents/Resources
cp -rv resources/Frameworks $D/Contents/
cp -v Info.plist $D/Contents
cp -v build/release/mupdf-macos $D/Contents/MacOS/fbvpdf
cp -v icon-128.png $D/Contents/Resources/fbvpdf.icns


echo "Done"

#hdiutil create tmp/tmp.dmg -ov -volname "FlexBV PDF Viewer" -fs HFS+ -srcfolder fbvpdf.app
#hdiutil convert tmp/tmp.dmg -format UDZO -o fbvpdf.dmg

#zip -r ${PP} $D
#ls -la ${PP}
