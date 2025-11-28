#!/bin/bash

SOURCE=`mktemp -d -t 'source'`
TARGET=`mktemp -d -t 'target'`

mkdir -p ${TARGET}/bin
export PATH=${TARGET}/bin:$PATH

function check_status {
    local STATUS=$1
    local COMMAND=$2
    if [ "${STATUS}" != 0 ]; then
        echo "Problem encountered with ${COMMAND} - Please check ${LOG}"
        exit 1
    fi
}

function compile_build_tools {
    export LOG="${SOURCE}/builddeps.log"
    echo "Building dependencies to compile FFmpeg. Log: ${LOG}"

    # gas-preprocessor
    echo "  Building gas-preprocessor..."
    curl -sSo ${TARGET}/bin/gas-preprocessor.pl https://github.com/FFmpeg/gas-preprocessor/raw/master/gas-preprocessor.pl
    chmod +x ${TARGET}/bin/gas-preprocessor.pl

    # Build yasm
    echo "  Building yasm..."
    cd ${SOURCE}
    curl -sSo ${SOURCE}/yasm.tar.gz http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz
    tar xzpf ${SOURCE}/yasm.tar.gz
    cd yasm-*
    ./configure --prefix=${TARGET} >> "${LOG}" 2>&1; check_status $? "configure yasm"
    make -j8 >> "${LOG}" 2>&1; check_status $? "build yasm"
    make install >> "${LOG}" 2>&1; check_status $? "install yasm"
}

function build_arch {
    export LOG="${SOURCE}/ffmpeg-${ARCH}.log"
    echo "Building FFmpeg for ${ARCH}. Log: ${LOG}"
    cd ${SOURCE}/ffmpeg-*
    export SDKVERSION="10.0"

    if [ "${ARCH}" == "x86_64" ] || [ "${ARCH}" == "i386" ]; then
        export SDK="iphonesimulator"
        if [ "${PLATFORM_TARGET}" == "iOSMac" ]; then
            export SDK="macosx"
            export SDKVERSION="13.0"
            # Also need to patch an ffmpeg source file with a necessary change for Catalyst
            # Reference: https://github.com/kewlbear/FFmpeg-iOS-build-script/pull/147
            sed -i '' 's/#if TARGET_OS_IPHONE/#if TARGET_ABI_USES_IOS_VALUES/' libavcodec/videotoolbox.c
            sed -i '' 's/#else/#elif TARGET_OS_OSX/' libavcodec/videotoolbox.c
        fi
    else
        export SDK="iphoneos"
    fi
    PREFIX="${TARGET}/${ARCH}"
    if [ "${PLATFORM_TARGET}" == "iOSMac" ]; then
        CFLAGS="-arch $ARCH -mios-version-min=$SDKVERSION -target x86_64-apple-ios-macabi -fembed-bitcode"
    else
        CFLAGS="-arch $ARCH -mios-version-min=$SDKVERSION -fembed-bitcode"
    fi
    CC=$(xcrun -f --sdk $SDK clang)
    SYSROOT=$(xcrun --sdk $SDK --show-sdk-path)
    CXXFLAGS="$CFLAGS"
    LDFLAGS="$CFLAGS"
    export PKG_CONFIG_PATH="$PKG_CONFIG_PATH":"$SDKROOT/usr/lib/pkgconfig"
    if [ "${ARCH}" == "i386" ]; then
        # Fix some errors about too many registers being used in inline assembly.
        # Building FFmpeg for i386 is just to silence some errors in interface builder, efficiency is not a concern.
        ARCH_FLAGS="--disable-inline-asm"
    fi

    echo "  Configuring..."
    ./configure \
        --prefix="$PREFIX" \
        --target-os=darwin \
        --arch="$ARCH" \
        --cc="$CC -arch ${ARCH}" \
        --host-cc="$CC -arch ${ARCH}" \
        --sysroot="$SYSROOT" \
        --extra-cflags="$CFLAGS" \
        --extra-ldflags="$CFLAGS" \
        --enable-cross-compile \
        $ARCH_FLAGS \
        $FFMPEG_FLAGS >> "${LOG}" 2>&1
    check_status $? "configure"

    echo "  Building..."
    make -j8 >> "${LOG}" 2>&1
    check_status $? "make"

    echo "  Installing..."
    make install >> "${LOG}" 2>&1
    check_status $? "make install"

    echo "  Cleaning..."
    make clean >> "${LOG}" 2>&1
    check_status $? "make clean"
}

