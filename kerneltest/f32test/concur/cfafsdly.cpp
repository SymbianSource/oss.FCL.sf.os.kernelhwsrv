// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// @file f32test\concur\cfafsdly.cpp
//


#include "cfafsdly.h"
#include "cfafsdlyif.h"
#include "cfafshmem.h"

IMPORT_C TUint32 DebugRegister();

const TInt KMajorVersionNumber=1;
const TInt KMinorVersionNumber=0;

class TTestDebug
/// Class containing debugging functions.
    {
public:
    static void Printf(TRefByValue<const TDesC> aFmt, ...)
        {
        if (DebugRegister() & KDLYTRC)
            {
            VA_LIST list;
            VA_START(list, aFmt);
            TBuf<256>buf;
            buf.FormatList(aFmt, list);
            RDebug::Print(_L("%S"), &buf);
            }
        }
	static void After(TInt usec)
		{
		RTimer timer;
		timer.CreateLocal();
		TRequestStatus stat;
		timer.After(stat, usec);
		User::WaitForRequest(stat);
		timer.Close();
		}
private:
    };

TTestFile::TTestFile()
/// Initialise a file (clear the size).
	{
	iSize = 0;
	}

void TTestFile::Entry(TEntry& aEntry) const
/// Set up a standard entry item for the file.  Note that it strips off any
/// path from the name of the file.
/// @param aEntry Item to receive data about the file.
	{
	TInt i = iName.LocateReverse('\\');
	if (i == KErrNotFound)
		i = 0;
	else
		++i;
	aEntry.iName     = iName.Mid(i);
	aEntry.iAtt      = KEntryAttNormal;
	aEntry.iSize     = iSize;
	aEntry.iModified = iTime;
	}


TTestDir::TTestDir()
	{
	}

TTestFile* TTestDir::Create(const TDesC& aName)
/// Create a new file, if there is any space left.
/// @param aName Name of the file to be created.
/// @return Pointer to the file structure if it there is space, NULL if not.
	{
	TInt i;
	for (i = 0; i < KMaxFiles; i++)
		{
		if (iFile[i].iName.Length() == 0)
			{
			iFile[i].iName = aName;
			iFile[i].iSize = 0;
			return &iFile[i];
			}
		}
	return NULL;
	}

const TTestFile* TTestDir::Find(const TDesC& aName) const
/// Find a file by name.
/// @param aName Name of file to find (no wildcards).
/// @return Pointer to the file structure if it is found, NULL if not.
	{
	TInt i;
	for (i = 0; i < KMaxFiles; i++)
		{
		if (aName == iFile[i].iName)
			{
			return &iFile[i];
			}
		}
	return NULL;
	}

TTestFile* TTestDir::Find(const TDesC& aName)
/// Find a file by name.
/// @param aName Name of file to find (no wildcards).
/// @return Pointer to the file structure if it is found, NULL if not.
	{
	TInt i;
	for (i = 0; i < KMaxFiles; i++)
		{
		if (aName == iFile[i].iName)
			{
			return &iFile[i];
			}
		}
	return NULL;
	}

void TTestDir::Delete(const TDesC& aName)
/// Delete a file by name (no wildcards) by setting its name to zero length.
/// No error is returned if it doesn't exist.
/// @param aName Name of file to delete (no wildcards).
	{
	TInt i;
	for (i = 0; i < KMaxFiles; i++)
		{
		if (aName == iFile[i].iName)
			{
			iFile[i].iName.SetLength(0);
			iFile[i].iSize = 0;
			}
		}
	}

TTestFile* TTestDir::Entry(TInt aIndex)
/// Return a pointer to the specified file by number, or NULL if the index
/// is out of range.
	{
	return (aIndex >= 0 && aIndex < KMaxFiles ? &iFile[aIndex] : NULL);
	}

CTestFileSystem::CTestFileSystem()
///
/// Constructor
///
	{
	__DECLARE_NAME(_S("CTestFileSystem"));
	TTestDebug::Printf(_L("CTestFileSystem::CTestFileSystem()\n"));
	}

CTestFileSystem::~CTestFileSystem()
///
/// Destructor
///
	{
	TTestDebug::Printf(_L("CTestFileSystem::~CTestFileSystem()\n"));
	}

