// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfsrv\cl_find.cpp
// 
//

#include "cl_std.h"

#define gPathDelimiter TChar(';')
enum TMode {EFindByDrives,EFindByDrivesInPath,EFindByPath};


//
// Look for aFileName in aDir
//
TInt TFindFile::DoFindInDir()
	{

	if (!iDir)
		{
		TEntry entry;
		TInt r=iFs->Entry(iFile.FullName(),entry);

		if (r != KErrNone && r != KErrNoMemory && r != KErrPermissionDenied)
			r = KErrNotFound;

			return r;
		}
	
	TInt r=iFs->GetDir(iFile.FullName(),KEntryAttMaskSupported|KEntryAttAllowUid,ESortByName,*iDir);

    if(r == KErrNone)
        {
        if(!(*iDir)->Count())
            r = KErrNotFound; //-- empty directory
        }
    else if(r != KErrNoMemory && r != KErrPermissionDenied)
        {
            r = KErrNotFound;        
        }

	if (r != KErrNone && iDir)
		{
		delete (*iDir);
		*iDir=NULL;
		}

	return r; 
	}


//
// Look for aFileName along the path and increment aPathPos
//
TInt TFindFile::DoFindNextInPath()
	{

	if (iMode==EFindByDrivesInPath)
		{
		TInt r=DoFindNextInDriveList();
		if (r!=KErrNotFound)
			return(r);

		iMode=EFindByPath;
		}

	FOREVER
		{
		if (iPath->Length()<iPathPos)
			return(KErrNotFound);

		TPtrC path(iPath->Ptr()+iPathPos,iPath->Length()-iPathPos);

		TInt r=path.Locate(gPathDelimiter);

		if (r==KErrNotFound)
			r=path.Length();

		path.Set(path.Ptr(),r);
		iPathPos+=r+1;
		TFileName fileName=iFile.NameAndExt();
		iFile.Set(fileName,&path,NULL);

		if (iFile.FullName().Length()>=2 && iFile.FullName()[1]==KDriveDelimiter)
			{
			TInt r=DoFindInDir();
			if (r == KErrNotFound)
			continue;
			
            return(r);
			}
		
		iMode=EFindByDrivesInPath;
		
		r=FindByDir(fileName,path);
		
        if (r == KErrNotFound)
			continue;
		
			return(r);
		}

	}


