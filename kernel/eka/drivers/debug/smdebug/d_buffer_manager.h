// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Kernel-side managing of stop-mode buffers
//

#ifndef D_BUFFER_MANAGER_H
#define D_BUFFER_MANAGER_H

#include <sm_debug_api.h>

// The global class which creates and deletes buffers used by the stop-mode API

class DBufferManager : public DBase
	{
	public:
		DBufferManager();
		~DBufferManager();

		TInt CreateBuffer(Debug::TTag& aBufferDetails);
		TInt GetBufferDetails(const Debug::TBufferType aBufferType, Debug::TTag& aTag) const;

	private:
		RArray<Debug::TTag> iBuffers;
	};

// static global object
extern DBufferManager TheDBufferManager;

#endif // D_BUFFER_MANAGER_H
