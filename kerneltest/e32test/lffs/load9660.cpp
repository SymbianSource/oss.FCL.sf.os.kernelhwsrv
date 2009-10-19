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
// e32test\lffs\load9660.cpp
// 
//

#include <f32file.h>

_LIT(KFileSystemDllName, "iso9660.fsy");
_LIT(KFileSystemName, "iso9660");

RFs TheFs;

TInt MountISO9660()
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

    RDebug::Print(_L("Mount ISO9660 on drive %S\r\n"), &driveLetter);
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
		r = MountISO9660();
	RDebug::Print(_L("Mount ISO9660 ret %d"),r);
	
    TheFs.Close();
    delete cleanup;
    return KErrNone;
	}

