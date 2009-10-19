// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include <drivers/sdio/sdio.h>
#include <drivers/sdio/function.h>
#include <drivers/sdio/regifc.h>
#include <drivers/sdio/sdiocard.h>
#include <drivers/sdio/cisreader.h>
#include "utraceepbussdio.h"

EXPORT_C TSDIOFunction::TSDIOFunction(TSDIOCard* aCardP, TUint8 aFunctionNumber) :
/**
Contructs a TSDIOFunction for the specified card and function number.

@param aCardP A pointer to the card object containing the required function.
@param aFunctionNumber The number of the function upon which the register interface operates.
*/
	iClientHandle(NULL),
	iRegisterInterfaceP(NULL),
	iFunctionNumber(aFunctionNumber),
	iCisPtr(0),
	iCsaPtr(0),
	iInterrupt(&aCardP->InterruptController(), aFunctionNumber),
	iCapabilities(),
	iCardP(aCardP),
	iInstanceCount(1)
	{
	TRACE3(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionConstructor, reinterpret_cast<TUint32>(this), reinterpret_cast<TUint32>(aCardP), aFunctionNumber); // @SymTraceDataPublishedTvk	

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionConstructorReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	
	}

EXPORT_C TSDIOFunction::~TSDIOFunction()
/**
Destroys the TSDIOFunction instance.
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionDestructor, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	

	__ASSERT_ALWAYS(iInstanceCount == 0, TSDIOFunction::Panic(TSDIOFunction::ESDIOFunctionBadDeletion));
	delete iRegisterInterfaceP;

	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionDestructorReturning, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	
	}

void TSDIOFunction::Close()
/**
Destroys the DSDIORegisterInterface instance.
*/
	{
	TInt oldCount = __e32_atomic_tas_ord32(&iInstanceCount, 1, -1, 0);
	if (oldCount == 1)
		delete(this);
	}

EXPORT_C TInt TSDIOFunction::RegisterClient(DBase* aClientHandle, DMutex* aMutexLockP)
/**
@publishedPartner
@released 

Registers the client with the function.

@param aClientHandle The unique Client ID
@param aMutexLock The client's data access mutex

@return KErrNone if successful, otherwise a standard Symbian OS error code.

@see TSDIOFunction::DeregisterClient
*/
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionRegisterClient, reinterpret_cast<TUint32>(this), reinterpret_cast<TUint32>(aClientHandle)); // @SymTraceDataPublishedTvk	

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">TSDIOFunction::RegisterClient()");) // @SymTraceDataInternalTechnology
	
	__ASSERT_DEBUG(aClientHandle != NULL, TSDIOFunction::Panic(TSDIOFunction::ESDIOFunctionBadClientHandle));

	if(iClientHandle != NULL)
		{
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "ERR:KErrAlreadyExists");) // @SymTraceDataInternalTechnology
		return(KErrAlreadyExists);	// Already registered with a Client
		}

	iClientHandle = aClientHandle;
	
	if((iRegisterInterfaceP = new DSDIORegisterInterface(iCardP, iFunctionNumber, aMutexLockP)) == NULL)
		{
		TRACE1(TTraceContext(EError), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionRegisterClientOOM, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	
		return(KErrNoMemory);
		}

	__e32_atomic_tas_ord32(&iInstanceCount, 1, 1, 0);
	iCardP->ClientRegistered();
	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<TSDIOFunction::RegisterClient()")); // @SymTraceDataInternalTechnology

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionRegisterClient, reinterpret_cast<TUint32>(this), KErrNone); // @SymTraceDataPublishedTvk	
	
	return(KErrNone);
	}

EXPORT_C TInt TSDIOFunction::DeregisterClient(DBase* aClientHandle)
/**
@publishedPartner
@released 

Deregisters the client from the function.

@param aClientHandle The unique Client ID

@return KErrNone if successful, otherwise a standard Symbian OS error code.

@see TSDIOFunction::RegisterClient

@todo Need to ensure that all outstanding sessions have completed
*/
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionDeregisterClient, reinterpret_cast<TUint32>(this), reinterpret_cast<TUint32>(aClientHandle)); // @SymTraceDataPublishedTvk	

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), ">TSDIOFunction::DeregisterClient()");) // @SymTraceDataInternalTechnology
	
	__ASSERT_DEBUG(aClientHandle != NULL, TSDIOFunction::Panic(TSDIOFunction::ESDIOFunctionBadClientHandle));

	TInt ret = KErrPermissionDenied;
	if(iClientHandle != NULL && (iClientHandle == aClientHandle))
		{
		delete iRegisterInterfaceP;
		iRegisterInterfaceP = NULL;

		iClientHandle = NULL;
				
		iInterrupt.Unbind();
		
		iCardP->ClientDeregistered();

		Close();
		
		ret = KErrNone;
		}

    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "<TSDIOFunction::DeregisterClient()")); // @SymTraceDataInternalTechnology

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionDeregisterClientReturning, reinterpret_cast<TUint32>(this), ret); // @SymTraceDataPublishedTvk	
 
	return(ret);
	}

