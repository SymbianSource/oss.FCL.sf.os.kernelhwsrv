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
// Implements an Active Scheduler for the server to use
// 
//



/**
 @file
*/

#ifndef __CUSBSCHEDULER_H__
#define __CUSBSCHEDULER_H__



class CUsbMassStorageServer;

/**
 * Implements an Active Scheduler for the server to use. This is necessary
 * in order to provide an Error() function which does something useful instead
 * of panicking.
 */
class CUsbMassStorageScheduler : public CActiveScheduler
	{
public:
	static CUsbMassStorageScheduler* NewL();
	~CUsbMassStorageScheduler();

	void SetServer(CUsbMassStorageServer& aServer);

private:
	inline CUsbMassStorageScheduler() {};
	void ConstructL();
	// from CActiveScheduler
	void Error(TInt aError) const;

public:
	CUsbMassStorageServer* iMsServer;
	};

#endif //__CUSBMASSSTORAGESCHEDULER_H__
