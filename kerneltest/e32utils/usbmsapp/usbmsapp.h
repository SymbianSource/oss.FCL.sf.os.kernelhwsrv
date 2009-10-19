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
//

/**
 @file
 @internalComponent
*/

#ifndef USBMSAPP_H
#define USBMSAPP_H

#include <e32base.h>
#include <e32property.h>		// RProperty
#include "usbmsshared.h"		// TUsbMsBytesTransferred
#include <d32usbcsc.h>
#include <d32usbc.h>

typedef TUint TUsbDeviceState;

//-----------------------------------------------------------------------------
/**
    Class describing a file system mounted on the drive.
    It has a limitation: full list of possible FS extensions is not supported, only the primary one.
*/
class CFileSystemDescriptor : public CBase
    {
 public:
    ~CFileSystemDescriptor();    
    static CFileSystemDescriptor* NewL(const TDesC& aFsName, const TDesC& aPrimaryExtName, TBool aDrvSynch);

    TPtrC FsName() const            {return iFsName;}
    TPtrC PrimaryExtName() const    {return iPrimaryExtName;}
    TBool DriveIsSynch() const      {return iDriveSynch;}

 private:
    CFileSystemDescriptor() {}
    CFileSystemDescriptor(const CFileSystemDescriptor&);
    CFileSystemDescriptor& operator=(const CFileSystemDescriptor&);
 
 private:   

    RBuf    iFsName;        ///< file system name
    RBuf    iPrimaryExtName;///< name of the primary extension if present. Empty otherwise
    TBool   iDriveSynch;    ///< ETrue if the drive is synchronous

    };
    

//-----------------------------------------------------------------------------
class PropertyHandlers
	{
public:
	typedef void(*THandler)(RProperty&);
	static TBuf8<16> allDrivesStatus;
	static TUsbMsBytesTransferred iKBytesRead;
	static TUsbMsBytesTransferred iKBytesWritten;
	static TInt iMediaError;

	static void Transferred(RProperty& aProperty, TUsbMsBytesTransferred& aReadOrWritten);
	static void Read(RProperty& aProperty);
	static void Written(RProperty& aProperty);
	static void DriveStatus(RProperty& aProperty);
	static void MediaError(RProperty& aProperty);
	};

//-----------------------------------------------------------------------------
/**
An active object that tracks changes to the KUsbMsDriveState properties
*/
class CPropertyWatch : public CActive
	{
public:
	static CPropertyWatch* NewLC(TUsbMsDriveState_Subkey aSubkey, PropertyHandlers::THandler aHandler);
private:
	CPropertyWatch(PropertyHandlers::THandler aHandler);
	void ConstructL(TUsbMsDriveState_Subkey aSubkey);
	~CPropertyWatch();
	void RunL();
	void DoCancel();
	
	RProperty iProperty;
	PropertyHandlers::THandler iHandler;
	};

//-----------------------------------------------------------------------------
/**
An active object that tracks changes to the KUsbMsDriveState properties
*/
class CUsbWatch : public CActive
	{
public:
	static CUsbWatch* NewLC(TAny* aUsb);
private:
	CUsbWatch(TAny* aUsb);
	void ConstructL();
	~CUsbWatch();
	void RunL();
	void DoCancel();

	TAny* iUsb;
	TUsbDeviceState iUsbDeviceState;
	TBool iWasConfigured;
	};

//-----------------------------------------------------------------------------
class CMessageKeyProcessor : public CActive
	{
public:
	static CMessageKeyProcessor* NewLC(CConsoleBase* aConsole);
	static CMessageKeyProcessor* NewL(CConsoleBase* aConsole);
	~CMessageKeyProcessor();

public:
	// Issue request
	void RequestCharacter();
	// Cancel request.
	// Defined as pure virtual by CActive;
	// implementation provided by this class.
	void DoCancel();
	// Service completed request.
	// Defined as pure virtual by CActive;
	// implementation provided by this class,
	void RunL();
	// Called from RunL() to handle the completed request
	void ProcessKeyPress(TChar aChar);

private:
	CMessageKeyProcessor(CConsoleBase* aConsole);
	void ConstructL();
#ifndef USB_BOOT_LOADER
	void MakePassword(TMediaPassword &aPassword);
#endif
	CConsoleBase* iConsole; // A console for reading from
	};






#endif // USBMSAPP_H