EXPORT_C TInt TSDIOFunction::Enable(TBool aPollReady)
/**
@publishedPartner
@released 

Enables the function by writing to the appropriate register in the CCCR.

This function has two modes of operation. If aPollReady == ETrue, then the 
function shall attempt to enable the function, then poll for the recommended
enable period until the function is enabled, or times out.  Otherwise, flow
returns control to the calling function who should be responsible for polling
the IO_READY bit (using TSDIOFunction::IsReady)

@param aPollReady Specify ETrue to perform timeout and polling on behalf of the client.

@return KErrNone if successful, otherwise a standard Symbian OS error code.

@see TSDIOFunction::Disable
@see TSDIOFunction::IsReady
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionEnable, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOFunction::Enable");) // @SymTraceDataInternalTechnology
	//
	// The FBR SHP[1:0] bits determine the power requirements of this function.
	// If the function 'Requires' high power, then the EHP bit must be set prior
	// to enabling the function otherwise the function shall not be enabled.
	//
	// TODO: This needs to be a little more complex to support 'Optional' HP
	//
	TBool enableHighPower = EFalse;
	switch(iCapabilities.iPowerFlags & KFBRRegPowerSupportMask)
		{
		case KFBRRegHighPowerRequired:
			enableHighPower = ETrue;
			break;
		case KFBRRegHighPowerSupported:
			enableHighPower = ETrue;
			break;
		case KFBRRegStandardPower:
			enableHighPower = EFalse;
			break;
		default:
			enableHighPower = EFalse;
			break;
		}
		
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Enable High Power: %X",(iCapabilities.iPowerFlags & KFBRRegPowerSupportMask));) // @SymTraceDataInternalTechnology
	
	TInt err = KErrNone;
	if(enableHighPower)
		{
		err = iCardP->CommonRegisterInterface()->Modify8((KFBRFunctionOffset * iFunctionNumber) + KFBRRegPowerFlags, KFBRRegEnableHighPower, 0);
		
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Enabled High Power err:%d",err);) // @SymTraceDataInternalTechnology
		}
		

	if(err == KErrNone)
		{
		// Write to IO Enable Registers
		err = iCardP->CommonRegisterInterface()->Modify8(KCCCRRegIoEnable, (TUint8)(0x01 << iFunctionNumber), 0x00);
		
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Write to IO Enable Registers err:%d",err);) // @SymTraceDataInternalTechnology
	    }
		
	if((err == KErrNone) && aPollReady)
		{
		// Determine the timeout period (this is expressed in 10mS steps)
		TUint32 timeout = iCapabilities.iEnableTimeout ? iCapabilities.iEnableTimeout : KDefaultFunctionEnableTimeout;
			
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Timeout period:%d",timeout);) // @SymTraceDataInternalTechnology

		// Synchronous enable polls here, returning to the calling function
		// after the function is initialise, or timeout has been exceeded
		const TUint32 pollPeriod = 100;	// 100mS poll period
		const TUint32 pollCount  = (10*timeout) / pollPeriod;
			
		err = Kern::PollingWait(PollFunctionReady, this, pollPeriod, pollCount); // KErrNone or KErrTimedOut
		}

    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOFunction::Enable err:%d", err);) // @SymTraceDataInternalTechnology

    TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionEnableReturning, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk	

    return(err);
	}

TBool TSDIOFunction::PollFunctionReady(TAny* aSelfP)
/**
Poll function used to determine the state of the function after enable.
@see TSDIOFunction::Enable
*/
	{
	TSDIOFunction& self = *(TSDIOFunction*)aSelfP;
	
	TBool isReady = EFalse;
	(void)self.IsReady(isReady);
	
	return(isReady);
	}

