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
// Component test of Publish and Subscribe
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __CPROPERTYWATCH_H__
#define __CPROPERTYWATCH_H__

#include <e32base.h>
#include <e32property.h>
#include "usbmsshared.h"

class CPropertyHandler;
class CStateMachine;

enum 
	{
	EUsbMsState_Read = EUsbMsDriveState_Error + 100,
	EUsbMsState_Written
	};
	
/**
 An active object that subscribes to a specified Mass Storage property and
 calls a provided handler each time the property is published.
 */
class CPropertyWatch : public CActive
	{
public:
	static CPropertyWatch* NewLC(TUsbMsDriveState_Subkey aSubkey, CPropertyHandler& aHandler);

private:
	CPropertyWatch(CPropertyHandler& aHandler);
	void ConstructL(TUsbMsDriveState_Subkey aSubkey);
	~CPropertyWatch();
	void RunL();
	void DoCancel();
	
	RProperty iProperty;
	CPropertyHandler& iHandler;
	};
	
/**
 A property handler class that handles the change of ms drive status
 */
 class CPropertyHandler : public CBase
 	{
 public:
 	virtual void HandleStatusChange(RProperty& aProperty) = 0;
 	
 protected:
 	CPropertyHandler(TInt aDriveNo, CStateMachine& aSm);
 	virtual ~CPropertyHandler();
 
 protected:
 	TInt iDriveNo; 
 	CStateMachine& iStateMachine;
 	};
 	
 class CMsDriveStatusHandler : public CPropertyHandler
 	{
 public:
 	static CMsDriveStatusHandler* NewLC(TInt aDriveNo, CStateMachine& aSm);
 	void HandleStatusChange(RProperty& aProperty);
 	
 protected:
  	CMsDriveStatusHandler(TInt aDriveNo, CStateMachine& aSm);
 	};
 	
 class CMsReadStatusHandler : public CPropertyHandler
 	{
 public:
 	static CMsReadStatusHandler* NewLC(TInt aDriveNo, CStateMachine& aSm);
 	void HandleStatusChange(RProperty& aProperty);
 	
 private:
  	CMsReadStatusHandler(TInt aDriveNo, CStateMachine& aSm);
 	};

 class CMsWrittenStatusHandler : public CPropertyHandler
 	{
 public:
 	static CMsWrittenStatusHandler* NewLC(TInt aDriveNo, CStateMachine& aSm);
 	void HandleStatusChange(RProperty& aProperty);
 	
 private:
  	CMsWrittenStatusHandler(TInt aDriveNo, CStateMachine& aSm);
 	};

#endif // __CPROPERTYWATCH_H__

