/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999 Apple Computer, Inc.  All Rights Reserved.
 * The contents of this file constitute Original Code as defined in and are 
 * subject to the Apple Public Source License Version 1.1 (the "License").  
 * You may not use this file except in compliance with the License.  Please 
 * obtain a copy of the License at http://www.apple.com/publicsource and 
 * read it before using this file.
 * 
 * This Original Code and all software distributed under the License are 
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the License for 
 * the specific language governing rights and limitations under the 
 * License.
 * 
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
	File:		GenerateXMLPrefs.h

	Contains:	Routine that updates a QTSS 1.x 2.x PrefsSource to the new XMLPrefsSource.

*/

#include "GenerateXMLPrefs.h"
#include "QTSSDataConverter.h"
#include "QTSS.h"

struct PrefConversionInfo
{
	char*				fPrefName;
	char*				fModuleName;
	QTSS_AttrDataType	fPrefType;
};

static const PrefConversionInfo	kPrefs[] =
{
	{ "rtsp_timeout",					NULL,			qtssAttrDataTypeUInt32 },
	{ "real_rtsp_timeout",				NULL,			qtssAttrDataTypeUInt32 },
	{ "rtp_timeout",					NULL,			qtssAttrDataTypeUInt32 },
	{ "maximum_connections",			NULL,			qtssAttrDataTypeSInt32 },
	{ "maximum_bandwidth",				NULL,			qtssAttrDataTypeSInt32 },
	{ "movie_folder",					NULL,			qtssAttrDataTypeCharArray },
	{ "bind_ip_addr",					NULL,			qtssAttrDataTypeCharArray },
	{ "break_on_assert",				NULL,			qtssAttrDataTypeBool16 },
	{ "auto_restart",					NULL,			qtssAttrDataTypeBool16 },
	{ "total_bytes_update",				NULL,			qtssAttrDataTypeUInt32 },
	{ "average_bandwidth_update",		NULL,			qtssAttrDataTypeUInt32 },
	{ "safe_play_duration",						NULL,	qtssAttrDataTypeUInt32 },
	{ "module_folder",							NULL,	qtssAttrDataTypeCharArray },
	{ "error_logfile_name",						NULL,	qtssAttrDataTypeCharArray },
	{ "error_logfile_dir",						NULL,	qtssAttrDataTypeCharArray },
	{ "error_logfile_interval",					NULL,	qtssAttrDataTypeUInt32 },
	{ "error_logfile_size",						NULL,	qtssAttrDataTypeUInt32 },
	{ "error_logfile_verbosity",				NULL,	qtssAttrDataTypeUInt32 },
	{ "screen_logging",							NULL,	qtssAttrDataTypeBool16 },
	{ "error_logging",							NULL,	qtssAttrDataTypeBool16 },
	{ "tcp_min_thin_delay_tolerance",			NULL,	qtssAttrDataTypeSInt32 },
	{ "tcp_max_thin_delay_tolerance",			NULL,	qtssAttrDataTypeSInt32 },
	{ "tcp_max_video_delay_tolerance",			NULL,	qtssAttrDataTypeSInt32 },
	{ "tcp_max_audio_delay_tolerance",			NULL,	qtssAttrDataTypeSInt32 },
	{ "min_tcp_buffer_size",					NULL,	qtssAttrDataTypeUInt32 },
	{ "max_tcp_buffer_size",					NULL,	qtssAttrDataTypeUInt32 },
	{ "tcp_seconds_to_buffer",					NULL,	qtssAttrDataTypeFloat32 },
	{ "do_report_http_connection_ip_address",	NULL,	qtssAttrDataTypeBool16 },
	{ "default_authorization_realm",			NULL,	qtssAttrDataTypeCharArray },
	{ "pid_file_name",							NULL,	qtssAttrDataTypeCharArray },
	{ "set_user_name",							NULL,	qtssAttrDataTypeCharArray },
	{ "set_group_name",							NULL,	qtssAttrDataTypeCharArray },
	{ "append_source_addr_in_transport",		NULL,	qtssAttrDataTypeBool16 },
	{ "rtsp_port",								NULL,	qtssAttrDataTypeUInt16 },
	
	{ "request_logging",						"QTSSAccessLogModule",	qtssAttrDataTypeBool16 },
	{ "request_logfile_name",					"QTSSAccessLogModule",	qtssAttrDataTypeCharArray },
	{ "request_logfile_dir",					"QTSSAccessLogModule",	qtssAttrDataTypeCharArray },
	{ "request_logfile_size",					"QTSSAccessLogModule",	qtssAttrDataTypeUInt32 },
	{ "request_logfile_interval",				"QTSSAccessLogModule",	qtssAttrDataTypeUInt32 },

	{ "history_update_interval",				"QTSSSvrControlModule",	qtssAttrDataTypeUInt32 },

	{ "buffer_seconds",							"QTSSFileModule",	qtssAttrDataTypeUInt32 },
	{ "sdp_url",								"QTSSFileModule",	qtssAttrDataTypeCharArray },
	{ "admin_email",							"QTSSFileModule",	qtssAttrDataTypeCharArray },
	{ "max_advance_send_time",					"QTSSFileModule",	qtssAttrDataTypeUInt32 },

	{ "reflector_delay",						"QTSSReflectorModule",	qtssAttrDataTypeUInt32 },
	{ "reflector_bucket_size",					"QTSSReflectorModule",	qtssAttrDataTypeUInt32 },

	{ "web_stats_url",							"QTSSWebStatsModule",	qtssAttrDataTypeCharArray },

	{ "loss_thin_tolerance",					"QTSSFlowControlModule",	qtssAttrDataTypeUInt32 },
	{ "num_losses_to_thin",						"QTSSFlowControlModule",	qtssAttrDataTypeUInt32 },
	{ "loss_thick_tolerance",					"QTSSFlowControlModule",	qtssAttrDataTypeUInt32 },
	{ "num_losses_to_thick",					"QTSSFlowControlModule",	qtssAttrDataTypeUInt32 },
	{ "num_worses_to_thin",						"QTSSFlowControlModule",	qtssAttrDataTypeUInt32 },

	{ "relay_stats_url",						"QTSSRelayModule",	qtssAttrDataTypeCharArray },
	{ "relay_prefs_file",						"QTSSRelayModule",	qtssAttrDataTypeCharArray },

	{ "num_conns_per_ip_addr",					"QTSSSpamDefenseModule",	qtssAttrDataTypeUInt32 },

	{ "modAccess_usersfilepath",				"QTSSAccessModule",	qtssAttrDataTypeCharArray },
	{ "modAccess_groupsfilepath",				"QTSSAccessModule",	qtssAttrDataTypeCharArray },
	{ "modAccess_qtaccessfilename",				"QTSSAccessModule",	qtssAttrDataTypeCharArray },

	//
	// This element will be used if the pref is something we don't know about.
	// Just have unknown prefs default to be server prefs with a type of char
	{ NULL, 									NULL, 				qtssAttrDataTypeCharArray }
};

