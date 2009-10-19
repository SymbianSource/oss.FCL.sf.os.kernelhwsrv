/*
* Copyright (c) 2003 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef __FUNCTION_INL__
#define __FUNCTION_INL__

// ======== TSDIOFunctionCaps ========

TSDIOFunctionCaps::TSDIOFunctionCaps()
/**
Constructs a TSDIOFunctionCaps object
*/
	{
	memclr(this, sizeof(TSDIOFunctionCaps));
	}



// ======== TSDIOFunction ========

inline const TSDIOFunctionCaps& TSDIOFunction::Capabilities() const
/**
@publishedPartner
@released

Returns information about the basic capabilities of the function (function number, function type etc.).

This generally contains the information obtained from the FBR and common CIS during initialisation, 
and may be used by the client to verify the basic suitability of the function during initialisation. 

@return A reference to the TSDIOFunctionCaps containing the capabilities of the function.

@see TSDIOFunctionCaps
*/
	{ return(iCapabilities); }



inline DSDIORegisterInterface* TSDIOFunction::RegisterInterface(DBase* aClientHandle) const
/**
@publishedPartner
@released
 
Returns a pointer to an instance of a DSDIORegisterInterface class
that may be used bythe client to talk to the Function Specific Registers.

@param aClientHandle The ID of the client (as registered using RegisterClient)
@return a pointer to the DSDIORegisterInterface associated with this function

@see TSDIOFunction::RegisterClient
*/
	{ 
	if(aClientHandle != NULL && (aClientHandle == iClientHandle))
		return(iRegisterInterfaceP);
	return(NULL);
	}



inline TSDIOInterrupt& TSDIOFunction::Interrupt()
/**
@publishedPartner
@released

Returns a reference to the TSDIOInterrupt class associated with the function
that may be used bythe client to talk to the Function Specific Registers.

@return a reference to the interrupt class associated with this function
*/
	{ return iInterrupt; };



inline TUint TSDIOFunction::CisPtr() const
/**
@publishedPartner
@released

Returns the address of the Function CIS
@return The address of the Function CIS
*/
	{ return(iCisPtr); }



inline TUint TSDIOFunction::CsaPtr() const
/**
@publishedPartner
@released

Returns the address of the Function CSA
@return The address of the Function CSA
*/
	{ return(iCsaPtr); }



inline TUint8 TSDIOFunction::FunctionNumber() const
/**
@publishedPartner
@released

Returns the number of the IO Function
@return The number of the IO Function
*/
	{ return(iFunctionNumber); }


#endif	// #ifndef __FUNCTION_INL__

