set -e # fails build if any command fails

if [ ${CI_XCODEBUILD_EXIT_CODE} != 0 ]
then
	exit 1
fi

# Find the dSYM path
DSYM_PATH="$CI_ARCHIVE_PATH/dSYMs"

if [[ -n "$DSYM_PATH" ]]; then
    # Upload dSYMs to Crashlytics
    "${CI_DERIVED_DATA_PATH}/SourcePackages/checkouts/firebase-ios-sdk/Crashlytics/upload-symbols" -gsp "${CI_PRIMARY_REPOSITORY_PATH}/GoogleService-Info.plist" -p ios "$DSYM_PATH"
fi

if [[ -n $CI_APP_STORE_SIGNED_APP_PATH ]]; # checks if there is an AppStore signed archive after running xcodebuild
then
	# Set the "What to test" field for TestFlight builds with changelog of last 3 commits
	TESTFLIGHT_DIR_PATH=../TestFlight
	mkdir -p "$TESTFLIGHT_DIR_PATH"
	git fetch --deepen 3 && git log -3 --pretty=format:"%s" > "$TESTFLIGHT_DIR_PATH/WhatToTest.en-US.txt"

	# Use scheme, marketing version, and build number to create tag like ntouch-ios-XCBuild-999
	SAFE_SCHEME=${CI_XCODE_SCHEME}-XCBuild-${CI_BUILD_NUMBER}
	
	# Removing whitespaces and changing them to dashes to prevent CLI error
	BUILD_TAG=$(echo "${SAFE_SCHEME}" | tr -s ' ' '-' | tr -cd '[:alnum:]-')
	
	# echo "Tagging build with ${BUILD_TAG}"
	git tag ${BUILD_TAG} || {
		echo "Failed to create tag";
		exit 1;
	}

	# echo "Pushing tag ${BUILD_TAG} to remote repository with auth code ${GIT_AUTH}"
	git push https://${GIT_AUTH}@github.com/sorenson-eng/ntouch-ios.git ${BUILD_TAG} || {
		echo "Failed to push tag ${BUILD_TAG}";
		exit 1;
	}
fi

# use workflow Environment to configure your GIT_AUTH variable - username:personalAccessToken
