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
// This is the PDD. This typically implements the physical device.
// All PSL code and the unit specific functions are implemented
// in variant specific file, i.e d_exsync_h4.cpp
// 
//
 
// include files
// 
#include "d_exsync_pdd.h"

/*
 Pdd Factory class implementation
 */

/**
 PDD factory constructor. This is called while creating the PDD factory
 object as a part of the driver (PDD) loading. 
 */
DExDriverPhysicalDevice::DExDriverPhysicalDevice()
	{	
	// Set the version of the interface supported by the driver, that consists 
	// of driver major number, device minor number and build version number.
	// It will normally be incremented if the interface changes.Validating 
	// code assumes that clients requesting older versions will be OK with
	// a newer version, but clients requesting newer versions will not want 
	// an old version.	
	//
    iVersion=TVersion(KExPddMajorVerNum,KExPddMinorVerNum,KExPddBuildVerNum);
	}

/**
 Install the PDD. This is second stage constructor for physical device, 
 called after creating PDD factory object on the kernel heap to do further
 initialization of the object.
 
 @return	KErrNone or standard error code
 */
TInt DExDriverPhysicalDevice::Install()
	{
	// Install() should by minimum, set the pdd object name. Name is important 
	// as it is the way in which these objects are subsequently found and
	// is property of reference counting objects. SetName() sets the name
	// of the refernce counting object created. Device driver framework finds
	// the factory object by matching the name.
	//
	return SetName(&KDriverPddName);
	}

//
// End of d_exsync_pdd.cpp
