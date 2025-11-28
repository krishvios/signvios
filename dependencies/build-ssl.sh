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

function build_config {
    export LOG="${SOURCE}/openssl-${CONFIG}.log"
    echo "Building OpenSSL for ${CONFIG}. Log: ${LOG}"

    export PREFIX=${TARGET}/${CONFIG}

    echo "  Configuring..."
    ./Configure ${CONFIG} -fembed-bitcode --prefix="$PREFIX" --openssldir=${PREFIX}/openssl >> "${LOG}" 2>&1
    check_status $? "configure"

    echo "  Building..."
    make -j8 >> "${LOG}" 2>&1
    check_status $? "make"

    echo "  Installing..."
    make install_sw >> "${LOG}" 2>&1
    check_status $? "make install"

    echo "  Cleaning..."
    make clean >> "${LOG}" 2>&1
    check_status $? "make clean"

    echo "  Build Complete."
}

function build_all {
    echo "Downloading OpenSSL..."
    curl -#o ${SOURCE}/openssl.tar.gz https://www.openssl.org/source/openssl-1.1.1h.tar.gz
    cd ${SOURCE}
    tar xzpf ${SOURCE}/openssl.tar.gz
    cd ${SOURCE}/openssl-*

    LIBSSL_SLICES=()
    LIBCRYPTO_SLICES=()

    for CONFIG in "${CONFIGS[@]}"; do
        export CONFIG=$CONFIG
        build_config
        LIBSSL_SLICES+=("$TARGET/$CONFIG/lib/libssl.a")
        LIBCRYPTO_SLICES+=("$TARGET/$CONFIG/lib/libcrypto.a")
    done

    lipo -create ${LIBSSL_SLICES[@]} -output "${OUTPUT}/lib/libssl.a"
    check_status $? "lipo (with libssl)"
    lipo -create ${LIBCRYPTO_SLICES[@]} -output "${OUTPUT}/lib/libcrypto.a"
    check_status $? "lipo (with libcrypto)"
    cp -R "${TARGET}/${CONFIGS[0]}/include/openssl" "${OUTPUT}/include"
    check_status $? "copying include directory"

    echo "Done."
}

OUTPUT=$(pwd)
echo "Building in ${SOURCE}"
echo "Installing in ${TARGET}"
echo "Copying libraries/headers to ${OUTPUT}"
CONFIGS=( ios-xcrun ios64-xcrun iossimulator-xcrun )
build_all
