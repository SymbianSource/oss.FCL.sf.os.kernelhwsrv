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
// f32\sfsrv\cl_scan.h
// 
//

#if !defined(__CL_SCAN_H__)
#define __CL_SCAN_H__
#if !defined(__E32BASE_H__)
#include <e32base.h>
#endif

class CDirFactory : public CDir
	{
public:
	static CDir* NewL(const TDesC& anEntryName);
	static CDir* NewL();
	};	

NONSHARABLE_CLASS(CDirList) : public CBase
	{
public:
	CDirList();
	static CDirList* NewL(CDir& aDirList);
	~CDirList();
	const TEntry& Next();
	TBool MoreEntries() const;
private:
	CDirList(const CDirList&);
	TBool operator=(const CDirList&);
private:
	CDir* iDirList;
	TInt iCurrentPos;
	};

NONSHARABLE_CLASS(CDirStack) : public CBase
	{
public:
	static CDirStack* NewL();
	~CDirStack();
	void ResetL(const TDesC& aStartDir);
	TBool IsEmpty();
	void PushL(CDir& aSubDirs);
	void Pop();
	CDirList* Peek();
private:
	CDirStack();
private:
	RPointerArray<CDirList> iDirStack;
	TParse iPath;
	};

#endif