//
// Look for aFileName in all available drives in order
//
TInt TFindFile::DoFindNextInDriveList()
	{
	
	TInt found;	
	TDriveInfo driveInfo;
	const TUint matchedFlags= iMatchMask & KDriveAttMatchedFlags;  //KDriveAttMatchedFlags = 0xFFF
	const TUint matchedAtt = iMatchMask & KDriveAttMatchedAtt;	 //KDriveAttMatchedAtt = 0x0FFF0000
	
	if (iMatchMask == (KDriveAttExclude | KDriveAttMatchedFlags ) ) //If all drives are excluded.
		return KErrNotFound;

	FOREVER
		{
				
		found =0;
		TInt currentDrive=iCurrentDrive;
		
		
		if (iCurrentDrive==-1)
			currentDrive=EDriveZ;
			
		
		if (currentDrive<=-2)
			return(KErrNotFound);	
			
				
		iCurrentDrive--;

		if (!iDrvList[currentDrive])
			continue;

		TInt err = iFs->Drive(driveInfo,currentDrive);

		if(iMatchMask == 0)
			{
			if (iDrvList[currentDrive] & KDriveAttRemote)
				continue;									// NOT allowed on REMOTE DRIVE
			if ((iDrvList[currentDrive] & (KDriveAttLocal|KDriveAttRom))==0)
				continue;
			}
		else 	
			{									
 
			if(matchedFlags != 0 )
				{

				switch(matchedAtt)
					{ //found ==0 means that this drive attributes didn't match the flags
					
					case KDriveAttExclude :
						found = (driveInfo.iDriveAtt & matchedFlags ) ? 0:iDrvList[currentDrive] ;
						break;

					case 0:
						found = (driveInfo.iDriveAtt & matchedFlags) ? iDrvList[currentDrive]:0 ;
						break;


					case KDriveAttExclusive :
						if(matchedFlags != KDriveAttLogicallyRemovable)
							{

							found = ((TUint8)(driveInfo.iDriveAtt) == matchedFlags)  ?iDrvList[currentDrive]:0;
								
							}
						else 
							{
							found = (driveInfo.iDriveAtt == (matchedFlags | KDriveAttRemovable) )  ?iDrvList[currentDrive]:0;
							}
						
						break;

					case KDriveAttExclude | KDriveAttExclusive:
						if(matchedFlags != KDriveAttLogicallyRemovable)
							{
						
							found = ((TUint8)(driveInfo.iDriveAtt) == matchedFlags)  ?0:iDrvList[currentDrive];
								
							}
						else 
							{
							found = (driveInfo.iDriveAtt == (matchedFlags | KDriveAttRemovable))  ?0:iDrvList[currentDrive];
							}
						
						break;

					default: 
						Panic(EFindFileIllegalAttribute);			
							
					}
				}

			else  //matchedFlags == 0.
				{
				
				if (matchedAtt== KDriveAttAll)		
					found= iDrvList[currentDrive];
				else 
					Panic(EFindFileIllegalAttribute);		
									
				}
			
		
		
			if( found == 0)
				continue;			

			}
		
		// Don't scan a locked drive
		
		if( err == KErrNone)
			{		
			if(driveInfo.iMediaAtt & KMediaAttLocked)
				continue;
			}
		

		TParse fileName;
		TChar driveLetter;
		RFs::DriveToChar(currentDrive,driveLetter);
		TBuf<4> drive;
		drive.SetLength(2);
		drive[0]=(TText)driveLetter;
		drive[1]=':';
		TPtrC nameAndExt(iFile.NameAndExt());
		TPtrC path(iFile.Path());
		fileName.Set(nameAndExt,&path,&drive);
		iFile=fileName;
		
		TInt r=DoFindInDir();
		
		if (r==KErrNone)
			return(KErrNone);
		
		if (r!=KErrNotFound)
			return(r);
		}	
	}




/**
Constructor taking a file server session.

@param aFs File server session.
*/
EXPORT_C TFindFile::TFindFile(RFs& aFs)
	               :iFs(&aFs), iPathPos(0), iCurrentDrive(0), iMode(-1), iDir(NULL), iMatchMask(0)
	{
	iFile.Set(_L(""),NULL,NULL);
	}




TInt TFindFile::DoFindByPath(const TDesC& aFileName,const TDesC* aPath)
//
// Look for a file in each directory in the path
// Make initial check for aFileName in the current directory
//
	{
	
	if (aFileName.Length() <= 0) 
		return(KErrArgument);
	
	TInt r=iFile.Set(aFileName,NULL,NULL);
	if (r!=KErrNone)
		return(r);

	iPath=aPath;
	iPathPos=0;
	iMode=EFindByPath;
	r=DoFindInDir();	
	
	// if it's not in the current dir and a search path was specified, look there.
	if (r == KErrNotFound && iPath && iPath->Length())
	r=DoFindNextInPath();

	return(r);
	}


//
// Look for aFileName in aDir on each connected drive
// Make initial check for aFileName in aDir on current drive
//
TInt TFindFile::DoFindByDir(const TDesC& aFileName,const TDesC& aDir)
	{
		
	if (aFileName.Length() <= 0) 
		return(KErrArgument);

	TInt r=iFs->Parse(aFileName,aDir,iFile);
	if (r!=KErrNone)
		return(r);
	
	TInt searchResult=DoFindInDir();
	if(searchResult == KErrNoMemory || searchResult == KErrPermissionDenied)
		return(searchResult);
	
	r=iFs->DriveList(iDrvList,KDriveAttAll);
	if (r!=KErrNone)
		return(r);
	
	TInt drive;
	r=RFs::CharToDrive(iFile.Drive()[0],drive);
	if (r!=KErrNone)
		return(r);

	iDrvList[drive]=0; // Drive 'drive' has already been searched
	iCurrentDrive=EDriveY;
	iMode=EFindByDrives;
	
	if (searchResult==KErrNone)
		return(KErrNone);
	
	
	return(DoFindNextInDriveList());
	}

