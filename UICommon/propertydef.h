/*!
 * \file propertydef.h
 * \brief Definitions of properties used by the UI that are not provided by CCI
 *
 * \date Tue Aug 17 2004
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef PROPERTYDEF_H
#define PROPERTYDEF_H

/*!
 * \brief Namespace for UI property names
 */
namespace PropertyDef
{

extern const char AutoAnswer[];              //!< Automatically answer incoming calls.
extern const char AutoHideUi[];              //!< Hide the UI 5-seconds into a call in full-screen mode.
extern const char MuteRinger[];              //!< Mute the ringer.
extern const char DisplayMode[];             //!< Windowed or full-screen modes.

extern const char EnableVco[];               //!< Enable VCO support
extern const char VCOPreference[];           //!< Choice of 1-Line or 2-Line VCO Support
extern const char VCOEnabledForAllCalls[];   //!< Enable VCO for ALL calls, else only for those with a marked contact.
extern const char VCOActive[];               //!< Enable VCO for ALL VRS calls.
extern const char VoiceCallbackNumber[];     //!< VCO callback phone number for 2-Line VCO.
extern const char NetPublicIp[];             //!< Public IP Address override.
extern const char PPPoEWanted [];            //!< Use PPPoE
extern const char PPPoEUsername[];           //!< PPPoE Username
extern const char PPPoEPassword[];           //!< PPPoE Password
extern const char ValidConfiguration[];      //!< The configured settings are valid.
extern const char Address1[];                //!< Street address line 1.
extern const char Address2[];                //!< Street address line 2.
extern const char City[];                    //!< City
extern const char State[];                   //!< State
extern const char ZipCode[];                 //!< Zip Code
extern const char AreaCode[];
extern const char DefUserPhone[];            //!< Default user phone number.
extern const char DefUserPassword[];         //!< Default user password.
extern const char DefUserEmail[];            //!< Default user e-mail.
extern const char DefUserLogoutTime[];       //!< Default user logout time
extern const char DefUserWantLogout[];       //!< Default user auto-logout.
/*extern const char UserVcoOption[];           //!< VCO option (use, don't use, etc.).
extern const char UserVcoIsDefault[];        //!< Use VCO by default.
extern const char UserVcoCallback[];         //!< User's personal VCO callback number.*/
extern const char AdminPassword[];           //!< Administrator passwordl
extern const char FullScreenSelfPos[];       //!< Self view position enum for full screen mode
extern const char DualModeSelfPos[];         //!< Self view position enum for dual mode
extern const char VideoSelfPosX[];           //!< Self view X positon.
extern const char VideoSelfPosY[];           //!< Self view Y position.
extern const char VideoSelfWidth[];          //!< Width of self view.
extern const char VideoSelfHeight[];         //!< Height of self view.
extern const char VideoSelfVisible[];        //!< Self view is shown or hidden.
extern const char VideoRemotePosX[];         //!< Remote view X position.
extern const char VideoRemotePosY[];         //!< Remote view Y posiiton.
extern const char VideoRemoteWidth[];        //!< Remote view width.
extern const char VideoRemoteHeight[];       //!< Remote view height.
extern const char Wizard1Completed[];        //!< The first setup wizard is completed or not completed.
extern const char WizardCompleted[];         //!< Both setup wizards completed or not completed.
extern const char LoggedInPassword[];        //!< Password of the currently logged in user.
extern const char LoggedInUser[];            //!< User name (phone number) of the currently logged in user.
extern const char DoNotLogCall[];            //!< Do not log call in call lists.
extern const char AutoLogout[];              //!< Auto logout
extern const char AutoLogoutTime[];          //!< Auto logout time.
extern const char RingPattern[];             //!< Default ring pattern.
extern const char RingColor[];               //!< Default light ring color
extern const char LedBrightness[];           //!< LED Brightness setting
extern const char TimeZone[];                //!< Time zone index.
extern const char DST[];                     //!< DST enabled or disabled.
extern const char TimeZoneString[];          //!< Unix time zone string
extern const char WizardUserOp[];            //!< Create new or associate existing phone number in wizard.
extern const char LastMissedTime[];          //!< Time of the last missed call.
extern const char LicenseAccepted[];         //!< The user has accepted the license agreement or not.
extern const char UpdateServer[];            //!< The update server for developer override.
extern const char ScreenSaverDelay[];        //!< The number of minutes of inactivity before starting the screen saver.
extern const char ScreenSaverDelaySeconds[]; //!< The number of seconds of inactivity before starting the screen saver.
extern const char ScreenSaver[];             //!< The current screensaver index (deprecated)
extern const char ScreenSaverId[];           //!< The current screensaver ID (GUID)
extern const char ScreenSaverName[];         //!< The current screensaver name
extern const char ScreenSaverPreviewUrl[];   //!< The current screensaver PreviewUrl
extern const char ThemeId[];                 //!< The current theme ID (GUID)
extern const char ThemeName[];               //!< The current theme name
extern const char UpdateHourStart[];         //!< Hour of the day (0-23) to start looking for updates.
extern const char UpdateHourEnd[];           //!< The hour of the day (0-23) to stop looking for updates.
extern const char AutoHideSeconds[];         //!< Number of seconds to be in a call before hiding the UI.
extern const char VRCLPortOverride[];        //!< VRCL port number to use when not in interpreter mode.
extern const char NoShowDeleteMissed[];      //!< Do not show the delete from missed list confirmation.
extern const char NoShowDeleteAllMissed[];   //!< Do not show the delete all from missed list confirmation.
extern const char NoShowDeleteRedialed[];
extern const char NoShowDeleteAllRedialed[];
extern const char NoShowDeleteDialed[];
extern const char NoShowDeleteAllDialed[];
extern const char NoShowDeleteRecv[];
extern const char NoShowDeleteAllRecv[];
extern const char NoShowDeleteAllCalls[];
extern const char NoShowDeleteBlocked[];
extern const char NoShowSettingsAlert[];
extern const char NoShowDeleteContact[];
extern const char NoShowDeleteMsg[];
extern const char NoShowDeleteAllMsg[];
extern const char NoShowAddVcoContact[];     //!< Do not show the warning to add vco contact without vco enabled
extern const char NoShowOtherVRSWarning[];   //!< Do not show the warning about other VRS providers when in Standard service.
extern const char EnableStatistics[];
extern const char PowerSaveEnabled[];        //!< Was the unit in PowerSave mode during a reboot?
extern const char ShowMiniStatus[];          //!< Continuously show the mini status bar during call
extern const char ShowInCallStatus[];          //!< Continuously show the In-Call status bar during call
extern const char DisableAudio[];            //!< Personal Preference setting to mute the microphone.
extern const char CameraAllowRmtCtrl[];      //!< Personal Preference setting to allow others control your camera
extern const char AutoVideoPrivacy[];        //!< Personal Preference settins to allow auto video privacy
extern const char EnableSystemInfo[];
extern const char VideoSaturation[];         //!< The user-modified settings for the video saturation
extern const char VideoBrightness[];         //!< The user-modified settings for the video brightness
extern const char VideoTone[];         //!< The user-modified settings for the video tone
extern const char AutoRejectIncoming[];
extern const char AutoHideIncomingAlert[];
extern const char IncomingAlertAutoHideSeconds[];
extern const char IncomingAlertSize[];
extern const char PromoContactPhotosReminderOn[];
extern const char PromoContactPhotosReminderTime[];
extern const char PromoPSMGOn[];
extern const char PromoPSMGTime[];
extern const char PromoShareTextOn[];
extern const char PromoShareTextTime[];
extern const char PromoVrsAnnounceOn[];
extern const char PromoVrsAnnounceTime[];
extern const char ScheduledReminderTimeOffset[];
extern const char CallCIRDismissibleOn[];
extern const char CallCIRDismissibleTime[];
extern const char StayOptedOut[];
extern const char OptOutExplained[];
extern const char SorensonServicesInfoVideo[];
extern const char DialCompetitorVideo[];
extern const char OptOutTime[];
extern const char OptInReminderSecondsInterval[];
extern const char CoreSyncValue[];            //!< Holds time that user setting values were last changed (on core server or phone)
extern const char TollFreeNumberEnabled[];    //!< Holds the phone setting which specifies if Toll Free NANP number is enabled
extern const char LocalNumberEnabled[];       //!< Holds the phone setting which specifies if Local NANP number is enabled
extern const char LastMessageViewTime[];
extern const char LastChannelMessageViewTime[];
extern const char AnswerOn[];				//!< Holds whether the "answer with message" feature is turned on
extern const char NumRingsBeforeMessage[];	//!< Holds the number of rings before going to the message
extern const char UpdateCheckInterval[];		//!< Number of minutes between "check for updates"
extern const char WizardPrimaryUserExists[];	//!< Indicates that there is an existing primary user for this VP
extern const char NetSettingsAccessCode[];	//!< Holds the access code to get into the Network Settings menu area
extern const char VideoMessageEnabled[];		//!< Indicates whether the Video message feature is enabled
extern const char Location911Enabled[];
extern const char AllowUserNameChange[];
extern const char MovingDialType[];
extern const char MovingDialString[];
extern const char MovingName[];
extern const char ProviderAgreementStatus[];
extern const char NewAccount[];
extern const char CallerIdNumber[];
extern const char DisplayedNumber[];
extern const char RegWizardHelpDialType[];
extern const char RegWizardHelpDialString[];
extern const char RegWizardHelpName[];
extern const char AgeVerificationStatus[];
extern const char PortedVrsDomainName[];
extern const char PortedPhoneNumber[];
extern const char PortedPassword[];
extern const char PortedLastUpdateCheck[];
extern const char PortedUpdateCheckInterval[];
extern const char PortedModeFlag[];
extern const char StunServer[];
extern const char StunServerRefresh[];
extern const char ReconnectNumber[];
extern const char cmAR_MUTEMODE[];
extern const char cmVR_MUTEMODE[];
extern const char cmAUTO_REJECT[];
extern const char uiNetActiveProfile[];
extern const char NetworkSettingsSet[];
extern const char uiNetLastActiveProfile[];
extern const char WirelessEnabled[];
extern const char WirelessInUse[];
extern const char ScreenRes[];
extern const char WarmReboot[];
extern const char ButtonTextColor[];
extern const char ButtonBack1Color[];
extern const char ButtonBack2Color[];
extern const char RingGroupCreatedByUser[];
extern const char RingGroupWizardPin[];
extern const char RingGroupPin[];
extern const char RingGroupHelpVideoUrl[];
extern const char AudioMeterEnabled[];
extern const char VideoRedOffset[];
extern const char VideoGreenOffset[];
extern const char VideoBlueOffset[];
extern const char EulaNotAccepted[];
extern const char AnnounceEnabled[];
extern const char AnnouncePreferenceH2D[];
extern const char AnnounceTextH2D[];
extern const char AnnouncePreferenceD2H[];
extern const char AnnounceTextD2H[];
extern const char LastCIRCallTime[];
extern const char DTMFEnabled[];
extern const char RealTimeTextEnabled[];
extern const char ContactDisplayOrder[];
extern const char DisplayContactImages[];
extern const char DisplayVideoBadge[];
extern const char HideFavorites[];
extern const char ShareTextSavedStr1[];
extern const char ShareTextSavedStr2[];
extern const char ShareTextSavedStr3[];
extern const char ShareTextSavedStr4[];
extern const char ShareTextSavedStr5[];
extern const char ShareTextSavedStr6[];
extern const char ShareTextSavedStr7[];
extern const char ShareTextSavedStr8[];
extern const char ShareTextSavedStr9[];
extern const char ShareTextSavedStr10[];
extern const char RingerVolume[];
extern const char RingerTone[];
extern const char InCallOptionsTimeout[];
extern const char InCallOptionsHint[];
extern const char IncomingPosition[];
extern const char ProfileImagePrivacyLevel[];
extern const char Theme[];
extern const char FocusValue[];
extern const char CallTransferEnabled[];
extern const char AgreementsEnabled[];
extern const char TakeATourVideoUrl[];
extern const char SpanishFeatures[];
extern const char SavedTextsCount[];
extern const char BluetoothEnabled[];
extern const char LDAP_LoginName[];
extern const char LDAP_Password[]; 
extern const char PowerOnDisplay[];
extern const char UseLEDNotifications[];
extern const char BlockedCallerIDEnabled[];
extern const char LowLightBorderEnabled[];
extern const char LowLightBorderFull[];
extern const char LowLightBorderWidth[];
extern const char LowLightBorderColor[];
extern const char CameraSerialNumber[];
extern const char MPUSerialNumber[];
extern const char VideoScreenSaverEnabled[];
extern const char CallGuard[];
extern const char VRIFeatures[];
}

#endif


