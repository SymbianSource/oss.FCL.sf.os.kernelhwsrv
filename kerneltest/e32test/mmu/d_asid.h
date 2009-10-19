// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\d_memorytest.h
// 
//

#ifndef __D_ASID_H__
#define __D_ASID_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KMemoryTestLddName,"d_asid");

struct SDesHeader 
	{
	TAny* iDes;			// Pointer to the descriptor to read the header of.
	TInt iLength;		// Length of the descriptor.
	TInt iMaxLength;	// Maximum length of the descriptor.
	};
	

class RAsidLdd : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EGetCurrentThread,
		EOpenThread,
		ECloseThread,
		EReadDesHeader,
		};

#ifndef __KERNEL_MODE__
public:
	inline TInt Open()
		{
		TInt r=User::LoadLogicalDevice(KMemoryTestLddName);
		if(r==KErrNone || r==KErrAlreadyExists)
			r=DoCreate(KMemoryTestLddName,TVersion(),KNullUnit,NULL,NULL,EOwnerProcess,ETrue);
		return r;
		};
	inline TInt GetCurrentThread(TAny*& aPtr)
		{ return DoControl(EGetCurrentThread, &aPtr); }
	inline TInt OpenThread(TAny* aPtr)
		{ return DoControl(EOpenThread, aPtr); }
	inline TInt CloseThread()
		{ return DoControl(ECloseThread); }
	inline TInt ReadDesHeader(TAny* aThread, SDesHeader& aHdr)
		{ return DoControl(EReadDesHeader, aThread, &aHdr); }
#endif
	};


#endif
