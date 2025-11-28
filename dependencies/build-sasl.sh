#!/bin/bash

SCRIPT_DIR=`pwd`
SOURCE=`mktemp -d -t 'source'`
TARGET=`mktemp -d -t 'target'`

function check_status {
    local STATUS=$1
    local COMMAND=$2
    if [ "${STATUS}" != 0 ]; then
        echo "Problem encountered with ${COMMAND} - Please check ${LOG}"
        exit 1
    fi
}

function build_arch {
    export LOG="${SOURCE}/libsasl2-${ARCH}.log"
    echo "Building Cyrus SASL for ${ARCH}. Log: ${LOG}"

    if [ "${ARCH}" == "x86_64" ] || [ "${ARCH}" == "i386" ]; then
        export SDK="iphonesimulator"
    else
        export SDK="iphoneos"
    fi
    SDKROOT=$(xcrun --sdk $SDK --show-sdk-path)
    export PREFIX=${TARGET}/${ARCH}
    export CC=$(xcrun --sdk $SDK --find gcc)
    export CPP=$(xcrun --sdk $SDK --find gcc)" -E"
    export CXX=$(xcrun --sdk $SDK --find g++)
    export LD=$(xcrun --sdk $SDK --find ld)
    export LDFLAGS="-v"
    export CC=$(xcrun --sdk $SDK --find gcc)
    export CC_FOR_BUILD=$(xcrun --find gcc)
    export CFLAGS="-arch $ARCH -isysroot $SDKROOT -miphoneos-version-min=$SDKVERSION -fembed-bitcode"
    export PKG_CONFIG_PATH="$PKG_CONFIG_PATH":"$SDKROOT/usr/lib/pkgconfig"

    if [ "${ARCH}" == "x86_64" ] || [ "${ARCH}" == "i386" ]; then
        export CHOST=$ARCH-apple-darwin*
    else
        export CHOST=arm-apple-darwin*
    fi

    echo "  Configuring..."
    ./configure \
        --prefix="$PREFIX" \
        --host="$CHOST" \
        --enable-static \
        --with-openssl="${OUTPUT}" \
        --disable-macos-framework \
        --disable-shared >> "${LOG}" 2>&1
    check_status $? "configure"

    echo "  Building..."
    make LIBS="-lresolv -lcrypto -lssl" >> "${LOG}" 2>&1 # FIX: something is broken in the config script so we manually set the libraries to link with
    check_status $? "make"

    echo "  Installing..."
    make install >> "${LOG}" 2>&1
    check_status $? "make install"

    echo "  Cleaning..."
    make clean >> "${LOG}" 2>&1
    check_status $? "make clean"
    rm config.cache
}

function build_all {
    echo "Downloading Cyrus SASL..."
    #curl -#o ${SOURCE}/cyrus-sasl.tar.gz ftp://ftp.cyrusimap.org/cyrus-sasl/cyrus-sasl-2.1.27.tar.gz
    # Cyrus's FTP is regularly down...
    curl -#Lo ${SOURCE}/cyrus-sasl.tar.gz https://github.com/cyrusimap/cyrus-sasl/releases/download/cyrus-sasl-2.1.27/cyrus-sasl-2.1.27.tar.gz
    cd ${SOURCE}
    tar xzpf ${SOURCE}/cyrus-sasl.tar.gz
    cd ${SOURCE}/cyrus-sasl-*

    SLICES=()

    for ARCH in "${ARCHS[@]}"; do
        export ARCH=$ARCH
        build_arch
        SLICES+=("$TARGET/$ARCH/lib/libsasl2.a")
    done

    lipo -create ${SLICES[@]} -output "${OUTPUT}/lib/libsasl2.a"
    check_status $? "lipo"
    cp -R "${TARGET}/${ARCHS[0]}/include/sasl" "${OUTPUT}/include" # TODO
    check_status $? "copying include directory"
    
    echo "Done."
}

echo "Note - Make sure OpenSSL has been built (otherwise run build-ssl.sh first)"
OUTPUT=$(pwd)
echo "Building in ${SOURCE}"
echo "Installing in ${TARGET}"
echo "Copying libraries/headers to ${OUTPUT}"
SDKVERSION="10.0"
ARCHS=( armv7 arm64 x86_64 )
build_all
