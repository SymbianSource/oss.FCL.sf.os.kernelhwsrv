/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/


#include <f32fsys.h>


class CEmptyExtProxyDrive : public CBaseExtProxyDrive
	{
public:
	static CEmptyExtProxyDrive* NewL(CProxyDrive* aProxyDrive, CMountCB* aMount);
	~CEmptyExtProxyDrive();

private:
	CEmptyExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount);
	};

class CEmptyProxyDriveFactory : public CProxyDriveFactory
	{
public:
	CEmptyProxyDriveFactory();
	virtual TInt Install();
	virtual CProxyDrive* NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount);
	};

CEmptyExtProxyDrive* CEmptyExtProxyDrive::NewL(CProxyDrive* aProxyDrive, CMountCB* aMount)
	{
	CEmptyExtProxyDrive* temp=new(ELeave) CEmptyExtProxyDrive(aProxyDrive,aMount);
	return(temp);
	}


CEmptyExtProxyDrive::CEmptyExtProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount):CBaseExtProxyDrive(aProxyDrive,aMount)
	{
	}

CEmptyExtProxyDrive::~CEmptyExtProxyDrive()
	{
	}

CEmptyProxyDriveFactory::CEmptyProxyDriveFactory()
	{
	}

TInt CEmptyProxyDriveFactory::Install()
	{
	_LIT(KEmptyName,"TestFileExtension");
	return(SetName(&KEmptyName));
	}

CProxyDrive* CEmptyProxyDriveFactory::NewProxyDriveL(CProxyDrive* aProxy,CMountCB* aMount)
	{
	return(CEmptyExtProxyDrive::NewL(aProxy,aMount));
	}

extern "C" {

EXPORT_C CProxyDriveFactory* CreateFileSystem()
//
// Create a new file system
//
	{
	return(new CEmptyProxyDriveFactory());
	}
}
