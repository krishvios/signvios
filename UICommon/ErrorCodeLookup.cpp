//
//  ErrorCodeLookup.cpp
//  ntouchMac
//
//  Created by Isaac Roach on 9/9/12.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//

#include "ErrorCodeLookup.h"

namespace Vp200Ui
{
	struct SstiErrorMessage
	{
		unsigned int ErrorCode;
		std::string sErrorMessage;
	};
	
	const SstiErrorMessage g_CoreErrorMessages[] =
	{
		{0, "Unknown error. (0)"},
		{1, "You must be logged in to proceed. (1)"},
		{2, "The password you entered is incorrect. (2)"},
		{3, "Phone Number does not exist. (3)"},
		{4, "Phone Number already exists. (4)"},
		{5, "Could not edit user. (5)"},
		{6, "Phone Number could not be associated. (6)"},
		{7, "Phone Number could not be associated - not controlled by user or availabl.e (7)"},
		{8, "Phone Number could not be disassociated. (8)"},
		{9, "Phone Number could not be disassociated - not controlled by user. (9)"},
		{10, "Unknown Error. (10)"},
		{11, "UniqueId not recognized. (11)"},
		{12, "Phone Number is not registered with Directory. (12)"},
		{13, "UniqueId Already Exists. (13)"},
		{14, "Unable to connect to SignMail Server. (14)"},
		{15, "No Information Sent. (15)"},
		{16, "Unknown Error. (16)"},
		{17, "UniqueId cannot be NULL. (17)"},
		{18, "Service Level Change Not Allowed. (18)"},
		{19, "Unknown Error. (19)"},
		{20, "Unknown Error. (20)"},
		{21, "Unknown Error. (21)"},
		{22, "RealNumber service failed to release the list of numbers. (22)"},
		{23, "Request did not contain phone numbers to release. (23)"},
		{24, "Unknown Error. (24)"},
		{25, "Count attribute has an invalid value. (25)"},
		{26, "RealNumber service failed to get a list of numbers. (26)"},
		{27, "RealNumber service cannot be reached. (27)"},
		{28, "User is a not the Primary User of this Phone. (28)"},
		{29, "User is a Primary User on another Phone. (29)"},
		{30, "Provision request failed. (30)"},
		{31, "No current emergency provision status found. (31)"},
		{32, "No current emergency address found. (32)"},
		{33, "Some or all submitted address data is blank. (33)"},
		{34, "Deprovision request failed. (34)"},
		{35, "Unable to contact database. (35)"},
		{36, "No numbers left of the desired type in Real Numbers. (36)"},
		{37, "Unable to acquire number in Real Numbers. (37)"},
		{38, "No pending numbers available for this user. (38)"},
		{39, "Router information could not be cleared in database. (39)"},
		{40, "No information found for logged in phone. (40)"},
		{41, "Query Interface Connector does not exist on specified machine. (41)"},
		{42, "Query Interface Connector cannot be reached. (42)"},
		{43, "Query Interface Connector not declared in configuration. (43)"},
		{44, "Image Get/Edit/Delete problem. (44)"},
		{45, "Image Get/Edit/Delete problem saving data. (45)"},
		{46, "Image Delete problem deleting data. (46)"},
		{47, "Image List Get problem. (47)"},
		{48, "The user's bound endpoint type group does not match the endpoint's. (48)"},
		{49, "Blocked List Item Add/Edit was not successful, the Dial String is in the White List. (49)"},
		{50, "Call List Plan for endpoint type [TYPE HERE] could not be found. (50)"},
		{51, "The endpoint type [TYPE HERE] could not be found. (51)"},
		{52, "PhoneType not recognized. (52)"},
		{53, "Unable to get Service Contact Overrides. (53)"},
		{54, "Could not contact iTRS. (54)"},
		{55, "URI List Set error with mismatching priorities and/or URIs. (55)"},
		{56, "URI List Set error with deletion. (56)"},
		{57, "URI List Set error with addition. (57)"},
		{58, "Unable to register for SignMail, Main Email not set. (58)"},
		{59, "Attempted to add xx contacts more than the quota of xx allows. (59)"},
		{60, "Unable to determine which user to log in as. (60)"},
		{61, "LogInAs value invalid. (61)"},
		{62, "The phone description you entered is invalid. (62)"},
		{63, "There was an error creating the myPhone group. Please try again later. (63)"},
		{64, "The password you entered is invalid. (64)"},
		{65, "You cannot create a myPhone Group from this phone. (65)"},
		{66, "Endpoint is not ring group capable. (66)"},
		{67, "Call list item is not owned by user. (67)"},
		{68, "Video Messaging Server could not be contacted. (68)"},
		{69, "Error changing the myPhone Group password. (69)"},
		{70, "Call list item does not exist. (70)"},
		{71, "Unable to update the ENUM Server and Error Rolling back Ring Group. (71)"},
		{72, "The myPhone Group invitation is no longer valid. (72)"},
		{73, "You are not a member of a myPhone Group. (73)"},
		{74, "Error adding phone to this group. Check to make sure the phone is logged in and is not already a member of another group. (74)"},
		{75, "Error adding phone to the myPhone Group. (75)"},
		{76, "Error getting myPhone Group information. (76)"},
		{77, "Error locating the myPhone Group phone number. (77)"},
		{78, "Error adding phone to this myPhone Group. (78)"},
		{79, "Error cancelling myPhone Group invitation. (79)"},
		{80, "You must enter a myPhone Group phone number. (80)"},
		{81, "The description field must be 1 to 50 characters long. (81)"},
		{82, "The phone number you entered could not be found. (82)"},
		{83, "Invalid user. (83)"},
		{84, "The group user specified could not be found. (84)"},
		{85, "The user being removed is not a member of the Ring Group. (85)"},
		{86, "Error removing phone from the myPhone Group. (86)"},
		{87, "The phone number you entered is not a member of your myPhone Group. (87)"},
		{88, "Element <the element> has invalid phone number. (88)"},
		{89, "The Password and Confirm Password do not match. (89)"},
		{90, "Blocked List Item cannot have DialString of ring group member. (90)"},
		{91, "You cannot remove the last member of a myPhone Group. (91)"},
		{92, "URIList must contain 1 or more iTRS URIs. (92)"},
		{93, "Invalid URI [<uri>]. (93)"},
		{94, "The redirect from userId and the redirect to UserId cannot be the same. (94)"},
		{95, "There are more than 1 members in the ring group, the others need to be removed before removing the entire ring group. (95)"},
		{96, "There should be at least 1 member left in the ring group. (96)"},
		{97, "Could not locate an active local number for the redirect from UserId. (97)"},
		{98, "Unknown error. (98)"},
		{99, "Error authenticating myPhone Group. (99)"},
		{100, "You are not a member of this myPhone Group. (100)"},
		{101, "Error setting the myPhone Group password. (101)"},
		{102, "You must enter a valid phone number. (102)"},
		{103, "Error saving the myPhone member information. (103)"},
		{104, "The dial string is required. (104)"},
		{105, "The myPhone group creator cannot be removed from the group. (105)"},
		{106, "The image was not found. (106)"}
	};
	
