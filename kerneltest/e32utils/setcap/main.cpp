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
// e32utils\setcap\main.cpp
// SETCAP.EXE
// Makes a copy of an executable file and gives it the specified capabilities.
// It may also, optionally, modify the Secure or Vendor IDs.
// This runs under the Symbian OS - it is not a native PC utility.
// Command line syntax:
// SETCAP source_exe capability [-SID secureId] [-VID vendorId] [destination_path]
// source_exe         Name and path of an executable file (default path is Z:\SYS\BIN\)
// capability         Hexadecimal value for capabilities
// secureId			 Optional hexadecimal value of secure ID
// vendorId			 Optional hexadecimal value of vendor ID
// destination_path   Optional name and path to copy the exe to
// (defaults to C:\SYS\BIN\source_exe_name)
// Notes
// 1.  The 'capability' command line argument is the hexadecimal value of the
// capabilities when they are represented as a bit-field. E.g. the 3 capabilities
// LocalServices, ReadUserData and WriteUserData would together have a value of:
// (1<<ECapabilityLocalServices) | (<<ECapabilityReadUserData) | (1<<ECapabilityWriteUserData)
// Which in hexadecimal is '1c000'
// If the value supplied includes capabilities which aren't supported by the current
// OS version, then these are ignored and not added to the file.
// 2.  If the source executable is in ROM it must be a RAM executable image, not an
// execute-in-place image. I.e. its entry in an OBY file must start with "data=" and
// not "file=".
// For OBY files generated automatically by "ABLD ROMFILE" this needs to be achieved by
// using lines similar to the following in the executables MMP file:
// ROMTARGET    // Empty ROM path means don't include normal execute-in-place file
// RAMTARGET \sys\bin\    // Target path (in ROM) for RAM executable image
// 3.  The Symbian OS only allows one binary file with a given name; the name doesn't
// include file path or extension. This means if SETCAP is used to make a copy of a
// binary which is already loaded then the copy will not get loaded when used with
// RProcess::Create(), instead the already loaded version will be used. To avoid this,
// use SETCAP to give the copy a different name. E.g. "SETCAP test.exe 1234 test2.exe"
// 
//

/**
 @file
*/

#include "setcap.h"

#include <f32file.h>

TParse SourceName;
TParse DestinationName;
RFs Fs;

#ifdef __WINS__

TInt DoIt()
	{
	TInt  r;

	TBuf<MAX_PATH> sName;
	r = MapEmulatedFileName(sName, SourceName.NameAndExt());
	if(r!=KErrNone)
		return r;

	TBuf<MAX_PATH> dName;
	r = MapEmulatedFileName(dName, DestinationName.FullName());
	if(r!=KErrNone)
		return r;

	if(!Emulator::CopyFile((LPCTSTR)sName.PtrZ(),(LPCTSTR)dName.PtrZ(),FALSE))
		return KErrGeneral;

	HANDLE hFile=Emulator::CreateFile((LPCTSTR)dName.PtrZ(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return KErrArgument;

	return SetCap(hFile);
	}

#else // Not WINS

#include <f32file.h>

RFile Source;
RFile Destination;

TInt DoIt()
	{
	TInt r;

	r=Source.Open(Fs,SourceName.FullName(),EFileRead);
	if(r!=KErrNone)
		return r;

	r = Destination.Replace(Fs,DestinationName.FullName(),EFileWrite);
	if(r!=KErrNone)
		return r;

	TUint8* buffer;
	const TInt KBufferSize = 0x10000;

	buffer = (TUint8*)User::Alloc(KBufferSize);
	if(!buffer)
		return KErrNoMemory;

	TPtr8 p(buffer,KBufferSize,KBufferSize);
	TInt n = 0;
	while (r==KErrNone)
		{
		r = Source.Read(p);
		if(r!=KErrNone || p.Size()==0)
			break;
		if (n==0)
			{
			// first block contains header
			if ((TUint)p.Size() < sizeof(E32ImageHeader))
				{
				r = KErrCorrupt;
				break;
				}
			E32ImageHeader* h = (E32ImageHeader*)buffer;
			r = SetCap(h);
			}
		if (r==KErrNone)
			r = Destination.Write(p);
		++n;
		}

	delete buffer;

	Source.Close();
	Destination.Close();

	return r;
	}

#endif

_LIT(KDefaultSourcePath,"z:\\sys\\bin\\");
_LIT(KDefaultDestinationPath,"?:\\sys\\bin\\");
_LIT(KSIDOption,"-SID");
_LIT(KVIDOption,"-VID");

TInt ParseCommandLine()
	{
	TBuf<256> c;
	User::CommandLine(c);

	// Get exe name
	TLex l(c);
	if(SourceName.SetNoWild(l.NextToken(),0,&KDefaultSourcePath)!=KErrNone)
		return KErrArgument;

	// Get capability
	TLex cl(l.NextToken());
	if(cl.Val((TInt64&)Capability,EHex)!=KErrNone)
		return KErrArgument;

	// Mask out unsupported capabilities
	TCapabilitySet all;
	all.SetAllSupported();
	((TCapabilitySet&)Capability).Intersection(all);

	// We always update capabilities in the headers
	CapabilitySet = ETrue;

	// Get options
	SecureIdSet = EFalse;
	VendorIdSet = EFalse;
	TPtrC nextToken;
	for (;;)
		{
		nextToken.Set(l.NextToken());
		if (nextToken == KSIDOption)
			{
			// SID specified
			nextToken.Set(l.NextToken());
			if (nextToken == KNullDesC)
				return KErrArgument;				
			TLex sl(nextToken);
			if(sl.Val(SecureId.iId,EHex)!=KErrNone)
				return KErrArgument;
			SecureIdSet = ETrue;
			}
		else if (nextToken == KVIDOption)
			{
			// VID specified
			nextToken.Set(l.NextToken());
			if (nextToken == KNullDesC)
				return KErrArgument;				
			TLex sl(nextToken);
			if(sl.Val(VendorId.iId,EHex)!=KErrNone)
				return KErrArgument;
			VendorIdSet = ETrue;
			}
		else
			break;
		}
				
	// Get target path
	TPtrC s(SourceName.NameAndExt());
	TBuf<sizeof(KDefaultDestinationPath)> defaultDestinationPath(KDefaultDestinationPath);
	defaultDestinationPath[0] = (TUint8) RFs::GetSystemDriveChar();
	
	if(DestinationName.SetNoWild(nextToken,&s,&defaultDestinationPath)!=KErrNone)
		return KErrArgument;

	// Check we used all the arguments
	if (l.NextToken() != KNullDesC)
		return KErrArgument;

	return KErrNone;
	}


TInt E32Main()
	{
	TInt r;

	 // Turn off lazy dll unloading
	RLoader l;
	if ((r=l.Connect())!=KErrNone)
		return r;
	r = l.CancelLazyDllUnload();
	l.Close();
	if (r!=KErrNone)
		return r;
	
	r = ParseCommandLine();
	if(r!=KErrNone)
		return r;
	r = Fs.Connect();
	if(r!=KErrNone)
		return r;
	r = Fs.MkDirAll(DestinationName.FullName());
	if(r==KErrNone || r==KErrAlreadyExists)
		r = DoIt();
	Fs.Close();
	return r;
	}
