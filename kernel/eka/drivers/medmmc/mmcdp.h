// e32\drivers\medmmc\mmcdp.h
//

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
//

#ifndef __MMCDP_H__
#define __MMCDP_H__


#ifdef __TEST_PAGING_MEDIA_DRIVER__
const TInt KMmcGetStats	= 0x00000001;
struct SMmcStats
	{
	TInt iReqPage;
	TInt iReqNormal;
	TUint16 iNormalFragmenting;
	TUint8 iClashFragmenting;
	TUint8 iSpare;
	};

#endif	// __TEST_PAGING_MEDIA_DRIVER__

#endif	// __MMCDP_H__
