/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
// Ethernet.h
//
//
/** @file ethernet.h
Base classes for implementating ethernet support (Kernel-side only)
@internalComponent
*/

#ifndef __ETHERNET_H__
#define __ETHERNET_H__
#include <platform.h>
#include <d32ethernet.h>
#include <e32ver.h>

/** @addtogroup enet Ethernet Drivers
 *  Kernel Ethernet Support
 */

/** @addtogroup enet_ldd Driver LDD's
 * @ingroup enet
 */

const TInt KEthernetMajorVersionNumber = 1;
const TInt KEthernetMinorVersionNumber = 0;
const TInt KEthernetBuildVersionNumber = KE32BuildVersionNumber;

/**
	@publishedPartner
	@released
*/
const TInt KTxWorkBudgetLimit  = 10;

// Card Rx constants.
const TInt KNumRXBuffers       = 40;
/**
	@publishedPartner
	@released
*/
const TInt KIRQWorkBudgetLimit = 4;
/**
	@publishedPartner
	@released
*/
const TInt KRxWorkBudgetLimit  = 6;

/**
	@publishedPartner
	@released
*/
const TInt KMaxEthernetPacket = 1518;
/**
	@publishedPartner
	@released
*/
const TUint16 CRC_LEN = 4;

/** @addtogroup enet_pdd Driver PDD's
 * @ingroup enet
 * @{
 */

/**
    Different Stop Modes
	@publishedPartner
	@released
 */
enum TStopMode 
    {
    EStopNormal,    /**< Finish sending and then stop */
    EStopEmergency  /**< Just stop now */
    };


class DChannelEthernet;


/**
	Ethernet driver base class
	The base class for an ethernet driver that doesn't support power control
	@publishedPartner
	@released
*/
class DEthernet : public DBase
    {
    public:
    /**
     * Start receiving frames
     * @return KErrNone if driver started
     */
    virtual TInt Start() =0;
    /**
     * Stop receiving frames
     * @param aMode The stop mode
     */
    virtual void Stop(TStopMode aMode) =0;

    /**
     * Validate a new config
     * Validates a new configuration should be called before Configure
     * @param aConfig is the configuration to be validated
     * @return ETrue or EFalse if the Configuration is allowed
     * @see Configure()
     */
    virtual TInt ValidateConfig(const TEthernetConfigV01 &aConfig) const =0;
    /**
     * Configure the device
     * Reconfigure the device using the new configuration supplied.
     * This should not change the MAC address.
     * @param aConfig The new configuration
     * @see ValidateConfig()
     * @see MacConfigure()
     */
    virtual TInt Configure(TEthernetConfigV01 &aConfig) =0;
    /**
     * Change the MAC address
     * Attempt to change the MAC address of the device
     * @param aConfig A Configuration containing the new MAC
     * @see Configure()
     */
    virtual void MacConfigure(TEthernetConfigV01 &aConfig) =0;
    /**
     * Get the current config from the chip
     * This returns the current configuration of the chip with the folling fields
     * The Transmit Speed
     * The Duplex Setting
     * The MAC address
     * @param aConfig is a TEthernetConfigV01 referance that will be filled in
     */
    virtual void GetConfig(TEthernetConfigV01 &aConfig) const =0;
    /**
     * Check a configuration
     * @param aConfig	a reference to the structure TEthernetConfigV01 with configuration to check
     */
    virtual void CheckConfig(TEthernetConfigV01& aConfig)=0;

    /**
     * Querie the device's capibilitys
     * @param aCaps To be filled in with the capibilites
     */
    virtual void Caps(TDes8 &aCaps) const =0;

    /**
     * Transmit data
     * @param aBuffer referance to the data to be sent
     * @return KErrNone if the data has been sent
     */
    virtual TInt Send(TBuf8<KMaxEthernetPacket+32> &aBuffer) =0;
    /**
     * Retrieve data from the device
     * Pull the received data out of the device and into the supplied buffer. 
     * Need to be told if the buffer is OK to use as if it not we could dump 
     * the waiting frame in order to clear the interrupt if necessory.
     * @param aBuffer Referance to the buffer to be used to store the data in
     * @param okToUse Bool to indicate if the buffer is usable
     * @return KErrNone if the buffer has been filled.
     */
    virtual TInt ReceiveFrame(TBuf8<KMaxEthernetPacket+32> &aBuffer, 
                              TBool okToUse) =0;

    /**
     * Disables all IRQ's
     * @return The IRQ level before it was changed
     * @see RestoreIrqs()
     */
    virtual TInt DisableIrqs()=0;
    /**
     * Restore the IRQ's to the supplied level
     * @param aIrq The level to set the irqs to.
     * @see DisableIrqs()
     */
    virtual void RestoreIrqs(TInt aIrq)=0;
    /**
     * Return the DFC Queue that this device should use
     * @param aUnit The Channel number
     * @return Then DFC Queue to use
     */
    virtual TDfcQue* DfcQ(TInt aUnit)=0;
    /**
     * The Receive Isr for the device
     */
    inline void ReceiveIsr();
    
#ifdef ETH_CHIP_IO_ENABLED
    virtual TInt BgeChipIOCtrl(TPckgBuf<TChipIOInfo> &aIOData)=0;
#endif
    /**
     * A pointer to the logical device drivers channel that owns this device
     */
    DChannelEthernet * iLdd ;
    };