/**
    internal helper method that deletes the (*iDir) object in the case of errors and assigns NULL to the client's pointer top it.
*/
TInt TFindFile::CallSafe(TInt aResult)
	{
	if (aResult != KErrNone && iDir)
		{
		delete *iDir;
		*iDir = NULL;
		iDir = NULL;
		}
	return aResult;
	}


/**
Searches for a file/directory in one or more directories in the path.

The search ends when the file/directory is found, or when every directory
specified in the path list has been unsuccessfully searched.

Notes:
	
1. For the equivalent search using wildcards, use FindWildByPath().

2. After a file has been found, use TFindFile::File() to get the fully qualified path of the file. To
   search for the next occurrence, use TFindFile::Find().

@param aFileName The filename to search for. If this specifies a directory as well
                 as a filename, then that directory is searched first.
@param aPath     A list of directories to be searched. Paths in this list must
                 be separated by a semicolon character, but a semicolon is not
                 required after the	final path. The directories are searched in
                 the order in which they occur in the list. If a path in
                 the list contains a drive letter, that drive alone is searched.
                 If a path contains no drive letter, the function searches for
                 the file in that directory on every available drive in turn,
                 beginning with drive Y:, in descending alphabetical order
                 and ending with drive Z:.When path is empty then session path 
                 will be used for the search.

@return KErrNone, if the filename was found;
        KErrNotFound, if the filename was not found.
        KErrArgument, if the filename is empty. 

@see TFindFile::FindWildByPath
@see TFindFile::File
@see TFindFile::Find
*/
EXPORT_C TInt TFindFile::FindByPath(const TDesC& aFileName,const TDesC* aPath)
	{

	iDir=NULL;
	return CallSafe(DoFindByPath(aFileName,aPath));
	}




/**
Searches for a file/directory in a directory on all available drives.

The	search ends when the file/directory is found, or when every available
drive has been unsuccessfully searched.

Notes:

1. A drive letter may be specified in aDirPath.
   If a drive is specified, then that drive is searched first. If no drive is
   specified, then the drive specified in the session path is searched first.
   The remaining available drives are then searched in descending alphabetical
   order, from Y: to A:, and ending with the Z: drive. Using function SetFindMask
   it is possible to specify a combination of attributes that the drives to be 
   searched must match.
   

2. For the corresponding search using wildcards, use FindWildByDir().

3. After a file has been found, use TFindFile::File() to get the fully
   qualified path and filename. To search for the next occurrence,
   use TFindFile::Find().

@param aFileName The filename to search for. If a path is specified, it
                 overrides the path specified in aDir. If no path is specified,
                 the path contained in aDir is used.
@param aDir      A single path indicating a directory to be searched on each
                 drive.When path is empty then session path will be used for the search.

@return KErrNone, if the file was found, otherwise one of the system-wide error codes, including:
        KErrNotFound, if the file was not found;
        KErrPermissionDenied, if the client does not have appropriate capabilities for the directory to be searched;
		KErrArgument, if the filename is empty.

@see TFindFile::FindWildByDir()
@see TFindFile::File()
@see TFindFile::Find()
@see TFindFile::SetFindMask()
*/
EXPORT_C TInt TFindFile::FindByDir(const TDesC& aFileName,const TDesC& aDir)
	{

	iDir=NULL;
	return CallSafe(DoFindByDir(aFileName,aDir));
	}