TInt CTestFileSystem::Install()
///
/// Install the file system
///
	{
	TTestDebug::Printf(_L("CTestFileSystem::Install()\n"));
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KF32BuildVersionNumber);
	TPtrC name=_L("DelayFS");
	return(SetName(&name));
	}

CMountCB* CTestFileSystem::NewMountL() const
///
/// Create a new mount control block
///
	{
	TTestDebug::Printf(_L("CTestFileSystem::NewMountL()\n"));
	return (new(ELeave) CTestMountCB);
	}

CFileCB* CTestFileSystem::NewFileL() const
///
/// Create a new file
///
	{
	TTestDebug::Printf(_L("CTestFileSystem::NewFileL()\n"));
	return (new(ELeave) CTestFileCB);
	}

CDirCB* CTestFileSystem::NewDirL() const
///
/// create a new directory lister
///
	{
	TTestDebug::Printf(_L("CTestFileSystem::NewDirL()\n"));
	return (new(ELeave) CTestDirCB);
	}

CFormatCB* CTestFileSystem::NewFormatL() const
///
/// Create a new media formatter
///
	{
	TTestDebug::Printf(_L("CTestFileSystem::NewFormatL()\n"));
	return (new(ELeave) CTestFormatCB);
	}

TInt CTestFileSystem::DefaultPath(TDes& aPath) const
///
/// Return the intial default path
///
	{
	TTestDebug::Printf(_L("CTestFileSystem::DefaultPath(%S)\n"), aPath.Ptr());
	aPath=_L("C:\\");
	return (KErrNone);
	}

void CTestFileSystem::DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const
///
/// Return drive info - iDriveAtt already set
///
	{
	TTestDebug::Printf(_L("CTestFileSystem::DriveInfo(%d)\n"), aDriveNumber);
	anInfo.iMediaAtt = KMediaAttFormattable;
	anInfo.iType     = EMediaHardDisk;
//	anInfo.iDriveAtt = KDriveAttRemote | KDriveAttRemovable;
	anInfo.iDriveAtt = KDriveAttRemote;
	}

TBusLocalDrive& CTestFileSystem::DriveNumberToLocalDrive(TInt aDriveNumber)
///
/// Return the local drive associated with aDriveNumber
///
	{
	TTestDebug::Printf(_L("CTestFileSystem::DriveNumberToLocalDrive()\n"));
	return(GetLocalDrive(aDriveNumber));
	}

/**
Reports whether the specified interface is supported - if it is,
the supplied interface object is modified to it

@param aInterfaceId     The interface of interest
@param aInterface       The interface object
@return                 KErrNone if the interface is supported, otherwise KErrNotFound 

@see CFileSystem::GetInterface()
*/
TInt CTestFileSystem::GetInterface(TInt aInterfaceId, TAny*& aInterface,TAny* aInput)
    {
    switch(aInterfaceId)
        {
        case CFileSystem::EProxyDriveSupport: // The Filesystem supports proxy drives
			return KErrNone;

        default:
            return(CFileSystem::GetInterface(aInterfaceId, aInterface, aInput));
        }
    }

CFileSystem* CTestFileSystem::NewL()
///
/// Create a new filesystem and return the pointer to its structure.
///
	{
	CFileSystem* testFSys = new(ELeave) CTestFileSystem;
	return testFSys;
	}


/////////////////////////////////////////////////////////////////////////////

static void cvtbuff(TBuf<64>& aOut, const TDesC8& aIn)
/// Copy an 8-bit buffer to a generic one.
/// @param aOut Generic text buffer.
/// @param aIn  8-bit buffer to be copied.
    {
    TInt i;
    TInt n = aIn.Length();
    if (n > 64)
        n = 64;
    aOut.SetLength(n);
    for (i = 0; i < n; i++)
        aOut[i] = TText(aIn[i]);
    }

CTestFileCB::CTestFileCB() : CFileCB(), iFile(NULL), iData(NULL)
	{
	TTestDebug::Printf(_L("CTestFileCB::CTestFileCB()\n"));
	};

CTestFileCB::~CTestFileCB()
	{
	CTestFileCB* file = (CTestFileCB*)this;
	if (file->iFile)
		TTestDebug::Printf(_L("CTestFileCB::~CTestFileCB(%S)\n"), &file->iFile->iName);
	else
		TTestDebug::Printf(_L("CTestFileCB::~CTestFileCB()\n"));
	};

