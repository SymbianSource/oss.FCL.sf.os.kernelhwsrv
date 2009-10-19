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
// This is the header file for LDD. This typically define the Logical channel 
// and LDD factory (Logical device) derived classes. The requests passed from
// RExDriverChannel will be implemented here.
// ifndef __D_EXPIO_LDD_H__ will resolve the multiple inclusion of this header 
// file in different source files. If the file is already included, then the 
// following switch will not re-include the file.
// 
//

#ifndef __D_EXPIO_LDD_H__
#define __D_EXPIO_LDD_H__

// include files
//
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>	// for DThread class

// KEXDEBUG macro is to convieniently enable or disable the kernel print 
// messages. _EXDRIVER_DEBUG_ is enabled or disabled by specifying macro
// in mmp file. Debug messages will be displayed only if the macro is enabled
//
#ifdef _EXDRIVER_DEBUG_
#define KEXDEBUG(a)	a		// Enable prints
#else
#define KEXDEBUG(a) 		// Supress prints
#endif

// Constants
//
const TUint KTxBufferSize=256;	// Tx Buffer size
const TUint KRxBufferSize=256;	// Rx Buffer size

// Version numbers for LDD
const TInt KExLddMajorVerNum=1;						// Major number of LDD
const TInt KExLddMinorVerNum=0;						// Minor number of LDD
const TInt KExLddBuildVerNum=KE32BuildVersionNumber;// Build version of LDD
		
// Physical channel class forward declaration.
// This will be used in logical channel class
//
class DExDriverPhysicalChannel;

/** 
 Logical channel class for tutorial driver
 
 Note: This is not using the standard LDD interface provided by the Symbian
 for ecom (serial) driver. Since this is a tutorial driver, we are defining
 our own simple LDD interface
 */
class DExDriverLogicalChannel:public DLogicalChannel
{
public:
	// Constructor		
	DExDriverLogicalChannel();	
	// Destructor
	virtual ~DExDriverLogicalChannel();	
	// Inherited from DLogicalChannelBase	
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);	
	// Inherited from DObject	
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	 // Inherited from DLogicalChannel	
	virtual void HandleMsg(TMessageBase *aMsg);
	// Handler function for synchronous request sent through RBusLogicalChannel
	TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	// Accessor for PDD
    inline DExDriverPhysicalChannel* Pdd();
private:
	// Get Channel Capabilities
	TInt Caps(TDes8* aCaps);			
    // Configure function
    TInt SetConfig(const TDesC8* aConfig);
    // Transmit Data function
    TInt TransmitData(const TDesC8* aData);
    // Receive Data function
    TInt ReceiveData(TDes8* aData,const TAny* aLen);
private:
	// User thread object reference
	DThread* iClient;
	// Buffer pointer for transmit data
	TUint8* iTxBuffer;	
	// Buffer pointer for receive data
	TUint8* iRxBuffer;
};

/**
 Pdd() inline function, to ease accessing the associated PDD for this LDD.
 iPdd is a DBase object, and to use it, everytime it needs to be typecasted.
 This function will help in to use pdd object in cleaner and easier way in LDD.
  
 @return	PDD, DExDriverPhysicalChannel object
 */
inline DExDriverPhysicalChannel* DExDriverLogicalChannel::Pdd()
    { return (DExDriverPhysicalChannel*)iPdd; }

/** 
 LDD factory, Logical Device class
 This is the LDD object that will be created while loading the LDD.LDD needs
 to provide mandatory implementation for the virtual member functions, as 
 they are inherited and are purely virtual functions in DLogicalDevice. 
 */
class DExDriverLogicalDevice:public DLogicalDevice
	{
public:	
	// Constructor
	DExDriverLogicalDevice();
	// Destructor
	~DExDriverLogicalDevice();
	// Inherited from DLogicalDevice
	// Install logical device, 2nd stage constructor	
	virtual TInt Install();	
	// Get the device capabilities, typically just version		
	virtual void GetCaps(TDes8& aCaps) const;
	// Create the logical channel, called while channel open
	virtual TInt Create(DLogicalChannelBase*& aChannel);	
	};

/**
 Physical Channel for tutorial driver. As per framework, physical channel is
 derived from DBase and can have its own member functions and implementation.
 This is an abstract class, interface with pure virtual functions. PDD will
 implement the derived physical channel class.
 */
class DExDriverPhysicalChannel: public DBase
	{
public:
	// Get Capabilities of the channel 
	virtual void Caps(TDes8& aCaps)=0;	
	// Configure the device 
	virtual TInt Configure(const TCommConfigV01& aConfig)=0;
	// Transmit the data to the device
	virtual TInt TransmitData(const TDesC8& aData)=0;
	// Receive the data from the device
	virtual TInt ReceiveData(TDes8& aData, TInt aLen)=0;
	// Create and return the DFC for driver
	virtual TDynamicDfcQue* DfcQ()=0;
public:
	// Pointer refernce to LDD
	DExDriverLogicalChannel* iLdd;
	};
	
#endif	// __D_EXPIO_LDD_H__

//
// End of d_expio_ldd.h
