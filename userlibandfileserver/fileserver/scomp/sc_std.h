// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include "common.h"
#include <f32fsys.h>
#include <f32ver.h>
#include <f32dbg.h>

IMPORT_C TUint32 DebugRegister();
#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
#define __PRINT(t) {if (DebugRegister()&KCOMPFS) RDebug::Print(t);}
#define __PRINT1(t,a) {if (DebugRegister()&KCOMPFS) RDebug::Print(t,a);}
#define __PRINT2(t,a,b) {if (DebugRegister()&KCOMPFS) RDebug::Print(t,a,b);}
#define __PRINT3(t,a,b,c) {if (DebugRegister()&KCOMPFS) RDebug::Print(t,a,b,c);}
#define __PRINT4(t,a,b,c,d) {if (DebugRegister()&KCOMPFS) RDebug::Print(t,a,b,c,d);}
#else
#define __PRINT(t)
#define __PRINT1(t,a)
#define __PRINT2(t,a,b)
#define __PRINT3(t,a,b,c)
#define __PRINT4(t,a,b,c,d)
#endif


enum TCompFault
	{
	ECompMountRemount,
	ECompDirReadPending,
	ECompFsDefaultPath,
	ECompDirStoreLongEntryNameL,
	};

class TCompMount
	{
public:
		inline TCompMount(CFileSystem* aFs, CMountCB* aMount);
			
		CFileSystem* iFs;
		CMountCB* iMount;
	};


// CCompMountCB

class CDirCB;
class CCompFileSystem;
class CCompMountCB : public CMountCB, public CMountCB::MFileAccessor
	{
public:
	void MountL(TBool aForceMount);
	TInt ReMount();
	void Dismounted();
	void VolumeL(TVolumeInfo& aVolume) const;
	void SetVolumeL(TDes& aName);
	void MkDirL(const TDesC& aName);
	void RmDirL(const TDesC& aName);
	void DeleteL(const TDesC& aName);
	void RenameL(const TDesC& anOldName,const TDesC& anNewName);
	void ReplaceL(const TDesC& anOldName,const TDesC& anNewName);
	void EntryL(const TDesC& aName,TEntry& anEntry) const;
	void SetEntryL(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	void FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile);
	void DirOpenL(const TDesC& aName,CDirCB* aDir);
	void RawReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt anOffset,const RMessagePtr2& aMessage) const;
	void RawWriteL(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt anOffset,const RMessagePtr2& aMessage);
	void GetShortNameL(const TDesC& aLongName,TDes& aShortName);
	void GetLongNameL(const TDesC& aShortName,TDes& aLongName);
	void IsFileInRom(const TDesC& aFileName,TUint8*& aFileStart);
	void ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage);

	inline void SetMountNumber(TInt aNum);

	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
	virtual TInt GetFileUniqueId(const TDesC& aName, TInt64& aUniqueId);
	virtual TInt Spare3(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	virtual TInt Spare2(TInt aVal, TAny* aPtr1, TAny* aPtr2);
	virtual TInt Spare1(TInt aVal, TAny* aPtr1, TAny* aPtr2);

private:
	CCompMountCB(CCompFileSystem * aOwner) {iFileSystem=aOwner;}
	~CCompMountCB();

	inline CMountCB* RomMount() const;
	inline void NullCompFileSystem(void);
	void NewRomMountL();
//	TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
	TInt AddFsToCompositeMount(CFileSystem* aFileSystem);

	TInt64 iSize;
	CCompFileSystem * iFileSystem;
protected:
	RArray<TCompMount> iMounts;

	friend class CCompFileSystem;
	friend class CCompDirCB;
	};


// CCompDirCB

class CCompDirCB : public CDirCB
	{
public:
	void ReadL(TEntry& anEntry);
	void StoreLongEntryNameL(const TDesC& aName);

	TInt InitDir(CDirCB* aTrg, CMountCB* aMount);
	TInt InitDir(CMountCB* aMount);
	
public:
	HBufC* iMatch;
private:
	CCompDirCB();
	~CCompDirCB();
	
	TBool IsDuplicate(TFileName& aFilename);

	RPointerArray<CDirCB> iDirs;
	TInt	iCurrentDir;

	friend class CCompFileSystem;
	friend class CCompMountCB;
	};


// CCompFileCB

class CCompFileCB : public CFileCB
	{
public:
	CCompFileCB();
	~CCompFileCB();
	
	void RenameL(const TDesC& aNewName);
	void ReadL(TInt aPos,TInt& aLength,const TAny* aDes);
	void ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	void WriteL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	TInt Address(TInt& aPos) const;
	void SetSizeL(TInt aSize);
	void SetEntryL(const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	void FlushDataL();
	void FlushAllL();

	inline void SetTrueFile(CFileCB* aFile);
	inline CFileCB* TrueFile() const;

protected:
	
	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);

private:

	CFileCB* iTrueFile;
	};


// CCompFileSystem

class CCompFileSystem : public CFileSystem
	{
public:
	CCompFileSystem();
	~CCompFileSystem();
	static CFileSystem* NewL();
	TInt Install();
	CMountCB* NewMountL() const;
	CFileCB* NewFileL() const;
	CDirCB* NewDirL() const;
	CFormatCB* NewFormatL() const;
	void DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const;
	TInt DefaultPath(TDes& aPath) const;

	inline void NullMount(void);
private:

	CCompMountCB* iMount;
	};


#include "sc_std.inl"

