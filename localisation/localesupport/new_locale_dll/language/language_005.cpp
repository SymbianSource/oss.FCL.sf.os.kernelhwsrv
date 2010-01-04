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


#include "ls_std.h"
#include "locl_language.h"

// The configuration data
const TLanguage LanguageAspect::Language = ELangItalian;
const TLanguage LanguageAspect::LanguageDowngrade[3] = {ELangNone, ELangNone, ELangNone};
const TDigitType LanguageAspect::DigitType = EDigitTypeWestern;
const TFatUtilityFunctions* const LanguageAspect::FatUtilityFunctions = NULL;


const TText * const LanguageAspect::MsgTable[ELocaleMessages_LastMsg] =
	{
// Fileserver
	_S("Riprova"),					                       // Button 1
	_S("Stop"),					        	       // Button 2
	_S("Reinserisci il disco"),			                       // Put the card back - line1
	_S("Oppure perderai le informazioni"),			               // Put the card back - line2
	_S("Batterie troppo basse"),			                       // Low power - line1
	_S("Impossibile completare scrittura sul disco"),		       // Low power - line2
	_S("Errore disco: impossibile completare la scrittura"),	       // Disk error - line1
	_S("Riprova o perderai le informazioni"),		               // Disk error - line2
// SoundDriver
	_S("Carillon"),								// Chimes
	_S("Squilli"),								// Rings
	_S("Segnale"),								// Signal
// MediaDriver diskname (max 16 chars)
	_S("Interno"),								// Internal
	_S("Esterno(01)"),							// External(01)
	_S("Esterno(02)"),							// External(02)
	_S("Esterno(03)"),							// External(03)
	_S("Esterno(04)"),							// External(04)
	_S("Esterno(05)"),							// External(05)
	_S("Esterno(06)"),							// External(06)
	_S("Esterno(07)"),							// External(07)
	_S("Esterno(08)"),							// External(08)
// MediaDriver socketname (max 16 chars)
	_S("Presa(01)"),							// Socket(01)
	_S("Presa(02)"),							// Socket(02)
	_S("Presa(03)"),							// Socket(03)
	_S("Presa(04)")							// Socket(04)
	};
	
	// The suffix table
const TText * const LanguageAspect::DateSuffixTable[KMaxSuffixes] =
	{
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S(""),_S(""),_S(""),_S(""),_S(""),
	_S("")
	};
// The day names
const TText * const LanguageAspect::DayTable[KMaxDays] =
	{
	_S("Luned\354"),
	_S("Marted\354"),
	_S("Mercoled\354"),
	_S("Gioved\354"),
	_S("Venerd\354"),
	_S("Sabato"),
	_S("Domenica")
	};
// The abbreviated day names
const TText * const LanguageAspect::DayAbbTable[KMaxDays] =
	{
	_S("Lun"),
	_S("Mar"),
	_S("Mer"),
	_S("Gio"),
	_S("Ven"),
	_S("Sab"),
	_S("Dom")
	};
// The month names
const TText * const LanguageAspect::MonthTable[KMaxMonths] =
	{
	_S("Gennaio"),
	_S("Febbraio"),
	_S("Marzo"),
	_S("Aprile"),
	_S("Maggio"),
	_S("Giugno"),
	_S("Luglio"),
	_S("Agosto"),
	_S("Settembre"),
	_S("Ottobre"),
	_S("Novembre"),
	_S("Dicembre")
	};
// The abbreviated month names
const TText * const LanguageAspect::MonthAbbTable[KMaxMonths] =
	{
	_S("Gen"),
	_S("Feb"),
	_S("Mar"),
	_S("Apr"),
	_S("Mag"),
	_S("Giu"),
	_S("Lug"),
	_S("Ago"),
	_S("Set"),
	_S("Ott"),
	_S("Nov"),
	_S("Dic")
	};
// The am/pm strings
const TText * const LanguageAspect::AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};
