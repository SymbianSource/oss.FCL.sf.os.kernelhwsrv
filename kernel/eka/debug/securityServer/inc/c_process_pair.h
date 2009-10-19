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

#ifndef C_PROCESS_PAIR_H
#define C_PROCESS_PAIR_H

/**
@file
@internalTechnology
@released
*/

/**
CProcessPair is a mapping between a debug agent's process Id, and
the process fileName of a process the agent is interested in debugging.
*/
class CProcessPair : public CBase
	{
public:
	static CProcessPair* NewL(const TDesC& aProcessName, const TProcessId aProcessId);
	~CProcessPair();
	TBool operator==(const CProcessPair &aProcessPair) const;
	TBool Equals(const TDesC& aProcessName, const TProcessId aProcessId) const;
	TBool ProcessIdMatches(const CProcessPair &aProcessPair) const;
	TBool ProcessNameMatches(const CProcessPair &aProcessPair) const;
	TBool ProcessIdMatches(const TProcessId &aProcessId) const;
	TBool ProcessNameMatches(const TDesC& aProcessName) const;

private:
	CProcessPair();
	void ConstructL(const TDesC& aProcessName, TProcessId aProcessId);

private:
	HBufC16* iProcessName;
	TProcessId iProcessId;
	};

#endif //C_PROCESS_PAIR_H

