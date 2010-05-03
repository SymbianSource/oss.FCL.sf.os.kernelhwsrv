// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\defrag\t_ramdefrag.h
// 
//

#ifndef _T_RAMDEFRAG_H_
#define _T_RAMDEFRAG_H_

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

#define DEFRAG_TYPE_GEN 		(1)
#define DEFRAG_TYPE_EMPTY 		(2)
#define DEFRAG_TYPE_CLAIM		(3)

#define DEFRAG_VER_SYNC 		(1)
#define DEFRAG_VER_SEM	 		(2)
#define DEFRAG_VER_DFC			(3)

#define NO_FIXED_FLAG			(1)
#define NO_MOVE_FLAG			(2)
#define NO_DISCARD_FLAG			(3)
#define NO_ALLOC_FLAG			(4)
#define ONLY_DISCARD_FLAG		(5)
#define RESET_FLAG				(6)
#define ORIG_FLAG				(7)

#define FREE_ALL_FIXED			(0)
#define FILL_ALL_FIXED			(-1)

#define FREE_VALID				(0)
#define FREE_INVALID			(-1)
//#define DEBUG_VER
#ifdef DEBUG_VER
#define TESTDEBUG(a) {a;}
#else
#define TESTDEBUG(a) {}
#endif

struct SCapsPageMoveV01
	{
	TVersion	iVersion;
	};
	
struct STestRamDefragVars
	{
	TInt iRamSize;
	TInt iFreeRam;
	TInt iPageSize;
	TInt iRamUsed;
	TInt iNumFreePages;
	};

struct STestUserSidePageCount
	{
	TUint iFreePages;		/**<the number of free pages in the RAM zone*/
	TUint iFixedPages;		/**<the number of fixed pages in the RAM zone*/
	TUint iMovablePages;	/**<the number of movable pages in the RAM zone*/
	TUint iDiscardablePages;/**<the number of discardable pages in the RAM zone*/
	TUint iReserved[4];		/**<@internalAll reserved for internal use only*/
	};

struct STestPageCount
	{
	TUint iFreePages;
	TUint iUnknownPages;
	TUint iFixedPages;
	TUint iMovablePages;
	TUint iDiscardablePages;
	TUint iOtherPages;
	};

struct STestFlagParams
	{
	TUint	iZoneID;
	TUint8	iZoneFlag;
	TInt	iSetFlag;
	TUint8	iOptSetFlag;
	};


struct STestParameters
	{
	TInt iDefragVersion; 
	TInt iDefragType;
	TInt iPriority;
	TInt iMaxPages;
	TUint iID;
	TRequestStatus* iReqStat;
	TRequestStatus* iReqStat2;
	TRequestStatus* iReqStat3;
	};

// struct for passing multiple RAM zone IDs to the LDD.
struct SMultiZoneAlloc
	{
	TUint* iZoneId;			/**< An array of RAM zone IDs*/
	TUint iZoneIdSize;		/**< The number of IDs in iZoneId*/
	};

_LIT(KRamDefragFuncTestLddName,"D_RAMDEFRAG");

class RRamDefragFuncTestLdd : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EDoConsumeRamFinish,
		EDoSetDebugFlag,
		EAllocateFixed,
		EAllocFixedArray, 
		EAllocateFixed2,
		EGetAllocDiff,
		EFreeAllFixed,
		EAllocateFixedWrite,
		EFreeAllFixedRead,
		EPageCount,
		EZoneAllocContiguous,
		EMultiZoneAllocContiguous,
		EZoneAllocDiscontiguous,
		EMultiZoneAllocDiscontiguous,
		EZoneAllocDiscontiguous2,
		EZoneAllocToMany,
		EZoneAllocToManyArray,
		EZoneAllocToMany2,
		EAllocContiguous,
		EFreeZone,
		EFreeZoneId,
		EFreeFromAllZones,
		EFreeFromAddr, 
		ECheckCancel,
		ECheckPriorities,
		ECallDefrag,
		ESetZoneFlag,
		EGetDefragOrder,
		EResetDriver,
		};