/**
Searches for one or more files/directories in the directories contained in a path list.

Wildcard characters can be specified. The search ends when one or more filenames matching aFileName is found, or when every
directory in the path list has been unsuccessfully searched. To begin searching again after a successful match has been made, use FindWild().

Using function SetFindMask it is possible to specify a combination of  attributes that the drives to be searched must match.

Notes:

1. The function sets aDir to NULL, then allocates memory for it before appending entries to the list. Therefore, 
   aDir should have no memory allocated to it before this function is called, otherwise this memory will become orphaned.

2. The caller of the function is responsible for deleting aDir after the function has returned. On error this pointer will be set NULL,
   thus safe to delete.     

3. Calling TFindFile::File() after a successful search gets the drive letter and directory containing the file(s). 
   The filenames can be retrieved via the array of TEntry::iName objects contained in aDir. If you want to  retrieve the fully 
   qualified path of a file, you need to parse the path and the filename.
   
@param aFileName The filename to search for. May contain wildcards. If it specifies a directory as well as a filename, then that
                 directory is searched first.

@param aPath     List of directories to search. Paths in this list must be separated by a semicolon character, but a semicolon is not
                 required after the final path. The directories are searched in the order in which they occur in the list.
                 Directories must be fully qualified, including a drive letter, and the name must end with a backslash.

@param aDir      in: a reference to the pointer that will be modified by this method.
                 
                 out: On success a pointer to the internally allocated by this method CDir object, which in turn contains the entries for 
                 all files matching aFileName in the first directory in which a match occurred. In this case this API caller is responsible
                 for deleting aDir.
                 If some error occured (including KErrNotFound meaning that nothing found) this pointer will be set to NULL, which is also safe to delete.

@return KErrNone, if one or more matching files was	found;
        KErrNotFound, if no matching file was found in any of the directories.
        KErrArgument, if the filename is empty.

@see TFindFile::FindWild
@see TFindFile::File
@see TEntry::iName
@see TFindFile::SetFindMask()
*/
EXPORT_C TInt TFindFile::FindWildByPath(const TDesC& aFileName,const TDesC* aPath,CDir*& aDir)
	{

	iDir=&aDir;
	*iDir=NULL;

	return CallSafe(DoFindByPath(aFileName,aPath));
	}





/**
Searches, using wildcards, for one or more files/directories in a specified directory.

If no matching file is found in that directory, all available drives are searched in descending alphabetical order, from Y: to A:, and ending
with the Z: drive.Using function SetFindMask it is possible to specify a combination of attributes that the drives to be searched must match.

The search ends when one or more matching filenames are found, or when every  available drive has been unsuccessfully searched. 
To begin searching again after a successful match has been made, use FindWild(). Wildcards may be specified in the filename.

Notes:

1. A drive letter may be specified in aDirPath (or in aFileName). If a drive  is specified, that drive is searched first, 
   followed by the other available drives, in descending alphabetical order. If no drive is specified, the drive contained in the session 
   path is searched first.

2. The function sets aDir to NULL, then allocates memory for it before appending entries to the list. Therefore, 
   aDir should have no memory allocated to it before this function is called, otherwise this memory will become orphaned.

3. The caller of the function is responsible for deleting aDir after the function has returned. On error this pointer will be set NULL,
   thus safe to delete.     


4. Calling TFindFile::File() after a successful search returns the drive letter and directory containing the file(s). 
   Filenames may be retrieved via the array of TEntry::iNames contained in aDir. If you want to retrieve the fully 
   qualified path of a file, you will need to parse the path and the filename.

@param aFileName The filename to search for. May contain wildcards. If a path is specified, it overrides the path specified in aDirPath.
                 If no path is specified, the path contained in aDirPath is used in the search.

@param aDirPath  Path indicating a directory to search on each drive.

@param aDir      in: a reference to the pointer that will be modified by this method.
                 
                 out: On success a pointer to the internally allocated by this method CDir object, which in turn contains the entries for 
                 all files matching aFileName in the first directory in which a match occurred. In this case this API caller is responsible
                 for deleting aDir.
                 If some error occured (including KErrNotFound meaning that nothing found) this pointer will be set to NULL, which is also safe to delete.
                 
@return KErrNone if one or more matching files was found;
        KErrNotFound if no matching file was found in the directory on any of the drives.
        KErrArgument, if the filename is empty. 
                
@see TFindFile::FindWild
@see TFindFile::File
@see TFindFile::SetFindMask()
*/
EXPORT_C TInt TFindFile::FindWildByDir(const TDesC& aFileName,const TDesC& aDirPath,CDir*& aDir)
	{
	iDir=&aDir;
    *iDir=NULL;
	
	return CallSafe(DoFindByDir(aFileName,aDirPath));
	}




