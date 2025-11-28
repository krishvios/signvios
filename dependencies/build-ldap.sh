#!/bin/bash

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
    export LOG="${SOURCE}/libldap-${ARCH}.log"
    echo "Building OpenLDAP for ${ARCH}. Log: ${LOG}"
    cd ${SOURCE}/openldap-*

    if [ "${ARCH}" == "x86_64" ] || [ "${ARCH}" == "i386" ]; then
        export SDK="iphonesimulator"
    else
        export SDK="iphoneos"
    fi
    export SDKROOT=$(xcrun --sdk $SDK --show-sdk-path)
    export PREFIX=${TARGET}/${ARCH}
    export CC=$(xcrun --sdk $SDK --find gcc)
    export CPP=$(xcrun --sdk $SDK --find gcc)" -E"
    export CXX=$(xcrun --sdk $SDK --find g++)
    export LD=$(xcrun --sdk $SDK --find ld)
    export CC=$(xcrun --sdk $SDK --find gcc)
    export CFLAGS="-arch $ARCH -isysroot $SDKROOT -miphoneos-version-min=$SDKVERSION -I$OUTPUT/include -fembed-bitcode -Wno-implicit-function-declaration"
    export CPPFLAGS="-arch $ARCH -isysroot $SDKROOT -miphoneos-version-min=$SDKVERSION -I$OUTPUT/include -fembed-bitcode"
    export CXXFLAGS="-arch $ARCH -isysroot $SDKROOT -I$OUTPUT/include -fembed-bitcode"
    export LDFLAGS="-arch $ARCH -isysroot $SDKROOT -L$OUTPUT/lib"
    export PKG_CONFIG_PATH="$PKG_CONFIG_PATH":"$SDKROOT/usr/lib/pkgconfig"
    export LIBS="-lssl -lcrypto"

    # Things will fail to compile due to missing symbol _lutil_mcmp if this is not defined before configuring.
    export ac_cv_func_memcmp_working="yes"

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
        --disable-shared \
        --with-cyrus-sasl \
        --disable-slapd \
        --with-yielding_select=yes \
        --with-tls="openssl" >> "${LOG}" 2>&1
    check_status $? "configure"

    echo "  Building..."
    make -j8 >> "${LOG}" 2>&1
    check_status $? "make lib"

    echo "  Installing..."
    make install >> "${LOG}" 2>&1
    check_status $? "make install"

    echo "  Cleaning..."
    make clean >> "${LOG}" 2>&1
    check_status $? "make clean"
}

function build_all {
    echo "Downloading OpenLDAP..."
    curl -#o ${SOURCE}/openldap.tar.gz ftp://ftp.openldap.org/pub/OpenLDAP/openldap-release/openldap-2.4.56.tgz
    cd ${SOURCE}
    tar xzpf ${SOURCE}/openldap.tar.gz

    LDAP_SLICES=()
    LBER_SLICES=()

    for ARCH in "${ARCHS[@]}"; do
        export ARCH=$ARCH
        build_arch
        LDAP_SLICES+=("$TARGET/$ARCH/lib/libldap.a")
        LBER_SLICES+=("$TARGET/$ARCH/lib/liblber.a")
    done

    lipo -create ${LDAP_SLICES[@]} -output "${OUTPUT}/lib/libldap.a"
    check_status $? "lipo (with ldap)"
    lipo -create ${LBER_SLICES[@]} -output "${OUTPUT}/lib/liblber.a"
    check_status $? "lipo (with lber)"
    cp -R ${TARGET}/${ARCHS[0]}/include/* "${OUTPUT}/include"
    check_status $? "copying include directory"

    echo "Done."
}

echo "Note - Make sure Cyrus SASL and OpenSSL has been built (otherwise run build-ssl.sh and build-sasl.sh first)"
OUTPUT=$(pwd)
echo "Building in ${SOURCE}"
echo "Installing in ${TARGET}"
echo "Copying libraries/headers to ${OUTPUT}"
SDKVERSION="10.0"
ARCHS=( armv7 arm64 x86_64 )
build_all
