/*
* Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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

const TText hi[]={0x65e5,0};
const TText getsuyoubi[]={0x6708,0x66dc,0x65e5,0};
const TText kayoubi[]={0x706b,0x66dc,0x65e5,0};
const TText suiyoubi[]={0x6c34,0x66dc,0x65e5,0};
const TText mokuyoubi[]={0x6728,0x66dc,0x65e5,0};
const TText kinyoubi[]={0x91d1,0x66dc,0x65e5,0};
const TText doyoubi[]={0x571f,0x66dc,0x65e5,0};
const TText nichiyoubi[]={0x65e5,0x66dc,0x65e5,0};
const TText getsu[]={0x6708,0};
const TText ka[]={0x706b,0};
const TText sui[]={0x6c34,0};
const TText moku[]={0x6728,0};
const TText kin[]={0x91d1,0};
const TText dou[]={0x571f,0};
const TText nichi[]={0x65e5,0};
const TText ichigatsu[]={0xff11,0};
const TText nigatsu[]={0xff12,0};
const TText sangatsu[]={0xff13,0};
const TText shigatsu[]={0xff14,0};
const TText gogatsu[]={0xff15,0};
const TText rokugatsu[]={0xff16,0};
const TText shichigatsu[]={0xff17,0};
const TText hachigatsu[]={0xff18,0};
const TText kugatsu[]={0xff19,0};
const TText jyugatsu[]={0x0031,0x0030,0};
const TText jyuichigatsu[]={0x0031,0x0031,0};
const TText jyunigatsu[]={0x0031,0x0032,0};
const TText a_ichigatsu[]={0x0031,0x6708,0};
const TText a_nigatsu[]={0x0032,0x6708,0};
const TText a_sangatsu[]={0x0033,0x6708,0};
const TText a_shigatsu[]={0x0034,0x6708,0};
const TText a_gogatsu[]={0x0035,0x6708,0};
const TText a_rokugatsu[]={0x0036,0x6708,0};
const TText a_shichigatsu[]={0x0037,0x6708,0};
const TText a_hachigatsu[]={0x0038,0x6708,0};
const TText a_kugatsu[]={0x0039,0x6708,0};
const TText a_jyugatsu[]={0x0031,0x0030,0x6708,0};
const TText a_jyuichigatsu[]={0x0031,0x0031,0x6708,0};
const TText a_jyunigatsu[]={0x0031,0x0032,0x6708,0};
const TText gozen[]={0x5348,0x524d,0};
const TText gogo[]={0x5348,0x5f8c,0};



// The suffix table
const TText * const LLanguage::DateSuffixTable[KMaxSuffixes] =
	{
	hi,hi,hi,hi,hi,
	hi,hi,hi,hi,hi,
	hi,hi,hi,hi,hi,
	hi,hi,hi,hi,hi,
	hi,hi,hi,hi,hi,
	hi,hi,hi,hi,hi,
	hi
	};
// The day names
const TText * const LLanguage::DayTable[KMaxDays] =
	{
	getsuyoubi,
	kayoubi,
	suiyoubi,
	mokuyoubi,
	kinyoubi,
	doyoubi,
	nichiyoubi
	};
// The abbreviated day names
const TText * const LLanguage::DayAbbTable[KMaxDays] =
	{
	getsu,
	ka,
	sui,
	moku,
	kin,
	dou,
	nichi
	};
// The month names
const TText * const LLanguage::MonthTable[KMaxMonths] =
	{
	ichigatsu,
	nigatsu,
	sangatsu,
	shigatsu,
	gogatsu,
	rokugatsu,
	shichigatsu,
	hachigatsu,
	kugatsu,
	jyugatsu,
	jyuichigatsu,
	jyunigatsu
	};
// The abbreviated month names
const TText * const LLanguage::MonthAbbTable[KMaxMonths] =
	{
	a_ichigatsu,
	a_nigatsu,
	a_sangatsu,
	a_shigatsu,
	a_gogatsu,
	a_rokugatsu,
	a_shichigatsu,
	a_hachigatsu,
	a_kugatsu,
	a_jyugatsu,
	a_jyuichigatsu,
	a_jyunigatsu
	};
// The am/pm strings
const TText * const LLanguage::AmPmTable[KMaxAmPms] = {gozen,gogo};

