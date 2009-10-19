// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32/include/iic.inl
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

inline RPointerArray<DIicBusChannel>* DIicBusController::ChannelArray()
	{return &iChannelArray;}

typedef DIicBusController::TCapturedChannel CtrllerCapChan;

inline CtrllerCapChan::TCapturedChannel() :
	iChannelId(0), iChanPtr(NULL) {}

inline CtrllerCapChan::TCapturedChannel(TInt aChannelId, DIicBusChannelSlave* aChanPtr) :
	iChannelId(aChannelId), iChanPtr(aChanPtr) {}

inline CtrllerCapChan& CtrllerCapChan::operator=(CtrllerCapChan& aChan)
	{
	iChannelId=aChan.iChannelId;
	iChanPtr=aChan.iChanPtr;
	return *this;
	}

inline TBool CtrllerCapChan::operator==(CtrllerCapChan& aChan)
	{	
	if((iChannelId==aChan.iChannelId)&&(iChanPtr==aChan.iChanPtr))
		return ETrue;
	return EFalse;
	}

