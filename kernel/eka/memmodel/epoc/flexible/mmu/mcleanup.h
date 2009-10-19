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
//

/**
 @file
 @internalComponent
*/

#ifndef MCLEANUP_H
#define MCLEANUP_H


typedef void (*TMemoryCleanupCallback)(TAny*);

class TMemoryCleanup
	{
public:
	void Add(TMemoryCleanupCallback aCallback, TAny* aArg);
	static void Cleanup(TAny* aDummy=0);
	static void Init2();

	FORCE_INLINE TMemoryCleanup()
		: iCallback(0)
		{}

	FORCE_INLINE ~TMemoryCleanup()
		{
		__NK_ASSERT_DEBUG(iCallback==0);
		// This 'generic' cleanup API isn't safe in the situation where an object
		// being cleaned up is deleted. This is currently not a problem because
		// we only use this for permanent objects, but to help prevent future bugs
		// assert that this object is never deleted...
		__NK_ASSERT_ALWAYS(0);
		}
private:
	TMemoryCleanup* iNext;
	TMemoryCleanupCallback iCallback;
	TAny* iArg;
	};


#endif