//
// Find the next match
//
TInt TFindFile::DoFind()
	{

	TInt ret=KErrNone;
	switch(iMode)
		{
	case EFindByDrives:
		ret=DoFindNextInDriveList();
		break;
	case EFindByPath:
	case EFindByDrivesInPath:
		ret=DoFindNextInPath();
		break;
	default:
		Panic(EFindFileIllegalMode);
		}
	return(ret);
	}





/**
Searches for the next file/directory.

This should be used after a successful call to FindByPath() or FindByDir(), 
to find the next occurrence of the filename in the path or drive list.

Using function SetFindMask it is possible to specify a combination of 
attributes that the drives to be searched must match.

Note:

1. After a file/directory has been found, use TFindFile::File() to get the
   fully qualified path and filename.

@return KErrNone, if another occurrence of the file was found;
        KErrNotFound, if no more occurrences were found.
        
@see TFindFile::FindByPath
@see TFindFile::FindByDir
@see TFindFile::File
@see TFindFile::SetFindMask()

*/
EXPORT_C TInt TFindFile::Find()
	{

//	iDir=NULL;
	return(DoFind());
	}





/**
Searches for the next file/directory.

This should be used after a successful call to FindWildByPath()
or FindWildByDir(), for the next occurrences of the filename in the
path or drive list.Using function SetFindMask it is possible to specify a 
combination of attributes that the drives to be searched must match.

Notes:

1. The function sets aDir to NULL, then allocates memory for it before appending entries to the list. Therefore, 
   aDir should have no memory allocated to it before this function is called, otherwise this memory will become orphaned.

2. The caller of the function is responsible for deleting aDir after the function has returned. On error this pointer will be set NULL,
   thus safe to delete.     

3. Calling TFindFile::File() after a successful search, will return the drive letter and the directory containing the file(s).
   The filenames may be retrieved via the array of TEntry::iName objects contained in aDir. If you want to retrieve the fully qualified
   path of a file, you will need to parse the path and the filename using the TParse class or derived classes.

@param aDir      in: a reference to the pointer that will be modified by this method.
                 
                 out: On success a pointer to the internally allocated by this method CDir object, which in turn contains the entries for 
                 all files matching aFileName in the first directory in which a match occurred. In this case this API caller is responsible
                 for deleting aDir.
                 If some error occured (including KErrNotFound meaning that nothing found) this pointer will be set to NULL, which is also safe to delete.
            
@return KErrNone      if further occurrences were found;
        KErrNotFound  if no more matching files were found.


@see TParse
@see TEntry::iName
@see TFindFile::File
@see TFindFile::FindWildByPath
@see TFindFile::FindWildByDir
@see TFindFile::SetFindMask()
*/
EXPORT_C TInt TFindFile::FindWild(CDir*& aDir)
	{

	iDir=&aDir;
    *iDir=NULL;

	return CallSafe(DoFind());
	}




/**
Can be used in order to specify a combination of drive attributes that the drives 
to be searched must match. When searching without specifying a mask, all drives, except the 
remote ones will be returned.

@param aMask The combination of drive attributes that we want the drives to match. 

@return KErrNone, if the mask supplied is correct.
        KErrArgument, if the mask supplied is invalid.
*/

EXPORT_C TInt TFindFile::SetFindMask(TUint aMask)
	 {	
	 TInt r =ValidateMatchMask(aMask);
	 if(r!=KErrNone) 
	 	return r;
	 else
	 	{
	 	if(aMask != 0)
			iMatchMask = aMask;
		else 		
			iMatchMask  = (KDriveAttExclude |KDriveAttMatchedFlags) ; //KDriveAttMatchedFlags ==0xFF so this exclude all drives in DoFindNextInDriveList
	 	
	 	return KErrNone;
	 	
	 	}
	 }

