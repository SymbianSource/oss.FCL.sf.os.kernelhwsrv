// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\d32ethernet.h
// User side class definition for using ethernet support
// 
//

/**
 @file d32ethernet.h
 @publishedPartner
 @released
*/

#ifndef __D32ETHERNET_H__
#define __D32ETHERNET_H__
#include <e32cmn.h>
#include <e32ver.h>


/** @addtogroup enet Ethernet Drivers
 *  Kernel Ethernet Support
 */

/** @addtogroup enet_external The External Intarface to the Ethernet support
 * @ingroup enet
 * @{
 */
// Ethernet Specific Error Codes
/** Transmit out of memory error */
const TInt KErrTxOutOfMemory = (-302);
/** General Transmit Error */
const TInt KErrTxError       = (-303);
/** Trying to send a frame that is too big */
const TInt KErrTxFrameToBig  = (-304);
/** General Recieve Error */
const TInt KErrRxError       = (-305);

// Card configuration - speed settings.
/** Ethernet speed unknown */
const TUint8 KEthSpeedUnknown=0x00;
/** Ethernet speed autosensing */
const TUint8 KEthSpeedAuto=0x01;
/** Ethernet speed 10Mbits */
const TUint8 KEthSpeed10BaseT=0x02;
/** Ethernet speed 100Mbits */
const TUint8 KEthSpeed100BaseTX=0x03;

// Card configuration - duplex settings.
/** Ethernet duplex setting unknown */
const TUint8 KEthDuplexUnknown=0x00;
/** Ethernet duplex setting automatic */
const TUint8 KEthDuplexAuto=0x01;
/** Ethernet full duplex */
const TUint8 KEthDuplexFull=0x02;
/** Ethernet helf duplex */
const TUint8 KEthDuplexHalf=0x03;

// Default Ethernet Address
const TUint8 KDefEtherAddByte0=0x30; // MSB
const TUint8 KDefEtherAddByte1=0x32;
const TUint8 KDefEtherAddByte2=0x33;
const TUint8 KDefEtherAddByte3=0x34;
const TUint8 KDefEtherAddByte4=0x35;
const TUint8 KDefEtherAddByte5=0x36; // LSB

/** Ethernet address length */
const TUint KEthernetAddressLength=6;

/**
 * The ethernet configuration class
 */
class TEthernetConfigV01
    {
    public:
    /**
     * The speed 
     */
    TUint8 iEthSpeed;
    /**
     * The duplex setting
     */
    TUint8 iEthDuplex;
    /** 
     * The MAC address
     */
    TUint8 iEthAddress[KEthernetAddressLength];
    };
typedef TPckgBuf<TEthernetConfigV01> TEthernetConfig;

/**
 * The ethernet capibility class
 */
class TEthernetCapsV01
    {
    public:

    };
typedef TPckgBuf<TEthernetCapsV01> TEthernetCaps;

/**
 * The Ethernet device capibility class
 */
class TCapsDevEthernetV01
    {
    public:
    /**
     * The device version
     */
    TVersion version;
    };

    
#ifdef ETH_CHIP_IO_ENABLED
enum TMemSpace 
    {
    BGE_SPACE_CFG =  0,       /* PCI config space */
    BGE_SPACE_REG = 1,       /* PCI memory space */
    BGE_SPACE_NIC =  2,       /* on-chip memory   */
    BGE_SPACE_MII =  3,       /* PHY's MII registers  */
    BGE_SPACE_BGE =  4,       /* driver's soft state  */
    BGE_SPACE_TXDESC = 5,       /* TX descriptors   */
    BGE_SPACE_TXBUFF = 6,       /* TX buffers       */
    BGE_SPACE_RXDESC = 7,       /* RX descriptors   */
    BGE_SPACE_RXBUFF = 8,       /* RX buffers       */
    BGE_SPACE_STATUS = 9,       /* status block     */
    BGE_SPACE_STATISTICS = 10,      /* statistics block */
    BGE_SPACE_SEEPROM =  11,      /* SEEPROM (if fitted)  */
    BGE_SPACE_FLASH = 12      /* FLASH (if fitted)    */
    };