int	GenerateAllXMLPrefs(FilePrefsSource* inPrefsSource, XMLPrefsParser* inXMLPrefs)
{
	for (UInt32 x = 0; x < inPrefsSource->GetNumKeys(); x++)
	{
		//
		// Get the name of this pref
		char* thePrefName = inPrefsSource->GetKeyAtIndex(x);

		//
		// Find the information corresponding to this pref in the above array
		UInt32 y = 0;
		for ( ; kPrefs[y].fPrefName != NULL; y++)
			if (::strcmp(thePrefName, kPrefs[y].fPrefName) == 0)
				break;
		
		char* theTypeString = (char*)QTSSDataConverter::GetDataTypeStringForType(kPrefs[y].fPrefType);
		SInt32 theXMLPrefIndex = inXMLPrefs->AddPref(kPrefs[y].fModuleName, thePrefName, theTypeString);

		char* theValue = inPrefsSource->GetValueAtIndex(x);
			
		static char* kTrue = "true";
		static char* kFalse = "false";
		
		//
		// If the pref is a Bool16, the new pref format uses "true" & "false",
		// the old one uses "enabled" and "disabled", so we have to explicitly convert.
		if (kPrefs[y].fPrefType == qtssAttrDataTypeBool16)
		{
			if (::strcmp(theValue, "enabled") == 0)
				theValue = kTrue;
			else
				theValue = kFalse;
		}
		inXMLPrefs->AddPrefValue((UInt32)theXMLPrefIndex, theValue);
	}
	
	return inXMLPrefs->WritePrefsFile();
}

int	GenerateStandardXMLPrefs(PrefsSource* inPrefsSource, XMLPrefsParser* inXMLPrefs)
{
	char thePrefBuf[1024];
	
	for (UInt32 x = 0; kPrefs[x].fPrefName != NULL; x++)
	{
		SInt32 theXMLPrefIndex = -1;
		
		char* theTypeString = (char*)QTSSDataConverter::GetDataTypeStringForType(kPrefs[x].fPrefType);
		if (inPrefsSource->GetValueByIndex(kPrefs[x].fPrefName, 0, thePrefBuf) != 0)
			theXMLPrefIndex = inXMLPrefs->AddPref(kPrefs[x].fModuleName, kPrefs[x].fPrefName, theTypeString);

		for (UInt32 y = 0; true; y++)
		{
			if (inPrefsSource->GetValueByIndex(kPrefs[x].fPrefName, y, thePrefBuf) == 0)
				break;
				
			//
			// If the pref is a Bool16, the new pref format uses "true" & "false",
			// the old one uses "enabled" and "disabled", so we have to explicitly convert.
			if (kPrefs[x].fPrefType == qtssAttrDataTypeBool16)
			{
				if (::strcmp(thePrefBuf, "enabled") == 0)
					::strcpy(thePrefBuf, "true");
				else
					::strcpy(thePrefBuf, "false");
			}
			inXMLPrefs->AddPrefValue((UInt32)theXMLPrefIndex, thePrefBuf);
		}
	}
	
	return inXMLPrefs->WritePrefsFile();
}