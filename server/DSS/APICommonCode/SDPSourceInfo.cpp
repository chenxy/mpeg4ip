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
	File:		SDPSourceInfo.cpp

	Contains:	Implementation of object defined in .h file

	Copyright:	� 1998 by Apple Computer, Inc., all rights reserved.


*/

#include "SDPSourceInfo.h"

#include "StringParser.h"
#include "StringFormatter.h"
#include "OSMemory.h"
#include "SocketUtils.h"
#include "StrPtrLen.h"

static StrPtrLen	sCLine("c=IN IP4 0.0.0.0\r\na=control:*\r\n");
static StrPtrLen	sVideoStr("video");
static StrPtrLen	sAudioStr("audio");
static StrPtrLen	sRtpMapStr("rtpmap");
static StrPtrLen	sControlStr("control");
static StrPtrLen	sBufferDelayStr("x-bufferdelay");

SDPSourceInfo::~SDPSourceInfo()
{
	if (fStreamArray != NULL)
	{
		char* theCharArray = (char*)fStreamArray;
		delete [] theCharArray;
	}
}

char* SDPSourceInfo::GetLocalSDP(UInt32* newSDPLen)
{
	Bool16 appendCLine = true;
	UInt32 trackIndex = 0;
	
	char *localSDP = NEW char[fSDPData.Len * 2];
	StringFormatter localSDPFormatter(localSDP, fSDPData.Len * 2);

	StrPtrLen sdpLine;
	StringParser sdpParser(&fSDPData);
	char trackIndexBuffer[50];
	
	// Only generate our own trackIDs if this file doesn't have 'em.
	// Our assumption here is that either the file has them, or it doesn't.
	// A file with some trackIDs, and some not, won't work.
	Bool16 hasTrackIDs = false;

	while (sdpParser.GetDataRemaining() > 0)
	{
		//stop when we reach an empty line.
		sdpParser.GetThruEOL(&sdpLine);
		if (sdpLine.Len == 0)
			continue;
			
		switch (*sdpLine.Ptr)
		{
			case 'c':
				break;//ignore connection information
			case 'm':
			{
				//append new connection information right before the first 'm'
				if (appendCLine)
				{
					localSDPFormatter.Put(sCLine);
					appendCLine = false;
				}
				//the last "a=" for each m should be the control a=
				if ((trackIndex > 0) && (!hasTrackIDs))
				{
					::sprintf(trackIndexBuffer, "a=control:trackID=%ld\r\n",trackIndex);
					localSDPFormatter.Put(trackIndexBuffer, ::strlen(trackIndexBuffer));
				}
				//now write the 'm' line, but strip off the port information
				StringParser mParser(&sdpLine);
				StrPtrLen mPrefix;
				mParser.ConsumeUntil(&mPrefix, StringParser::sDigitMask);
				localSDPFormatter.Put(mPrefix);
				localSDPFormatter.Put("0", 1);
				(void)mParser.ConsumeInteger(NULL);
				localSDPFormatter.Put(mParser.GetCurrentPosition(), mParser.GetDataRemaining());
				localSDPFormatter.PutEOL();
				trackIndex++;
				break;
			}
			case 'a':
			{
				localSDPFormatter.Put(sdpLine);
				localSDPFormatter.PutEOL();

				StringParser aParser(&sdpLine);
				aParser.ConsumeLength(NULL, 2);//go past 'a='
				StrPtrLen aLineType;
				aParser.ConsumeWord(&aLineType);
				if (aLineType.Equal(sControlStr))
					hasTrackIDs = true;
				break;
			}
			default:
			{
				localSDPFormatter.Put(sdpLine);
				localSDPFormatter.PutEOL();
			}
		}
	}
	
	if ((trackIndex > 0) && (!hasTrackIDs))
	{
		::sprintf(trackIndexBuffer, "a=control:trackID=%ld\r\n",trackIndex);
		localSDPFormatter.Put(trackIndexBuffer, ::strlen(trackIndexBuffer));
	}
	*newSDPLen = (UInt32)localSDPFormatter.GetCurrentOffset();
	return localSDP;
}