EXPORT_C TInt TSDIOFunction::Disable()
/**
@publishedPartner
@released 

Disables the function by writing to the appropriate register in the CCCR.

@return KErrNone if successful, otherwise a standard Symbian OS error code.

@see TSDIOFunction::Enable
@see TSDIOFunction::IsReady
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionDisable, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	

	// Write to IO Enable Registers
	TInt err = iCardP->CommonRegisterInterface()->Modify8(KCCCRRegIoEnable, 0, TUint8(0x01 << iFunctionNumber));
	if(err == KErrNone)
		{
		err = iCardP->CommonRegisterInterface()->Modify8((KFBRFunctionOffset * iFunctionNumber) + KFBRRegPowerFlags, 0, KFBRRegEnableHighPower);
		}

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionDisable, reinterpret_cast<TUint32>(this), err); // @SymTraceDataPublishedTvk	
	return(err);
	}

EXPORT_C TInt TSDIOFunction::IsReady(TBool& aIsReady)
/**
@publishedPartner
@released 

Used to determine if the function is powered up and ready.

Reads the state of the functions IO_READY to determine if the function is enabled.

@param aIsReady Returns the current state of the function.

@return KErrNone if successful, otherwise a standard Symbian OS error code.

@see TSDIOFunction::Enable
@see TSDIOFunction::Disable
*/
	{
	TRACE1(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionIsReady, reinterpret_cast<TUint32>(this)); // @SymTraceDataPublishedTvk	

	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "Checking whether the function is ready")); // @SymTraceDataInternalTechnology	

	TInt err;
	
	TUint8 ioRdy = EFalse;
	if((err = iCardP->CommonRegisterInterface()->Read8(KCCCRRegIoReady, &ioRdy)) == KErrNone)
		{
		aIsReady = (ioRdy & (0x01 << iFunctionNumber)) != 0;
		}

	TRACE3(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionIsReadyReturning, reinterpret_cast<TUint32>(this), static_cast<TUint32>(aIsReady), err); // @SymTraceDataPublishedTvk	

	return(err);
	}

EXPORT_C TInt TSDIOFunction::SetPriority(TSDIOFunctionPriority aPriority)
/**
This sets the priority of accesses to this function.  

This is intended to allow the suspend/resume protocol to determine whether an access to a lower 
priority function should become suspended while a higher priority function is accessed.

Note that the Suspend/Resume protocol is not currently implemented, but may be in a future release.

@param aPriority The requested function priority.

@return KErrNone if successful, otherwise a standard Symbian OS error code.

@see TSDIOFunctionPriority
*/
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionSetPriority, reinterpret_cast<TUint32>(this), static_cast<TUint32>(aPriority)); // @SymTraceDataPublishedTvk	

	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionSetPriorityReturning, reinterpret_cast<TUint32>(this), KErrNone); // @SymTraceDataPublishedTvk	

	return(KErrNone);
	}


void TSDIOFunction::Panic(TSDIOFunction::TPanic aPanic)
/**
Function Panic
*/
	{
	Kern::Fault("SDIO_FN", aPanic);
	}


TInt TSDIOFunction::ParseCIS()
	{
	TCisReader iCisRd;

	TInt err = KErrNone;	
	
	const TInt socketNum = iCardP->iStackP->MMCSocket()->iSocketNumber;

	if((err = iCisRd.SelectCis(socketNum,0,0,iFunctionNumber)) == KErrNone)
		{	    
		err = iCisRd.FindReadFunctionConfig(iCapabilities);
		}
	
	return err;
	}


