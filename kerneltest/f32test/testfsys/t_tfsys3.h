// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\testfsys\t_tfsys3.h
// 
//

#include <f32file.h>
#include "common.h"
#include <f32fsys.h>
#include <f32ver.h>
#include <f32dbg.h>
#include <e32svr.h>

const TUint KTestClusterSize=1024;

class CTestMountCB : public CMountCB, 
					 public MFileSystemSubType,
					 public MFileSystemClusterSize
	{
public:
	CTestMountCB();
	~CTestMountCB();
	void MountL(TBool /*aForceMount*/){}
	TInt ReMount(){return KErrNone;}
	void Dismounted(){}
	void VolumeL(TVolumeInfo& /*aVolume*/) const{}
	void SetVolumeL(TDes& /*aName*/){}
	void MkDirL(const TDesC& /*aName*/){}
	void RmDirL(const TDesC& /*aName*/){}
	void DeleteL(const TDesC& /*aName*/){}
	void RenameL(const TDesC& /*anOldName*/,const TDesC& /*anNewName*/){}
	void ReplaceL(const TDesC& /*anOldName*/,const TDesC& /*anNewName*/){}
	void EntryL(const TDesC& /*aName*/,TEntry& /*anEntry*/) const{}
	void SetEntryL(const TDesC& /*aName*/,const TTime& /*aTime*/,TUint /*aSetAttMask*/,TUint /*aClearAttMask*/){}
	void FileOpenL(const TDesC& /*aName*/,TUint /*aMode*/,TFileOpen /*anOpen*/,CFileCB* /*aFile*/){}
	void DirOpenL(const TDesC& /*aName*/,CDirCB* /*aDir*/){}
	void RawReadL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aTrg*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/) const{}
	void RawWriteL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aSrc*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/){}
	void ReadUidL(const TDesC& /*aName*/,TEntry& /*anEntry*/) const{}
	void GetShortNameL(const TDesC& /*aLongName*/,TDes& /*aShortName*/){}
	void GetLongNameL(const TDesC& /*aShortName*/,TDes& /*aLongName*/){}
	void IsFileInRom(const TDesC& /*aFileName*/,TUint8*& /*aFileStart*/){}
	void ReadSectionL(const TDesC& /*aName*/,TInt /*aPos*/,TAny* /*aTrg*/,TInt /*aLength*/,const RMessagePtr2& /*aMessage*/){}

public:
	// interface extension implementation
	virtual TInt GetInterface(TInt aInterfaceId, TAny*& aInterface,TAny* aInput);
	virtual TInt SubType(TDes& aName) const;
	virtual TInt ClusterSize() const;
	};



class CTestFileCB : public CFileCB
	{
public:
	CTestFileCB();
	~CTestFileCB();
	void RenameL(const TDesC& /*aNewName*/){}
	void ReadL(TInt /*aPos*/,TInt& /*aLength*/,const TAny* /*aDes*/,const RMessagePtr2& /*aMessage*/){}
	void WriteL(TInt /*aPos*/,TInt& /*aLength*/,const TAny* /*aDes*/,const RMessagePtr2& /*aMessage*/){}
	TInt Address(TInt& /*aPos*/) const{return 0;}
	void SetSizeL(TInt /*aSize*/){}
	void SetEntryL(const TTime& /*aTime*/,TUint /*aSetAttMask*/,TUint /*aClearAttMask*/){}
	void FlushDataL(){}
	void FlushAllL(){}
	void CheckPos(TInt /*aPos*/){}
	};

class CTestDirCB : public CDirCB
	{
public:
	CTestDirCB();
	~CTestDirCB();
	void ReadL(TEntry& /*anEntry*/){}
	};

class CTestFormatCB : public CFormatCB
	{
public:
	CTestFormatCB();
	~CTestFormatCB();
	void DoFormatStepL(){}
	};

class CTestFileSystem : public CFileSystem
	{
public:
	CTestFileSystem();
	~CTestFileSystem();
	TInt Install();
	TInt DefaultPath(TDes& aPath) const;
	TBusLocalDrive& DriveNumberToLocalDrive(TInt aDriveNumber);
	TInt GetInterface(TInt aInterfaceId, TAny*& aInterface,TAny* aInput);
private:
	CMountCB* NewMountL() const;
	CFileCB* NewFileL() const;
	CDirCB* NewDirL() const;
	CFormatCB* NewFormatL() const;
public:
	static CFileSystem* NewL();
	};
