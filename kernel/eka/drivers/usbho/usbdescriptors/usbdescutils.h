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
// Description:
//

/**
 @file
 @internalComponent
*/

#ifndef USBDESCUTILS_H
#define USBDESCUTILS_H

#include <d32usbdescriptors.h>
#include <d32usbdi_errors.h>

inline void UsbDescFault(UsbdiFaults::TUsbDescFaults aFault)
	{
	User::Panic(UsbdiFaults::KUsbDescFaultCat, aFault);
	}

inline void UsbDescPanic(UsbdiPanics::TUsbDescPanics aPanic)
	{
	User::Panic(UsbdiPanics::KUsbDescPanicCat, aPanic);
	}

/**
Utility function for retrieving a TUint8 from a Little Endian USB descriptor.
@param aDes The descriptor to parse.
@param aOffset The offset in the descriptor where to parse.
@return The TUint8 value parsed.
*/
inline TUint8 ParseTUint8(TPtrC8 aDes, TInt aOffset)
	{
	return aDes[aOffset];
	}

/**
Utility function for retrieving a TUint16 from a Little Endian USB descriptor.
@param aDes The descriptor to parse.
@param aOffset The offset in the descriptor where to parse.
@return The TUint16 value parsed.
*/
inline TUint16 ParseTUint16(TPtrC8 aDes, TInt aOffset)
	{
	return ((TUint16)aDes[aOffset]) | ( ((TUint16)aDes[aOffset+1]) << 8 );
	}
	
/**
Utility function for retrieving a TUint32 from a Little Endian USB descriptor.
@param aDes The descriptor to parse.
@param aOffset The offset in the descriptor where to parse.
@return The TUint32 value parsed.
*/
inline TUint32 ParseTUint32(TPtrC8 aDes, TInt aOffset)
	{
	// Put enough brackets to ensure that all casting is correct
	// and the expression looks symmetrical
	return 	( ((TUint32)(aDes[aOffset])) ) | 
			( ((TUint32)(aDes[aOffset + 1])) << 8 ) | 
			( ((TUint32)(aDes[aOffset + 2])) << 16 ) | 
			( ((TUint32)(aDes[aOffset + 3])) << 24 );
	}

/**
A utility class to store the custom descriptor parsers.
The USBDI descriptor parsing framework creates and stores an instance
of this class in TLS when a custom parse is registered.
*/
NONSHARABLE_CLASS(CUsbCustomDescriptorParserList) : public CBase
	{
public:
	static CUsbCustomDescriptorParserList* NewL();
	~CUsbCustomDescriptorParserList();

	void RegisterParserL(UsbDescriptorParser::TUsbDescriptorParserL aParserFunc);
	void UnregisterParser(UsbDescriptorParser::TUsbDescriptorParserL aParserFunc);
	TInt NumOfRegisteredParsers() const;
	UsbDescriptorParser::TUsbDescriptorParserL RegisteredParser(TInt aIndex) const;

private:
	RArray<UsbDescriptorParser::TUsbDescriptorParserL> iParserList;
	};


#endif // USBDESCUTILS_H
