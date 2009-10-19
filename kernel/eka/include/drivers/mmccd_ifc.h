// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\mmccd_ifc.h
// Factory class for creating platform-specific MMC Drivers
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __MMCCD_IFC_H__
#define __MMCCD_IFC_H__

#include <drivers/mmc.h>
#include <drivers/locmedia.h>




/**
The factory class for creating platform specific MultiMediaCard objects.

The class defines the interface that must be implemented by a derived class.
*/
class TMMCardControllerInterface
	{
public:
	IMPORT_C TInt Create();
	
	
	/**
	Creates a platform specific socket object.
	
	The function is called from TMMCardControllerInterface::Create(), and
	an implementation must be provided by the platform specific layer in
	the Variant DLL.
	
    @param aSocketNum     The socket number.
	@param aPasswordStore A pointer to the password store.
	
	@return A pointer to the new socket object.
	
	@see Create()
	*/
	virtual DMMCSocket* NewSocket(TInt aSocketNum, TMMCPasswordStore* aPasswordStore)=0;
	
	
	/**
	Creates a platform specific stack object.
	
	The function is called from TMMCardControllerInterface::Create(), and
	an implementation must be provided by the platform specific layer in
	the Variant DLL.
	
    @param aSocketNum The socket number.
	@param aSocket    A pointer to the socket object.
	
	@return A pointer to the new stack object.
	
	@see Create()
	*/
	virtual DMMCStack* NewStack(TInt aSocketNum, DMMCSocket* aSocket)=0;
	
	
	/**
	Creates a platform specific media change object.
	
	The function is called from TMMCardControllerInterface::Create(), and
	an implementation must be provided by the platform specific layer in
	the Variant DLL.
	
	@param aMcId The media change number.
	
	@return A pointer to the new media change object.
   
    @see Create()
    @see MediaChangeID()
	*/
	virtual DMMCMediaChange* NewMediaChange(TInt aMcId)=0;
	
	
	/**
	Creates a platform specific power supply unit object.
	
	The function is called from TMMCardControllerInterface::Create(), and
	an implementation must be provided by the platform specific layer in
	the Variant DLL.
	
    @param aVccNum The power supply unit number
    @param aMcId   The associated media change number.

	@return A pointer to the new power supply unit object.
    
    @see Create()
    @see MediaChangeID()
    @see VccID()
	*/
	virtual DMMCPsu* NewVcc(TInt aVccNum, TInt aMcId)=0;
	
	/**
	Creates a platform specific VccCore power supply unit object.
	
	The function is called from TMMCardControllerInterface::Create(), and
	an implementation can be provided by the platform specific layer in
	the Variant DLL.
	
    @param aVccQNum The power supply unit number (must match paired Vcc unit number).
    @param aMcId   The associated media change number.

	@return A pointer to the new power supply unit object.
    
    @see Create()
    @see MediaChangeID()
    @see VccID()
	*/
	virtual inline DMMCPsu* NewVccCore(TInt aVccCoreNum, TInt aMcId);
	
	
	/**
	Performs platform specific initialisation.
	
	The function is called from TMMCardControllerInterface::Create(), and
	an implementation must be provided by the platform specific layer in
	the Variant DLL.
	
	@return KErrNone to indicate that initialisation has completed
	        successfully, otherwise one of the other system-wide error codes
	        to indicate initialisation failure.
    
    @see Create()
	*/
	virtual TInt Init()=0;
	
	
	/**
	Indicates whether the peripheral bus socket, as identified by the specified
	peripheral bus socket number, is designated as a MultiMediaCard socket on
	this platform.
	
	If the socket is designated as a MultiMediaCard socket, then the function
	must also provide the media information for that socket.
	
	The function is called from TMMCardControllerInterface::Create(), and
	an implementation must be provided by the platform specific layer in
	the Variant DLL.
	
	@param aSocket          The peripheral bus socket number.
	@param aMediaDeviceInfo The media information for that socket, if
	                        the socket is designated as a MultiMediaCard
	                        socket.
	
	@return True, if the socket is designated as a MultiMediaCard socket;
	        false, otherwise.
	
	@see Create()
	*/
	virtual TBool IsMMCSocket(TInt aSocket,SMediaDeviceInfo& aMediaDeviceInfo)=0;
	
	
	/**
	Reports which media change object is to be associated with the specified
	peripheral bus socket number.

	The function is called from TMMCardControllerInterface::Create(), and
	an implementation must be provided by the platform specific layer in
	the Variant DLL.
	
    @param aSocket The socket number

	@return The media change number.
	
	@see Create()
	@see NewMediaChange()
	*/
	virtual TInt  MediaChangeID(TInt aSocket)=0;
	
	
	/**
	Reports which power supply unit (PSU) object is to be associated with
	the specified peripheral bus socket number.

    The function is called from TMMCardControllerInterface::Create(), and
	an implementation must be provided by the platform specific layer in
	the Variant DLL.
	
	@param aSocket The socket number.
	
	@return The power supply unit number.
	
	@see Create()
	@see NewVcc()
	*/
	virtual TInt  VccID(TInt aSocket)=0;

protected:
	
	
	/**
	 * Performs registration of the media devices on the current socket
	 * @internalComponent	
	 */
	IMPORT_C virtual TInt RegisterMediaDevices(TInt aSocket);
    };


DMMCPsu* TMMCardControllerInterface::NewVccCore(TInt /*aVccCoreNum*/, TInt /*aMcId*/)
/**
 * Default implementation.
 * Only eMMC v4.3+ media supports a seperate supply for VccCore. 
 */
	{return NULL;};

#endif