#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{ return DoCreate(KRamDefragFuncTestLddName,TVersion(),KNullUnit,NULL,NULL,EOwnerProcess,ETrue); }

	inline TInt ResetDriver()
		{ return DoControl(EResetDriver,(TAny*)NULL, (TAny*)NULL); }

	inline TInt DoSetDebugFlag(TInt aState)
		{ return DoControl(EDoSetDebugFlag,(TAny*)aState, (TAny*)NULL); }

	inline TInt AllocateFixed(TInt aNumOfPages)
		{ return DoControl(EAllocateFixed,(TAny*)aNumOfPages, (TAny*)NULL); }

	inline TInt AllocFixedArray(TInt aNumOfPages)
		{ return DoControl(EAllocFixedArray,(TAny*)aNumOfPages, (TAny*)NULL); }

	inline TInt AllocateFixed2(TInt aNumOfPages)
		{ return DoControl(EAllocateFixed2,(TAny*)aNumOfPages, (TAny*)NULL); }

	inline TInt GetAllocDiff(TInt aNumOfPages)
		{ return DoControl(EGetAllocDiff,(TAny*)aNumOfPages, (TAny*)NULL); }

	inline TInt FreeAllFixedPages()
		{ return DoControl(EFreeAllFixed); }

	inline TInt AllocateFixedWrite(TInt aNumOfPages)
		{ return DoControl(EAllocateFixedWrite,(TAny*)aNumOfPages, (TAny*)NULL); }

	inline TInt FreeAllFixedPagesRead()
		{ return DoControl(EFreeAllFixedRead,(TAny*)NULL, (TAny*)NULL); }

	inline TInt ZoneAllocContiguous(TUint aZoneID, TInt aSize)
		{ return DoControl(EZoneAllocContiguous,(TAny*)aZoneID, (TAny*)aSize); }

	inline TInt MultiZoneAllocContiguous(TUint* aZoneId, TUint aZoneIdSize, TInt aSize)
		{ 
		SMultiZoneAlloc multiZoneAlloc;
		multiZoneAlloc.iZoneId = aZoneId;
		multiZoneAlloc.iZoneIdSize = aZoneIdSize;
		return DoControl(EMultiZoneAllocContiguous,(TAny*)&multiZoneAlloc, (TAny*)aSize); 
		}

	inline TInt ZoneAllocDiscontiguous(TUint aZoneID, TInt aNumPages)
		{ return DoControl(EZoneAllocDiscontiguous,(TAny*)aZoneID, (TAny*)aNumPages); }

	inline TInt MultiZoneAllocDiscontiguous(TUint* aZoneId, TUint aZoneIdSize, TInt aNumPages)
		{ 
		SMultiZoneAlloc multiZoneAlloc;
		multiZoneAlloc.iZoneId = aZoneId;
		multiZoneAlloc.iZoneIdSize = aZoneIdSize;
		return DoControl(EMultiZoneAllocDiscontiguous,(TAny*)&multiZoneAlloc, (TAny*)aNumPages);
		}

	inline TInt ZoneAllocDiscontiguous2(TUint aZoneID, TInt aNumPages)
		{ return DoControl(EZoneAllocDiscontiguous2,(TAny*)aZoneID, (TAny*)aNumPages); }

	inline TInt ZoneAllocToMany(TInt aZoneIndex, TInt aNumPages)
		{ return DoControl(EZoneAllocToMany,(TAny*)aZoneIndex, (TAny*)aNumPages); }

	inline TInt ZoneAllocToManyArray(TInt aZoneIndex, TInt aNumPages)
		{ return DoControl(EZoneAllocToManyArray,(TAny*)aZoneIndex, (TAny*)aNumPages); }

	inline TInt ZoneAllocToMany2(TInt aZoneIndex, TInt aNumPages)
		{ return DoControl(EZoneAllocToMany2,(TAny*)aZoneIndex, (TAny*)aNumPages); }
	
	inline TInt AllocContiguous(TInt aSize)
		{ return DoControl(EAllocContiguous,(TAny*)aSize, (TAny*)NULL); }

	inline TInt FreeZone(TInt aNumPages)
		{ return DoControl(EFreeZone,(TAny*)aNumPages, (TAny*)NULL); }

	inline TInt FreeFromAllZones()
		{ return DoControl(EFreeFromAllZones,(TAny*)NULL, (TAny*)NULL); }

	inline TInt FreeZoneId(TUint aZoneId)
		{ return DoControl(EFreeZoneId, (TAny*)aZoneId); }

	inline TInt CheckCancel(TInt aDefragType, TUint aID = 0)
		{ 
		STestParameters params;
		
		params.iDefragType = aDefragType;
		params.iDefragVersion = 0;
		params.iID = aID;
		params.iPriority = 0;
		params.iMaxPages = 0;
		params.iReqStat = NULL; 
		params.iReqStat2 = NULL; 
		params.iReqStat3 = NULL; 
		return DoControl(ECheckCancel,(TAny*)&params); 
		}

	inline TInt FreeFromAddr(TInt aNumPages, TUint32 aAddr)
		{ return DoControl(EFreeFromAddr,(TAny*)aNumPages, (TAny*)aAddr); }

	inline TInt PageCount(TUint aID, STestUserSidePageCount* aPageData)
		{ return DoControl(EPageCount,(TAny*)aID, (TAny*)aPageData); }

	inline TInt CallDefrag(TInt aDefragType, TInt aDefragVersion, TUint aID = 0, TInt aMaxPages = 0, TInt aPriority = -1, TRequestStatus* aReqStat = NULL)
		{ 
		STestParameters params;
		
		params.iDefragType = aDefragType;
		params.iDefragVersion = aDefragVersion;
		params.iID = aID;
		params.iPriority = aPriority;
		params.iMaxPages = aMaxPages;
		params.iReqStat = aReqStat;
		if (aReqStat)
			{
			*aReqStat = KRequestPending;
			}
		params.iReqStat2 = NULL; 
		params.iReqStat3 = NULL; 
		TInt ret = DoControl(ECallDefrag,(TAny*)&params); 
		return ret;
		}

	/**
	@param aZoneID		The ID of the RAM zone to modify
	@param aZoneFlag	The flags to clear, if want only aSetFlags to be enabled after this then
						aZoneFlag should be the current flags sets on the RAM zone.
	@param aSetFlag		The flags to set
	@param aOptSetFlag	If aSetFlag==ORIG_FLAG then aOptSetFlag will be the flags to set 
						on the RAM zone.
	*/
	inline TInt SetZoneFlag(TUint aZoneID, TUint8 aZoneFlag, TInt aSetFlag, TUint8 aOptSetFlag = 0x00)
		{ 
		STestFlagParams flagParams;
		
		flagParams.iZoneID = aZoneID;
		flagParams.iZoneFlag = aZoneFlag;
		flagParams.iSetFlag = aSetFlag;
		flagParams.iOptSetFlag = aOptSetFlag;
		
		return DoControl(ESetZoneFlag,(TAny*)&flagParams, (TAny*)NULL); 
		}

	inline TInt CheckPriorities(TInt aDefragType, TUint aID = 0, TRequestStatus* aReqStat = NULL, TRequestStatus* aReqStat2 = NULL, TRequestStatus* aReqStat3 = NULL)
		{ 
		STestParameters params;
		
		params.iDefragType = aDefragType;
		params.iDefragVersion = 0;
		params.iID = aID;
		params.iPriority = 0;
		params.iMaxPages = 0;
		params.iReqStat = aReqStat;
		if (aReqStat)
			{
			*aReqStat = KRequestPending;
			}
		params.iReqStat2 = aReqStat2; 
		if (aReqStat2)
			{
			*aReqStat2 = KRequestPending;
			}
		params.iReqStat3 = aReqStat3;  
		if (aReqStat3)
			{
			*aReqStat3 = KRequestPending;
			}

		return DoControl(ECheckPriorities,(TAny*)&params); 
		}

	inline TInt GetDefragOrder()
		{ return DoControl(EGetDefragOrder,(TAny*)NULL, (TAny*)NULL); }


#endif
	};


#endif //_T_RAMDEFRAG_H_
