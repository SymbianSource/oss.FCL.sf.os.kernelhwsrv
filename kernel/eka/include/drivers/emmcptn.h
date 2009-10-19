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
// Partition Management Abstract class for Embedded MMC devices
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __EMMCPTN_H__
#define __EMMCPTN_H__

#include <drivers/mmc.h>
#include <drivers/locmedia.h>
#include <drivers/pbusmedia.h>

class DEMMCPartitionInfo : public DBase
/**
 *  Base Abstract class for classes that define a partitioning scheme for Embedded MMC devices
 * 
 * 	@publishedPartner
 *  @released
 */
	{
public:
	/**
	 * Initialise the Partition Object.
	 * 
	 * Called as part of the creation process for the MMC Media driver.
	 * 
	 * Initialisation can include activities such as Memory structure allocation and
	 * creation of any required driver session/handles. 
	 * 
	 * @param aDriver Owning DMediaDriver class
	 * @return KErrNone if successful, standard error code otherwise.
	 */
	virtual TInt Initialise(DMediaDriver* aDriver) =0;
	
	/**
	 * Read the partition information for the media.
	 * 
	 * Called as a child function of the PartitionInfo() method.
	 * 
	 * @see DMmcMediaDriverFlash::PartitionInfo() 
	 * 
	 * @param anInfo An object that, on successful return, contains the partition information.
	 * @return KErrNone if successful, standard error code otherwise.
	 */
	virtual TInt PartitionInfo(TPartitionInfo& anInfo, const TMMCCallBack& aCallBack) =0;
	
	/**
	 * Returns partition specific drive capability attributes.
	 * 
	 * Called as a child function of the Caps() method.
	 * 
	 * @see DMmcMediaDriverFlash::Caps()
	 * 
	 * @param aDrive Local drive to be queried. 
	 * @param aInfo Media drive capability object to be populated.
	 * @return KErrNone if successful, standard error code otherwise. 
	 */
	virtual TInt PartitionCaps(TLocDrv& aDrive, TDes8& aInfo) =0;
	};

/**
 * Factory Method that returns an instance of a DEMMCPartitionInfo class.
 * 
 * @return Newly created DEMMCPartitionInfo object.
 */
IMPORT_C DEMMCPartitionInfo* CreateEmmcPartitionInfo();

#endif
