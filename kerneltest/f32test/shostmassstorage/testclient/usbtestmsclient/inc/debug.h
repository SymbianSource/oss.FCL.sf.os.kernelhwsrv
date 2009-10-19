// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
//



/**
 @file
 @internalTechnology
*/

#ifndef DEBUG_H
#define DEBUG_H


#include <e32debug.h>

//#define _USBMS_DEBUG_PRINT_
//#define _SCSI_DEBUG_PRINT_
//#define _BOT_DEBUG_PRINT_
//#define _CLIENT_DEBUG_PRINT_
//#define _TESTREPORT_PRINT_
#define _TESTMODE_PRINT_

#if (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#include <e32debug.h>
#endif

#if defined(_USBMS_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define __PRINT(t) {RDebug::Print(t);}
#define __PRINT1(t,a) {RDebug::Print(t,a);}
#define __PRINT2(t,a,b) {RDebug::Print(t,a,b);}
#define __PRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
#define __PRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
#define __PRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#define __PRINT8BIT1(t,a) {TFileName temp;temp.Copy(a);RDebug::Print(t,&temp);}
#define __PRINT1TEMP(t,a) {TBuf<KMaxFileName>temp(a);RDebug::Print(t,&temp);}
#define __PRINTERR(txt, err) {if(KErrNone != err) __PRINT1(txt, err);}

_LIT(KMsgIn, ">>%S\n");
_LIT(KMsgOut,"<<%S\n");

class TMSLogFn
{
	protected:
	TBuf<100> iName;

	public:
	TMSLogFn(const TDesC& aName){iName = aName; RDebug::Print(KMsgIn, &iName);};
	~TMSLogFn(){RDebug::Print(KMsgOut, &iName);};
};

#define __FNLOG(name) TMSLogFn __fn_log__(_L(name))

#else
#define __PRINT(t)
#define __PRINT1(t,a)
#define __PRINT2(t,a,b)
#define __PRINT3(t,a,b,c)
#define __PRINT4(t,a,b,c,d)
#define __PRINT5(t,a,b,c,d,e)
#define __PRINT8BIT1(t,a)
#define __PRINT1TEMP(t,a)
#define __PRINTERR(txt,err)
#define __FNLOG(name)
#endif


#if defined(_SCSI_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define __SCSIPRINT(t) {RDebug::Print(t);}
#define __SCSIPRINT1(t,a) {RDebug::Print(t,a);}
#define __SCSIPRINT2(t,a,b) {RDebug::Print(t,a,b);}
#define __SCSIPRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
#define __SCSIPRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
#define __SCSIPRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#else
#define __SCSIPRINT(t)
#define __SCSIPRINT1(t,a)
#define __SCSIPRINT2(t,a,b)
#define __SCSIPRINT3(t,a,b,c)
#define __SCSIPRINT4(t,a,b,c,d)
#define __SCSIPRINT5(t,a,b,c,d,e)
#endif


#if defined(_BOT_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define __BOTPRINT(t) {RDebug::Print(t);}
#define __BOTPRINT1(t,a) {RDebug::Print(t,a);}
#define __BOTPRINT2(t,a,b) {RDebug::Print(t,a,b);}
#define __BOTPRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
#define __BOTPRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
#define __BOTPRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#else
#define __BOTPRINT(t)
#define __BOTPRINT1(t,a)
#define __BOTPRINT2(t,a,b)
#define __BOTPRINT3(t,a,b,c)
#define __BOTPRINT4(t,a,b,c,d)
#define __BOTPRINT5(t,a,b,c,d,e)
#endif


#if defined(_CLIENT_DEBUG_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define __CLIENTPRINT(t) {RDebug::Print(t);}
#define __CLIENTPRINT1(t,a) {RDebug::Print(t,a);}
#define __CLIENTPRINT2(t,a,b) {RDebug::Print(t,a,b);}
#define __CLIENTPRINT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
#define __CLIENTPRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
#define __CLIENTPRINT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#else
#define __CLIENTPRINT(t)
#define __CLIENTPRINT1(t,a)
#define __CLIENTPRINT2(t,a,b)
#define __CLIENTPRINT3(t,a,b,c)
#define __CLIENTPRINT4(t,a,b,c,d)
#define __CLIENTPRINT5(t,a,b,c,d,e)
#endif


#if defined(_TESTREPORT_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define __TESTREPORT(t) {RDebug::Print(t);}
#define __TESTREPORT1(t,a) {RDebug::Print(t,a);}
#define __TESTREPORT2(t,a,b) {RDebug::Print(t,a,b);}
#define __TESTREPORT3(t,a,b,c) {RDebug::Print(t,a,b,c);}
#define __TESTREPORT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
#define __TESTREPORT5(t,a,b,c,d,e) {RDebug::Print(t,a,b,c,d,e);}
#else
#define __TESTREPORT(t)
#define __TESTREPORT1(t,a)
#define __TESTREPORT2(t,a,b)
#define __TESTREPORT3(t,a,b,c)
#define __TESTREPORT4(t,a,b,c,d)
#define __TESTREPORT5(t,a,b,c,d,e)
#endif


#if defined(_TESTMODE_PRINT_) && (defined(_DEBUG) || defined(_DEBUG_RELEASE))
#define PREFIX(aMsg) TPtrC( (const TText*)L"[TM]: " L##aMsg )

#define __TESTMODEPRINT(t) {RDebug::Print(PREFIX(t));}
#define __TESTMODEPRINT1(t,a) {RDebug::Print(PREFIX(t),a);}
#define __TESTMODEPRINT2(t,a,b) {RDebug::Print(PREFIX(t),a,b);}
#define __TESTMODEPRINT3(t,a,b,c) {RDebug::Print(PREFIX(t),a,b,c);}
#define __TESTMODEPRINT4(t,a,b,c,d) {RDebug::Print(PREFIX(t),a,b,c,d);}
#define __TESTMODEPRINT5(t,a,b,c,d,e) {RDebug::Print(PREFIX(t),a,b,c,d,e);}
#else
#define __TESTMODEPRINT(t)
#define __TESTMODEPRINT1(t,a)
#define __TESTMODEPRINT2(t,a,b)
#define __TESTMODEPRINT3(t,a,b,c)
#define __TESTMODEPRINT4(t,a,b,c,d)
#define __TESTMODEPRINT5(t,a,b,c,d,e)
#endif

#endif // DEBUG_H
