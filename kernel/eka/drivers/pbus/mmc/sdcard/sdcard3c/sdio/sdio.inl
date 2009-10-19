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

/** 
  @file sdio.inl
  @internalTechnology
  Inline functions for the DSDIOSession and DSDIOStack classes
*/

#ifndef __SDIO_INL__
#define __SDIO_INL__

//	--------  class TSDIOResponseR5  --------

inline TSDIOResponseR5::TSDIOResponseR5(const TUint8* aPtr) 
/**
Constructs a TSDIOResponseR5 from the supplied response.
@param aPtr A pointer to the Little-Endian Response to the IO command.
*/
	: iData(TMMC::BigEndian32(aPtr))
	{}
	 
inline TSDIOResponseR5::TSDIOResponseR5(const TUint32& aData)
/**
Constructs a TSDIOResponseR5 from the supplied response.
@param aData A reference to the Big-Endian Response to the IO command.
*/
	 : iData(aData)
	 {}
	 
inline TSDIOResponseR5::operator TUint32() const 
/**
TUint32 Conversion Operator.
@return The response in Big-Endian format.
*/
	{ return(iData); }
	
inline TUint32 TSDIOResponseR5::Error() const 
/**
Returns the error contained within the R5 response.
@return The error contained within the R5 response.
*/
	{ return(iData & KSDIOErrorMask); }
	
inline TSDIOCardStateEnum TSDIOResponseR5::State() const
/**
Returns the current state of the SDIO bus.
@return The current state of the SDIO bus.
*/
	{ return((TSDIOCardStateEnum)(iData & KSDIOCurrentStateMask)); }


inline TUint8 TSDIOResponseR5::Data() const
/**
Returns the data field contained in the R5 response.
@return The data field contained in the R5 response.
*/
	{ return((TUint8)(iData & KSDIODataMask)); }


// ======== DSDIOSession ========

inline DSDIOSession::DSDIOSession(const TMMCCallBack& aCallBack)
	:	DSessionBase(aCallBack),
		iFunctionNumber(0)
/**
@publishedPartner
@released

Constructs a DSDIOSession object using the specified client callback.

@param aCallBack Completion Callback
*/
	{}




inline DSDIOSession::DSDIOSession(const TMMCCallBack& aCallBack, TUint8 aFunctionNumber)
	:	DSessionBase(aCallBack),
		iFunctionNumber(aFunctionNumber)
/**
@publishedPartner
@released

Constructs a DSDIOSession object using the specified client callback and function number.

@param aCallBack Completion callback
@param aFunctionNumber Function number
*/
	{}




inline void DSDIOSession::SetCallback(const TMMCCallBack& aCallBack)
/**
Returns the function number upon which this session operates

@return The function number
*/
	{ iCallBack = aCallBack; }




inline TUint8 DSDIOSession::FunctionNumber() const
/**
Returns the function number upon which this session operates

@return The function number
*/
	{ return(iFunctionNumber); }




inline void DSDIOSession::SetFunctionNumber(TUint8 aFunctionNumber)
/**
Sets the function number upon which this session operates
*/
	{ iFunctionNumber = aFunctionNumber; }




inline void DSDIOSession::FillAddressParam(TUint32& aParam, TUint8 aFunction, TUint32 aAddr)
/**
Fills the address and function number parameters into the 32-bit command parameter
(Assumes that the function and address bits are already stuffed with zero's)
*/
	{
	aParam |= (aFunction & KSdioCmdFunctionMask) << KSdioCmdFunctionShift;
	aParam |= (aAddr & KSdioCmdAddressMask) << KSdioCmdAddressShift;
	}




inline void DSDIOSession::ModifyBits(TUint8& aValue)
/**
Modifies the parameter aValue using iSetBits/iClrBits
*/
	{
	aValue &= ~iClrBits;
	aValue |= iSetBits;
	}




inline void DSDIOSession::UnblockInterrupt(TMMCErr aReason)
/**
Unblocks the session with the KMMCBlockOnInterrupt flag
*/
	{	
	iState |= KMMCSessStateDoDFC;
	UnBlock(KMMCBlockOnInterrupt, aReason);
	}




// ======== DSDIOStack ========

inline DSDIOStack::DSDIOStack(TInt aBus, DMMCSocket* aSocketP)
	:	DStackBase(aBus, aSocketP)
/**
@publishedPartner
@released 

Constructs a DSDIOStack object

@param aBus Unused
@param aSocketP A pointer to the associated socket.
*/
	{}



inline DSDIOSession& DSDIOStack::SDIOSession()
/**
@publishedPartner
@released 

Returns a reference to the current session

@return A reference to the current session
*/
	{ return static_cast<DSDIOSession&>(Session()); }

	
inline DSDIOSession* DSDIOStack::CommandSessionP()
/**
Returns the pointer to the current Command Session.
Valid if the Variant has blocked using BlockIOSession specifying ESDIOBlockOnCommand.

@return The pointer to the current Command Session

@see DSDIOStack::BlockIOSession
@see DSDIOStack::DataSessionP
*/
	{ return(iCmdSessionP); }


inline DSDIOSession* DSDIOStack::DataSessionP()
/**
Returns the pointer to the current Data Session.
Valid if the Variant has blocked using BlockIOSession specifying ESDIOBlockOnData.

@return The pointer to the current Data Session

@see DSDIOStack::BlockIOSession
@see DSDIOStack::CommandSessionP
*/
	{ return(iDataSessionP); }
	
inline TMMCErr DSDIOStack::BaseModifyCardCapabilitySMST( TAny* aStackP )
	{ return( static_cast<DSDIOStack *>(aStackP)->DSDIOStack::ModifyCardCapabilitySM() ); }

inline TSDIOCardArray& DSDIOStack::CardArray() const
/**
@publishedPartner
@released 

Returns a reference to the card array

@return A reference to the card array
*/
	{ return *(TSDIOCardArray*)iCardArray; }

#endif	// #ifndef __SDIO_INL__

