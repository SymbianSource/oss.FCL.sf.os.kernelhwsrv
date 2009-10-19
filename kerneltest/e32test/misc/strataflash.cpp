// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\misc\strataflash.cpp
// 
//

#include <e32def.h>
#include <e32def_private.h>
#include "flash.h"

class StrataFlash : public Flash
	{
public:
	virtual TInt Read(TUint32 anAddr, TUint32 aSize, TUint8* aDest);
	virtual TInt BlankCheck(TUint32 anAddr, TUint32 aSize);
	virtual TInt Erase(TUint32 anAddr, TUint32 aSize);
	virtual TInt Write(TUint32 anAddr, TUint32 aSize, const TUint8* aSrc);
	};


Flash* Flash::New(TUint32 /*anAddr*/)
	{
	return new StrataFlash;
	}

TInt StrataFlash::Read(TUint32 anAddr, TUint32 aSize, TUint8* aDest)
	{
	Mem::Copy(aDest,(const TUint8*)anAddr,aSize);
	return KErrNone;
	}

TInt StrataFlash::BlankCheck(TUint32 anAddr, TUint32 aSize)
	{
	const TUint8* p=(const TUint8*)anAddr;
	const TUint8* pE=p+aSize;
	while(p<pE)
		{
		if (*p++!=0xff)
			return (TUint32)p-anAddr;
		}
	return 0;
	}

TInt StrataFlash::Erase(TUint32 anAddr, TUint32 aSize)
	{
	TUint32 base=anAddr&~0x1ffff;	// round base address down to block
	TUint32 end=anAddr+aSize;
	end=(end+0x1ffff)&~0x1ffff;	// round end address up to block
	TUint32 size=end-base;
	for (; size; size-=0x20000, base+=0x20000)
		{
		volatile TUint8* p=(volatile TUint8*)base;
		*p=0x20;		// block erase
		*p=0xd0;		// block erase confirm
		TUint s=0;
		while ((s&0x80)==0)
			s=*p;
		*p=0x50;		// clear status reg
		*p=0xff;		// read mode
		if (s&0x20)
			{
			// error
			return (TUint32)p-anAddr+1;
			}
		}
	return 0;
	}

TInt StrataFlash::Write(TUint32 anAddr, TUint32 aSize, const TUint8* aSrc)
	{
	volatile TUint8* p=(volatile TUint8*)anAddr;
	const TUint8* pE=aSrc+aSize;
	for (; aSrc<pE; aSrc++, p++)
		{
		*p=0x40;		// byte write
		*p=*aSrc;		// write data
		TUint s=0;
		while ((s&0x80)==0)
			s=*p;
		*p=0x50;		// clear status reg
		*p=0xff;		// read mode
		if (s&0x10)
			{
			// error
			return (TUint32)p-anAddr+1;
			}
		}
	return 0;
	}