void CTestFileCB::RenameL(const TDesC& aNewName)
/// Rename a file (not supported).
	{
	TTestDebug::Printf(_L("CTestFileCB::RenameL(%S)\n"), aNewName.Ptr());
	}

void CTestFileCB::ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage)
/// Read from a file.  This introduces a fixed delay of 1 second (by default;
/// 10mS if KDLYFAST bit is set in the debug register), gets the data from the
/// file area and optionally traces it.
	{
	// set up lengths
	TInt max = (aPos > iSize ? 0 : iSize - aPos);
	TInt len = (aLength > max ? max : aLength);
	// set up return data
	THMem  dataBuffer(aDes, aMessage);
	dataBuffer.Write(iData+aPos, len, 0);
	aLength = len;
	/// now output traces
	TTestDebug::Printf(_L("CTestFileCB::ReadL(%4d, %4d, %.8X)\n"), aPos, aLength, aDes);
	TPtr8    pbuf(iData+aPos, len, max);
	TBuf<64> buf;
	cvtbuff(buf, pbuf);
	TTestDebug::After(DebugRegister() & KDLYFAST ? 10000 : 1000000);
	TTestDebug::Printf(_L("CTestFileCB::ReadL(%4d, %4d, %.8X=%.32S) exit\n"), aPos, aLength, aDes, &buf);
	}

void CTestFileCB::WriteL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage)
/// Write to a file.  This introduces a fixed delay of 1 second (by default;
/// 10mS if KDLYFAST bit is set in the debug register), puts the data into the
/// file area if there is room and optionally traces it.
	{
	// Set the modified attribute (so Flush() makes its way through)
	iAtt |= KEntryAttModified;
	// set up maximum length
	TInt max = (aPos > KMaxFileLen ? 0 : KMaxFileLen - aPos);
	TInt len = (aLength > max ? max : aLength);
	TInt pos = aPos + len;
	// copy the data
	THMem  dataBuffer(aDes, aMessage);
	dataBuffer.Read(iData+aPos, len, 0);
	if (pos > iSize)
		iSize = pos;
	iFile->iSize = iSize;
	aLength = len;
	// trace it
	TPtr8    pbuf(iData+aPos, len, max);
	TBuf<64> buf;
	cvtbuff(buf, pbuf);
	TTestDebug::Printf(_L("CTestFileCB::WriteL(%4d, %4d, %.8X=%.32S)\n"), aPos, aLength, aDes, &buf);
	TTestDebug::After(DebugRegister() & KDLYFAST ? 10000 : 1000000);
	TTestDebug::Printf(_L("CTestFileCB::WriteL(%4d, %4d, %.8X) exit\n"), aPos, aLength, aDes);
	}

TInt CTestFileCB::Address(TInt& aPos) const
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestFileCB::Address(%d)\n"), aPos);
	return 0;
	}

void CTestFileCB::SetSizeL(TInt aSize)
/// Does nothing
	{
	TTestDebug::Printf(_L("CTestFileCB::SetSizeL(%d)\n"), aSize);
	}

void CTestFileCB::SetEntryL(const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask)
/// Does nothing
	{
	TTestDebug::Printf(_L("CTestFileCB::SetEntryL(%.8X, %.8X, %.8X)\n"), aTime.Int64(), aSetAttMask, aClearAttMask);
	}

void CTestFileCB::FlushDataL()
/// Flushes the file (well, actually does nothing other than introduce a fixed delay 
//	of 2.5 seconds (by default; 10mS if KDLYFAST bit is set in the debug register).
	{
	TTestDebug::Printf(_L("CTestFileCB::FlushDataL()\n"));
	if(iAtt & KEntryAttModified)
		{
		iAtt &= ~KEntryAttModified;
		TTestDebug::After(3500000);
		}
	}

void CTestFileCB::FlushAllL()
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestFileCB::FlushAllL()\n"));
	}

void CTestFileCB::CheckPos(TInt aPos)
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestFileCB::CheckPos(%d)\n"), aPos);
	}

/////////////////////////////////////////////////////////////////////////////

CTestMountCB::CTestMountCB() : CMountCB()
	{
	TTestDebug::Printf(_L("CTestMountCB::CTestMountCB()\n"));
	iSize = KMaxFileLen;
	SetVolumeName(HBufC::NewL(0));
	};

