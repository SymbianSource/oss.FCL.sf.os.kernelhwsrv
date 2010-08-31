/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Implements a Symbian OS server that exposes the RUsb API
* 
*
*/



/**
 @file
*/

#ifndef __CTRANSFERSERVER_H__
#define __CTRANSFERSERVER_H__

#include "general.h"
#include "config.h"


class CTransferHandle;

 NONSHARABLE_CLASS(CTransferServer) : public CPolicyServer
	{
public:
	static CTransferServer* NewLC();
	virtual ~CTransferServer();

	virtual CSession2* NewSessionL(const TVersion &aVersion, const RMessage2& aMessage) const;
	void Error(TInt aError);

	void IncrementSessionCount();
	void DecrementSessionCount();
	inline TInt SessionCount() const;
	void LaunchShutdownTimerIfNoSessions();
	TInt SetupLdds(TDes& aFileName);
	void TransferHandleL();

protected:
	CTransferServer();
	void ConstructL();
	
private:	
	TInt iSessionCount;
	enum {KShutdownDelay = 2 * 1000 * 1000};	// 2 seconds
	class CShutdownTimer : public CTimer
		{
	public:
		CShutdownTimer();
		void ConstructL();
		virtual void RunL();
		};
	CShutdownTimer* iShutdownTimer;
	
	void SetupInterface(IFConfigPtr* aIfPtr, TInt aPortNumber);
	void FillEndpointsResourceAllocation(IFConfigPtr aIfCfg);
	void PopulateInterfaceResourceAllocation(IFConfigPtr aFirstIfCfg, TInt aPortNumber);
	void QueryUsbClientL(LDDConfigPtr aLddPtr, RDEVCLIENT* aPort);	

	TInt iTotalChannels;
	RFs iFs;
	RFile iConfigFile;
	LDDConfigPtr iLddPtr;
	RDEVCLIENT iPort[KMaxInterfaces];
	TBool iSupportResourceAllocationV2;
	TBool iSoftwareConnect;
	CConsoleBase* iConsole;

	CTransferHandle* iTransferHandle;
	};

#endif
