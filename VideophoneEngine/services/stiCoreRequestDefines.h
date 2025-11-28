////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  File Name:	stiCoreRequestDefines.h
//
//  Owner:		Lane Walters
//
//	Abstract:
//
////////////////////////////////////////////////////////////////////////////////
#ifndef STICOREREQUESTDEFINES_H
#define STICOREREQUESTDEFINES_H

//
// Includes
//


//
// Constants
//

// Constant strings representing the CoreRequest tag names and associated attributes, etc
const char CRQ_COREREQUEST[]  = "CoreRequest";
const char CRQ_CoreResponse[] = "CoreResponse";


const char CRQ_AgreementGet[] =                     "AgreementGet";
const char CRQ_AgreementStatusGet[] =               "AgreementStatusGet";
const char CRQ_AgreementStatusSet[] =               "AgreementStatusSet";
const char CRQ_BlockedListCountGet[] =              "BlockedListCountGet";
const char CRQ_BlockedListGet[] =                   "BlockedListGet";
const char CRQ_CallListCountGet[] =                 "CallListCountGet";
const char CRQ_CallListGet[] =                      "CallListGet";
const char CRQ_CallListItemAdd[] =                  "CallListItemAdd";
const char CRQ_CallListItemEdit[] =                 "CallListItemEdit";
const char CRQ_CallListItemGet[] =                  "CallListItemGet";
const char CRQ_CallListItemRemove[] =               "CallListItemRemove";
const char CRQ_CallListItemModify[] =               "CallListItemModify";
const char CRQ_CallListSet[] =                      "CallListSet";
const char CRQ_ClientAuthenticate[] =               "ClientAuthenticate";
const char CRQ_GetUserDefaults[] =                  "DefaultUserSettingsGet";
const char CRQ_ClientLogout[] =                     "ClientLogout";
const char CRQ_DirectoryRegister[] =                "DirectoryRegister";
const char CRQ_DirectoryResolve[] =                 "DirectoryResolve";
const char CRQ_DirectoryUnregister[] =              "DirectoryUnregister";

const char CRQ_EmergencyAddressDeprovision[] = 		"EmergencyAddressDeprovision";
const char CRQ_EmergencyAddressGet[] = 				"EmergencyAddressGet";
const char CRQ_EmergencyAddressProvision[] = 		"EmergencyAddressProvision";
const char CRQ_EmergencyAddressProvisionStatusGet[] ="EmergencyAddressProvisionStatusGet";
const char CRQ_EmergencyAddressUpdateAccept[] =		"EmergencyAddressUpdateAccept";
const char CRQ_EmergencyAddressUpdateReject[] =		"EmergencyAddressUpdateReject";

const char CRQ_FavoriteAdd[] =                      "FavoriteAdd";
const char CRQ_FavoriteEdit[] =                     "FavoriteEdit";
const char CRQ_FavoritesListGet[] =                 "FavoriteListGet";
const char CRQ_FavoritesListSet[] =                 "FavoriteListSet";
const char CRQ_FavoriteRemove[] =                   "FavoriteRemove";