CTestMountCB::~CTestMountCB()
	{
	TTestDebug::Printf(_L("CTestMountCB::~CTestMountCB()\n"));
	};

void CTestMountCB::MountL(TBool aForceMount)
/// Does nothing.
	{
	if (aForceMount)
		TTestDebug::Printf(_L("CTestMountCB::MountL(forced)\n"));
	else
		TTestDebug::Printf(_L("CTestMountCB::MountL()\n"));
	}

TInt CTestMountCB::ReMount()
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::ReMount()\n"));
	return KErrNone;
	}

void CTestMountCB::Dismounted()
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::Dismounted()\n"));
	}

void CTestMountCB::VolumeL(TVolumeInfo& aVolume) const
/// Gets data about the volume -- the name is fixed and the size and free
/// space remaining are constant.
	{
	TTestDebug::Printf(_L("CTestMountCB::VolumeL()\n"));
	aVolume.iName = _L("DelayTest");
	aVolume.iSize = KMaxFileLen;
	aVolume.iFree = KMaxFileLen;
	}

void CTestMountCB::SetVolumeL(TDes& aName)
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::SetVolumeL(%S)\n"), &aName);
	}

void CTestMountCB::MkDirL(const TDesC& aName)
	{
	TTestDebug::Printf(_L("CTestMountCB::MkDirL(%S)\n"), &aName);
	User::Leave(KErrNotSupported);
	}

void CTestMountCB::RmDirL(const TDesC& aName)
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::RmDirL(%S)\n"), &aName);
	}

void CTestMountCB::DeleteL(const TDesC& aName)
	{
	TTestDebug::Printf(_L("CTestMountCB::DeleteL(%S)\n"), &aName);
	iDir.Delete(aName);
	}

void CTestMountCB::RenameL(const TDesC& anOldName,const TDesC& anNewName)
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::RenameL(%S, %S)\n"), &anOldName, &anNewName);
	}

void CTestMountCB::ReplaceL(const TDesC& /*anOldName*/,const TDesC& /*anNewName*/)
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::ReplaceL()\n"));
	}

void CTestMountCB::EntryL(const TDesC& aName, TEntry& aEntry) const
/// Gets the data associated with the specified file.
	{
	const TTestFile* e = iDir.Find(aName);
	if (e)
		{
		e->Entry(aEntry);
		}
	else
		{
		aEntry.iName = _L("");
		aEntry.iAtt  = 0;
		aEntry.iSize = 0;
		aEntry.iType = TUidType();
		TTestDebug::Printf(_L("CTestMountCB::EntryL(%S) leave KErrNotFound\n"), &aName);
		User::Leave(KErrNone);
		}
	TTestDebug::Printf(_L("CTestMountCB::EntryL(%S, %S)\n"), &aName, &aEntry.iName);
	}

void CTestMountCB::SetEntryL(const TDesC& aName, const TTime& aTime, TUint aSetAttMask, TUint aClearAttMask)
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::SetEntryL(%S, %.8X, %.8X, %.8X)\n"),
					   &aName, aTime.Int64(), aSetAttMask, aClearAttMask);
	}

void CTestMountCB::FileOpenL(const TDesC& aName, TUint aMode, TFileOpen aOpen, CFileCB* aFile)
/// Opens a file in the specified mode (the mode is ignored after this,
/// both read and write will work with any open file).  If a file is opened
/// for writing then it will be overwritten without comment.  The timestamp
/// of the file will be set when it is opened for writing.
/// @param aName Name of the file.
/// @param aMode Read, write etc.
/// @param aOpen ???
/// @param aFile Pointer to the already allocated file descriptor area.
	{
	TTestDebug::Printf(_L("CTestMountCB::FileOpenL(%S, %.8X, %.8X, %.8X)\n"),
				  &aName, aMode, aOpen, aFile);
	CTestFileCB* file = (CTestFileCB*)aFile;
	if (aMode & EFileWrite)
		{
		TTestDebug::Printf(_L("Open for writing\n"));
		file->iFile = iDir.Find(aName);
		if (!file->iFile)
			file->iFile = iDir.Create(aName);
		if (file->iFile)
			{
			TTime now;
			now.HomeTime();
			file->iFile->iTime = now.DateTime();
			file->iData = file->iFile->iData;
			}
		else
			User::Leave(KErrDiskFull);
		}
	else
		{
		TTestDebug::Printf(_L("Open for reading\n"));
		file->iFile = iDir.Find(aName);
		if (file->iFile)
			{
			file->iData = file->iFile->iData;
			aFile->SetSize(file->iFile->iSize);
			}
		else
			User::Leave(KErrNotFound);
		}
	}

