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
// This is the header file for the PDD. It typically defines PDD factory object
// ifndef __D_EXSYNC_PDD_H__ will resolve the multiple inclusion of this 
// header file in different source files. If the file is already included,
// then the following switch will not re-include the file.
// 
//

#ifndef __D_EXSYNC_PDD_H__
#define __D_EXSYNC_PDD_H__

// system include files
#include <kernel/kernel.h>
#include <d32comm.h>
#include <assp.h>

// LDD header file
#include "d_exsync_ldd.h"

// constants 

// Literal string descriptor constants for driver name. These descriptors 
// are used by the driver to associate a name for registering to the 
// Symbian OS. LDD will have a name to associate with.
//
_LIT(KDriverPddName,"d_exsync.pdd");	// Pdd name

// Version numbers for PDD
const TInt KExPddMajorVerNum=1;						// Major number of PDD
const TInt KExPddMinorVerNum=0;						// Minor number of PDD
const TInt KExPddBuildVerNum=KE32BuildVersionNumber;	// Build version of PDD

/**
 PDD factory for tutorial driver
 This is the PDD factory object that will be created while loading the 
 PDD. PDD needs to provide mandatory implementation for the virtual 
 member functions, as they are inherited and are purely virtual functions
 in DPhysicalDevice.
 */
class DExDriverPhysicalDevice: public DPhysicalDevice
	{
public:	
	// Constructor
	DExDriverPhysicalDevice();
	// Inherited from DPhysicalDevice
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const=0;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* aInfo, 
														const TVersion& aVer)=0;
	virtual TInt Validate(TInt aUnit, const TDesC8* aInfo, 
														const TVersion& aVer)=0;	
	};

#endif // __D_EXSYNC_PDD_H__

//
// End of d_exsync_pdd.h

