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
// f32test\virus\t_vshook.h
// 
//

#if !defined(__T_VSSHOOK_H__)
#define __T_VSSHOOK_H__

#include <f32plugin.h>

/**
The buffer sized used when scanning a file for a virus.
@internalComponent
*/
const TInt KScanBufferSize = 512;

/**
The maximum number of virus signatures that can be stored
in a virus scanner definition file.
@internalComponent
*/
const TInt KMaxVirusSignatures = 100;


/**
The actual implementation of the test virus scanning hook.
It implements all of the pure virtual functions from CVirusHook.
@internalComponent
*/
class CTestVirusHook: public CFsPlugin
	{
public:
	static CTestVirusHook* NewL();
	~CTestVirusHook();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);

private:
	enum TOperation {EFileOpen, EFileDelete, EFileRename, EFileClose};

private:
	CTestVirusHook();

	TInt VsFileOpen(TFsPluginRequest& aRequest);
	void VsFileClose(TFsPluginRequest& aRequest);
	TInt VsFileRename(TFsPluginRequest& aRequest);
	TInt VsDirRename(TFsPluginRequest& aRequest);
	TInt VsReadFileSection(TFsPluginRequest& aRequest);
	TInt VsFileDelete(TFsPluginRequest& aRequest);
	TInt VirusScannerName(TDes& aName);

public:
/**
Signature for binary compatibility testing
@internalComponent
*/
	TInt			iSignature;
/**
The virus scanners file server session
@internalComponent
*/
	RFs		   		iFs;
/**
The virus scanner thread.
@internalComponent
*/
	RThread		   iVsThread;
/**
An array containing the known virus signatures for the test virus
scanner.  This array is filled up by the virus scanner thread when
it initialised.
@internalComponent
*/
	HBufC8*		   iKnownSignatures[KMaxVirusSignatures];
/**
The number of signatures which have been loaded in the iKnownSignatures
array.
@internalComponent
*/
	TInt		   iSignaturesLoaded;
/**
A pointer to the class containing all of the functions executed within the
virus scanning thread.
@internalComponent
*/
//	CTestVsThread* iVsThreadClass;

private:
	TInt ScanFile(const TDesC& aName);
	void CleanFile(const TDesC& aName, TInt aOperation);
	TInt ScanBuffer();
	TInt ValidateRequest(TFsPluginRequest& aRequest, TFileName& aFileName);
	TInt ReadVirusDefinitionFile();
	
/**
An internal buffer used when scanning a file for virus signatures.
@internalComponent
*/
	TBuf8<KScanBufferSize> iScanBuf;

private:
	TInt iDrvNumber;
	};

#endif
