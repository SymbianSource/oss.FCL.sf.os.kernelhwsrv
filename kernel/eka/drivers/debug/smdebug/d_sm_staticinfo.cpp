// Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// Responsible for dealing with static info in the Stop Mode framework
//

/**
 * @file
 * @internalComponent
 * @prototype
 */

#include <sm_debug_api.h>
#include <e32rom.h>

using namespace Debug;

const TInt KBUFSIZE = 128;

/**
 * Stop Mode routine to retrieve the static info and place it in the response buffer
 * @param aItem List item describing the list
 * @return One of the system wide error codes
 */
TInt StopModeDebug::GetStaticInfo(const TListItem* aItem, bool aCheckConsistent)
	{
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("\nDumping the static information"));

	if(aItem->iListScope != EScopeGlobal)
		{
		return KErrArgument; //No other scope makes sense for static info
		}

	TUint8* buffer = (TUint8*)aItem->iBufferAddress;
	TUint8* bufferPos = Align4(buffer + sizeof(TListReturn));
	TUint8* bufferEnd = buffer + aItem->iBufferSize;

	TListReturn* listResp = (TListReturn*)buffer;
	listResp->iReqNo = EStaticInfo;
	listResp->iNumberItems = 1; //there in only one structure of TStaticListEntry
	listResp->iDataSize = sizeof(TStaticListEntry);
	
	if (bufferPos < bufferEnd)
		{
		// making sure we have enough space to write the static info
		TStaticListEntry& entry = *(TStaticListEntry*)(bufferPos);

		// build version and rom build time 
		TRomHeader rHdr = Epoc::RomHeader();
		entry.iTime = rHdr.iTime; 
		entry.iBuildNumber = rHdr.iVersion.iBuild;
		entry.iMajorVersion = rHdr.iVersion.iMajor;
		entry.iMinorVersion = rHdr.iVersion.iMinor;

		//Number of CPUs
		entry.iCpuNumbers = NKern::NumberOfCpus();

		TBuf<KBUFSIZE> bufTime64; 
		bufTime64.Num(rHdr.iTime); 

		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Version%d.%02d(%03d) %d", rHdr.iVersion.iMajor, rHdr.iVersion.iMinor, rHdr.iVersion.iBuild));
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Build Time %S ms", &bufTime64));
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("Number of CPUs in the system: %d", entry.iCpuNumbers));
		}

	return KErrNone;
	}


