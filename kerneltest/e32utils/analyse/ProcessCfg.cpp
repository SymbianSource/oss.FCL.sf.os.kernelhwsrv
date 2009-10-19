/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
#include <windows.h>
#include "analyse.h"
#include "nonxip.h"


extern NonXIP gNonXIP;

const char* COMMON_SECT="Common";
const char* PROFILE_SECT="Profile";
const char* PARTITION_SECT="Partition";
const char* FORMAT_SECT="Format";
const char* NON_XIP_SECT="NonXIP";

std::string Analyse::iRomFile;
std::string Analyse::iThread;
std::string Analyse::iDll;
std::string Analyse::iFunction;
std::vector<std::string> Analyse::iTraces;

int Analyse::ProcessCfgFile(const char* aCfgFileName)
	{
	char buf[1024];
	char path[MAX_PATH];
	if (aCfgFileName[0] != '\\' || aCfgFileName[1] != ':')
		{
		char ** file_name = 0;
		if (!GetFullPathNameA(aCfgFileName, sizeof path, path, file_name))
			return ENoCfgFile;
		aCfgFileName = path;
		}

	if (GetFileAttributesA(aCfgFileName) == 0xffffffff) // no ini file
		return ENoCfgFile;
	int rc = GetPrivateProfileStringA(COMMON_SECT, "TraceFile", "", buf, sizeof buf, aCfgFileName);
	if (rc)
		{
		iTraces.push_back(buf);
		sTraces.push_back(iTraces[iTraces.size()-1].c_str());
		}

	GetPrivateProfileStringA(COMMON_SECT, "Mode", "profile", buf, sizeof buf, aCfgFileName);
	switch(buf[0])
		{
	case 'l': case 'L':
		sAction = ETrace;
		break;
	case 'p': case 'P':
		sAction = EProfile;
		break;
	case 'a': case 'A':
		sAction = EActivity;
		break;
	default:
		return EErrorCfgFile;
		}

	rc = GetPrivateProfileStringA(COMMON_SECT, "SymbolFile", "", buf, sizeof buf, aCfgFileName);
	if (rc) 
		{
		iRomFile = buf;
		sRomFile = iRomFile.c_str();
		}

	rc = GetPrivateProfileStringA(COMMON_SECT, "Range", "", buf, sizeof buf, aCfgFileName);
	if (rc) 
		{
		sOptions |= ERange;
		bool plus = false;
		char * p = strrchr(buf, '+');
		if (p)
			plus = true;
		else
			p = strrchr(buf, '-');
		if (!p) return EErrorCfgFile;
		*p = '\0';
		sBeginSample = atoi(buf);
		sEndSample = atoi(++p);
		if (plus) 
			sEndSample = sEndSample += sBeginSample;
		}

	rc = GetPrivateProfileIntA(COMMON_SECT, "IncludeNullThread", 0, aCfgFileName);
	if (rc) 
		sOptions|=ENull;
	
	rc = GetPrivateProfileStringA(COMMON_SECT, "Cutoff", "", buf, sizeof buf, aCfgFileName);
	if (rc) 
		sCutOff = atof(buf);


	rc = GetPrivateProfileStringA(PROFILE_SECT, "Thread", "", buf, sizeof buf, aCfgFileName);
	if (rc) 
		{
		iThread = buf;
		sThread = iThread.c_str();
		}

	rc = GetPrivateProfileStringA(PROFILE_SECT, "Dll", "", buf, sizeof buf, aCfgFileName);
	if (rc) 
		{
		iDll = buf;
		sDll = iDll.c_str();
		}

	rc = GetPrivateProfileStringA(PROFILE_SECT, "Function", "", buf, sizeof buf, aCfgFileName);
	if (rc) 
		{
		iFunction = buf;
		sFunction = iFunction.c_str();
		}

	rc = GetPrivateProfileStringA(PROFILE_SECT, "Range", "", buf, sizeof buf, aCfgFileName);
	if (rc) 
		{
		sOptions |= EAddress;
		bool plus = false;
		char * p = strrchr(buf, '+');
		if (p)
			plus = true;
		else
			p = strrchr(buf, '-');
		if (!p) return EErrorCfgFile;
		*p = '\0';
		sBase = strtoul(buf,0,16);
		sLim = strtoul(++p,0,16);
		if (plus) 
			sLim = sLim += sBase;
		}
	

	rc = GetPrivateProfileStringA(PARTITION_SECT, "Mode", "", buf, sizeof buf, aCfgFileName);
	if (rc)
		{
		switch(buf[0])
			{
		case 'd': case 'D':
			sPartition = EDll;
			break;
		case 'f': case 'F':
			sPartition = EFunction;
			break;
		default:
			return EErrorCfgFile;
			}
		}

	rc = GetPrivateProfileIntA(PARTITION_SECT, "BucketSize", 0, aCfgFileName);
	if (rc)
		{
		sPartition = ESize;
		sBucketSize = rc;
		}

	rc = GetPrivateProfileIntA(PARTITION_SECT, "NumberOfBuckets", 0, aCfgFileName);
	if (rc)
		{
		sPartition = EBuckets;
		sBuckets = rc;
		}



	rc = GetPrivateProfileStringA(FORMAT_SECT, "Mode", "", buf, sizeof buf, aCfgFileName);
	if (rc)
		{
		switch(buf[0])
			{
		case 'p': case 'P':
			sFormat = EPercent;
			break;
		case 's': case 'S':
			sFormat = ESamples;
			break;
		case 'e': case 'E':
			sFormat = EExcel;
			break;
		default:
			return EErrorCfgFile;
			}
		}

	rc = GetPrivateProfileIntA(FORMAT_SECT, "ZeroValues", 0, aCfgFileName);
	if (rc)
		sOptions |= EZeros;

	rc = GetPrivateProfileIntA(FORMAT_SECT, "NoOthers", 0, aCfgFileName);
	if (rc)
		sOptions |= ENoOther;

	rc = GetPrivateProfileIntA(FORMAT_SECT, "TotalOnly", 0, aCfgFileName);
	if (rc)
		sOptions |= ETotalOnly;


	char key[256];
	for(int i=1;sprintf(key,"ObyFile%d",i),
		GetPrivateProfileStringA(NON_XIP_SECT, key, "", buf, sizeof buf, aCfgFileName);i++)
		gNonXIP.AddObyFile(buf);

	for(i=1;sprintf(key,"RofsSymbolFile%d",i),
		GetPrivateProfileStringA(NON_XIP_SECT, key, "", buf, sizeof buf, aCfgFileName);i++)
		gNonXIP.AddSymbolFile(buf);

	return EOk;
	}

