#!/bin/bash

function Exit
{
	if [ $1 -gt 0 ]; then
		echo "UNIT TEST FAILED"

		exit 1
	else
		echo "UNIT TEST SUCCEEDED"
	fi
}

function CheckResult
{
	if [ $1 -gt 0 ]; then
		Exit $1
	fi
}

function GenerateHtml
{
	# Collect the stats after the tests were run.
	for folder in $FOLDERS
	do
		folder2=`echo $folder | sed s#/#_#g`
		lcov -q -c -b ../$folder -d ../$folder/$builddir -o ./Coverage/$folder2.info
		if [ -e ./Coverage/test_total.info ]; then
			lcov -q -a ./Coverage/$folder2.info -a ./Coverage/test_total.info -o ./Coverage/test_total.info
		else
			lcov -q -a ./Coverage/$folder2.info -o ./Coverage/test_total.info
		fi
	done

	# Remove entries we don't want to report on
	lcov --remove ./Coverage/test_total.info "/usr/include/*" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "udp.cpp" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "udp.h" -o ./Coverage/test_total.info
	# Remove files in MPEG4/Atoms that are not used.
	lcov --remove ./Coverage/test_total.info "ElstAtom.cpp" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "ElstAtom.h" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "HmhdAtom.cpp" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "HmhdAtom.h" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "NmhdAtom.cpp" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "NmhdAtom.h" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "SmhdAtom.cpp" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "SmhdAtom.h" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "TrefAtom.cpp" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "TrefAtom.h" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "UdtaAtom.cpp" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "UdtaAtom.h" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "UnknownAtom.cpp" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "UnknownAtom.h" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "UrnAtom.cpp" -o ./Coverage/test_total.info
	lcov --remove ./Coverage/test_total.info "UrnAtom.h" -o ./Coverage/test_total.info

	# Generate the HTML files
	genhtml -q -o coverage_report ./Coverage/test_total.info
}

if [ x$builddir == x ]; then
	echo The environment variable "builddir" needs to be defined.
	echo For example, export builddir=Build/vp200_x86/debug
	exit 1
fi

FOLDERS="
	BlockListMgr \
	cci \
	common \
	contacts \
	ConfMgr \
	ConfMgr/channels \
	ConfMgr/DataTasks/AudioPlayback \
	ConfMgr/DataTasks/AudioRecord \
	ConfMgr/DataTasks/DataCommon \
	ConfMgr/DataTasks/VideoPlayback \
	ConfMgr/DataTasks/VideoRecord \
	ConfMgr/DataTasks/TextPlayback \
	ConfMgr/DataTasks/TextRecord \
	ConfMgr/Nat \
	CoreServices \
	FilePlay \
	http \
	ImageMgr \
	MPEG4 \
	MPEG4/Atoms \
	MPEG4/Toolbox \
	MsgMgr \
	os \
	pm \
	update \
	update/method3 \
	vrcl \
	xmlmgr \
"

#
# Create the coverage data folder to store the coverage data.
#
if [ ! -e ./Coverage ]; then
	mkdir Coverage
fi

#
# Remove the report files
#
for folder in $FOLDERS
do
	folder2=`echo $folder | sed s#/#_#g`

	if [ -e ./Coverage/"$folder2"_base.info ]; then
		rm ./Coverage/"$folder2"_base.info
	fi
	if [ -e ./Coverage/"$folder2".info ]; then
		rm ./Coverage/"$folder2".info
	fi
done

#
# Remove the summary file
#
if [ -e ./Coverage/test_total.info ]; then
	rm ./Coverage/test_total.info
fi

#
# Remove the base information.
#
for folder in $FOLDERS
do
	rm ../$folder/$builddir/*.gcda
done

#
# Collect the base information.
#
# for folder in $FOLDERS
# do
# 	folder2=`echo $folder | sed s#/#_#g`
# 	lcov -q -c -i -b ../$folder -d ../$folder/$builddir -o ./Coverage/"$folder2"_base.info
# done

#
# Run the tests
#
#gdb -batch -ex "run" -ex "bt" ./Test 2>&1
./$builddir/Test

if [ -s VideophoneEngineTests.xml ]; then
	#
	# Grep for something that indicates that at least one
	# of the tests has failed.
	# If not found, grep will set the return status to non-zero.
	# If it is non-zero then the tests must have been successful
	# so invert the result by executing test.
	#
	grep "FailedTest id" VideophoneEngineTests.xml
	test $? -ne 0
	UnitTestResult=$?
else
	UnitTestResult=1
fi

CheckResult $UnitTestResult

GenerateHtml

Exit 0
