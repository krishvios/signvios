#!/bin/bash

function AnalyzeFiles
{
	for File in $( find $1 -maxdepth 1 -name "$2"  ); do

		if [ $File != "../os/stiOSWin32.cpp" \
			-a $File != "../os/stiOSWin32.h" \
			-a $File != "../os/stiOSWin32Task.cpp" \
			-a $File != "../os/stiOSWin32Task.h" \
			-a $File != "../os/stiOSWin32Wd.cpp" \
			-a $File != "../os/stiOSWin32Signal.cpp" \
			-a $File != "../os/stiOSWin32Signal.h" \
			-a $File != "../os/stiOSWin32Mutex.cpp" \
			-a $File != "../os/stioswin32mutex.h" \
			-a $File != "../os/stiOSWin32Cond.cpp" \
			-a $File != "../os/stiOSWin32Cond.h" \
			-a $File != "../os/stiOSWin32Msg.cpp" \
			-a $File != "../os/stiOSMacOSXNet.cpp" \
			-a $File != "../os/stiOSWin32Net.cpp" \
			-a $File != "../os/stiOSWin32Net.h" \
			-a $File != "../os/stiOSWin32Sem.cpp" \
			-a $File != "../os/stiOSWin32Sem.h" \
			-a $File != "../os/stiOSWin32unistd.h" \
			-a $File != "../os/stiOSMacOSX.h" \
			-a $File != "../cci/AstiVideophoneEngine.cpp" \
			-a $File != "../cci/AstiVideophoneEngine.h" \
			-a $File != "../os/stiOSWin32Pipe.cpp" \
			-a $File != "../ConfMgr/CstiH323Manager.cpp" \
			-a $File != "../ConfMgr/stiH323Tools.cpp" \
			-a $File != "../ConfMgr/CstiH323Call.cpp" \
			-a $File != "../ConfMgr/CstiH323Manager.h" \
			-a $File != "../ConfMgr/CstiH323Call.h" \
			-a $File != "../ConfMgr/channels/CstiH323VideoRecordChannel.cpp" \
			-a $File != "../ConfMgr/channels/CstiH323PlaybackChannel.cpp" \
			-a $File != "../ConfMgr/channels/CstiH323VideoPlaybackChannel.cpp" \
			-a $File != "../ConfMgr/channels/CstiH323RecordChannel.cpp" \
			-a $File != "../ConfMgr/channels/CstiH323Channel.cpp" \
			-a $File != "../ConfMgr/channels/CstiH323Channel.h" \
		]; then
#			echo Processing $File
			vera++ --root ./Vera/ --profile vpe $File
		fi
	done
}


function AnalyzeFolder
{
#	echo Parent folder $1

	for Folder in $( find $1 -maxdepth 1 -type d ); do
	
		if [ $Folder != $1 \
			-a $Folder != ".." \
			-a $Folder != "." \
			-a $Folder != "../Platform/ntouch-ios" \
			-a $Folder != "../Platform/ntouch-mac" \
			-a $Folder != "../Platform/ntouch-android" \
			-a $Folder != "../Platform/ntouch-pc" \
			-a $Folder != "../Platform/ntouch/codecEngine" \
			-a $Folder != "../Platform/mediaserver" \
			-a $Folder != "../Platform/test" \
			-a $Folder != "../Platform/holdserver" \
			-a $Folder != "../Platform/common" \
			-a $Folder != "../Libraries" \
			-a $Folder != "../stun" \
			-a $Folder != "../Distribution" \
			]; then

#			echo Analyzing $Folder
			
			AnalyzeFiles $Folder "*.cpp"
			AnalyzeFiles $Folder "*.h"

			AnalyzeFolder $Folder
		fi
		
	done
}


AnalyzeFolder ..

