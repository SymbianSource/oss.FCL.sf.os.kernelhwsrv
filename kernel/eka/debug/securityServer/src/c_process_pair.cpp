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
// Provides a helper class for process security management
// 
//

#include <e32base.h>
#include <e32base_private.h>

// Required for logging
#include <rm_debug_api.h>

#include "c_process_pair.h"
#include "rm_debug_logging.h"


CProcessPair* CProcessPair::NewL(const TDesC& aProcessName, const TProcessId aProcessId)
	{
	CProcessPair* self=new (ELeave) CProcessPair();
	CleanupStack::PushL(self);
	self->ConstructL(aProcessName, aProcessId);
	CleanupStack::Pop(self);
	return self;
	}

void CProcessPair::ConstructL(const TDesC& aProcessName, const TProcessId aProcessId)
	{
	//allocate the process name buffer and fill with aProcessName
	iProcessName = aProcessName.Alloc();
	if(iProcessName == NULL)
		User::Leave(KErrNoMemory);

	LOG_MSG2( "CProcessPair::ConstructL() process name: %S", &TPtr8((TUint8*)iProcessName->Ptr(), 2*iProcessName->Length(), 2*iProcessName->Length()) );

	//set process id
	iProcessId = aProcessId;
	}

CProcessPair::CProcessPair()
	{
	}

CProcessPair::~CProcessPair()
	{
	delete iProcessName;
	}

/**
Check whether two CProcessPair objects are equal

@param aProcessPair a CProcessPair object to match with this one

@return ETrue is process id and name match, EFalse otherwise
*/
TBool CProcessPair::operator==(const CProcessPair &aProcessPair) const
	{
	return Equals(*aProcessPair.iProcessName, aProcessPair.iProcessId);
	}
	
/**
Check whether this CProcessPair object has these values set

@param aProcessName process name to check
@param aProcessId process id to check

@return ETrue is process id and name match, EFalse otherwise
*/
TBool CProcessPair::Equals(const TDesC& aProcessName, const TProcessId aProcessId) const
	{
	return (ProcessIdMatches(aProcessId) && (ProcessNameMatches(aProcessName)));
	}

/**
Check whether the process ids of two objects match

@param aProcessPair a CProcessPair object to compare with this one

@return ETrue is process id matches, EFalse otherwise
*/
TBool CProcessPair::ProcessIdMatches(const CProcessPair &aProcessPair) const
	{
	return ProcessIdMatches(aProcessPair.iProcessId);
	}

/**
Check whether two process ids match

@param aProcessId a process ID to compare with this pair's process ID

@return ETrue is process id matches, EFalse otherwise
*/
TBool CProcessPair::ProcessIdMatches(const TProcessId &aProcessId) const
	{
	return iProcessId == aProcessId;
	}

/**
Check whether the process names of two objects match in-case-sensitively

@param aProcessPair a CProcessPair object to compare with this one

@return ETrue is process names match, EFalse otherwise
*/
TBool CProcessPair::ProcessNameMatches(const CProcessPair &aProcessPair) const
	{
	return ProcessNameMatches(*aProcessPair.iProcessName);
	}

/**
Check whether two strings match in-case-sensitively

@param aProcessName a process name to compare with this pair's process name

@return ETrue is process names match, EFalse otherwise
*/
TBool CProcessPair::ProcessNameMatches(const TDesC& aProcessName) const
	{
	TInt equal = iProcessName->CompareF(aProcessName);
	return (equal == 0) ? ETrue : EFalse;
	}