EXPORT_C TBool TSDIOFunctionCaps::CapabilitiesMatch(TSDIOFunctionCaps& aCaps, TUint32 aMatchFlags)
/**
@publishedPartner
@released 

Compares the capabilities provided in aCaps with the capabilities of the function.

@param aCaps The capabilities provided by the user.
@param aMatchFlags A bitmask of specific capabilities to match.

@return ETrue if the capabilities match.

@see TSDIOCard::FindFunction
@see TSDIOFunctionCaps
@see TSDIOCapsMatch
*/
	{
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionCapabilitiesMatch, reinterpret_cast<TUint32>(this), aMatchFlags); // @SymTraceDataPublishedTvk	
	
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOFunctionCaps::CapabilitiesMatch - aMatchFlags: %d",aMatchFlags);) // @SymTraceDataInternalTechnology
	
	if(aMatchFlags == TSDIOFunctionCaps::EDontCare)
		{
		// Always match if EDontCare is specified
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOFunctionCaps::CapabilitiesMatch - EDontCare Specified!");) // @SymTraceDataInternalTechnology
		return(ETrue);
		}
	if(aMatchFlags & TSDIOFunctionCaps::EFunctionNumber)
		{
		// Function numbers must match
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOFunctionCaps - Match Function Numbers");) // @SymTraceDataInternalTechnology
		aMatchFlags = (iNumber == aCaps.iNumber) ? aMatchFlags : 0;
		}
	if(aMatchFlags & TSDIOFunctionCaps::EFunctionType)
		{
		// Function type must match
		aMatchFlags = (iType == aCaps.iType) ? aMatchFlags : 0;
		}
	if(aMatchFlags & TSDIOFunctionCaps::EHasCSA)
		{
		// Must contain a Code Storage Area
		aMatchFlags = (iHasCSA == aCaps.iHasCSA) ? aMatchFlags : 0;
		}
	if(aMatchFlags & TSDIOFunctionCaps::EPowerFlags)
		{
		// High-Power support requirements must match
		aMatchFlags = (iPowerFlags == aCaps.iPowerFlags) ? aMatchFlags : 0;
		}	
	if(aMatchFlags & EFunctionInfo)
		{
		// Specified bits in TPLFE_FUNCTION_INFO definition must match
		aMatchFlags = ((iFunctionInfo && aCaps.iFunctionInfo) == aCaps.iFunctionInfo) ? aMatchFlags : 0;
		}
	if(aMatchFlags & TSDIOFunctionCaps::ERevision)
		{
		// Revision code must match
		aMatchFlags = (iRevision == aCaps.iRevision) ? aMatchFlags : 0;
		}
	if(aMatchFlags & TSDIOFunctionCaps::ESerialNumber)
		{
		// Serial number must match
		aMatchFlags = (iSerialNumber == aCaps.iSerialNumber) ? aMatchFlags : 0;
		}
	if(aMatchFlags & ECSASize)
		{
		// Size of CSA must be greater than or equal to that requested
		aMatchFlags = (iCSASize >= aCaps.iCSASize) ? aMatchFlags : 0;
		}
	if(aMatchFlags & ECSAProperties)
		{
		// Specified bits in TPLFE_CSA_PROPERTY definition must match
		aMatchFlags = ((iCSAProperties && aCaps.iCSAProperties) == aCaps.iCSAProperties) ? aMatchFlags : 0;
		}
	if(aMatchFlags & EMaxBlockSize)
		{
		// Maximum block size must be greater than or equal to that specified
		aMatchFlags = (iMaxBlockSize >= aCaps.iMaxBlockSize) ? aMatchFlags : 0;
		}
	if(aMatchFlags & TSDIOFunctionCaps::EOcr)
		{
		// Requested OCR is must be a subset of the card's supported OCR.
		aMatchFlags = (iOCR & aCaps.iOCR) ? aMatchFlags : 0;
		}
	if(aMatchFlags & EMinPwrStby)
		{
		// Minimum Standby Power must not exceed that specified
		aMatchFlags = (iMinPwrStby <= aCaps.iMinPwrStby) ? aMatchFlags : 0;
		}
	if(aMatchFlags & TSDIOFunctionCaps::EAvePwrStby)
		{
		// Average Standby Power must not exceed that specified
		aMatchFlags = (iAvePwrStby <= aCaps.iAvePwrStby) ? aMatchFlags : 0;
		}
	if(aMatchFlags & EMaxPwrStby)
		{
		// Maximum Standby Power must not exceed that specified
		aMatchFlags = (iMaxPwrStby <= aCaps.iMaxPwrStby) ? aMatchFlags : 0;
		}
	if(aMatchFlags & EMinPwrOp)
		{
		// Minimum Operating Power must not exceed that specified
		aMatchFlags = (iMinPwrOp <= aCaps.iMinPwrOp) ? aMatchFlags : 0;
		}
	if(aMatchFlags & TSDIOFunctionCaps::EAvePwrOp)
		{
		// Average Operating Power must not exceed that specified
		aMatchFlags = (iAvePwrOp <= aCaps.iAvePwrOp) ? aMatchFlags : 0;
		}
	if(aMatchFlags & EMaxPwrOp)
		{
		// Maximum Operating Power must not exceed that specified
		aMatchFlags = (iMaxPwrOp <= aCaps.iMaxPwrOp) ? aMatchFlags : 0;
		}
	if(aMatchFlags & EAveHiPwr)
		{
		// Average High Power must not exceed that specified
		aMatchFlags = (iAveHiPwr <= aCaps.iAveHiPwr) ? aMatchFlags : 0;
		}
	if(aMatchFlags & EMaxHiPwr)
		{
		// Maximum High Power must not exceed that specified
		aMatchFlags = (iMaxHiPwr <= aCaps.iMaxHiPwr) ? aMatchFlags : 0;
		}
	if(aMatchFlags & EMinBandwidth)
		{
		// Minumum bandwidth must be less than or equal to that specified
		aMatchFlags = (iMinBandwidth <= aCaps.iMinBandwidth) ? aMatchFlags : 0;
		}
	if(aMatchFlags & EOptBandwidth)
		{
		// Optimum bandwidth must be less than or equal to that specified
		aMatchFlags = (iOptBandwidth <= aCaps.iOptBandwidth) ? aMatchFlags : 0;
		}
	if(aMatchFlags & EStandardFunctionID)
		{
		// The standard function ID must match
		aMatchFlags = (iStandardFunctionID == aCaps.iStandardFunctionID) ? aMatchFlags : 0;
		}
	if(aMatchFlags & EStandardFunctionType)
		{
		// The standard function type must match
		SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOFunctionCaps - Match standard function type");) // @SymTraceDataInternalTechnology
		aMatchFlags = (iStandardFunctionType == aCaps.iStandardFunctionType) ? aMatchFlags : 0;
		}

    SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "TSDIOFunctionCaps::CapabilitiesMatch aMatchFlags: 0x%x",aMatchFlags);) // @SymTraceDataInternalTechnology
	TRACE2(TTraceContext(EBorder), UTraceModuleEPBusSDIO::ESDIOTSDIOFunctionCapabilitiesMatchReturning, reinterpret_cast<TUint32>(this), static_cast<TUint32>(aMatchFlags ? ETrue : EFalse)); // @SymTraceDataPublishedTvk	
    
	return(aMatchFlags ? ETrue : EFalse);
	}
	
