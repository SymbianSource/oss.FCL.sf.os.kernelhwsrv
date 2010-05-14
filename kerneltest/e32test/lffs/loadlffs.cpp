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
// e32test\lffs\loadlffs.cpp
// 
//

#include <f32file.h>

LOCAL_D RFs TheFs;

LOCAL_C void FormatLFFS(TDes &drive)
	{

    RFormat format;
    TInt    count;
    
    RDebug::Print(_L("Format LFFS drive %S\r\n"), &drive);
	format.Open(TheFs, drive, EHighDensity, count);
    
    while (count)
		{
        format.Next(count);
    	}
    
    format.Close();
	}


//LOCAL_C void ForceMediaRemount( TMediaDevice aMedia )
//	//
//	// Force a remount on next access
//	//
//	{
//	UserSvr::ForceRemountMedia( aMedia );
//	}



TInt MountLFFS()
	{
    RDebug::Print(_L("Read machine information"));
    
    TInt LFFSdriveNumber;
    TBuf<4> LFFSdriveLetter;
    
    LFFSdriveLetter.Zero();
    LFFSdriveLetter.Append(_L("K:\\"));
    LFFSdriveNumber = EDriveK;
    
    RDebug::Print(_L("Load device driver: MEDLFS"));
    TInt r=User::LoadPhysicalDevice(_L("MEDLFS"));
    if(r != KErrNone && r != KErrAlreadyExists)
		{
		RDebug::Print(_L("Loading device driver failed"));
		return(-1);
		}
	
    RDebug::Print(_L("Add file system: ELFFS"));
    r=TheFs.AddFileSystem(_L("ELFFS"));
    if(r != KErrNone && r != KErrAlreadyExists)
		RDebug::Print(_L("Failed to add ELFFS"));
	
    
    TFullName name;
    r = TheFs.FileSystemName(name, LFFSdriveNumber);
    
    if (name.Length() != 0)
		{
        RDebug::Print(_L("Dismounting %S on drive %S\r\n"), &name, &LFFSdriveLetter);
        r=TheFs.DismountFileSystem(name, LFFSdriveNumber);
		RDebug::Print(_L("Dismount ret=%d"), r);
    	}

//    ForceMediaRemount( EFixedMedia1 );

    RDebug::Print(_L("Mount LFFS on drive %S\r\n"), &LFFSdriveLetter);
    r=TheFs.MountFileSystem(_L("Lffs"), LFFSdriveNumber);
	RDebug::Print(_L("Mount r=%d"),r);
//uncommented	
    if (r == KErrCorrupt || r == KErrNotReady) 
		{
		RDebug::Print(_L("FS Corrupt, formatting"));
	  	FormatLFFS(LFFSdriveLetter);
    	}
 ///to here   
    TheFs.SetSessionPath(LFFSdriveLetter);
    return(LFFSdriveNumber);
	}

GLDEF_C TInt E32Main()
//
// Load the LFFS on C:
//
	{

    CTrapCleanup* cleanup;
    cleanup=CTrapCleanup::New();

	TInt r=TheFs.Connect();
	RDebug::Print(_L("Connect ret %d"),r);
	
	TRAP(r,MountLFFS());
	RDebug::Print(_L("Mount LFFS ret %d"),r);
	
    TheFs.Close();
    delete cleanup;
    return(KErrNone);
	}

