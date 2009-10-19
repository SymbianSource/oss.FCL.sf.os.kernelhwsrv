// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\dobject.h
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __DOBJECT_H__
#define __DOBJECT_H__

#ifdef _DEBUG
// In DEBUG builds use linear growth by 1 to aid kernel heap checking
#else
const TInt KObjectConMinSize=8;
#endif
const TInt KObjectIxGranularity=8;
const TInt KObjectIndexMask=0x7fff;
const TInt KObjectMaxIndex=0x7fff;
const TInt KObjectInstanceShift=16;
const TInt KObjectInstanceMask=0x3fff;
const TInt KObjectIxMaxHandles=0x8000;

inline TInt index(TInt aHandle)
	{return(aHandle&KObjectIndexMask);}
inline TInt instance(TInt aHandle)
	{return((aHandle>>KObjectInstanceShift)&KObjectInstanceMask);}
inline TInt instanceLimit(TInt& aCount)
	{return ((aCount&KObjectInstanceMask)==0) ? ((++aCount)&KObjectInstanceMask) : aCount&KObjectInstanceMask;}
inline TInt makeHandle(TInt aIndex, TInt aInstance)
	{return((TInt)((aInstance<<KObjectInstanceShift)|aIndex));}

enum TDObjectPanic
	{
	EObjObjectStillReferenced,
	EObjNegativeAccessCount,
	EObjRemoveObjectNotFound,
	EObjRemoveContainerNotFound,
	EObjRemoveBadHandle,
	EObjFindBadHandle,
	EObjFindIndexOutOfRange,
	EDObjectConDestroyed,
	EArrayIndexOutOfRange,
	EObjInconsistent,
	};

inline void Panic(TDObjectPanic aPanic)
	{ Kern::Fault("DOBJECT",aPanic); }


#endif
