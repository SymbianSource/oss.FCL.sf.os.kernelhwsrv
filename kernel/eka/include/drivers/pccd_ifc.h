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
// e32\include\drivers\pccd_ifc.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __PCCD_IFC_H__
#define __PCCD_IFC_H__
#include <pccard.h>
#include <drivers/locmedia.h>
#include <platform.h>
#include <assp.h>

#define __PCCD_MACHINE_CODED__

const TInt KMemConfigByteAccess=1;

class PccdIfc
	{
public:
	static DPcCardSocket* NewSocket(TInt aSocketNum);
	static DPcCardMediaChange* NewMediaChange(TInt aMcId);
	static DPcCardVcc* NewVcc(TInt aVccNum, TInt aMcId);
	static DPccdChunkBase* NewChunk(TPccdMemType aType);
	};

	/**
	 A base class that defines a Pc Card controller interface.
	 
	 @publishedPartner
	 @released
	 */
class TPcCardControllerInterface
	{
public:
	/**
	 Allocate any resources. Only done once on kernel initialization so don't worry about cleanup if it fails.
     
	 @return KErrNone if successful in allocating memory for the Pc Card. otherwise return KErrNoMemory.
	 */
	IMPORT_C TInt Create();
public:
    /**
 	 Gets the physical address of Pc Card memory.
     
	 @param aCard  A Pc Card whose physical address to be returned.
     
	 @param aType  Memory type  of the Pc Card whose physical address to be returned.
     
	 @return Physical address of Pc Card for the given memory type.
	 
	 @see TPccdMemType
	 */    
	virtual TUint32 MemPhysicalAddress(TInt aCard,TPccdMemType aType)=0;
	/** 
	 Sets the Pc Card controller h/w.
	 
	 Initialise the Pc Card Controller h/w.
	 
	 @return KErrNone if Pc Card contorller h/w is initialised.
     */
	virtual TInt Init()=0;
    /**
	 Gets the current settings for the Pc Card.
     
	 @param aCard  A Pc Card whose signals to be indicated.
     
	 @param anInd  Has current settings for the PC Card indicator signals into 'anInd' for the specified aCard.
   	 
	 @return KErrNone if successful.
	 
	 @see TSocketIndicators.
	 */
	virtual TInt Indicators(TInt aCard,TSocketIndicators &anInd)=0;
    /**
 	 Enables the Pc Card Interface.
     
	 @param aCard  A Pc Card whose interface to be enabled.
	 */
	virtual void InterfaceOn(TInt aCard)=0;
	/**
 	 Disables the Pc Card Interface.
     
	 @param aCard  A PC Card whose interface to be disabled.
	 */
	virtual void InterfaceOff(TInt aCard)=0;
	/**
 	 To Apply/remove h/w reset.

     @param aCard  A Pc Card which has to be h/w reseted.

     @param anAssert To make the h/w reset guarented.
	 */
    virtual void CardReset(TInt aCard,TBool anAssert)=0;
	/**
 	 Gets Pc Card Rdy/Bsy status.

     @param aCard  Pc Card whose status to be returned.

	 @return True if the card is ready.
	 */
    virtual TBool CardReady(TInt aCard)=0;
	/** 
	 Configure memory configuration options.

     @param aCard   Pc Card whose memory to be configured.
     
	 @param aMemType  Type of memory to be configured.

     @param aSpeed    Access speed of the memory involved.
	 
	 @param aWaitSig  This WaitSig should never checked since repeated accesses to MECR seem to crash the system
	  in mysterious ways.

	 @param aFlag    To modify the memory configuration attributes by ORing with memory chunk setup flags pKPccdBusWidth32 and KPccdDisableWaitStateCntrl  ,

	 @return KErrNone if successful, KErrNotSupported if unsuccessful.

	 @see TPccdMemType.

	 @see TPccdAccessSpeed.
     */
	virtual TInt MemConfig(TInt aCard,TPccdMemType aMemType,TPccdAccessSpeed aSpeed,TBool aWaitSig,TUint aFlag)=0;
    /**
     Gets the information of the socket.

     @param aSocket  Socket whose information to be retrieved.

     @param anInfo  Gets the socket information in anInfo.
     
	 @see TPcCardSocketInfo.
	 */
    virtual void SocketInfo(TSocket aSocket,TPcCardSocketInfo& anInfo)=0;
	/**
     Checks the specified socket contains a card (and the media door for the socket is closed).

  	 @param anInfo  Has the socket information.

     @param aCard  Socket for which a card presence is checked.

	 @return True if the socket contians a card (and the media door for the socket is closed).
	 */
	virtual TBool CardIsPresent(TInt aCard)=0;
    /**
  	 Gets the changed state  of the specified media (ex,removed,inserted..)
     
     @param aMediaChangeId  stores the state of the specified media change

	 @return Mediastate when change to media takes place.

	 @see TMediaState.
	 */   
	virtual TMediaState MediaState(TInt aMediaChangeId)=0;
    // Pc Card PSUs
	/**
  	 Sets Vcc information for the Pc Card.

     @param aPsu    Power supply unit value supplied for the Pc Card.

     @param aninfo  Object of TPBusPsuInfo type passed to store the PSU information.

	 @see TPBusPsuInfo.
	 */
	virtual void VccInfo(TInt aPsu,TPBusPsuInfo &aninfo)=0;
	/**
     Sets Pc Card Vcc power supply OFF.

     @param aPsu  Power supply unit  for the Pc Card.
	 */
    virtual void VccOff(TInt aPsu)=0;
	/**
  	 Sets Pc Card Vcc power supply ON.

     @param aPsu  Power supply unit  for the Pc Card.

     @param aVoltageSetting  A voltage to power on Pc Card which is defined in enum TPccdSocketVcc.

	 @see TPccdSocketVcc
	 */
    virtual void VccOnFull(TInt aPsu,TPccdSocketVcc aVoltageSetting)=0;
    /**
	 Sets Pc Card Vcc power supply ON with current limit.

	 @param aPsu  Power supply unit  for the Pc Card.

 	 @param aVoltageSetting  A voltage of current limt to power on Pc Card which is defined in enum TPccdSocketVcc.

	 @see TPccdSocketVcc
	 */
    virtual void VccOnCurrentLimit(TInt aPsu,TPccdSocketVcc aVoltageSetting)=0;
    /**
	 Sets Pc Card Vcc level (normally ADC reading). Return KErrNotReady if the voltage check isn't complete.

	 @param aPsu  Power supply unit  for the Pc Card.
  	 */
    virtual TInt VccInMilliVolts(TInt aPsu)=0;
	/*
	 Gets Pc Card Vcc voltage status. 

	 An alternative to using ADC, when voltage checking is performed using a comparator arrangement. 

	 @param aPsu Power supply unit for the Pc Card.

	 @return KErrNone if successful, KErrGeneral if failed.
	 */
	virtual TInt VccVoltCheck(TInt aPsu)=0;
	// interrupts
	/** 
	 Gets a interrupt Id for Pc card.

	 @param aCard  Pc Card to which interrupts Id is requested.

	 @return InterruptId of the Pc Card.
     */
	virtual TInt IntIdIReq(TInt aCard)=0;
    /**
     Gets ready interrupt Id when ready signal is asserted  on Pc Card.
	 
     @param aCard  Pc Card which is interrupted through a ready signal.

	 @return Ready interrupt Id for the Pc Card.
     */
	virtual TInt IntIdRdyC(TInt aCard)=0;
    /**
     Gets interrupt status Id when status of Pc Card changes.
     
	 @param aCard  Pc Card which is interrupted through a change in state.

	 @return Interrupt status Id, for the Pc Card.
     */
	virtual TInt IntIdStsC(TInt aCard)=0;
    /**
     Gets Mediachange interrupt Id when media changes, i,e  when media door is Opend/Closed.

     @param aMediaChangeId  An  media chagne Id which tells whether media door is closed / Opened.

	 @return Media change interrupt id, when a change in media takes place.
     */
	virtual TInt IntIdMediaChange(TInt aMediaChangeId)=0;
	/**
     Acknowledge the media change interrupt.

     @param aMediaChangeId  An  media change Id which tells whether a door is closed / Opened.
	 */
	virtual void ClearMediaChange(TInt aMediaChangeId)=0;
	// machine configuration
	/**
     @param aSocket the socket ID.

	 @param aMediaDeviceInfo , Media information of the scoket which has to be checked.

	 @return True if Pc Card is present in the secified socket.
	 */
	virtual TBool IsPcCardSocket(TInt aSocket,SMediaDeviceInfo& aMediaDeviceInfo)=0;
	/**  
	 Gets information about from which socket does the media change took place.
	 
	 @param aSocket the socket ID.

	 @return socket from which socket does the media change took place.
	 */
	virtual TInt MediaChangeFromSocket(TInt aSocket)=0;
	/**
	 Gets from which socket does the Vcc signal is generated.

	 @param aSocket the socket ID.

	 @return socket Id from which Vcc signal is generated.
	 */
	virtual TInt VccFromSocket(TInt aSocket)=0;
    };

GLREF_D TPcCardControllerInterface* ThePccdCntrlInterface;

#endif


