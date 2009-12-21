/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/


/**
@test
@internalComponent

This contains CTestFileSystem
*/

#if (!defined __T_TEST_FSY_H__)
#define __T_TEST_FSY_H__

//	EPOC includes
#include <f32fsys.h>

class CTestMountCB : public CMountCB
	{
public:
	CTestMountCB() {}
	~CTestMountCB() {}
	virtual void	MountL(TBool /*aForceMount*/) {}
	virtual TInt	ReMount() { return KErrNone; }
	virtual void	Dismounted() {}
	virtual void	VolumeL(TVolumeInfo& /*aVolume*/) const {}
	virtual void	SetVolumeL(TDes& /*aName*/) {}
	virtual void	MkDirL(const TDesC& /*aName*/) {}
	virtual void	RmDirL(const TDesC& /*aName*/) {}
	virtual void	DeleteL(const TDesC& /*aName*/) {}
	virtual void	RenameL(const TDesC& /*anOldName*/,const TDesC& /*anNewName*/) {}
	virtual void	ReplaceL(const TDesC& /*anOldName*/,const TDesC& /*anNewName*/) {}
	virtual void	EntryL(const TDesC& /*aName*/,TEntry& /*anEntry*/) const {}
	virtual void	SetEntryL(const TDesC& /*aName*/,const TTime& /*aTime*/,TUint /*aSetAttMask*/,TUint /*aClearAttMask*/) {}
	virtual void	FileOpenL(const TDesC& /*aName*/,TUint /*aMode*/,TFileOpen /*anOpen*/,CFileCB* /*aFile*/) {}
	virtual void	DirOpenL(const TDesC& /*aName*/,CDirCB* /*aDir*/) {}
	virtual void	RawReadL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aTrg*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/) const {}
	virtual void	RawWriteL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aSrc*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/) {}
	virtual void	ReadUidL(const TDesC& /*aName*/,TEntry& /*anEntry*/) const {}
	virtual void	GetShortNameL(const TDesC& /*aLongName*/,TDes& /*aShortName*/) {}
	virtual void	GetLongNameL(const TDesC& /*aShortName*/,TDes& /*aLongName*/) {}
	virtual void	IsFileInRom(const TDesC& /*aFileName*/,TUint8*& /*aFileStart*/) {}
	virtual void	ReadSectionL(const TDesC& /*aName*/,TInt /*aPos*/,TAny* /*aTrg*/,TInt /*aLength*/,const RMessagePtr2& /*aMessage*/) {}
	};

class CTestFileCB : public CFileCB
	{
public:
	CTestFileCB() {}
	~CTestFileCB() {}
	virtual void	RenameL(const TDesC& /*aNewName*/) {}
	virtual void	ReadL(TInt /*aPos*/,TInt& /*aLength*/,const TAny* /*aDes*/,const RMessagePtr2& /*aMessage*/) {}
	virtual void	WriteL(TInt /*aPos*/,TInt& /*aLength*/,const TAny* /*aDes*/,const RMessagePtr2& /*aMessage*/) {}
	virtual TInt	Address(TInt& /*aPos*/) const {return 0;}
	virtual void	SetSizeL(TInt /*aSize*/) {}
	virtual void	SetEntryL(const TTime& /*aTime*/,TUint /*aSetAttMask*/,TUint /*aClearAttMask*/) {}
	virtual void	FlushDataL() {}
	virtual void	FlushAllL() {}
	virtual void	CheckPos(TInt /*aPos*/) {}
	};

class CTestDirCB : public CDirCB
	{
public:
	CTestDirCB() {}
	~CTestDirCB() {}
	virtual void	ReadL(TEntry& /*anEntry*/) {}
	};

class CTestFormatCB : public CFormatCB
	{
public:
	CTestFormatCB() {}
	~CTestFormatCB() {}
	virtual void	DoFormatStepL() {}
	};

class CTestFileSystem : public CFileSystem
	{
public:
	TInt			DefaultPath(TDes& aPath) const;
	void			DriveInfo(TDriveInfo& anInfo, TInt aDriveNumber) const;
	TBusLocalDrive&	DriveNumberToLocalDrive(TInt aDriveNumber) const;
	CMountCB*		NewMountL() const;
	CFileCB*		NewFileL() const;
	CDirCB*			NewDirL() const;
	CFormatCB*		NewFormatL() const;
 
protected:
	CTestFileSystem();
	};

#endif /* __T_TEST_FSY_H__ */