const char CRQ_ImageDelete[] =						"ImageDelete";
const char CRQ_ImageDownloadURLGet[] =				"ImageDownloadURLGet";
const char CRQ_ImageUploadURLGet[] =				"ImageUploadURLGet";
const char CRQ_MissedCallAdd[] =                    "MissedCallAdd";
const char CRQ_NumberChangeNotify[] =               "NumberChangeNotify";
const char CRQ_PhoneAccountCreate[] =               "PhoneAccountCreate";
const char CRQ_PhoneOnline[] =                      "PhoneOnline";
const char CRQ_PhoneNumberRequest[] =               "PhoneNumberRequest";
const char CRQ_PhoneSettingsGet[] =                 "PhoneSettingsGet";
const char CRQ_PhoneSettingsSet[] =                 "PhoneSettingsSet";
const char CRQ_PublicIPGet[] =                      "PublicIPGet";
const char CRQ_RegistrationInfoGet[] =              "RegistrationInfoGet";
const char CRQ_RingGroupCreate[] =                  "RingGroupCreate";
const char CRQ_RingGroupCredentialsUpdate[] =       "RingGroupCredentialsUpdate";
const char CRQ_RingGroupCredentialsValidate[] =     "RingGroupCredentialsValidate";
const char CRQ_RingGroupInfoGet[] =                 "RingGroupInfoGet";
const char CRQ_RingGroupInfoSet[] =                 "RingGroupInfoSet";
const char CRQ_RingGroupInviteAccept[] =            "RingGroupInviteAccept";
const char CRQ_RingGroupInviteInfoGet[] =           "RingGroupInviteInfoGet";
const char CRQ_RingGroupInviteReject[] =            "RingGroupInviteReject";
const char CRQ_RingGroupPasswordSet[] =             "RingGroupPasswordSet";
const char CRQ_RingGroupUserInvite[] =              "RingGroupUserInvite";
const char CRQ_RingGroupUserRemove[] =              "RingGroupUserRemove";
const char CRQ_ScreensaverListGet[] =               "ScreenSaverListGet";
const char CRQ_ServiceContact[] =                   "ServiceContact";
const char CRQ_ServiceContactsGet[] =               "ServiceContactsGet";
const char CRQ_ServiceLevelChange[] =               "ServiceLevelChange";
const char CRQ_ServiceMaskGet[] =                   "ServiceMaskGet";
const char CRQ_ServiceLevelCommitChanges[] =        "ServiceLevelCommitChanges";
const char CRQ_ServiceLevelChangeStatus[] =         "ServiceLevelChangeStatus";
const char CRQ_SignMailListGet[] =                  "SignMailListGet";
const char CRQ_SignMailRegister[] =                 "SignMailRegister";
const char CRQ_SignMailUnreadCount[] =              "SignMailUnreadCount";
const char CRQ_TimeGet[] =                          "TimeGet";
const char CRQ_UpdateVersionCheck[] =               "UpdateVersionCheck";
const char CRQ_URIListSet[] = 						"URIListSet";
const char CRQ_UserAccountAssociate[] =             "UserAccountAssociate";
const char CRQ_UserAccountInfoGet[] =               "UserAccountInfoGet";
const char CRQ_UserAccountInfoSet[] =               "UserAccountInfoSet";
const char CRQ_UserInterfaceGroupGet[] =            "UserInterfaceGroupGet";
const char CRQ_UserLoginCheck[] =                   "UserLoginCheck";
const char CRQ_UserRealNumberGet[] =                "UserRealNumberGet";
const char CRQ_UserSettingsGet[] =                  "UserSettingsGet";
const char CRQ_UserSettingsSet[] =                  "UserSettingsSet";
const char CRQ_VersionGet[] =                       "VersionGet";
const char CRQ_LogCallTransfer[] =                  "LogCallTransfer";
const char CRQ_ContactsImport[] =                   "ContactsImport";
const char CRQ_VrsAgentHistoryAnsweredAdd[] =		"VrsAgentHistoryAnsweredAdd";
const char CRQ_VrsAgentHistoryDialedAdd[] =			"VrsAgentHistoryDialedAdd";



////////////////////////////////////////////////////////////////////////////////
// DEFINES FOR SUB TAGS OF THE REQUESTS
//

const char TAG_Agreement[] =                     "Agreement";
const char TAG_AgreementPublicId[] =             "AgreementPublicId";
const char TAG_AgreementType[] =                 "AgreementType";
const char TAG_Status[] =                        "Status";
const char TAG_CoreService[] =                   "CoreService";
const char TAG_ServiceContact[] =                "ServiceContact";
const char TAG_PhoneIdentity[] =                 "PhoneIdentity";
const char TAG_UniqueId[] =                      "UniqueId";
const char TAG_PhoneType[] =                     "PhoneType";

const char TAG_CallList[] =                      "CallList";
const char TAG_CallListItem[] =                  "CallListItem";

const char TAG_FavoriteItem[] =                  "FavoriteItem";
const char TAG_FavoriteType[] =                  "FavoriteType";
const char TAG_Position[] =                      "Position";

