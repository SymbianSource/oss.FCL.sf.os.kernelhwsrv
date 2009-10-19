// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Definitions for the list manager
// 
//

#ifndef T_LIST_MANAGER_H
#define T_LIST_MANAGER_H

#include <e32cmn.h>
#include <kernel/kern_priv.h>
#include <rm_debug_api.h>

/**
@file
@internalComponent
@released
*/

class TListManager
{
public:
	TInt GetThreadListForThread(TDes8& aBuffer, TUint32& aDataSize, const TUint64 aTargetThreadId) const;
	TInt GetThreadListForProcess(TDes8& aBuffer, TUint32& aDataSize, const TUint64 aTargetProcessId) const;
	TInt GetGlobalThreadList(TDes8& aBuffer, TUint32& aDataSize) const;
	TInt GetProcessList(TDes8& aBuffer, TUint32& aDataSize) const;
	TInt GetCodeSegListForThread(TDes8& aBuffer, TUint32& aDataSize, const TUint64 aTargetThreadId) const;
	TInt GetCodeSegListForProcess(TDes8& aBuffer, TUint32& aDataSize, const TUint64 aTargetProcessId) const;
	TInt GetGlobalCodeSegList(TDes8& aBuffer, TUint32& aDataSize) const;
	TInt GetXipLibrariesList(TDes8& aBuffer, TUint32& aDataSize) const;
private:
	TInt GetThreadList(TDes8& aBuffer, TUint32& aDataSize, TBool aGlobal, const TUint64 aTargetProcessId) const;
	TInt GetDirectoryContents(RPointerArray<TRomEntry>& aRomEntryArray, const TLinAddr aAddress) const;
	TInt GetDirectoryEntries(RPointerArray<TRomEntry>& aRomEntryArray, const TDesC& aDirectoryName) const;
	TInt FindDirectory(const TDesC& aDirectory, TLinAddr& aAddress) const;
	TInt GetDirectoryEntries(RPointerArray<TRomEntry>& aRomEntryArray, RArray<TPtr8>& aArray, TLinAddr& aAddress) const;

	TInt AppendCodeSegData(TDes8& aBuffer, TUint32& aDataSize, const TModuleMemoryInfo& aMemoryInfo, const TBool aIsXip, const Debug::TCodeSegType aCodeSegType, const TDesC8& aFileName, const TUint32 aUid3) const;
	void AppendThreadData(TDes8& aBuffer, TUint32& aDataSize, DThread* aThread) const;
	TInt CopyAndExpandDes(const TDesC& aSrc, TDes& aDest) const;
	TInt GetCodeSegType(const DCodeSeg* aCodeSeg, Debug::TCodeSegType& aType) const;
	TInt SplitDirectoryName(const TDesC& aDirectoryName, RArray<TPtr8>& aSubDirectories) const;
};

#endif //T_LIST_MANAGER_H

