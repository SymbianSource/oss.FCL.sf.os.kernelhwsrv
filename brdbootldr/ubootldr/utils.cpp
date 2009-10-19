/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/


#define FILE_ID	0x594D555D
#include "bootldr.h"

_LIT(KLitPdd,".PDD");
_LIT(KLitLdd,".LDD");
TInt LoadDriver(const TDesC& aName, TBool aPdd)
	{
	TBuf<128> filename=aName;
	TInt rv=KErrNone;
	if (aPdd)
		{
		filename.Append(KLitPdd);
		rv = User::LoadPhysicalDevice(filename);
		}
	else
		{
		filename.Append(KLitLdd);
		rv = User::LoadLogicalDevice(filename);
		}
	if (rv==KErrAlreadyExists)
		rv=KErrNone;
	return rv;
	}

GLDEF_C void Printf(TRefByValue<const TDesC> aFmt,...)
	{
	TBuf<256> printBuf;
	VA_LIST list;
	VA_START(list,aFmt);
	printBuf.AppendFormatList(aFmt, list);
	printBuf.Append(TChar(0));
	RDebug::Print(printBuf);
	}


