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
// e32test\misc\strataflash32.cpp
// 
//

#include <e32def.h>
#include <e32def_private.h>
#include "flash.h"

#include <e32test.h>
GLREF_C RTest test;

class StrataFlash32 : public Flash
	{
public:
	virtual TInt Read(TUint32 anAddr, TUint32 aSize, TUint8* aDest);
	virtual TInt BlankCheck(TUint32 anAddr, TUint32 aSize);
	virtual TInt Erase(TUint32 anAddr, TUint32 aSize);
	virtual TInt Write(TUint32 anAddr, TUint32 aSize, const TUint8* aSrc);
	};


Flash* Flash::New(TUint32 /*anAddr*/)
	{
	return new StrataFlash32;
	}

TInt StrataFlash32::Read(TUint32 anAddr, TUint32 aSize, TUint8* aDest)
	{
	Mem::Move(aDest,(const TUint32*)anAddr,aSize);
	return KErrNone;
	}

TInt StrataFlash32::BlankCheck(TUint32 anAddr, TUint32 aSize)
	{
	const TUint32* p=(const TUint32*)anAddr;
	const TUint32* pE=p+(aSize+3)/4;
	while(p<pE)
		{
		if (*p++!=0xffffffff)
			return (TUint32)p-anAddr;
		}
	return 0;
	}

TInt StrataFlash32::Erase(TUint32 anAddr, TUint32 aSize)
	{
	TUint32 base=anAddr&~0x3ffff;	// round base address down to block
	TUint32 end=anAddr+aSize;
	end=(end+0x3ffff)&~0x3ffff;	// round end address up to block
	TUint32 size=end-base;
	volatile TUint32* p=(volatile TUint32*)base;
	*p=0x00500050;	// clear status reg
	for (; size; size-=0x40000, p+=0x40000/4)
		{
		*p=0x00200020;	// block erase
		*p=0x00d000d0;	// block erase confirm
		while ((*p & 0x00800080)!=0x00800080) {}
		TUint32 s=*p;
		*p=0x00500050;	// clear status reg
		*p=0x00ff00ff;	// read mode
		if (s&0x00200020)
			{
			// error
			return (TUint32)p-anAddr+1;
			}
		}
	return 0;
	}

TInt StrataFlash32::Write(TUint32 anAddr, TUint32 aSize, const TUint8* aSrc)
	{
	volatile TUint32* p=(volatile TUint32*)anAddr;
	const TUint32* pS=(const TUint32*)aSrc;
	aSize=(aSize+63)&~63;
/*
	const TUint32* pE=pS+aSize/4;
	for (; pS<pE; pS++, p++)
		{
		*p=0x00400040;	// word write
		*p=*pS;		// write data
		while ((*p & 0x00800080)!=0x00800080);
		TUint32 s=*p;
		*p=0x00500050;	// clear status reg
		*p=0x00ff00ff;	// read mode
		if (s&0x00100010)
			{
			// error
			return (TUint32)p-anAddr+1;
			}
		}
*/

	TUint32 s=0;
	*p=0x00500050;	// clear status reg
	while(aSize)
		{
		TUint32 wb_offset=((TUint32)p)&0x3f;
		TUint32 max_count=(64-wb_offset)/4;
		TUint32 count=Min(aSize/4,max_count);
		TUint32 cwd=count-1;
		cwd|=(cwd<<16);

		s=0;
		do	{
			*p=0x00e800e8;	// Write to Buffer
			*p=0x00700070;	// Read status register
			s=*p;
			} while ((s&0x00800080)!=0x00800080);
		s=*p;
		*p=cwd;
		TUint32 i;
		for (i=0; i<count; ++i)
			*p++=*pS++;
		*p=0x00d000d0;	// Write confirm
		aSize-=4*count;
		while ((*p & 0x00800080)!=0x00800080) {}	// Wait for write to complete
		s=*p;
		if (s&0x00300030)
			break;
		}
	*p=0x00500050;	// clear status reg
	*p=0x00ff00ff;	// read mode
	if (s&0x00300030)
		{
		// error
		return (TUint32)p-anAddr+1;
		}

	return 0;
	}


