// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\memmodel\epoc\mmubase\defrag.h
// 
//

#ifndef __DEFRAG_H__
#define __DEFRAG_H__

/******************************************************************************
 * Base class for RAM defragmentation implementation
 ******************************************************************************/

struct SZone;
class DRamAllocator;
/**
@internalComponent
*/
class Defrag
	{
public:
	enum TPanic
		{
		EDfcQInitFailed=0,
		};

public:
	
	// platform independent - defragbase.cpp
	Defrag();
	void Init3(DRamAllocator* aRamAllocator);
	static void DefragTask(TAny* aArg);
private:
	TInt GeneralDefrag(TRamDefragRequest* aRequest);
	TInt ClaimRamZone(TRamDefragRequest* aRequest);
	TInt EmptyRamZone(TRamDefragRequest* aRequest);
	TInt ClearZone(SZone& aZone, TUint aMaxRetries, TRamDefragRequest* aRequest);
	TInt ClearMovableFromZone(SZone& aZone, TBool aBestEffort, TRamDefragRequest* aRequest);
	TInt ClearDiscardableFromZone(SZone& aZone, TBool aBestEffort, TRamDefragRequest* aRequest, TUint* aMaxDiscard=NULL);
	static void Panic(TPanic aPanic);

	// data

private:
	TInt iDefragPriority;
	TDfcQue iTaskQ;
	DRamAllocator* iRamAllocator;

	static Defrag* TheDefrag; // single instance of the defragmentor

	friend class TRamDefragRequest;	// This needs access to iDefragPriority and iTaskQ
	};

#endif