	const SstiErrorMessage g_MessageErrorMessages[] =
	{
		{1, "Not logged In. (1)"},
		{3, "Recipient Phone Number is not valid. (3)"},
		{11, "User phone not recognized. (11)"},
		{15, "Recipient could not be found. (15)"},
		{36, "Download Server not set in database. (36)"},
		{37, "Message ID not valid. (37)"},
		{38, "Unable to contact Message Server. (38)"},
		{39, "Message Server returned error​ (Error text inside element text). (39)"},
		{40, "Call ID is not a valid number. (40)"},
		{41, "Interpreter ID is not a valid number. (41)"},
		{42, "Interpreter ID or Call ID was not valid. (42)"},
		{59, "Bitrate is not valid. (59)"},
		{60, "Message Seconds is not valid. (60)."},
		{61, "Message Seconds are 0. (61)"},
		{62, "Receiver device type is unknown. (62)"},
		{88, "Element <the element> has invalid phone number. (88)"},
		{999, "Unable to retrieve upload url. (999)"}
	};
	
	const SstiErrorMessage g_StateNotifyErrorMessages[] =
	{
		{1, "Not logged In. (1)"},
		{10, "Unable to update user or phone. (10)"},
		{11, "Unique ID not recognized. (11)"}
	};
	
	const unsigned int g_nNumberOfCoreErrorMessages = sizeof (g_CoreErrorMessages) / sizeof (g_CoreErrorMessages[0]);
	const unsigned int g_nNumberOfMessageErrorMessages = sizeof (g_MessageErrorMessages) / sizeof (g_MessageErrorMessages[0]);
	const unsigned int g_nNumberOfStateNotifyErrorMessages = sizeof (g_StateNotifyErrorMessages) / sizeof (g_StateNotifyErrorMessages[0]);

	std::string CErrorCodeLookup::GetCoreErrorMessage(unsigned int unErrorCode)
	{
		if (unErrorCode < g_nNumberOfCoreErrorMessages)
		{
			for (unsigned int i=0; i< g_nNumberOfCoreErrorMessages; i++)
			{
				if (g_CoreErrorMessages[i].ErrorCode == unErrorCode)
				{
					return g_CoreErrorMessages[i].sErrorMessage;
				}
			}
		}
		
		return "Unknown Error";
	}
	
	std::string CErrorCodeLookup::GetMessageErrorMessage(unsigned int unErrorCode)
	{
		if (unErrorCode < g_nNumberOfMessageErrorMessages)
		{
			for (unsigned int i=0; i< g_nNumberOfMessageErrorMessages; i++)
			{
				if (g_MessageErrorMessages[i].ErrorCode == unErrorCode)
				{
					return g_MessageErrorMessages[i].sErrorMessage;
				}
			}
		}
		
		return "Unknown Error";
	}
	
	std::string CErrorCodeLookup::GetStateNotifyErrorMessage(unsigned int unErrorCode)
	{
		if (unErrorCode < g_nNumberOfStateNotifyErrorMessages)
		{
			for (unsigned int i=0; i< g_nNumberOfStateNotifyErrorMessages; i++)
			{
				if (g_StateNotifyErrorMessages[i].ErrorCode == unErrorCode)
				{
					return g_StateNotifyErrorMessages[i].sErrorMessage;
				}
			}
		}
		
		return "Unknown Error";
	}

}
