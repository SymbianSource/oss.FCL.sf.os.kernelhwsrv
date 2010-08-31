/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: Contains TDmaCapability and associated definitions.
* These are used by the DMA tests in test_cases.cpp to express dependancies
* on various DMA controller/channel capabilities
*
*/
#ifndef __CAP_REQS_H__
#define __CAP_REQS_H__

#include <e32std.h>

/**
The various types of requirement on a
value that can be specified by a TDmaCapability
*/
enum TCapsReqType
		{
		EEqual, EGTE /* >= */, ELTE /* <= */, EBitsSet, EBitsClear
		};

/**
Enumerates all the various DMA channel capabilities
*/
enum TCapsReq
		{
		ENone,
		EChannelPriorities,
		EChannelPauseAndResume,
		EAddrAlignedToElementSize,
		E1DAddressing,
		E2DAddressing,
		ESynchronizationTypes,
		EBurstTransactions,
		EDescriptorInterrupt,
		EFrameInterrupt,
		ELinkedListPausedInterrupt,
		EEndiannessConversion,
		EGraphicsOps,
		ERepeatingTransfers,
		EChannelLinking,
		EHwDescriptors,
		ESrcDstAsymmetry,
		EAsymHwDescriptors,
		EBalancedAsymSegments,
		EAsymCompletionInterrupt,
		EAsymDescriptorInterrupt,
		EAsymFrameInterrupt,
		EPilVersion,
		};

enum TResult {ERun=0, ESkip=1, EFail=2}; //The ordering of these should not be changed

struct SDmacCaps;
struct TDmacTestCaps;

/**
Represents a requirement for some DMA capability
to be either present or not present, less than, equal to, or
greater than some value, or to have certain bits in a mask
set or unset.
*/
struct TDmaCapability
	{
	TDmaCapability()
		:iCapsReq(ENone), iCapsReqType(EEqual), iValue(ETrue), iFail(EFalse)
		{}

	TDmaCapability(TCapsReq aReq, TCapsReqType aReqType, TUint aValue, TBool aFail)
		:iCapsReq(aReq), iCapsReqType(aReqType), iValue(aValue), iFail(aFail)
		{}

	static void SelfTest();

	/**
	Compares the requirements held in the struct
	against those described in aChannelCaps and makes a decision
	as to whether this test case should be run, skipped, or failed.
	*/
	TResult CompareToDmaCaps(const SDmacCaps& aChannelCaps) const;
	TResult CompareToDmaCaps(const TDmacTestCaps& aChannelCaps) const;

private:
	TBool RequirementSatisfied(const SDmacCaps& aChannelCaps) const;
	TBool RequirementSatisfied(const TDmacTestCaps& aChannelCaps) const;

	TBool TestValue(TUint aValue) const;

public:
	TCapsReq		iCapsReq;
	TCapsReqType	iCapsReqType;
	TUint			iValue;
	// if HW capability is not available:-
	// 	ETrue - Fail the test
	//	EFalse - Skip the test
	TBool			iFail;
	};

//A set of DMA capability requirements
const TDmaCapability none(ENone, EEqual, 0, ETrue);

const TDmaCapability pauseRequired(EChannelPauseAndResume, EEqual, ETrue, ETrue);
const TDmaCapability pauseRequired_skip(EChannelPauseAndResume, EEqual, ETrue, EFalse);
const TDmaCapability pauseNotWanted(EChannelPauseAndResume, EEqual, EFalse, ETrue);

const TDmaCapability hwDesNotWanted(EHwDescriptors, EEqual, EFalse, ETrue);
const TDmaCapability hwDesNotWanted_skip(EHwDescriptors, EEqual, EFalse, EFalse);
const TDmaCapability hwDesWanted(EHwDescriptors, EEqual, ETrue, ETrue);
const TDmaCapability hwDesWanted_skip(EHwDescriptors, EEqual, ETrue, EFalse);

const TDmaCapability cap_2DRequired(E2DAddressing, EEqual, ETrue, EFalse);

const TDmaCapability capEqualV1(EPilVersion, EEqual, 1, EFalse);
const TDmaCapability capEqualV2(EPilVersion, EEqual, 2, EFalse);
const TDmaCapability capEqualV2Fatal(EPilVersion, EEqual, 2, ETrue);

const TDmaCapability capAboveV1(EPilVersion, EGTE, 2, EFalse);
const TDmaCapability capBelowV2(EPilVersion, ELTE, 1, EFalse);
const TDmaCapability LinkingNotWanted(EChannelLinking, EEqual, EFalse, ETrue);
#endif // #ifdef __CAP_REQS_H__