const char TAG_UserIdentity[] =                  "UserIdentity";
const char TAG_Name[] =                          "Name";
const char TAG_PhoneNumber[] =                   "PhoneNumber";
const char TAG_Pin[] =                           "Pin";
const char TAG_ItemId[] =                        "ItemId";
const char TAG_RingTone[] =                      "RingTone";
const char TAG_RingColor[] =                     "RingColor";
const char TAG_RelayLanguage[] =                 "RelayLanguage";
const char TAG_ImageId[] =                       "ImagePublicId";
const char TAG_ImageDate[] =                     "ImageDate";
const char TAG_CallPublicId[] =                  "CallPublicId";
const char TAG_IntendedDialString[] =            "IntendedDialString";

const char TAG_CalleeDialString[] =              "CalleeDialString";
const char TAG_DialString[] =                    "DialString";
const char TAG_DialType[] =                      "DialType";
const char TAG_DirectoryRegister[] =             "DirectoryRegister";
const char TAG_FromDialString[] =                "FromDialString";
const char TAG_BlockCallerId [] =                "BlockCallerId";
const char TAG_InterfaceMode [] =                "InterfaceMode";
const char TAG_FromName      [] =                "FromName";
const char TAG_FromDialType  [] =                "FromDialType";

const char TAG_ContactPhoneNumberList[] =        "ContactPhoneNumberList";
const char TAG_ContactPhoneNumber[] =            "ContactPhoneNumber";
const char TAG_PhoneNumberType[] =               "PhoneNumberType";
const char TAG_PublicId[] =                      "PublicId";

const char TAG_MissedCall[] =                    "MissedCall";
const char TAG_MacAddress[] =                    "MacAddress";

const char TAG_NewNumber[] =                     "NewNumber";
const char TAG_OldNumber[] =                     "OldNumber";

const char TAG_ServicePriority[] =               "ServicePriority";
const char TAG_ServiceUrl[] =                    "ServiceUrl";

const char TAG_Time[] =                          "Time";
const char TAG_PublicIP[] =                      "PublicIP";

const char TAG_UserSettingItem[] =               "UserSettingItem";
const char TAG_UserSettingName[] =               "UserSettingName";
const char TAG_UserSettingValue[] =              "UserSettingValue";
const char TAG_UserSettingType[] =               "UserSettingType";

const char TAG_PhoneSettingItem[] =              "PhoneSettingItem";
const char TAG_PhoneSettingName[] =              "PhoneSettingName";
const char TAG_PhoneSettingValue[] =             "PhoneSettingValue";
const char TAG_PhoneSettingType[] =              "PhoneSettingType";

const char TAG_AppVersion[] =                    "AppVersion";
const char TAG_AppLoaderVersion[] =              "AppLoaderVersion";

const char TAG_Action[] =                        "Action";
const char TAG_SubAction[] =                     "SubAction";
const char TAG_Detail[] =                        "Detail";

const char TAG_RealNumberList[] =                "RealNumberList";
const char TAG_RealNumberItem[] =                "RealNumberItem";

const char TAG_VrsAgentHistory[] = 				 "VrsAgentHistory";
const char TAG_Agent[] = 				 		 "Agent";
const char TAG_AgentId[] = 				 		 "Id";

// State Notify Response Tags
//
const char TAG_NotificationService[] =           "NotificationService";
const char TAG_MessageService[] =                "MessageService";
const char TAG_ConferenceService[] =             "ConferenceService";
const char TAG_RemoteLoggerService[] =           "RemoteLoggerService";