struct TChipIOInfo
    {
    TUint32 iSize; /* in bytes: 1,2,4,8    */
    TUint32 iSpace; /* See #defines below   */
    TUint32 iOffset;
    TUint32 iData; /* output for peek  */
    TUint32 iCnt; /* number of contigues items to be dumped*/
    };   
#endif 


/**
@publishedPartner
@released

The externally visible interface
*/
class RBusDevEthernet : public RBusLogicalChannel 
    {
    public:
	
	enum TVer {EMajorVersionNumber=1,EMinorVersionNumber=0,EBuildVersionNumber=KE32BuildVersionNumber};

    /**
     * Asyncorus requests
     */
    enum TRequest
	{
	ERequestRead=0x0,         /**< Read request */
        ERequestReadCancel=0x1,   /**< Cancel read request */
	ERequestWrite=0x1,        /**< Write request */
        ERequestWriteCancel=0x2   /**< Cancel write request */
#ifdef ETH_CHIP_IO_ENABLED
    ,EChipDiagIOCtrl=0x3
#endif
	};

    /**
     * Control requests
     */
    enum TControl
	{
	EControlConfig,    /**< Get the current configuration */
        EControlSetConfig, /**< Set the current configuration */
        EControlSetMac,    /**< Set the MAC address */
        EControlCaps       /**< Get ethernet capibilites */
	};

    public:
#ifndef __KLIB_H__
    /**
     * Open a channel
     * @param aUnit The channel number to open
     */
    inline TInt Open(TInt aUnit);
    /**
     * Get the ethernet version
     * @return The version
     */
    inline TVersion VersionRequired() const;
    /**
     * Read from the channel
     * @param aStatus The callback status 
     * @param aDes Buffer to be filled in
     */
    inline void Read(TRequestStatus &aStatus,TDes8 &aDes);
    /**
     * Read from the channel
     * @param aStatus The callback status 
     * @param aDes Buffer to be filled in
     * @param aLength The maximun length frame to read
     */
    inline void Read(TRequestStatus &aStatus,TDes8 &aDes,TInt aLength);
    /**
     * Cancel a pending read request
     */
    inline void ReadCancel();

    /**
     * Write to the channel
     * @param aStatus The callback status 
     * @param aDes Buffer containing the frame to be sent
     */
    inline void Write(TRequestStatus &aStatus,const TDesC8 &aDes);
    /**
     * Write to the channel
     * @param aStatus The callback status 
     * @param aDes Buffer containing the frame to be sent
     * @param aLength The length of the frame to send
     */
    inline void Write(TRequestStatus &aStatus,const TDesC8 &aDes,TInt aLength);
    /**
     * Cancel a pending write request
     */
    inline void WriteCancel();

    /**
     * Get the channels configuration
     * @param aConfig Buffer that will contain an configuration object
     */
    inline void Config(TDes8 &aConfig);
    /**
     * Set the channels configuration
     * @param aConfig Buffer containing an configuration object
     */
    inline TInt SetConfig(const TDesC8 &aConfig);
    /**
     * Set the channels MAC address
     * @param aConfig Buffer containing an configuration object with the MAC 
     * address set
     */
    inline TInt SetMAC(const TDesC8 &aConfig);

    /**
     * Request the channels capabilities
     * @param aCaps Buffer to contain the capibilites object
     */
    inline void Caps(TDes8 &aCaps);
    
#ifdef ETH_CHIP_IO_ENABLED    
    inline void ChipIOCtrl(TRequestStatus &aStatus,TPckgBuf<TChipIOInfo> &aDes);
#endif
    #endif
    };

/** @} */ // End of external interface
#include <d32ethernet.inl>

#endif