#ifdef _DEBUG
void TSDIOFunction::TraceFunctionInfo()
/**
Debug function to output the contents of the FBR
*/
	{
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iNumber:        0x%02X", iCapabilities.iNumber)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iDevCodeEx:     0x%02X", iCapabilities.iDevCodeEx)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iType:          0x%02X", iCapabilities.iType)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iHasCSA:        %d", 	   iCapabilities.iHasCSA)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iPowerFlags:    0x%02X", iCapabilities.iPowerFlags)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iFunctionInfo:  0x%02X", iCapabilities.iFunctionInfo)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iRevision:      0x%02X", iCapabilities.iRevision)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iSerialNumber:  0x%08X", iCapabilities.iSerialNumber)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iCSASize:       0x%08X", iCapabilities.iCSASize)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iCSAProperties: 0x%02X", iCapabilities.iCSAProperties)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMaxBlockSize:  0x%04X", iCapabilities.iMaxBlockSize)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iOCR:           0x%08X", iCapabilities.iOCR)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMinPwrStby:    0x%02X", iCapabilities.iMinPwrStby)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iAvePwrStby:    0x%02X", iCapabilities.iAvePwrStby)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMaxPwrStby:    0x%02X", iCapabilities.iMaxPwrStby)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMinPwrOp:      0x%02X", iCapabilities.iMinPwrOp)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iAvePwrOp:      0x%02X", iCapabilities.iAvePwrOp)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMaxPwrOp:      0x%02X", iCapabilities.iMaxPwrOp)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMinBandwidth:  0x%04X", iCapabilities.iMinBandwidth)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iOptBandwidth:  0x%04X", iCapabilities.iOptBandwidth)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iEnableTimeout: 0x%02X", iCapabilities.iEnableTimeout)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iAveHiPwr:      0x%02X", iCapabilities.iAveHiPwr)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iMaxHiPwr:      0x%02X", iCapabilities.iMaxHiPwr)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iCisPtr:        0x%08X", iCisPtr)); // @SymTraceDataInternalTechnology
	SYMBIAN_TRACE_SDIO_VERBOSE_ONLY(Printf(TTraceContext(EInternals), "iCsaPtr:        0x%08X", iCsaPtr)); // @SymTraceDataInternalTechnology
	}
#endif