void SDPSourceInfo::Parse(char* sdpData, UInt32 sdpLen)
{
	Assert(fSDPData.Ptr == NULL);
	Assert(fStreamArray == NULL);
	
	fSDPData.Set(sdpData, sdpLen);

	// If there is no trackID information in this SDP, we make the track IDs start
	// at 1 -> N
	UInt32 currentTrack = 1;
	
	Bool16 hasGlobalStreamInfo = false;
	StreamInfo theGlobalStreamInfo; //needed if there is one c= header independent of
									//individual streams

	StrPtrLen sdpLine;
	StringParser trackCounter(&fSDPData);
	StringParser sdpParser(&fSDPData);
	UInt32 theStreamIndex = 0;

	//walk through the SDP, counting up the number of tracks
	// Repeat until there's no more data in the SDP
	while (trackCounter.GetDataRemaining() > 0)
	{
		//each 'm' line in the SDP file corresponds to another track.
		trackCounter.GetThruEOL(&sdpLine);
		if ((sdpLine.Len > 0) && (sdpLine.Ptr[0] == 'm'))
			fNumStreams++;	
	}

	//We should scale the # of StreamInfos to the # of trax, but we can't because
	//of an annoying compiler bug...
	
	fStreamArray = (StreamInfo*)NEW char[(fNumStreams + 1) * sizeof(StreamInfo)];
	::memset(fStreamArray, 0, (fNumStreams + 1) * sizeof(StreamInfo));
	
	//Set bufferdelay to default of 3
	theGlobalStreamInfo.fBufferDelay = (Float32) eDefaultBufferDelay;
	
	//Now actually get all the data on all the streams
	while (sdpParser.GetDataRemaining() > 0)
	{
		sdpParser.GetThruEOL(&sdpLine);
		if (sdpLine.Len == 0)
			continue;//skip over any blank lines

		switch (*sdpLine.Ptr)
		{
			case 'm':
			{
				if (hasGlobalStreamInfo)
				{
					fStreamArray[theStreamIndex].fDestIPAddr = theGlobalStreamInfo.fDestIPAddr;
					fStreamArray[theStreamIndex].fTimeToLive = theGlobalStreamInfo.fTimeToLive;
				}
				fStreamArray[theStreamIndex].fTrackID = currentTrack;
				currentTrack++;
				
				StringParser mParser(&sdpLine);
				
				//find out what type of track this is
				mParser.ConsumeLength(NULL, 2);//go past 'm='
				StrPtrLen theStreamType;
				mParser.ConsumeWord(&theStreamType);
				if (theStreamType.Equal(sVideoStr))
					fStreamArray[theStreamIndex].fPayloadType = qtssVideoPayloadType;
				else if (theStreamType.Equal(sAudioStr))
					fStreamArray[theStreamIndex].fPayloadType = qtssAudioPayloadType;
					
				//find the port for this stream
				mParser.ConsumeUntil(NULL, StringParser::sDigitMask);
				SInt32 tempPort = mParser.ConsumeInteger(NULL);
				if ((tempPort > 0) && (tempPort < 65536))
					fStreamArray[theStreamIndex].fPort = tempPort;
					
				// find out whether this is TCP or UDP
				mParser.ConsumeWhitespace();
				StrPtrLen transportID;
				mParser.ConsumeWord(&transportID);
				
				static const StrPtrLen kTCPTransportStr("RTP/AVP/TCP");
				if (transportID.Equal(kTCPTransportStr))
					fStreamArray[theStreamIndex].fIsTCP = true;
					
				theStreamIndex++;
			}
			break;
			case 'a':
			{
				//if we haven't even hit an 'm' line yet, just ignore all 'a' lines
				if (theStreamIndex == 0)
					break;
					
				StringParser aParser(&sdpLine);
				aParser.ConsumeLength(NULL, 2);//go past 'a='
				StrPtrLen aLineType;
				aParser.ConsumeWord(&aLineType);
				
				if (aLineType.Equal(sRtpMapStr))
				{
					//mark the codec type if this line has a codec name on it. If we already
					//have a codec type for this track, just ignore this line
					if ((fStreamArray[theStreamIndex - 1].fPayloadName.Len == 0) &&
						(aParser.GetThru(NULL, ' ')))
						(void)aParser.GetThruEOL(&fStreamArray[theStreamIndex - 1].fPayloadName);
				}
				else if (aLineType.Equal(sControlStr))
				{			
					//mark the trackID if that's what this line has
					aParser.ConsumeUntil(NULL, StringParser::sDigitMask);
					fStreamArray[theStreamIndex - 1].fTrackID = aParser.ConsumeInteger(NULL);
				}
				else if (aLineType.Equal(sBufferDelayStr))
				{	// if a BufferDelay is found then set all of the streams to the same buffer delay (it's global)
					aParser.ConsumeUntil(NULL, StringParser::sDigitMask);
					theGlobalStreamInfo.fBufferDelay = aParser.ConsumeFloat();
				}
			}
			break;
			case 'c':
			{
				//get the IP address off this header
				StringParser cParser(&sdpLine);
				cParser.ConsumeLength(NULL, 9);//strip off "c=in ip4 "
				UInt32 tempIPAddr = SDPSourceInfo::GetIPAddr(&cParser, '/');
								
				//grab the ttl
				SInt32 tempTtl = kDefaultTTL;
				if (cParser.GetThru(NULL, '/'))
				{
					tempTtl = cParser.ConsumeInteger(NULL);
					Assert(tempTtl >= 0);
					Assert(tempTtl < 65536);
				}

				if (theStreamIndex > 0)
				{
					//if this c= line is part of a stream, it overrides the
					//global stream information
					fStreamArray[theStreamIndex - 1].fDestIPAddr = tempIPAddr;
					fStreamArray[theStreamIndex - 1].fTimeToLive = tempTtl;
				} else {
					theGlobalStreamInfo.fDestIPAddr = tempIPAddr;
					theGlobalStreamInfo.fTimeToLive = tempTtl;
					hasGlobalStreamInfo = true;
				}
			}
		}
	}		
	
	// Add the default buffer delay
	Float32 bufferDelay = (Float32) eDefaultBufferDelay;
	if (theGlobalStreamInfo.fBufferDelay != (Float32) eDefaultBufferDelay)
		bufferDelay = theGlobalStreamInfo.fBufferDelay;
	
	UInt32 count = 0;
	while (count < fNumStreams)
	{	fStreamArray[count].fBufferDelay = bufferDelay;
		count ++;
	}
		
}

UInt32 SDPSourceInfo::GetIPAddr(StringParser* inParser, char inStopChar)
{
	StrPtrLen ipAddrStr;

	// Get the IP addr str
	inParser->ConsumeUntil(&ipAddrStr, inStopChar);
	
	// NULL terminate it
	char endChar = ipAddrStr.Ptr[ipAddrStr.Len];
	ipAddrStr.Ptr[ipAddrStr.Len] = '\0';
	
	//inet_addr returns numeric IP addr in network byte order, make
	//sure to convert to host order.
	UInt32 ipAddr = SocketUtils::ConvertStringToAddr(ipAddrStr.Ptr);
	
	// Make sure to put the old char back!
	ipAddrStr.Ptr[ipAddrStr.Len] = endChar;
	
	return ipAddr;
}