function build_all {
    compile_build_tools

    echo "Downloading FFmpeg..."
    curl -#o ${SOURCE}/ffmpeg.tar.gz https://www.ffmpeg.org/releases/ffmpeg-4.3.1.tar.gz
    cd ${SOURCE}
    tar xzpf ${SOURCE}/ffmpeg.tar.gz

    LIBAVCODEC_SLICES=()
    LIBAVFORMAT_SLICES=()
    LIBAVUTIL_SLICES=()
    LIBSWSCALE_SLICES=()

    for PLATFORM_TARGET in "${PLATFORM_TARGETS[@]}"; do
        export PLATFORM_TARGET=$PLATFORM_TARGET
        if [ "${PLATFORM_TARGET}" == "iOS" ]; then
            export ARCHS=( armv7 arm64 x86_64 )
        else
            export ARCHS=( x86_64 )
        fi
        for ARCH in "${ARCHS[@]}"; do
            export ARCH=$ARCH
            build_arch
            if [ "${PLATFORM_TARGET}" == "iOS" ]; then
                LIBAVCODEC_SLICES+=("$TARGET/$ARCH/lib/libavcodec.a")
                LIBAVFORMAT_SLICES+=("$TARGET/$ARCH/lib/libavformat.a")
                LIBAVUTIL_SLICES+=("$TARGET/$ARCH/lib/libavutil.a")
                LIBSWSCALE_SLICES+=("$TARGET/$ARCH/lib/libswscale.a")
            else
                cp "$TARGET/$ARCH/lib/libavcodec.a" "${OUTPUT}/lib/libavcodec-catalyst.a"
                cp "$TARGET/$ARCH/lib/libavformat.a" "${OUTPUT}/lib/libavformat-catalyst.a"
                cp "$TARGET/$ARCH/lib/libavutil.a" "${OUTPUT}/lib/libavutil-catalyst.a"
                cp "$TARGET/$ARCH/lib/libswscale.a" "${OUTPUT}/lib/libswscale-catalyst.a"
            fi
        done
    done

    lipo -create ${LIBAVCODEC_SLICES[@]} -output "${OUTPUT}/lib/libavcodec.a"
    check_status $? "lipo (with libavcodec)"

    lipo -create ${LIBAVFORMAT_SLICES[@]} -output "${OUTPUT}/lib/libavformat.a"
    check_status $? "lipo (with libavformat)"

    lipo -create ${LIBAVUTIL_SLICES[@]} -output "${OUTPUT}/lib/libavutil.a"
    check_status $? "lipo (with libavutil)"

    lipo -create ${LIBSWSCALE_SLICES[@]} -output "${OUTPUT}/lib/libswscale.a"
    check_status $? "lipo (with libswscale)"

    cp -R ${TARGET}/${ARCHS[0]}/include/* "${OUTPUT}/include"
    check_status $? "copying include directory"

    echo "Done."
}

# mjpeg encoder/decoded needed to fix linker errors in rtp muxer.
FFMPEG_FLAGS="\
 --disable-swscale-alpha \
 --disable-all \
 --disable-doc \
 --enable-avutil \
 --enable-avcodec \
 --enable-avformat \
 --enable-swscale \
 --disable-everything \
 --enable-encoder=h263 \
 --enable-encoder=mjpeg \
 --enable-decoder=mjpeg \
 --enable-muxer=rtp \
 --enable-pic \
 --enable-lto"

OUTPUT=$(pwd)
echo "Building in ${SOURCE}"
echo "Installing in ${TARGET}"
echo "Copying libraries/headers to ${OUTPUT}"
PLATFORM_TARGETS=( iOSMac iOS )
build_all
