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
// e32test\lffs\loadntfs.cpp
// 
//

#include <f32file.h>

_LIT(KFileSystemDllName, "ntfs.fsy");
_LIT(KFileSystemName, "ntfs");

RFs TheFs;

TInt MountNTFS()
	{
	TBuf<256> cmd;
	User::CommandLine(cmd);
	TLex cmdlex(cmd);
	cmdlex.SkipSpace();
	TUint c = (TUint)cmdlex.Get();
	if (c>='a' && c<='z')
		c-=0x20;
	if (c<'A' || c>'Z')
		return KErrArgument;
	TBuf<4> driveLetter;
	driveLetter.SetLength(1);
	driveLetter[0] = (TText)c;
    RDebug::Print(_L("Drive %S"), &driveLetter);
    
    TInt driveNumber = TInt(c-'A') + TInt(EDriveA);
	TInt r;
	driveLetter.Append(_L(":\\"));
    
    RDebug::Print(_L("Add file system: %S"), &KFileSystemDllName);
    r=TheFs.AddFileSystem(KFileSystemDllName);
    if (r!=KErrNone && r!=KErrAlreadyExists)
		{
		RDebug::Print(_L("Failed: %d"), r);
		return r;
		}

	TFullName name;
	r = TheFs.FileSystemName(name, driveNumber);
	if (name.Length() != 0)
		{
        RDebug::Print(_L("Dismounting %S on drive %S\r\n"), &name, &driveLetter);
        r=TheFs.DismountFileSystem(name, driveNumber);
		RDebug::Print(_L("Dismount ret=%d"), r);
    	}

    RDebug::Print(_L("Mount NTFS on drive %S\r\n"), &driveLetter);
    r = TheFs.MountFileSystem(KFileSystemName, driveNumber);
	RDebug::Print(_L("Mount r=%d"),r);
	return KErrNone;
	}

GLDEF_C TInt E32Main()
	{

    CTrapCleanup* cleanup;
    cleanup=CTrapCleanup::New();

	TInt r=TheFs.Connect();
	RDebug::Print(_L("Connect ret %d"),r);

	if (r == KErrNone)
		r = MountNTFS();
	RDebug::Print(_L("Mount NTFS ret %d"),r);
	
    TheFs.Close();
    delete cleanup;
    return KErrNone;
	}

