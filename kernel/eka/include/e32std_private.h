// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32std_private.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
 @released
*/

#ifndef __E32STD_PRIVATE_H__
#define __E32STD_PRIVATE_H__

#ifdef __KERNEL_MODE__
#error !! Including e32std.h in kernel code !!
#endif

#include <e32cmn.h>
#include <e32cmn_private.h>

/**
@internalComponent
*/
const TUint KLocaleLanguageKey = 0x10208903;

/**
@internalComponent
*/
const TUint KLocaleDataKey = 0x10208904;

/**
@internalComponent
*/
const TUint KLocaleDataExtraKey = 0x10208905;

/**
@internalComponent
*/
const TUint KLocaleTimeDateFormatKey = 0x10208907;

/**
@internalComponent
*/
const TUint KLocaleDefaultCharSetKey = 0x10208908;

/**
@internalComponent
*/
const TUint KLocalePreferredCharSetKey = 0x10208909;

/**
@internalComponent
*/
enum TLocaleFunctions
	{
	FnDummy,
	FnAmPmTable,
	FnCharSet,
	FnCollTable,
	FnCurrencySymbol,
	FnDateSuffixTable,
	FnDayAbbTable,
	FnDayTable,
	FnFoldTable,
	FnLanguage,
	FnLocaleData,
	FnLowerTable,
	FnMonthAbbTable,
	FnMonthTable,
	FnMsgTable,
	FnTypeTable,
	FnUniCode,
	FnUpperTable,
	FnShortDateFormatSpec,
	FnLongDateFormatSpec,
	FnTimeFormatSpec,
	FnFatUtilityFunctions
	};
	
#ifdef SYMBIAN_DISTINCT_LOCALE_MODEL
/**
@internalComponent
*/
enum TLanguageLocaleFunctions
	{
	FnDummy1,
	FnMsgTableV2,
	FnLanguageDowngradeTableV2,
	FnAmPmTableV2,
	FnDateSuffixTableV2,
	FnMonthAbbTableV2,
	FnLanguageV2,
	FnFatUtilityFunctionsV2,
	FnDigitTypeV2,
	FnMonthTableV2,
	FnDayTableV2,
	FnDayAbbTableV2	
	};
	
/**
@internalComponent
*/
enum TRegionLocaleFunctions
	{
	FnDummy2,
	FnShortDateFormatSpecV2,
	FnLongDateFormatSpecV2,
	FnLocaleDataV2,	
	FnTimeFormatSpecV2,
	FnCurrencySymbolV2
	};
#endif

/**
@internalAll
*/
const TInt KMediaPasswordNotifyUid(0x10004c00);

/**
@internalAll
*/
enum TMediaPswdNotifyExitMode {EMPEMUnlock, EMPEMCancel, EMPEMUnlockAndStore};


/**
@internalAll
*/
struct TMediaPswdNotifyBase
	{
	enum TCardType {ECTMmcPassword} iCT;
	TVersion iVersion;
	};

/**
@internalAll
*/
struct TMediaPswdSendNotifyInfoV1 : public TMediaPswdNotifyBase
	{
	// empty.
	};

/**
@internalAll
*/
struct TMediaPswdSendNotifyInfoV1Debug : public TMediaPswdSendNotifyInfoV1
	{
	TInt iSleepPeriod;							// us, -ve means maximum range
	TMediaPswdNotifyExitMode iEM;
	TText8 iPW[KMaxMediaPassword];
	};

/**
@internalAll
*/
struct TMediaPswdReplyNotifyInfoV1 : public TMediaPswdNotifyBase
	{
	TText8 iPW[KMaxMediaPassword];
	TMediaPswdNotifyExitMode iEM;
	};


#endif

