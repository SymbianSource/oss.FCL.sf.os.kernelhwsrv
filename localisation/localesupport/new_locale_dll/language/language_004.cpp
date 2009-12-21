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
const TLanguage LanguageAspect::Language = ELangSpanish;
const TLanguage LanguageAspect::LanguageDowngrade[3] = {ELangNone, ELangNone, ELangNone};
const TDigitType LanguageAspect::DigitType = EDigitTypeWestern;
const TFatUtilityFunctions* const LanguageAspect::FatUtilityFunctions = NULL;

const TText * const LanguageAspect::MsgTable[ELocaleMessages_LastMsg] =
	{
// Fileserver
	_S("Reintentar"),					                       // Button 1
	_S("Detener"),					        	       // Button 2
	_S("Vuelva a introducir el disco"),			                       // Put the card back - line1
	_S("Inserte el disco o se perder\341n los datos"),		                       // Put the card back - line2
	_S("Nivel de pilas principales demasiado bajo"),			                       // Low power - line1
	_S("Imposible terminar escritura en el disco"),		                           // Low power - line2
	_S("Error de disco - imposible terminar escritura"),	                       // Disk error - line1
	_S("Reint\351ntelo o se perder\341n los datos"),	      	                   // Disk error - line2
// SoundDriver
	_S("Carill\363n"),							// Chimes
	_S("Rings"),								// Rings
	_S("Se\361al"),								// Signal
// MediaDriver diskname (max 16 chars)
	_S("Interno"),								// Internal
	_S("Externo(01)"),							// External(01)
	_S("Externo(02)"),							// External(02)
	_S("Externo(03)"),							// External(03)
	_S("Externo(04)"),							// External(04)
	_S("Externo(05)"),							// External(05)
	_S("Externo(06)"),							// External(06)
	_S("Externo(07)"),							// External(07)
	_S("Externo(08)"),							// External(08)
// MediaDriver socketname (max 16 chars)
	_S("Z\363calo(01)"),						// Socket(01)
	_S("Z\363calo(02)"),						// Socket(02)
	_S("Z\363calo(03)"),						// Socket(03)
	_S("Z\363calo(04)")							// Socket(04)
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
	_S("Lunes"),
	_S("Martes"),
	_S("Mi\351rcoles"),
	_S("Jueves"),
	_S("Viernes"),
	_S("S\341bado"),
	_S("Domingo")
	};
// The abbreviated day names
const TText * const LanguageAspect::DayAbbTable[KMaxDays] =
	{
	_S("Lun"),
	_S("Mar"),
	_S("Mi\351"),
	_S("Jue"),
	_S("Vie"),
	_S("S\341b"),
	_S("Dom")
	};
// The month names
const TText * const LanguageAspect::MonthTable[KMaxMonths] =
	{
	_S("Enero"),
	_S("Febrero"),
	_S("Marzo"),
	_S("Abril"),
	_S("Mayo"),
	_S("Junio"),
	_S("Julio"),
	_S("Agosto"),
	_S("Septiembre"),
	_S("Octubre"),
	_S("Noviembre"),
	_S("Diciembre")
	};
// The abbreviated month names
const TText * const LanguageAspect::MonthAbbTable[KMaxMonths] =
	{
	_S("Ene"),
	_S("Feb"),
	_S("Mar"),
	_S("Abr"),
	_S("May"),
	_S("Jun"),
	_S("Jul"),
	_S("Ago"),
	_S("Sep"),
	_S("Oct"),
	_S("Nov"),
	_S("Dic")
	};
// The am/pm strings
const TText * const LanguageAspect::AmPmTable[KMaxAmPms] = {_S("am"),_S("pm")};

