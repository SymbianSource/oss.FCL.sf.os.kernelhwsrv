// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\d_codemodifier.h
// 
//

#ifndef __D_CODEMODIFIER_H__
#define __D_CODEMODIFIER_H__

#include <e32cmn.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KCodeModifierName,"d_codemodifier");

/**
The user side class for controlling trace handler hook.
*/
class RCodeModifierDevice : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EControlThreadId,

		EInitialiseCodeModifier,
		ECloseCodeModifier,

		EControlWriteCode, 
		EControlRestoreCode,

		EControlReadWord,
		EControlWriteWord,

		EControlAllocShadowPage,
		EControlFreeShadowPage
		};

	//Used by all device calls	
	struct TData
		{
		TData(){};
		TData(TInt aServer, TInt aThreadId, TUint aAddress, TInt aSize):
		iServer(aServer),iThreadId(aThreadId),iAddress(aAddress),iSize(aSize){};
		
		TInt  iServer; //0-XIP server, otherwise - NONXIP server
		TInt  iThreadId; //threadID
		TUint iAddress; 
		TInt iSize;
		};
public:
#ifndef __KERNEL_MODE__
	TInt Open()
		{return DoCreate(KCodeModifierName,TVersion(1,0,0),KNullUnit,NULL,NULL);}

	TInt InitialiseCodeModifier(TInt aSize)
		{TData data(0,0,0,aSize);return DoControl(EInitialiseCodeModifier, reinterpret_cast<TAny*>(&data));}
	TInt CloseCodeModifier()
		{TData data(0,0,0,0);return DoControl(ECloseCodeModifier, reinterpret_cast<TAny*>(&data));}

	TInt ThreadId(TInt aServer, TInt aThreadId)
		{TData data(aServer,aThreadId,0,0);	return DoControl(EControlThreadId, reinterpret_cast<TAny*>(&data));}

	TInt ReadWord(TInt aServer, TInt aAddress, TInt* aValue)
		{TData data(aServer,0,aAddress,0);	return DoControl(EControlReadWord, reinterpret_cast<TAny*>(&data), reinterpret_cast<TAny*>(aValue));}
	TInt WriteWord(TInt aServer, TInt aAddress, TInt aValue)
		{TData data(aServer,0,aAddress,0);	return DoControl(EControlWriteWord, reinterpret_cast<TAny*>(&data), reinterpret_cast<TAny*>(aValue));}

	TInt WriteCode(TInt aServer, TUint aAddress, TInt aValue, TInt aSize)
		{TData data(aServer,0,aAddress,aSize);return DoControl(EControlWriteCode, reinterpret_cast<TAny*>(&data), reinterpret_cast<TAny*>(aValue));}
	TInt RestoreCode(TInt aServer, TUint aAddress)
		{TData data(aServer,0,aAddress,0);	return DoControl(EControlRestoreCode, reinterpret_cast<TAny*>(&data));}


	TInt AllocShadowPage(TInt aAddress)
		{TData data(0,0,aAddress,0);return DoControl(EControlAllocShadowPage, reinterpret_cast<TAny*>(&data));}
	TInt FreeShadowPage(TInt aAddress)
		{TData data(0,0,aAddress,0);return DoControl(EControlFreeShadowPage, reinterpret_cast<TAny*>(&data));}

#endif //__KERNEL_MODE__
	};

#endif //__D_LOGTOFILE_H__