/** @} */ // End of pdd group

/**
 * @addtogroup enet_ldd_nopm_nopccard Ethernet LDD Not PCMCIA and No Power Managment
 * @ingroup enet_ldd
 * @ingroup enet_misa
 * @ingroup enet_wins
 * @{
 */

/**
	Ethernet logical device
	The class representing the ethernet logical device
	@internalComponent
*/
class DDeviceEthernet : public DLogicalDevice
    {
    public:
    /**
     * The constructor
     */
    DDeviceEthernet();
    /**
     * Install the device
     */
    virtual TInt Install();
    /**
     * Get the Capabilites of the device
     * @param aDes descriptor that will contain the returned capibilites
     */
    virtual void GetCaps(TDes8 &aDes) const;
    /**
     * Create a logical channel to the device
     */
    virtual TInt Create(DLogicalChannelBase*& aChannel);
    };


/**
	Stores the Tx and RX buffers for use in a ethernet logical channel
	@internalComponent
 */
class DChannelEthernetFIFO 
    {
    public:
    /**
     * The TX Buffer
     */
    TBuf8<KMaxEthernetPacket+32> iTxBuf;

    /**
     * The constructor
     */
    DChannelEthernetFIFO();
    /**
     * The destructor
     */
    ~DChannelEthernetFIFO();
    /**
     * Get a empty receive buffer
     * @return NULL or a pointer to the free buffer
     */
    TBuf8<KMaxEthernetPacket+32> * GetFree();
    /**
     * Get the next full buffer
     * @return NULL or a pointer to the full buffer
     */ 
    TBuf8<KMaxEthernetPacket+32> * GetNext();
    /**
     * Move on to the next full buffer
     */
    void SetNext();

    private:
    /**
     * The array of receive buffers
     */
    TBuf8<KMaxEthernetPacket+32> iRxBuf[KNumRXBuffers];
    /**
     * Index into the array of the next full buffer
     */
    TInt iRxQueIterNext;
    /**
     * Index into the array of the next empty buffer
     */
    TInt iRxQueIterFree;
    /**
     * Count of the number of empty buffers
     */
    TInt iNumFree;
    };

/**
	The logical channel for ethernet devices
	The basic ethernet logical channel that doesn't support
	power control or PCCard ethernet devices
	@internalComponent
 */
