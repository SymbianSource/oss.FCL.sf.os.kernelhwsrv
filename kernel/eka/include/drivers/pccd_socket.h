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
// e32\include\drivers\pccd_socket.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __PCCD_SOCKET_H__
#define __PCCD_SOCKET_H__
#include <pccd_ifc.h>

NONSHARABLE_CLASS(DPlatPcCardSocket) : public DPcCardSocket
	{
public:
	DPlatPcCardSocket(TSocket aSocketNum);
	virtual TInt Create(const TDesC* aName);
	virtual void Reset1();
	virtual void HwReset(TBool anAssert);
	virtual TInt Indicators(TSocketIndicators &anInd);
	virtual TBool Ready(TInt aCardFunc=KInvalidFuncNum);
	virtual void SocketInfo(TPcCardSocketInfo& anInfo);
	virtual TBool CardIsPresent();
public:
	virtual DPccdChunkBase* NewPccdChunk(TPccdMemType aType);
	virtual TInt InterruptEnable(TPccdInt anInt, TUint aFlag);
	virtual void InterruptDisable(TPccdInt anInt);
public:
	static void CardIReqIsr(TAny* aPtr);
	static void StatusChangeIsr(TAny* aPtr);
	static void ReadyChangeIsr(TAny* aPtr);
public:
	TInt iIReqLevelMode;
	TInt iCardIReqIntId;
	TInt iStatusChangeIntId;
	TInt iReadyChangeIntId;
	};

#endif
