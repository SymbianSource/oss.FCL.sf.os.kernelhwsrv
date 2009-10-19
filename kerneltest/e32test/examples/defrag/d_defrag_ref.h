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
// e32test\defrag\d_defrag.h
// 
//

#if !defined(__D_DEFRAG_H__)
#define __D_DEFRAG_H__
#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif


_LIT(KLddName,"DefragRef");

class TCapsDefragTestV01
	{
public:
	TVersion	iVersion;
	};

/** User-side interface to the defrag LDD
*/
class RDefragChannel : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlGeneralDefragDfc,
		EControlGeneralDefragDfcComplete,
		EControlGeneralDefragSem,
		EControlGeneralDefrag,
		EControlAllocLowestZone,
		EControlClaimLowestZone,
		EControlCloseChunk,
		};
		
public:
	inline TInt Open();
	inline TInt GeneralDefragDfc(TRequestStatus* aReq);
	inline TInt GeneralDefragDfcComplete();
	inline TInt GeneralDefrag();
	inline TInt GeneralDefragSem();
	inline TInt AllocLowestZone();
	inline TInt ClaimLowestZone();
	inline TInt CloseChunk();
	};


#ifndef __KERNEL_MODE__
inline TInt RDefragChannel::Open()
	{
	return DoCreate(KLddName,TVersion(0,1,1),KNullUnit,NULL,NULL);
	}

inline TInt RDefragChannel::GeneralDefragDfc(TRequestStatus* aReq)
	{ 
	if (aReq != NULL)
		{
		*aReq = KRequestPending;
		}
	return DoControl(EControlGeneralDefragDfc, (TAny*)aReq);
	}

inline TInt RDefragChannel::GeneralDefragDfcComplete()
	{
	return DoControl(EControlGeneralDefragDfcComplete);
	}

inline TInt RDefragChannel::GeneralDefragSem()
	{ return DoControl(EControlGeneralDefragSem); }

inline TInt RDefragChannel::GeneralDefrag()
	{ return DoControl(EControlGeneralDefrag); }

inline TInt RDefragChannel::AllocLowestZone()
	{ return DoControl(EControlAllocLowestZone); }

inline TInt RDefragChannel::ClaimLowestZone()
	{ return DoControl(EControlClaimLowestZone);}

inline TInt RDefragChannel::CloseChunk()
	{ return DoControl(EControlCloseChunk);	}
#endif

#endif