void CTestMountCB::DirOpenL(const TDesC& aName, CDirCB* aDir)
/// Opens the directory for reading etc.
/// @param aName Name to look for.
/// @param aDir  Pointer to already allocated directory descriptor area.
	{
	CTestDirCB& dir = *(CTestDirCB*)aDir;
	dir.iName  = aName;
	dir.iDir   = &iDir;
	dir.iIndex = 0;
	TTestDebug::Printf(_L("CTestMountCB::DirOpenL(%S)\n"), &aName);
	}

void CTestMountCB::RawReadL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aDes*/,
					   TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/) const
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::RawReadL()\n"));
	}

void CTestMountCB::RawWriteL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aDes*/,
						TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/)
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::RawWriteL()\n"));
	}

void CTestMountCB::ReadUidL(const TDesC& /*aName*/,TEntry& /*anEntry*/) const
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::ReadUidL()\n"));
	}

void CTestMountCB::GetShortNameL(const TDesC& /*aLongName*/,TDes& /*aShortName*/)
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::GetShortNameL()\n"));
	}

void CTestMountCB::GetLongNameL(const TDesC& /*aShortName*/,TDes& /*aLongName*/)
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::GetLongNameL()\n"));
	}

void CTestMountCB::IsFileInRom(const TDesC& /*aFileName*/,TUint8*& /*aFileStart*/)
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::IsFileInRom()\n"));
	}

void CTestMountCB::ReadSectionL(const TDesC& /*aName*/,TInt /*aPos*/,TAny* /*aTrg*/,
						   TInt /*aLength*/,const RMessagePtr2& /*aMessage*/)
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestMountCB::ReadSectionL()\n"));
	}


//////////////////////////////////////////////////////////////

CTestDirCB::CTestDirCB() : CDirCB()
	{
	TTestDebug::Printf(_L("CTestDirCB::CTestDirCB()\n"));
	};

CTestDirCB::~CTestDirCB()
	{
	TTestDebug::Printf(_L("CTestDirCB::~CTestDirCB()\n"));
	};

void CTestDirCB::ReadL(TEntry& aEntry)
/// Read a directory entry.  It looks for the next entry which has
/// a name starting with the name specified when opening the directory
/// (this may be a pathname, for instance).
{
	TTestFile* e = NULL;
	TInt len = iName.Length();
	while ((e = iDir->Entry(iIndex))!=0 &&
		   (e->iName.Length() <= len || e->iName.Left(len) != iName))
		{
		iIndex++;
		}
	if (e)
		{
		e->Entry(aEntry);
		iIndex++;
		}
	else
		{
		aEntry.iName = _L("");
		aEntry.iAtt  = 0;
		aEntry.iSize = 0;
		aEntry.iType = TUidType();
		TTestDebug::Printf(_L("CTestDirCB::ReadL() leave KErrNotFound"));
		User::Leave(KErrNone);
		}
	TTestDebug::Printf(_L("CTestDirCB::ReadL(%S)\n"), &aEntry.iName);
}


//////////////////////////////////////////////////////////////

CTestFormatCB::CTestFormatCB() : CFormatCB()
	{
	TTestDebug::Printf(_L("CTestFormatCB::CTestFormatCB()\n"));
	};

CTestFormatCB::~CTestFormatCB()
	{
	TTestDebug::Printf(_L("CTestFormatCB::~CTestFormatCB()\n"));
	};

void CTestFormatCB::DoFormatStepL()
/// Does nothing.
	{
	TTestDebug::Printf(_L("CTestFormatCB::DoFormatStepL()\n"));
	}

//////////////////////////////////////////////////////////////

extern "C" {

EXPORT_C CFileSystem* CreateFileSystem()
///
/// Create a new file system.
///
	{
	TTestDebug::Printf(_L("CreateFileSystem()\n"));
	return(CTestFileSystem::NewL());
	}
}


