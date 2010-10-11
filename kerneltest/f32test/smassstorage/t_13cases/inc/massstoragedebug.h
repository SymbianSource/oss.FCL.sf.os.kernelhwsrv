/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Class declaration for MassStorageDebug
* 
*
*/



/**
 @file
*/
 
#ifndef __MASSSTORAGEDEBUG_H__
#define __MASSSTORAGEDEBUG_H__

#include <e32std.h>
#include <e32svr.h>

//#define _USBMS_DEBUG_PRINT_

// Display some info during IN double-buffering
//#define PRINT_MSDC_MULTITHREADED_READ_INFO


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

#endif // __MASSSTORAGEDEBUG_H__
