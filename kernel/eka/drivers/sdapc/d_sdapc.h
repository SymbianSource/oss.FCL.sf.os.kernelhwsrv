// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL " http://www.eclipse.org/legal/epl-v10.html ".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:


#ifndef __D_SDAPC_H__
#define __D_SDAPC_H__
#include <e32cmn.h>




class TCapsTestV01
	{
public:
	TVersion	iVersion;
	};

/**

A user-side interface to the SD PSU auxiliary-control driver.

*/
class RSDAuxiliaryPowerControlAPI : public RBusLogicalChannel
	{
public:
	enum
		{
		EMajorVersionNumber=1,
		EMinorVersionNumber=0,
		EBuildVersionNumber=1
		};

public:
	inline void Cancel();
	
	inline TInt Open(TInt aSocket,const TVersion& aVer)
		{return(DoCreate(_L("D_SDAPC"),aVer,(TInt)aSocket,NULL,NULL));}
	
	inline TVersion VersionRequired() const
		{return(TVersion(EMajorVersionNumber,EMinorVersionNumber,EBuildVersionNumber));}
	
	};

#endif
