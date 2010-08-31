// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/iic/iic_psl/iic_testpsl.cpp
//

#include <drivers/iic.h>
#include "iic_testpsl.h"

// Global Controller pointer
extern DIicBusController*& gTheController;

#ifndef IIC_SIMULATED_PSL

#error iic_testpsl.cpp being built when IIC_SIMULATED_PSL is not defined

#else

TVersion DIicPdd::VersionRequired()
	{
	const TInt KIicMajorVersionNumber=1;
	const TInt KIicMinorVersionNumber=0;
	const TInt KIicBuildVersionNumber=KE32BuildVersionNumber;
	return TVersion(KIicMajorVersionNumber,KIicMinorVersionNumber,KIicBuildVersionNumber);
	}

/** Factory class constructor */
DIicPdd::DIicPdd()
	{
    iVersion = DIicPdd::VersionRequired();
	}

DIicPdd::~DIicPdd()
	{
	delete gTheController;
	}

TInt DIicPdd::Install()
    {
    return(SetName(&KPddName));
    }

/**  Called by the kernel's device driver framework to create a Physical Channel. */
TInt DIicPdd::Create(DBase*& /*aChannel*/, TInt /*aUint*/, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
    {
    return KErrNone;
    }

/**  Called by the kernel's device driver framework to check if this PDD is suitable for use with a Logical Channel.*/
TInt DIicPdd::Validate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
    {
   	if (!Kern::QueryVersionSupported(DIicPdd::VersionRequired(),aVer))
		return(KErrNotSupported);
    return KErrNone;
    }

/** Return the driver capabilities */
void DIicPdd::GetCaps(TDes8& aDes) const
    {
	// Create a capabilities object
	TCaps caps;
	caps.iVersion = iVersion;
	// Zero the buffer
	TInt maxLen = aDes.MaxLength();
	aDes.FillZ(maxLen);
	// Copy cpabilities
	TInt size=sizeof(caps);
	if(size>maxLen)
	   size=maxLen;
	aDes.Copy((TUint8*)&caps,size);
    }

static DIicPdd* TheIicPdd;

DECLARE_STANDARD_PDD()
	{
	gTheController = new DIicBusController;
	if(!gTheController)
		return NULL;
	TInt r = gTheController->Create();
	if(r == KErrNone)
		{
		TheIicPdd = new DIicPdd;
		if(TheIicPdd)
			return TheIicPdd;
		}
	
	delete gTheController; 
	return NULL;
	}

#endif/*IIC_SIMULATED_PSL*/