class DChannelEthernet : public DLogicalChannel
    {
    public:
    /**
     * The state of the logical channel
     */
    enum TState 
        {
        EOpen,    /**< The channel is open */
        EActive,  /**< The channel is open and busy */
        EClosed   /**< The channel is closed */
        };

    /**
     * Requests that can be made on the channel
     */
    enum TRequest 
        {
        ERx=1,      /**< Receive a frame */
        ETx=2,      /**< Transmit a frame */
        EAll=0xff   /**< Complete/cancel all outstanding requests */
        };

    /**
     * The constructor 
     */
    DChannelEthernet();
    /**
     * The destructor
     */
    ~DChannelEthernet();

    /**
     * The ISR function for the channel
     * This is called by the pycical channels ISR when data is received
     * It passes a empty buffer to the PDD for it to store the frame in
     */
    virtual void ReceiveIsr();

    protected:
    /**
     * Create a logical ethernet channel
     * @param aUnit The channel number to create
     * @param anInfo not used, can be NULL
     * @param aVer The minimun driver version allowed
     * @return KErrNone if channel created
     */
    virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
    /**
     * Handle a message from the channels user
     * @param aMsg The message to handle
     */
    virtual void HandleMsg(TMessageBase* aMsg);
    /**
     * Cancel an outstanding request
     * @param aMask A mask containing the requests to be canceled
     */
    void DoCancel(TInt aMask);
    /**
     * Preform a control operation on the channel
     * Control operations are:
     * - Get the current configuration
     * - Configure the channel
     * - Set the MAC address for the channel
     * - Get the capibilities of the channel
     * @param aId The operation to preform
     * @param a1 The data to use with the operation
     * @param a2 can be NULL - not used
     * @return KErrNone if operation done
     */
    TInt DoControl(TInt aId, TAny* a1, TAny* a2);
    /**
     * Preform an asynchros operation on the channel
     * Operations are:
     * - Read data from the channel
     * - Write data to the channel
     * @param aId The operation to perform
     * @param aStatus The status object to use when complete
     * @param a1 The data to use
     * @param a2 The length of the data to use
     * @return KErrNone if operation started ok
     * @see Complete()
     */
    TInt DoRequest(TInt aId, TRequestStatus* aStatus, TAny* a1, TAny* a2);

	//Override sendMsg to copy clients memory in client context for WDP.
	virtual TInt SendMsg(TMessageBase* aMsg);

	TInt SendControl(TMessageBase* aMsg);

	TInt SendRequest(TMessageBase* aMsg);

    /**
     * Start the channel receiving
     */
    void Start();
    /**
     * Shutdown the channel
     */
    void Shutdown();

    /**
     * Complete an RX request
     * Is run as the RX DFC and reads a buffer from the receive FIFO into
     * the users buffer
     */
    void DoCompleteRx();
    /** 
     * Completes an outstanding request
     * @param aMask Specifies what request to complete
     * @param aReason The reason the request is complete
     * @see DoRequest()
     */
    void Complete(TInt aMask, TInt aReason);

    /**
     * Disables all IRQ's
     * @return The IRQ level before it was changed
     * @see RestoreIrqs()
     */
    inline TInt DisableIrqs();
    /**
     * Restore the IRQ's to the supplied level
     * @param aIrq The level to set the irqs to.
     * @see DisableIrqs()
     */
    inline void RestoreIrqs(TInt aIrq);

    /**
     * Calls the PDD's start method
     * @return KErrNone if the PDD started ok
     */
    inline TInt PddStart();
    /**
     * Calls the PDD's Validate method
     * @param aConfig The configuration to be validated
     * @return KErrNone if config is valid
     */
    inline TInt ValidateConfig(const TEthernetConfigV01 &aConfig) const;
    /**
     * Calls the PDD's Configure method
     * Should call Validate first
     * Will not change the MAC address
     * @param aConfig The new configuration
     * @see ValidateConfig()
     */
    inline void PddConfigure(TEthernetConfigV01 &aConfig);
    /**
     * Calls the PDD's MacConfigure method
     * Will not change anything but the MAC address
     * @param aConfig A configuration containing the new MAC address
     */
    inline void MacConfigure(TEthernetConfigV01 &aConfig);
    /**
     * Calls the PDD's ChheckConfig Method
     * @param aConfig The config to check
     */
    inline void PddCheckConfig(TEthernetConfigV01 &aConfig);

    /**
     * Calls the PDD's get capibilities method
     * @param aCaps Filled in with the PDD capibilites on return
     */
    inline void PddCaps(TDes8 &aCaps) const;

    /**
     * Sends data using the PDD
     * @param aBuffer A referance to a buffer to be sent
     * @return KErrNone if buffer sent
     */
    inline TInt PddSend(TBuf8<KMaxEthernetPacket+32> &aBuffer);
    /**
     * Receive a frame from the PDD
     * @param aBuffer A referance to the buffer that the frame is to be put in
     * @param okToUse Flag to say if the buffer is valid
     * @return KErrNone if the buffer now contains a frame
     */
    inline TInt PddReceive(TBuf8<KMaxEthernetPacket+32> &aBuffer, TBool okToUse);

    private:
    /**
     * The DFC called by the kernel
     * @param aPtr A pointer to the channel object
     */
    static void CompleteRxDfc(TAny* aPtr);

    /**
     * Start a read request
     * @param aRxDes The buffer to be filled with data
     * @param aLength The max size frame that can fit in the buffer
     */
    void InitiateRead(TAny* aRxDes, TInt aLength);
    /**
     * Start a write request
     * @param aTxDes The buffer containing the frame to be sent
     * @param aLength The length of the frame to be sent
     */
    void InitiateWrite(TAny* aTxDes, TInt aLength);
    /**
     * Validates and set a new configuration
     * @param c The configuration to try
     * @return KErrNone if new configuration set
     */
    TInt SetConfig(TEthernetConfigV01& c);
    /**
     * Validates and sets a new MAC address
     * @param c The configuration containing the MAC to be set
     * @return KErrNone if the MAC is set ok
     */
    TInt SetMAC(TEthernetConfigV01& c);

    /**
     * The current channel configuration
     */
    TEthernetConfigV01 iConfig;

    /**
     * Pointer to the client thread
     */
    DThread* iClient;

    /**
     * Current state of the LDD
     */
    TState iStatus;

    /**
     * The receive complete DFC
     */
    TDfc iRxCompleteDfc;
    
    /**
     * Records if the channel is being shutdown
     */
    TBool iShutdown;			// ETrue means device is being closed

    /**
     * The length of the clients buffer
     */
    TInt iRxLength;

    /**
     * The TX and RX buffers
     */
    DChannelEthernetFIFO iFIFO;

	//Read request to store user request status for WDP
	TClientBufferRequest* iReadRequest;
	//Read buffer to pin client buffer in client context for WDP
	TClientBuffer* iClientReadBuffer;

	//Write request to store user request status for WDP
	TClientRequest* iWriteRequest;

#ifdef ETH_CHIP_IO_ENABLED
	TPckgBuf<TChipIOInfo> iChipInfo;
#endif
    };
/** @} */

#include <drivers/ethernet.inl>

#endif
