/*
* Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Description: 
*
*/



#include <ls_std.h>
#include "locl_language.h"

EXPORT_C TLanguage LoclLanguage::Language()
	{
	return(LanguageAspect::Language);
	}

EXPORT_C TDigitType LoclLanguage::DigitType()
	{
	return(LanguageAspect::DigitType);
	}

EXPORT_C const TLanguage* LoclLanguage::LanguageDowngrade()
	{
	return(&LanguageAspect::LanguageDowngrade[0]);
	}

EXPORT_C const TFatUtilityFunctions* LoclLanguage::FatUtilityFunctions()
	{

	return(LanguageAspect::FatUtilityFunctions);
	}

EXPORT_C const TText * const * LoclLanguage::DateSuffixTable()
	{

	return(&LanguageAspect::DateSuffixTable[0]);
	}

EXPORT_C const TText * const * LoclLanguage::DayTable()
	{

	return(&LanguageAspect::DayTable[0]);
	}

EXPORT_C const TText * const * LoclLanguage::DayAbbTable()
	{

	return(&LanguageAspect::DayAbbTable[0]);
	}

EXPORT_C const TText * const * LoclLanguage::MonthTable()
	{

	return(&LanguageAspect::MonthTable[0]);
	}

EXPORT_C const TText * const * LoclLanguage::MonthAbbTable()
	{

	return(&LanguageAspect::MonthAbbTable[0]);
	}

EXPORT_C const TText * const * LoclLanguage::AmPmTable()
	{

	return(&LanguageAspect::AmPmTable[0]);
	}

EXPORT_C const TText * const * LoclLanguage::MsgTable()
	{

	return(&LanguageAspect::MsgTable[0]);
	}


