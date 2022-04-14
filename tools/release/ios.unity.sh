#!/bin/bash
#
# Creates a new ios release from the current branch
#

source ./common.sh
export SOURCE_PACKAGE="${BASEDIR}/../../prebuilt/bundle-apple-xcframework-ios"
export COCOAPODS_DIRECTORY="${BASEDIR}/../../prebuilt/bundle-apple-cocoapods-ios"

create_package() {
    local PACKAGE_NAME="ffmpeg-kit-ios-$1"
    local PACKAGE_VERSION="$2"
    local PACKAGE_DESCRIPTION="$3"

    local CURRENT_PACKAGE="${COCOAPODS_DIRECTORY}/${PACKAGE_NAME}"
    rm -rf "${CURRENT_PACKAGE}"
    mkdir -p "${CURRENT_PACKAGE}" || exit 1

    cp -R "${SOURCE_PACKAGE}"/* "${CURRENT_PACKAGE}" || exit 1
    cd "${CURRENT_PACKAGE}" || exit 1
    zip -r -y "../ffmpeg-kit-$1-${PACKAGE_VERSION}-ios-xcframework.zip" * || exit 1

    # COPY PODSPEC AS THE LAST ITEM
    cp "${BASEDIR}"/apple/ffmpeg-kit-ios-min.podspec "${CURRENT_PACKAGE}"/"${PACKAGE_NAME}".podspec || exit 1
    sed -i '' "s/VERSION/${PACKAGE_VERSION}/g" "${CURRENT_PACKAGE}"/"${PACKAGE_NAME}".podspec || exit 1
    sed -i '' "s/DESCRIPTION/${PACKAGE_DESCRIPTION}/g" "${CURRENT_PACKAGE}"/"${PACKAGE_NAME}".podspec || exit 1
    sed -i '' "s/\.framework/\.xcframework/g" "${CURRENT_PACKAGE}"/"${PACKAGE_NAME}".podspec || exit 1
    sed -i '' "s/-framework/-xcframework/g" "${CURRENT_PACKAGE}"/"${PACKAGE_NAME}".podspec || exit 1
    sed -i '' "s/ios\.xcframeworks/ios\.frameworks/g" "${CURRENT_PACKAGE}"/"${PACKAGE_NAME}".podspec || exit 1
    sed -i '' "s/10/12\.1/g" "${CURRENT_PACKAGE}"/"${PACKAGE_NAME}".podspec || exit 1
    sed -i '' "s/ffmpegkit\.xcframework\/LICENSE/ffmpegkit\.xcframework\/ios-arm64\/ffmpegkit\.framework\/LICENSE/g" "${CURRENT_PACKAGE}"/"${PACKAGE_NAME}".podspec || exit 1
}

if [[ $# -ne 1 ]];
then
    echo "Usage: ios.sh <version name>"
    exit 1
fi

# CREATE COCOAPODS DIRECTORY
rm -rf "${COCOAPODS_DIRECTORY}"
mkdir -p "${COCOAPODS_DIRECTORY}" || exit 1

cd "${BASEDIR}/../.." || exit 1
./ios.sh ${IOS_MAIN_OPTIONS} --enable-openh264 --enable-libvpx --enable-libaom --enable-opus --enable-libvorbis --enable-libtheora --enable-libwebp  --enable-openssl || exit 1
create_package "unity" "$1" "Ffmpeg for Unity" || exit 1
