// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Public header file for "AutoMounter" file system. Contains this file system name and optional file system - specific declarations.
//
//

/**
 @file
 @publishedAll
 @released
*/

#if !defined(__FILESYSTEM_AUTOMOUNTER_H__)
#define __FILESYSTEM_AUTOMOUNTER_H__


#if !defined(__F32FILE_H__)
#include <f32file.h>
#endif


          
/**
    automounter filesystem name, which shall be provided to RFs::MountFileSystem() and is returned by RFs::FileSystemName() if 
    this file system is mounted on the drive. The literal is case-insensitive.
    @see RFs::MountFileSystem()
    @see RFs::FileSystemName()
*/
_LIT(KFileSystemName_AutoMounter, "automounter");


//------------------------------------------------------------------------------

namespace FileSystem_AUTOMOUNTER
    {
    //-- some file system specific declarations can be placed here

    }//FileSystem_AUTOMOUNTER






#endif //__FILESYSTEM_AUTOMOUNTER_H__















