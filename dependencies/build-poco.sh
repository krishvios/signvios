#!/bin/bash

SOURCE=`mktemp -d -t 'source'`
TARGET=`mktemp -d -t 'target'`
OUTPUT=$(pwd)

function check_status {
	local STATUS=$1
	local COMMAND=$2
	if [ "${STATUS}" != 0 ]; then
		echo "Problem encountered with ${COMMAND} - Please check ${LOG}. Printing last 20 lines"
		tail -n 20 ${LOG}
		exit 1
	fi
}

function build_platform {
  export PREFIX=${TARGET}/${PLATFORM}
  export LOG="${SOURCE}/poco-${PLATFORM}.log"

	echo "Building POCO for ${PLATFORM}. Log: ${LOG}"

	# Generate an Xcode project to build for iOS
	echo "  Configuring POCO..."
	mkdir -p cmake-build
	cd cmake-build

	cmake .. \
		-G Xcode \
		-DPLATFORM=${PLATFORM} \
		-DCMAKE_INSTALL_PREFIX=${PREFIX} \
		-DCMAKE_TOOLCHAIN_FILE=${OUTPUT}/ios.toolchain.cmake \
		$POCO_FLAGS >> "${LOG}" 2>&1
	check_status $? "cmake"

	# Trigger an Xcode build
	echo "  Building POCO..."
	xcodebuild -project Poco.xcodeproj -target install -configuration Release >> "${LOG}" 2>&1 
	check_status $? "xcodebuild"

	cd ${SOURCE}/poco-source
	rm -R ${SOURCE}/poco-source/cmake-build
}

function build_all {
	# Cleanup and download a version of Poco
	echo "Note - Make sure OpenSSL has been built (otherwise run build-ssl.sh first)"
	echo "Downloading POCO..."
	curl -#Lo ${SOURCE}/poco.tar.gz https://github.com/pocoproject/poco/archive/poco-1.9.4-release.tar.gz
	cd ${SOURCE}
	mkdir -p poco-source
	tar xpf poco.tar.gz --strip-components 1 -C poco-source
	rm poco.tar.gz
	cd ${SOURCE}/poco-source

	for PLATFORM in "${PLATFORMS[@]}"; do
		export PLATFORM=$PLATFORM
		build_platform
	done	

	SLICES=(`ls ${TARGET}/${PLATFORMS[0]}/lib/*.a`)
	for SLICE_ABSOLUTE in "${SLICES[@]}"; do
		SLICE="$(basename ${SLICE_ABSOLUTE})"
		PLATFORM_DIRS=("${PLATFORMS[@]/#/${TARGET}/}")
		lipo -create "${PLATFORM_DIRS[@]/%//lib/${SLICE}}" -output "${OUTPUT}/lib/${SLICE}"
	done

}

POCO_FLAGS="\
	-DPOCO_STATIC=1 \
	-DENABLE_APACHECONNECTOR=0 \
	-DENABLE_CPPPARSER=0 \
	-DENABLE_CRYPTO=1 \
	-DENABLE_DATA=0 \
	-DENABLE_DATA_MYSQL=0 \
	-DENABLE_DATA_ODBC=0 \
	-DENABLE_DATA_SQLITE=0 \
	-DENABLE_ENCODINGS=0 \
	-DENABLE_ENCODINGS_COMPILER=0 \
	-DENABLE_JSON=1 \
	-DENABLE_MONGODB=0 \
	-DENABLE_NET=1 \
	-DENABLE_NETSSL=1 \
	-DENABLE_NETSSL_WIN=0 \
	-DENABLE_PAGECOMPILER=0 \
	-DENABLE_PAGECOMPILER_FILE2PAGE=0 \
	-DENABLE_PDF=0 \
	-DENABLE_POCODOC=0 \
	-DENABLE_REDIS=0 \
	-DENABLE_SEVENZIP=0 \
	-DENABLE_TESTS=0 \
	-DENABLE_UTIL=1 \
	-DENABLE_XML=0 \
	-DENABLE_ZIP=1 \
	-DOPENSSL_INCLUDE_DIR=${OUTPUT}/include \
	-DOPENSSL_CRYPTO_LIBRARY=${OUTPUT}/lib/libcrypto.a \
	-DOPENSSL_SSL_LIBRARY=${OUTPUT}/lib/libssl.a"

echo "Building in ${SOURCE}"
echo "Installing in ${TARGET}"
echo "Copying libraries/headers to ${OUTPUT}"
PLATFORMS=( OS SIMULATOR SIMULATOR64 )
build_all