// User Account Info Settings
//
const char TAG_AssociatedPhone[] =               "AssociatedPhone";
const char TAG_CurrentPhone[] =                  "CurrentPhone";
const char TAG_Address1[] =						 "Address1";
const char TAG_Address2[] =						 "Address2";
const char TAG_Street[] =						 "Street";
const char TAG_ApartmentNumber[] =				 "ApartmentNumber";
const char TAG_City[] =                          "City";
const char TAG_State[] =                         "State";
const char TAG_Zip[] =                           "Zip";
const char TAG_ZipCode[] =						 "ZipCode";
const char TAG_Email[] =                         "Email";
const char TAG_EmailMain[] =                     "EmailMain";
const char TAG_EmailPager[] =                    "EmailPager";
const char TAG_SignMailEnabled[] =               "SignMailEnabled";
const char TAG_MustCallCir[] =                   "MustCallCIR";
const char TAG_CLAnsweredQuota[] =               "CLAnsweredQuota";
const char TAG_CLDialedQuota[] =                 "CLDialedQuota";
const char TAG_CLMissedQuota[] =                 "CLMissedQuota";
const char TAG_CLRedialQuota[] =                 "CLRedialQuota";
const char TAG_CLBlockedQuota[] =                "CLBlockedQuota";
const char TAG_CLContactQuota[] =                "CLContactQuota";
const char TAG_CLFavoritesQuota[] =				 "FavoritesQuota";
const char TAG_TollFreeNumber[] =                "TollFreeNumber";
const char TAG_LocalNumber[] =                   "LocalNumber";
const char TAG_RingGroupLocalNumber[] = 		"RingGroupLocalNumber";
const char TAG_RingGroupTollFreeNumber[] = 		"RingGroupTollFreeNumber";
const char TAG_RingGroupName[] = 				"RingGroupName";
const char TAG_RingGroupCapable[] = 			"RingGroupCapable";
const char TAG_GroupPhoneNumber[] = 			"GroupPhoneNumber";
const char TAG_RingGroupMemberList[] = 			"RingGroupMemberList";
const char TAG_RingGroupMember[] = 				"RingGroupMember";
const char TAG_GroupState[] = 					"GroupState";
const char TAG_Description[] = 					"Description";
const char TAG_Alias[] = 						"Alias";
const char TAG_LogInAs[] = 						"LogInAs";

const char TAG_UserRegistrationDataRequired[] = "UserRegistrationDataRequired";
const char TAG_UserRegistrationData[] =			"UserRegistrationData";
const char TAG_HasIdentification[] =			"HasIdentification";
const char TAG_IdentificationData[] =			"IdentificationData";
const char TAG_DOB[] =							"DOB";

const char TAG_FriendlyName[] =                  "FriendlyName";
const char TAG_MfgName[] =                       "MfgName";
const char TAG_ModelDesc[] =                     "ModelDesc";
const char TAG_ModelName[] =                     "ModelName";
const char TAG_ModelNumber[] =                   "ModelNumber";

const char TAG_RegistrationStatus[] =			"RegistrationStatus";

const char TAG_URI[] = 							"URI";

const char TAG_TRANSFERRED_FROM_DIAL_STRING[] = "TransferredFromDialString";
const char TAG_TRANSFERRED_TO_DIAL_STRING[] =   "TransferredToDialString";
const char TAG_ORIGINAL_CALL_PUBLIC_ID[] =      "OriginalCallPublicId";
const char TAG_TRANSFERRED_CALL_PUBLIC_ID[] =   "TransferredCallPublicId";

const char TAG_AddedCount[] =                   "AddedCount";
const char TAG_DuplicateNotAddedCount[] =       "DuplicateNotAddedCount";
const char TAG_ContactCountExceedingQuota[] =   "ContactCountExceedingQuota";

// Conference Service Tags
const char TAG_ConferencePublicId[] = "ConferencePublicId";
const char TAG_ConferenceRoomUri[] = "ConferenceRoomUri";
const char TAG_ConferenceRoom[] = "ConferenceRoom";
const char TAG_ConferenceRoomStatus[] = "ConferenceRoomStatus";
const char TAG_ConferenceRoomAddAllowed[] = "Available";
const char TAG_ActiveParticipantsCount[] = "ActiveParticipantsCount";
const char TAG_AllowedParticipantsCount[] = "AllowedParticipantsCount";
const char TAG_Participant[] = "Participant";
const char TAG_ParticipantId[] = "ParticipantID";
const char TAG_ParticipantType[] = "ParticipantType";

const char TAG_PhoneUserID[] = "PhoneUserID";
const char TAG_RingGroupUserID[] = "RingGroupUserId";

////////////////////////////////////////////////////////////////////////////////
// ATTRIBUTE NAMES ON VARIOUS REQUESTS
//

