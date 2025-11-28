#!/bin/sh
#
# AppSimulator.command	
#
# Created by Kevin Selman on 8/27/20.
# Copyright © 2020 Sorenson Communications. All rights reserved.
#

: '
	This script requires that the Device IDs match what are installed
	on the machine.  Run the following command then replace the ID numbers in
	this script with what is found in the output in the "Devices" section

Sample output:	

	xcrun simctl list

	== Devices ==
	-- iOS 13.6 --
		iPhone 8 (A9AD4764-0148-49C8-A4DC-DA7C249714ED) (Shutdown) 
  	  	iPhone 8 Plus (68C3594B-8858-4FE3-88E0-6E123232294D) (Shutdown) 
  	  	iPhone 11 (34E894BE-2278-48B8-9207-8E596DDA6ECF) (Booted) 
  	  	iPhone 11 Pro (B20D68DD-9CFD-42E7-9C59-68091DCE2BB9) (Shutdown) 
 	  	iPhone 11 Pro Max (5ADC4145-99B3-4C5B-BDDD-9E9BB4D7C571) (Shutdown) 
  	  	iPhone SE (2nd generation) (23616779-2E68-458E-AD72-060F38DDE3A7) (Shutdown) 
	  	iPad Pro (9.7-inch) (0BB66545-A89E-4889-ACD0-F8528A0D09E6) (Shutdown) 
 	  	iPad (7th generation) (3F76D43E-E400-43AC-9DE7-01CE678B1A4C) (Shutdown) 
  	  	iPad Pro (11-inch) (2nd generation) (CCD440AA-B401-42D7-B6E5-E3F228586F77) (Shutdown) 
  	  	iPad Pro (12.9-inch) (4th generation) (A98B7AC9-F3B1-40F8-ACEB-8C0A0EAD13AD) (Shutdown) 
 	  	iPad Air (3rd generation) (DEE2B34A-E5AB-457E-BDF2-E9E6B2EEA980) (Shutdown)
'
SIMGUID=""
CURRENTDIR="$(dirname "$BASH_SOURCE")"

clear
echo "- - - - - - - - - - - - - - - - - - - - - -"
echo "       Select a Sorenson App to run        "
echo "											 "
echo "  This script will launch the selected	 "
echo "  app in the iOS simulator.                "
echo "- - - - - - - - - - - - - - - - - - - - - -"
echo " 1. - ntouch"
echo " 2. - Wavello"
echo " 3. - BuzzStickers"
echo " 4. - EXIT"
read -p "Enter choice [ 1 - 12 ]: " SELECTEDAPP
echo "\r\r"
echo "- - - - - - - - - - - - - - - - - - - - - -"
echo "      Select an iOS Simulator to use       "
echo "- - - - - - - - - - - - - - - - - - - - - -"
echo " 1. - iPhone 8"
echo " 2. - iPhone 8 Plus"
echo " 3. - iPhone 11"
echo " 4. - iPhone 11 Pro"
echo " 5. - iPhone 11 Pro Max"
echo " 6. - iPhone SE (2nd generation)"
echo " 7. - iPad Pro (9.7-inch)"
echo " 8. - iPad (7th generation)"
echo " 9. - iPad Pro (11-inch) (2nd generation)"
echo " 10. - iPad Pro (12.9-inch) (4th generation)"
echo " 11. - iPad Air (3rd generation)"
echo " 12. - EXIT"
echo ""
read -p "Enter choice [ 1 - 12 ]: " SELECTEDSIM
	case $SELECTEDSIM in
		1) SIMGUID='E93DF834-42E6-411E-A911-8CBA7CE217C3' ;;  # Replace this ID for iPhone 8 from your own output
		2) SIMGUID='A266F3E1-94AD-45DD-9B6A-1C9C2FED189F' ;;
		3) SIMGUID='2C36B47D-E110-4964-A0B3-81F0110DF211' ;;
		4) SIMGUID='CE22827E-3D93-4B02-BBEE-B12680230A86' ;;
		5) SIMGUID='BF88113A-C2D7-447F-8227-1C7267DE0018' ;;
		6) SIMGUID='4D643DFD-176A-41FF-9D27-D6C9B775953D' ;;
		7) SIMGUID='B8536E87-6A6C-45FD-B1F6-C266568497F7' ;;
		8) SIMGUID='53613C76-0D26-47D6-A75C-B78CF940927E' ;;
		9) SIMGUID='0165D7F9-032B-4232-9651-650D8E21887C' ;;
		10) SIMGUID='0ED50BA0-F0D5-4B1C-B6BF-7D2E8F9866E8' ;;
		11) SIMGUID='0EE16F8D-EE43-405A-872F-5E3CC1F09AF3' ;;
		12) exit 0;;
		*) echo -e "ERROR" && sleep 2
	esac

echo ""
echo "Booting simulator..."
xcrun simctl boot $SIMGUID

echo "Installing..."
	case $SELECTEDAPP in
		1) xcrun simctl install $SIMGUID "$CURRENTDIR/ntouch.app" ;; 
		2) xcrun simctl install $SIMGUID "$CURRENTDIR/Wavello.app" ;;
		3) xcrun simctl install $SIMGUID "$CURRENTDIR/BuzzStickers.app" ;;
		4) exit 0;;
		*) echo -e "ERROR" && sleep 2
	esac

echo "Opening simulator..."
open /Applications/Xcode.app/Contents/Developer/Applications/Simulator.app