const char ATT_BaseIndex[] =                     "BaseIndex";
const char ATT_Count[] =                         "Count";
const char ATT_SortDir[] =                       "SortDir";   // Sort Direction
const char ATT_SortField[] =                     "SortField";
const char ATT_Type[] =                          "Type";
const char ATT_Unique[] =                        "Unique";
const char ATT_FlagAddressBook[] =               "FlagAddressBook";
const char ATT_Threshold[] =                     "Threshold";
const char ATT_RequestType[] =                   "RequestType";
const char ATT_ServiceLevel[] =                  "ServiceLevel";
const char ATT_UserType[] =                      "UserType";
const char ATT_Result[] =						 "Result";
const char ATT_DateTime[] =						 "DateTime";
const char ATT_Priority[] =						 "Priority";
const char ATT_SignMailCount[] = 				 "SignMailCount";
const char ATT_SignMailLimit[] = 				 "SignMailLimit";
const char ATT_FavoriteLimit[] =                 "FavoriteLimit";
const char ATT_Network[] = 						 "Network";
const char ATT_VersionNumber[] =				 "VersionNumber";
const char ATT_APIVersion[] =					 "APIVersion";
const char ATT_ErrorNumber[] =                   "ErrorNum";
const char ATT_VrsCallId[] =                   	 "VrsCallId";
const char ATT_CallListItemId[] =				 "CallListItemId";
const char ATT_CallJoinUtc[] =					 "CallJoinUtc";
const char ATT_StateChangeUTC[] = 		  		 "StateChangeUTC";
const char ATT_NewTagValidDays[] =				 "NewTagValidDays";

////////////////////////////////////////////////////////////////////////////////
// VALUES FOR VARIOUS REQUESTS
//

const char VAL_OK[] =                            "OK";
const char VAL_Contact[] =                       "Contact";
const char VAL_Blocked[] =                       "Blocked";
const char VAL_Answered[] =                      "Answered";
const char VAL_Dialed[] =                        "Dialed";
const char VAL_Missed[] =                        "Missed";
const char VAL_Name[] =                          "Name";
const char VAL_Company[] =                       "CompanyName";
const char VAL_Time[] =                          "Time";
const char VAL_DialType[] =                      "DialType";
const char VAL_DialString[] =                    "DialString";
const char VAL_InAddressBook[] =                 "InAddressBook";
const char VAL_DESC[] =                          "DESC";
const char VAL_ASC[] =                           "ASC";
const char VAL_True[] =                          "True";
const char VAL_False[] =                         "False";
const char VAL_All[] =                           "All";
const char VAL_Premium[] =                       "Premium";
const char VAL_Basic[] =                         "Basic";
const char VAL_TollFree[] =                      "TOLLFREE";
const char VAL_Local[] =                         "LOCAL";
const char VAL_Home[] =                          "Home";
const char VAL_Work[] =                          "Work";
const char VAL_Cell[] =                          "Mobile";
const char VAL_iTRS[] =                          "iTRS";
const char VAL_Sorenson[] =                      "Sorenson";
const char VAL_Accepted[] =                      "Accepted";
const char VAL_Rejected[] =                      "Rejected";
const char VAL_PerformingTransfer[] =            "PerformingTransfer";
const char VAL_InitiatedTransfer[] =             "InititatedTransfer";

////////////////////////////////////////////////////////////////////////////////
// Constant strings representing the StateNotifyRequest tag names and associated attributes, etc
//
const char SNRQ_StateNotifyRequest[]  = "StateNotifyRequest";
const char SNRQ_Id[]                  = "Id";
const char SNRQ_ClientToken[]         = "ClientToken";
const char SNRQ_StateNotifyResponse[] = "StateNotifyResponse";

const char SNRQ_Heartbeat[] = "Heartbeat";

////////////////////////////////////////////////////////////////////////////////
// Constant strings representing the MessageRequest tag names and associated attributes, etc
//
const char MSGRQ_MessageRequest[] = "MessageRequest";
const char MSGRQ_MessageResponse[] = "MessageResponse";

////////////////////////////////////////////////////////////////////////////////
// Constant strings representing the ConferenceRequest tag names and associated attributes, etc
//
const char CNFRQ_ConferenceRequest[] = "ConferenceRequest";
const char CNFRQ_ConferenceResponse[] = "ConferenceResponse";

////////////////////////////////////////////////////////////////////////////////
// Constant strings representing the RemoteLoggerRequest tag names and associated attributes, etc
//
const char RLRQ_RemoteLoggerRequest[] = "RemoteLoggerRequest";
const char RLRQ_RemoteLoggerResponse[] = "RemoteLoggerResponse";


#endif // STICOREREQUESTDEFINES_H
