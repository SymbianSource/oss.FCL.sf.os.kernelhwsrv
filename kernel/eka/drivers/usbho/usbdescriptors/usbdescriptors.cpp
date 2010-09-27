// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Symbian USBDI Descriptors Parsing Routines.
// 
//

/**
 @file
 @publishedPartner
*/

#include <d32usbdescriptors.h>
#include "usbdescutils.h"


// ----------------
// TUsbGenericDescriptor
// ----------------

EXPORT_C TUsbGenericDescriptor::TUsbGenericDescriptor()
	: iRecognisedAndParsed(EUnrecognised)
	, iNextPeer(NULL)
	, iFirstChild(NULL)
	, iParent(NULL)
	{
	}

/**
Deletes all child and peer descriptors.  Does not delete this descriptor, the caller is responsible for
doing this separately.
*/
EXPORT_C void TUsbGenericDescriptor::DestroyTree()
	{
	// Store the tree pointers
	TUsbGenericDescriptor* child = this->iFirstChild;
	TUsbGenericDescriptor* peer = this->iNextPeer;

	// Now we chop off the tree from the root node, by doing this
	// we don't need to NULL pointers as we go down (which makes
	// the iterative algorithm more efficient).
	this->iFirstChild = NULL;
	this->iNextPeer = NULL;

	// Now we walk and destroy the tree from the two pointers
	// we have
	WalkAndDelete(child);
	WalkAndDelete(peer);
	}
	
void TUsbGenericDescriptor::WalkAndDelete(TUsbGenericDescriptor* aDesc)
	{
	if(!aDesc)
		{
		return;
		}

	TUsbGenericDescriptor* topLevel = aDesc->iParent;
	do
		{
		if(aDesc->iFirstChild)
			{
			// walk down the tree depth first.
			aDesc = aDesc->iFirstChild;
			}
		else if(aDesc->iNextPeer)
			{
			// Walk along each peer at the "bottom"
			TUsbGenericDescriptor* peer = aDesc->iNextPeer;
			delete aDesc;
			aDesc = peer;
			}
		else
			{
			// End of bottom tier, so we go back up to the parent
			// and null the first child pointer so we don't go back
			// down again.
			TUsbGenericDescriptor* parent = aDesc->iParent;
			delete aDesc;
			aDesc = parent;
			if(aDesc)
				{
				aDesc->iFirstChild = NULL;
				}
			
			// if we have gone up to the top level for destruction then we don't
			// do anymore.
			if(aDesc == topLevel)
				{
				break;
				}
			}
		}
	while(aDesc);
	}

/**
Utility method to retrieve a TUint8 value from a given offset in the descriptor.
@param aOffset The offset in the binary blob at which to retrieve the value.
@return The value from the descriptor.
*/
EXPORT_C TUint8 TUsbGenericDescriptor::TUint8At(TInt aOffset) const
	{
	return ParseTUint8(iBlob, aOffset);
	}

/**
Utility method to retrieve a TUint16 value from a given offset in the descriptor.
@param aOffset The offset in the binary blob at which to retrieve the value.
@return The value from the descriptor.
*/
EXPORT_C TUint16 TUsbGenericDescriptor::TUint16At(TInt aOffset) const
	{
	return ParseTUint16(iBlob, aOffset);
	}

/**
Utility method to retrieve a TUint32 value from a given offset in the descriptor.
@param aOffset The offset in the binary blob at which to retrieve the value.
@return The value from the descriptor.
*/
EXPORT_C TUint32 TUsbGenericDescriptor::TUint32At(TInt aOffset) const
	{
	return ParseTUint32(iBlob, aOffset);
	}

/**
Assignment operator to fill in the TUsbGenericDescriptor fields from a TUsbGenericDescriptor.
Note that if a TUsbGenericDescriptor derived class has additional member fields then
they should define a specialised assignment overload for that type.
*/
EXPORT_C TUsbGenericDescriptor& TUsbGenericDescriptor::operator=(const TUsbGenericDescriptor& aDescriptor)
	{
	ibLength = aDescriptor.ibLength;
	ibDescriptorType = aDescriptor.ibDescriptorType;
	iRecognisedAndParsed = aDescriptor.iRecognisedAndParsed;
	iNextPeer = aDescriptor.iNextPeer;
	iFirstChild = aDescriptor.iFirstChild;
	iParent = aDescriptor.iParent;
	iBlob.Set(aDescriptor.iBlob);
	return *this;
	}

/**
This function determines whether the given USB descriptor is a parent
of the descriptor the method is called on.  The implementation may be
specialised for each type of descriptor to ensure the tree is correctly
built up.
@param aPotentialRelative The USB descriptor that is being queried to see if it is a parent or peer.
@return TBool Efalse if the given USB descriptor is a parent of this USB descriptor, ETrue if a peer of this descriptor
*/
/*virtual*/ TBool TUsbGenericDescriptor::IsParent(TUsbGenericDescriptor& aPotentialParent)
	{
	// As generic descriptors we consider all other "unknown" descriptors as peers, and
	// all "known" descriptors as parents of the descriptor.
	switch(aPotentialParent.ibDescriptorType)
		{
	case EDevice:
	case EConfiguration:
	case EString:
	case EInterface:
	case EEndpoint:
	case EDeviceQualifier:
	case EOtherSpeedConfiguration:
	case EInterfacePower:
	case EOTG:
	case EDebug:
	case EInterfaceAssociation:
		return ETrue;
	default:
		return EFalse;
		}
	}

/**
This function determines whether the given USB descriptor is a peer
of the descriptor the method is called on.  The implementation may be
specialised for each type of descriptor to ensure the tree is correctly
built up.
@param aPotentialPeer The USB descriptor that is being queried to see if it is a peer.
@return TBool EFalse if the given USB descriptor is a parent of this USB descriptor, ETrue if a peer of this descriptor
*/
/*virtual*/ TBool TUsbGenericDescriptor::IsPeer(TUsbGenericDescriptor& /*aPotentialPeer*/)
	{
	// As generic descriptors we are very permissive in binding peers.
	return ETrue;
	}

/**
This function determines whether the given USB descriptor is a child
of the descriptor the method is called on.  The implementation may be
specialised for each type of descriptor to ensure the tree is correctly
built up.
@param aPotentialChild The USB descriptor that is being queried to see if it is a child.
@return TBool ETrue if the given USB descriptor is a child of this USB descriptor, ETrue if a peer of this descriptor
*/
/*virtual*/ TBool TUsbGenericDescriptor::IsChild(TUsbGenericDescriptor& /*aPotentialChild*/)
	{
	// We just use the logic in the IsParent.
	return EFalse;
	}

/**
Ensures no memory is leaked if an owned TUsbGenericDescriptor is no longer needed.
@param aPtr The TUsbGenericDescriptor that is to be cleaned up.
@internalComponent
*/
EXPORT_C /*static*/ void TUsbGenericDescriptor::Cleanup(TAny* aPtr)
	{
	TUsbGenericDescriptor* ptr = static_cast<TUsbGenericDescriptor*>(aPtr);
	ptr->DestroyTree(); // belt and braces really.
	delete ptr;
	}


// ----------------------
// TUsbDeviceDescriptor
// See section 9.6.1 of the USB 2.0 specification.
// ----------------------

EXPORT_C TUsbDeviceDescriptor::TUsbDeviceDescriptor()
	{
	}

EXPORT_C /*static*/ TUsbDeviceDescriptor* TUsbDeviceDescriptor::Cast(TUsbGenericDescriptor* aOriginal)
	{
	TUsbDeviceDescriptor* ret = NULL;
	// Only cast if correctly indentified as device descriptor
	if(	aOriginal &&
		aOriginal->ibDescriptorType == EDevice &&
		aOriginal->ibLength == TUsbDeviceDescriptor::KSizeInOctets &&
		aOriginal->iRecognisedAndParsed == ERecognised)
		{
		ret = static_cast<TUsbDeviceDescriptor*>(aOriginal);
		}
	return ret;
	}

EXPORT_C TUint16 TUsbDeviceDescriptor::USBBcd() const
	{
    return ParseTUint16(iBlob, EbcdUSB);
	}
	
EXPORT_C TUint8 TUsbDeviceDescriptor::DeviceClass() const
	{
	return ParseTUint8(iBlob, EbDeviceClass);
	}

EXPORT_C TUint8 TUsbDeviceDescriptor::DeviceSubClass() const
	{
	return ParseTUint8(iBlob, EbDeviceSubClass);
	}

EXPORT_C TUint8 TUsbDeviceDescriptor::DeviceProtocol() const
	{
	return ParseTUint8(iBlob, EbDeviceProtocol);
	}

EXPORT_C TUint8 TUsbDeviceDescriptor::MaxPacketSize0() const
	{
	return ParseTUint8(iBlob, EbMaxPacketSize0);
	}

EXPORT_C TUint16 TUsbDeviceDescriptor::VendorId() const
	{
	return ParseTUint16(iBlob, EidVendor);
	}

EXPORT_C TUint16 TUsbDeviceDescriptor::ProductId() const
	{
	return ParseTUint16(iBlob, EidProduct);
	}

EXPORT_C TUint16 TUsbDeviceDescriptor::DeviceBcd() const
	{
	return ParseTUint16(iBlob, EbcdDevice);
	}

EXPORT_C TUint8 TUsbDeviceDescriptor::ManufacturerIndex() const
	{
	return ParseTUint8(iBlob, EiManufacturer);
	}

EXPORT_C TUint8 TUsbDeviceDescriptor::ProductIndex() const
	{
	return ParseTUint8(iBlob, EiProduct);
	}

EXPORT_C TUint8 TUsbDeviceDescriptor::SerialNumberIndex() const
	{
	return ParseTUint8(iBlob, EiSerialNumber);
	}

EXPORT_C TUint8 TUsbDeviceDescriptor::NumConfigurations() const
	{
	return ParseTUint8(iBlob, EbNumConfigurations);
	}

/**
The parsing routine for device descriptors.
Here the previous descriptor parameter is ignored - because logically a device descriptor can be neither a peer
nor a child.

@internalComponent
*/
/*static*/ TUsbDeviceDescriptor* TUsbDeviceDescriptor::ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc)
	{
	TUsbDeviceDescriptor* devDes = NULL;

	const TInt KMinDeviceDesDecisionLength = 2;
	if(	aUsbDes.Length() >= KMinDeviceDesDecisionLength &&
		aUsbDes[KbDescriptorTypeOffset] == EDevice &&
		aUsbDes[KbLengthOffset] == TUsbDeviceDescriptor::KSizeInOctets)
		{
		// Robustness check - check the length field is valid, and that we have enough data.
		if(aUsbDes.Length() < TUsbDeviceDescriptor::KSizeInOctets)
			{
			User::Leave(KErrCorrupt);
			}
			
		// Robustness check - check that the device descriptor is the first to be parsed.
		if(aPreviousDesc)
			{
			User::Leave(KErrCorrupt);
			}

		// Looks ok to be a device descriptor.
		devDes = new(ELeave) TUsbDeviceDescriptor;
		// Set the standard fields
		devDes->ibLength = TUsbDeviceDescriptor::KSizeInOctets;
		devDes->ibDescriptorType = EDevice;
		// Set the blob appropriately
		devDes->iBlob.Set(aUsbDes.Left(TUsbDeviceDescriptor::KSizeInOctets));
		
		devDes->iRecognisedAndParsed = ERecognised;

		// Update the data-left-to-parse Symbian descriptor
		aUsbDes.Set(aUsbDes.Mid(TUsbDeviceDescriptor::KSizeInOctets));
		}

	return devDes;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbDeviceDescriptor::IsParent(TUsbGenericDescriptor& /*aPotentialParent*/)
	{
	// The device descriptor should only come by itself in a bundle, so must be top-level.
	return EFalse;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbDeviceDescriptor::IsPeer(TUsbGenericDescriptor& /*aPotentialPeer*/)
	{
	// The device descriptor should only come by itself in a bundle, so no other peers.
	return EFalse;
	}


// ------------------------------
// TUsbDeviceQualifierDescriptor
// See section 9.6.2 of the USB 2.0 specification.
// ------------------------------

EXPORT_C TUsbDeviceQualifierDescriptor::TUsbDeviceQualifierDescriptor()
	{
	}
	
EXPORT_C /*static*/ TUsbDeviceQualifierDescriptor* TUsbDeviceQualifierDescriptor::Cast(TUsbGenericDescriptor* aOriginal)
	{
	TUsbDeviceQualifierDescriptor* ret = NULL;
	// Only cast if correctly indentified as device qualifier descriptor
	if(	aOriginal &&
		aOriginal->ibDescriptorType == EDeviceQualifier &&
		aOriginal->ibLength == TUsbDeviceQualifierDescriptor::KSizeInOctets &&
		aOriginal->iRecognisedAndParsed == ERecognised)
		{
		ret = static_cast<TUsbDeviceQualifierDescriptor*>(aOriginal);
		}
	return ret;
	}

EXPORT_C TUint16 TUsbDeviceQualifierDescriptor::USBBcd() const
	{
	return ParseTUint16(iBlob, EbcdUSB);
	}

EXPORT_C TUint8 TUsbDeviceQualifierDescriptor::DeviceClass() const
	{
	return ParseTUint8(iBlob, EbDeviceClass);
	}

EXPORT_C TUint8 TUsbDeviceQualifierDescriptor::DeviceSubClass() const
	{
	return ParseTUint8(iBlob, EbDeviceSubClass);
	}

EXPORT_C TUint8 TUsbDeviceQualifierDescriptor::DeviceProtocol() const
	{
	return ParseTUint8(iBlob, EbDeviceProtocol);
	}

EXPORT_C TUint8 TUsbDeviceQualifierDescriptor::MaxPacketSize0() const
	{
	return ParseTUint8(iBlob, EbMaxPacketSize0);
	}

EXPORT_C TUint8 TUsbDeviceQualifierDescriptor::NumConfigurations() const
	{
	return ParseTUint8(iBlob, EbNumConfigurations);
	}

EXPORT_C TUint8 TUsbDeviceQualifierDescriptor::Reserved() const
	{
	return ParseTUint8(iBlob, EbReserved);
	}
	
/**
The parsing routine for device qualifier descriptors.

@internalComponent
*/
/*static*/ TUsbDeviceQualifierDescriptor* TUsbDeviceQualifierDescriptor::ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* /*aPreviousDesc*/)
	{
	TUsbDeviceQualifierDescriptor* devQualDes = NULL;

	const TInt KMinDevQualDesDecisionLength = 2;
	if(	aUsbDes.Length() >= KMinDevQualDesDecisionLength &&
		aUsbDes[KbDescriptorTypeOffset] == EDeviceQualifier &&
		aUsbDes[KbLengthOffset] == TUsbDeviceQualifierDescriptor::KSizeInOctets)
		{
		// Robustness check - check the length field is valid, and that we have enough data.
		if(aUsbDes.Length() < TUsbDeviceQualifierDescriptor::KSizeInOctets)
			{
			User::Leave(KErrCorrupt);
			}

		// Looks ok to be a device quialifier descriptor.
		devQualDes = new(ELeave) TUsbDeviceQualifierDescriptor;
		// Set the standard fields
		devQualDes->ibLength = TUsbDeviceQualifierDescriptor::KSizeInOctets;
		devQualDes->ibDescriptorType = EDeviceQualifier;
		// Set the blob appropriately
		devQualDes->iBlob.Set(aUsbDes.Left(TUsbDeviceQualifierDescriptor::KSizeInOctets));

		devQualDes->iRecognisedAndParsed = ERecognised;

		// Update the data-left-to-parse Symbian descriptor
		aUsbDes.Set(aUsbDes.Mid(TUsbDeviceQualifierDescriptor::KSizeInOctets));
		}

	return devQualDes;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbDeviceQualifierDescriptor::IsParent(TUsbGenericDescriptor& /*aPotentialParent*/)
	{
	// Like a device descriptor, they should be top-level.
	return EFalse;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbDeviceQualifierDescriptor::IsPeer(TUsbGenericDescriptor& /*aPotentialPeer*/)
	{
	// Like a device descriptor, they should come by themselves.
	return EFalse;
	}


// ----------------------------
// TUsbConfigurationDescriptor
// See section 9.6.3 of the USB 2.0 specification.
// ----------------------------

EXPORT_C TUsbConfigurationDescriptor::TUsbConfigurationDescriptor()
	{
	}
	
EXPORT_C /*static*/ TUsbConfigurationDescriptor* TUsbConfigurationDescriptor::Cast(TUsbGenericDescriptor* aOriginal)
	{
	TUsbConfigurationDescriptor* ret = NULL;
	// Only cast if correctly indentified as configuration descriptor
	if(	aOriginal &&
		aOriginal->ibDescriptorType == EConfiguration &&
		aOriginal->ibLength == TUsbConfigurationDescriptor::KSizeInOctets &&
		aOriginal->iRecognisedAndParsed == ERecognised)
		{
		ret = static_cast<TUsbConfigurationDescriptor*>(aOriginal);
		}
	return ret;
	}

EXPORT_C TUint16 TUsbConfigurationDescriptor::TotalLength() const
	{
	return ParseTUint16(iBlob, EwTotalLength);
	}

EXPORT_C TUint8 TUsbConfigurationDescriptor::NumInterfaces() const
	{
	return ParseTUint8(iBlob, EbNumInterfaces);
	}

EXPORT_C TUint8 TUsbConfigurationDescriptor::ConfigurationValue() const
	{
	return ParseTUint8(iBlob, EbConfigurationValue);
	}

EXPORT_C TUint8 TUsbConfigurationDescriptor::ConfigurationIndex() const
	{
	return ParseTUint8(iBlob, EiConfiguration);
	}

EXPORT_C TUint8 TUsbConfigurationDescriptor::Attributes() const
	{
	return ParseTUint8(iBlob, EbmAttributes);
	}

EXPORT_C TUint8 TUsbConfigurationDescriptor::MaxPower() const
	{
	return ParseTUint8(iBlob, EbMaxPower);
	}

/**
The parsing routine for configuration descriptors.

@internalComponent
*/
/*static*/ TUsbConfigurationDescriptor* TUsbConfigurationDescriptor::ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* /*aPreviousDesc*/)
	{
	TUsbConfigurationDescriptor* configDes = NULL;

	const TInt KMinConfigDesDecisionLength = 2;
	if(	aUsbDes.Length() >= KMinConfigDesDecisionLength &&
		aUsbDes[KbDescriptorTypeOffset] == EConfiguration &&
		aUsbDes[KbLengthOffset] == TUsbConfigurationDescriptor::KSizeInOctets)
		{
		// Robustness check - check the length field is valid, and that we have enough data.
		if(aUsbDes.Length() < TUsbConfigurationDescriptor::KSizeInOctets)
			{
			User::Leave(KErrCorrupt);
			}
			
		// Robustness check - check that there is sufficient data for whole bundle (wTotalLength)
		const TInt KwTotalLengthOffset = 2;
		if(aUsbDes.Length() < ParseTUint16(aUsbDes, KwTotalLengthOffset))
			{
			User::Leave(KErrCorrupt);
			}

		// Looks ok to be a configuration descriptor.
		configDes = new(ELeave) TUsbConfigurationDescriptor;
		// Set the standard fields
		configDes->ibLength = TUsbConfigurationDescriptor::KSizeInOctets;
		configDes->ibDescriptorType = EConfiguration;
		// Set the blob appropriately
		configDes->iBlob.Set(aUsbDes.Left(TUsbConfigurationDescriptor::KSizeInOctets));

		configDes->iRecognisedAndParsed = ERecognised;

		// Update the data-left-to-parse Symbian descriptor
		aUsbDes.Set(aUsbDes.Mid(TUsbConfigurationDescriptor::KSizeInOctets));
		}

	return configDes;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbConfigurationDescriptor::IsParent(TUsbGenericDescriptor& /*aPotentialParent*/)
	{
	// A configuration descriptor should always be the top-level descriptor in a configuration
	// bundle.
	return EFalse;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbConfigurationDescriptor::IsPeer(TUsbGenericDescriptor& /*aPotentialPeer*/)
	{
	// There should only ever be one configuration descriptor in a bundle.
	return EFalse;
	}


// --------------------------
// TUsbOtherSpeedDescriptor
// See section 9.6.4 of the USB 2.0 specification.
// --------------------------

EXPORT_C TUsbOtherSpeedDescriptor::TUsbOtherSpeedDescriptor()
	{
	}
	
EXPORT_C /*static*/ TUsbOtherSpeedDescriptor* TUsbOtherSpeedDescriptor::Cast(TUsbGenericDescriptor* aOriginal)
	{
	TUsbOtherSpeedDescriptor* ret = NULL;
	// Only cast if correctly indentified as other speed descriptor
	if(	aOriginal &&
		aOriginal->ibDescriptorType == EOtherSpeedConfiguration &&
		aOriginal->ibLength == TUsbOtherSpeedDescriptor::KSizeInOctets &&
		aOriginal->iRecognisedAndParsed == ERecognised)
		{
		ret = static_cast<TUsbOtherSpeedDescriptor*>(aOriginal);
		}
	return ret;
	}

EXPORT_C TUint16 TUsbOtherSpeedDescriptor::TotalLength() const
	{
	return ParseTUint16(iBlob, EwTotalLength);
	}

EXPORT_C TUint8 TUsbOtherSpeedDescriptor::NumInterfaces() const
	{
	return ParseTUint8(iBlob, EbNumInterfaces);
	}

EXPORT_C TUint8 TUsbOtherSpeedDescriptor::ConfigurationValue() const
	{
	return ParseTUint8(iBlob, EbConfigurationValue);
	}

EXPORT_C TUint8 TUsbOtherSpeedDescriptor::ConfigurationIndex() const
	{
	return ParseTUint8(iBlob, EiConfiguration);
	}

EXPORT_C TUint8 TUsbOtherSpeedDescriptor::Attributes() const
	{
	return ParseTUint8(iBlob, EbmAttributes);
	}

EXPORT_C TUint8 TUsbOtherSpeedDescriptor::MaxPower() const
	{
	return ParseTUint8(iBlob, EbMaxPower);
	}
	
/**
The parsing routine for other speed descriptors.

@internalComponent
*/
/*static*/ TUsbOtherSpeedDescriptor* TUsbOtherSpeedDescriptor::ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* /*aPreviousDesc*/)
	{
	TUsbOtherSpeedDescriptor* oSpeedDes = NULL;

	const TInt KMinOtherSpeedDesDecisionLength = 2;
	if(	aUsbDes.Length() >= KMinOtherSpeedDesDecisionLength &&
		aUsbDes[KbDescriptorTypeOffset] == EOtherSpeedConfiguration &&
		aUsbDes[KbLengthOffset] == TUsbOtherSpeedDescriptor::KSizeInOctets)
		{
		// Robustness check - check the length field is valid, and that we have enough data.
		if(aUsbDes.Length() < TUsbOtherSpeedDescriptor::KSizeInOctets)
			{
			User::Leave(KErrCorrupt);
			}
	
		// Robustness check - check that there is sufficient data for whole bundle (wTotalLength)
		const TInt KwTotalLengthOffset = 2;
		if(aUsbDes.Length() < ParseTUint16(aUsbDes, KwTotalLengthOffset))
			{
			User::Leave(KErrCorrupt);
			}

		// Looks ok to be an other speed descriptor.
		oSpeedDes = new(ELeave) TUsbOtherSpeedDescriptor;
		// Set the standard fields
		oSpeedDes->ibLength = TUsbOtherSpeedDescriptor::KSizeInOctets;
		oSpeedDes->ibDescriptorType = EOtherSpeedConfiguration;
		// Set the blob appropriately
		oSpeedDes->iBlob.Set(aUsbDes.Left(TUsbOtherSpeedDescriptor::KSizeInOctets));

		oSpeedDes->iRecognisedAndParsed = ERecognised;

		// Update the data-left-to-parse Symbian descriptor
		aUsbDes.Set(aUsbDes.Mid(TUsbOtherSpeedDescriptor::KSizeInOctets));
		}

	return oSpeedDes;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbOtherSpeedDescriptor::IsParent(TUsbGenericDescriptor& /*aPotentialParent*/)
	{
	// Other speed descriptor is like a configuration descriptor, in that it should
	// not have any parents in a bundle.
	return EFalse;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbOtherSpeedDescriptor::IsPeer(TUsbGenericDescriptor& /*aPotentialPeer*/)
	{
	// There should only ever be one other speed descriptor in a bundle.
	return EFalse;
	}


// ------------------------------------
// TUsbInterfaceAssociationDescriptor
// See the USB IAD ECN.
// ------------------------------------

EXPORT_C TUsbInterfaceAssociationDescriptor::TUsbInterfaceAssociationDescriptor()
	{
	}
	
EXPORT_C /*static*/ TUsbInterfaceAssociationDescriptor* TUsbInterfaceAssociationDescriptor::Cast(TUsbGenericDescriptor* aOriginal)
	{
	TUsbInterfaceAssociationDescriptor* ret = NULL;
	// Only cast if correctly indentified as interface association descriptor
	if(	aOriginal &&
		aOriginal->ibDescriptorType == EInterfaceAssociation &&
		aOriginal->ibLength == TUsbInterfaceAssociationDescriptor::KSizeInOctets &&
		aOriginal->iRecognisedAndParsed == ERecognised)
		{
		ret = static_cast<TUsbInterfaceAssociationDescriptor*>(aOriginal);
		}
	return ret;
	}

EXPORT_C TUint8 TUsbInterfaceAssociationDescriptor::FirstInterface() const
	{
	return ParseTUint8(iBlob, EbFirstInterface);
	}

EXPORT_C TUint8 TUsbInterfaceAssociationDescriptor::InterfaceCount() const
	{
	return ParseTUint8(iBlob, EbInterfaceCount);
	}

EXPORT_C TUint8 TUsbInterfaceAssociationDescriptor::FunctionClass() const
	{
	return ParseTUint8(iBlob, EbFunctionClass);
	}

EXPORT_C TUint8 TUsbInterfaceAssociationDescriptor::FunctionSubClass() const
	{
	return ParseTUint8(iBlob, EbFunctionSubClass);
	}

EXPORT_C TUint8 TUsbInterfaceAssociationDescriptor::FunctionProtocol() const
	{
	return ParseTUint8(iBlob, EbFunctionProtocol);
	}

EXPORT_C TUint8 TUsbInterfaceAssociationDescriptor::FunctionIndex() const
	{
	return ParseTUint8(iBlob, EiFunction);
	}
	
/*static*/ TUsbInterfaceAssociationDescriptor* TUsbInterfaceAssociationDescriptor::ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* /*aPreviousDesc*/)
	{
	TUsbInterfaceAssociationDescriptor* intAssocDes = NULL;

	const TInt KMinIntAssocDesDecisionLength = 2;
	if(	aUsbDes.Length() >= KMinIntAssocDesDecisionLength &&
		aUsbDes[KbDescriptorTypeOffset] == EInterfaceAssociation &&
		aUsbDes[KbLengthOffset] == TUsbInterfaceAssociationDescriptor::KSizeInOctets)
		{
		// Robustness check - check the length field is valid, and that we have enough data.
		if(aUsbDes.Length() < TUsbInterfaceAssociationDescriptor::KSizeInOctets)
			{
			User::Leave(KErrCorrupt);
			}

		// Looks ok to be a interface association descriptor.
		intAssocDes = new(ELeave) TUsbInterfaceAssociationDescriptor;
		// Set the standard fields
		intAssocDes->ibLength = TUsbInterfaceAssociationDescriptor::KSizeInOctets;
		intAssocDes->ibDescriptorType = EInterfaceAssociation;
		// Set the blob appropriately
		intAssocDes->iBlob.Set(aUsbDes.Left(TUsbInterfaceAssociationDescriptor::KSizeInOctets));
	
		intAssocDes->iRecognisedAndParsed = ERecognised;

		// Update the data-left-to-parse Symbian descriptor
		aUsbDes.Set(aUsbDes.Mid(TUsbInterfaceAssociationDescriptor::KSizeInOctets));
		}

	return intAssocDes;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbInterfaceAssociationDescriptor::IsParent(TUsbGenericDescriptor& aPotentialParent)
	{
	switch(aPotentialParent.ibDescriptorType)
		{
	case EConfiguration:
		return ETrue;
	case EOtherSpeedConfiguration:
		return ETrue;	// I think this should be EFalse by my reading of the USB spec - however
						// it is not explicitly clear, so play it safe.
	default:
		return EFalse;
		}
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbInterfaceAssociationDescriptor::IsPeer(TUsbGenericDescriptor& aPotentialPeer)
	{
	switch(aPotentialPeer.ibDescriptorType)
		{
	case EInterfaceAssociation:
		return ETrue;
	case EInterface:
		// Only interfaces are peers of IADs.
			{
			TUsbInterfaceDescriptor* intDesc = TUsbInterfaceDescriptor::Cast(&aPotentialPeer);
			if(intDesc)
				{
				TInt intNum = intDesc->InterfaceNumber();
				intNum -= FirstInterface();
				if(intNum < 0 || intNum >= InterfaceCount())
					{
					// The interface number is outside the IAD region.
					return ETrue;
					}
				}
			return EFalse;
			}
	default:
		return EFalse;
		}
	}
	
/*virtual*/ TBool TUsbInterfaceAssociationDescriptor::IsChild(TUsbGenericDescriptor& aPotentialChild)
	{
	switch(aPotentialChild.ibDescriptorType)
		{
	case EInterface:
		// Only interfaces are children of IADs. And only if they are special.
			{
			TUsbInterfaceDescriptor* intDesc = TUsbInterfaceDescriptor::Cast(&aPotentialChild);
			if(intDesc)
				{
				TInt intNum = intDesc->InterfaceNumber();
				intNum -= FirstInterface();
				if(intNum >= 0 && intNum < InterfaceCount())
					{
					// The interface number is within the IAD region required.
					return ETrue;
					}
				}
			return EFalse;
			}
	default:
		return EFalse;
		}
	}


// -------------------------
// TUsbInterfaceDescriptor
// See section 9.6.5 of the USB 2.0 specification.
// -------------------------

EXPORT_C TUsbInterfaceDescriptor::TUsbInterfaceDescriptor()
	{
	}

EXPORT_C /*static*/ TUsbInterfaceDescriptor* TUsbInterfaceDescriptor::Cast(TUsbGenericDescriptor* aOriginal)
	{
	TUsbInterfaceDescriptor* ret = NULL;
	// Only cast if correctly indentified as interface descriptor
	if(	aOriginal &&
		aOriginal->ibDescriptorType == EInterface &&
		aOriginal->ibLength == TUsbInterfaceDescriptor::KSizeInOctets &&
		aOriginal->iRecognisedAndParsed == ERecognised)
		{
		ret = static_cast<TUsbInterfaceDescriptor*>(aOriginal);
		}
	return ret;
	}

EXPORT_C TUint8 TUsbInterfaceDescriptor::InterfaceNumber() const
	{
	return ParseTUint8(iBlob, EbInterfaceNumber);
	}

EXPORT_C TUint8 TUsbInterfaceDescriptor::AlternateSetting() const
	{
	return ParseTUint8(iBlob, EbAlternateSetting);
	}

EXPORT_C TUint8 TUsbInterfaceDescriptor::NumEndpoints() const
	{
	return ParseTUint8(iBlob, EbNumEndpoints);
	}

EXPORT_C TUint8 TUsbInterfaceDescriptor::InterfaceClass() const
	{
	return ParseTUint8(iBlob, EbInterfaceClass);
	}

EXPORT_C TUint8 TUsbInterfaceDescriptor::InterfaceSubClass() const
	{
	return ParseTUint8(iBlob, EbInterfaceSubClass);
	}

EXPORT_C TUint8 TUsbInterfaceDescriptor::InterfaceProtocol() const
	{
	return ParseTUint8(iBlob, EbInterfaceProtocol);
	}

EXPORT_C TUint8 TUsbInterfaceDescriptor::Interface() const
	{
	return ParseTUint8(iBlob, EiInterface);
	}
	
/**
The parsing routine for interface descriptors.

@internalComponent
*/
/*static*/ TUsbInterfaceDescriptor* TUsbInterfaceDescriptor::ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* /*aPreviousDesc*/)
	{
	TUsbInterfaceDescriptor* intDes = NULL;

	const TInt KMinInterfaceDesDecisionLength = 2;
	if(	aUsbDes.Length() >= KMinInterfaceDesDecisionLength &&
		aUsbDes[KbDescriptorTypeOffset] == EInterface &&
		aUsbDes[KbLengthOffset] == TUsbInterfaceDescriptor::KSizeInOctets)
		{
		// Robustness check - check the length field is valid, and that we have enough data.
		if(aUsbDes.Length() < TUsbInterfaceDescriptor::KSizeInOctets)
			{
			User::Leave(KErrCorrupt);
			}

		// Looks ok to be an interface descriptor.
		intDes = new(ELeave) TUsbInterfaceDescriptor;
		// Set the standard fields
		intDes->ibLength = TUsbInterfaceDescriptor::KSizeInOctets;
		intDes->ibDescriptorType = EInterface;
		// Set the blob appropriately
		intDes->iBlob.Set(aUsbDes.Left(TUsbInterfaceDescriptor::KSizeInOctets));

		intDes->iRecognisedAndParsed = ERecognised;

		// Update the data-left-to-parse Symbian descriptor
		aUsbDes.Set(aUsbDes.Mid(TUsbInterfaceDescriptor::KSizeInOctets));
		}

	return intDes;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbInterfaceDescriptor::IsParent(TUsbGenericDescriptor& aPotentialParent)
	{
	switch(aPotentialParent.ibDescriptorType)
		{
	case EConfiguration:
		return ETrue;
	case EOtherSpeedConfiguration:
		return ETrue;	// I think this should be EFalse by my reading of the USB spec - however
						// it is not explicitly clear, so play it safe.
	// case EInterfaceAssociation:
	// 		We let the IAD descriptor handle the logic of how we bind to it.
	default:
		return EFalse;
		}
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbInterfaceDescriptor::IsPeer(TUsbGenericDescriptor& aPotentialPeer)
	{
	switch(aPotentialPeer.ibDescriptorType)
		{
	//case EInterfaceAssociation:
	//		We let the IAD descriptor handle the logic of how we bind to it.
	case EInterface:
		// If another interface descriptor then it is a peer not child.
		return ETrue;
	default:
		// Any other descriptors are ignored.
		return EFalse;
		}
	}


// ------------------------
// TUsbEndpointDescriptor
// See section 9.6.6 of the USB 2.0 specification.
// ------------------------

EXPORT_C TUsbEndpointDescriptor::TUsbEndpointDescriptor()
	{
	}
	
EXPORT_C /*static*/ TUsbEndpointDescriptor* TUsbEndpointDescriptor::Cast(TUsbGenericDescriptor* aOriginal)
	{
	TUsbEndpointDescriptor* ret = NULL;
	// Only cast if correctly indentified as endpoint descriptor
	if(	aOriginal &&
		aOriginal->ibDescriptorType == EEndpoint &&
		aOriginal->ibLength == TUsbEndpointDescriptor::KSizeInOctets &&
		aOriginal->iRecognisedAndParsed == ERecognised)
		{
		ret = static_cast<TUsbEndpointDescriptor*>(aOriginal);
		}
	return ret;
	}

EXPORT_C TUint8 TUsbEndpointDescriptor::EndpointAddress() const
	{
	return ParseTUint8(iBlob, EbEndpointAddress);
	}

EXPORT_C TUint8 TUsbEndpointDescriptor::Attributes() const
	{
	return ParseTUint8(iBlob, EbmAttributes);
	}

EXPORT_C TUint16 TUsbEndpointDescriptor::MaxPacketSize() const
	{
	return ParseTUint16(iBlob, EwMaxPacketSize);
	}

EXPORT_C TUint8 TUsbEndpointDescriptor::Interval() const
	{
	return ParseTUint8(iBlob, EbInterval);
	}
	
/**
The parsing routine for endpoint descriptors.

@internalComponent
*/
/*static*/ TUsbEndpointDescriptor* TUsbEndpointDescriptor::ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* /*aPreviousDesc*/)
	{
	TUsbEndpointDescriptor* endDes = NULL;

	const TInt KMinEndpointDesDecisionLength = 2;
	if(	aUsbDes.Length() >= KMinEndpointDesDecisionLength &&
		aUsbDes[KbDescriptorTypeOffset] == EEndpoint &&
		aUsbDes[KbLengthOffset] == TUsbEndpointDescriptor::KSizeInOctets)
		{
		// Robustness check - check the length field is valid, and that we have enough data.
		if(aUsbDes.Length() < TUsbEndpointDescriptor::KSizeInOctets)
			{
			User::Leave(KErrCorrupt);
			}

		// Looks ok to be an endpoint descriptor.
		endDes = new(ELeave) TUsbEndpointDescriptor;
		// Set the standard fields
		endDes->ibLength = TUsbEndpointDescriptor::KSizeInOctets;
		endDes->ibDescriptorType = EEndpoint;
		// Set the blob appropriately
		endDes->iBlob.Set(aUsbDes.Left(TUsbEndpointDescriptor::KSizeInOctets));

		endDes->iRecognisedAndParsed = ERecognised;

		// Update the data-left-to-parse Symbian descriptor
		aUsbDes.Set(aUsbDes.Mid(TUsbEndpointDescriptor::KSizeInOctets));
		}

	return endDes;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbEndpointDescriptor::IsParent(TUsbGenericDescriptor& aPotentialParent)
	{
	switch(aPotentialParent.ibDescriptorType)
		{
	case EInterface:
		return ETrue;
	default:
		return EFalse;
		}
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbEndpointDescriptor::IsPeer(TUsbGenericDescriptor& aPotentialPeer)
	{
	switch(aPotentialPeer.ibDescriptorType)
		{
	case EEndpoint:
		return ETrue;
	default:
		return EFalse;
		}
	}

// ------------------------
// TUsbOTGDescriptor
// See section 6.4 of the USB 2.0 On-The-Go Supplement Revision 1.3
// ------------------------

EXPORT_C TUsbOTGDescriptor::TUsbOTGDescriptor()
	{
	}
	
EXPORT_C /*static*/ TUsbOTGDescriptor* TUsbOTGDescriptor::Cast(TUsbGenericDescriptor* aOriginal)
	{
	TUsbOTGDescriptor* ret = NULL;
	// Only cast if correctly indentified as otg descriptor
	if(	aOriginal &&
		aOriginal->ibDescriptorType == EOTG &&
		TUsbOTGDescriptor::IsValidOTGDescriptorLength( aOriginal->ibLength ) &&
		aOriginal->iRecognisedAndParsed == ERecognised)
		{
		ret = static_cast<TUsbOTGDescriptor*>(aOriginal);
		}
	return ret;
	}

EXPORT_C TUint8 TUsbOTGDescriptor::Attributes() const
	{
	return ParseTUint8(iBlob, EbmAttributes);
	}

EXPORT_C TBool TUsbOTGDescriptor::HNPSupported() const
    {
    return (ParseTUint8(iBlob, EbmAttributes) & 0x02) == 0x02;
    }

EXPORT_C TBool TUsbOTGDescriptor::SRPSupported() const
    {
    // Note: an illegal device (see 6.4.2 of the OTG specification) could
    // incorrectly return False for SRP and True for HNP
    // However this function just extracts the bit rather than attempting to
    // fix up a broken device.  Devices broken in this way wouldn't be expected on
    // the TPL.
    return (ParseTUint8(iBlob, EbmAttributes) & 0x01) == 0x01;
    }

EXPORT_C TUint16 TUsbOTGDescriptor::BcdOTG() const
	{
    TUint16 bcdOTG = 0x0000;
    if ( iBlob[EbmLength] > TUsbOTGDescriptor::KSizeInOctets )
	    {
        bcdOTG = ( iBlob[EbcdOTG] ) | ( iBlob[EbcdOTG + 1] << 8 );
	    }
    return bcdOTG;
	}

/**
The parsing routine for OTG descriptors.

@internalComponent
*/
/*static*/ TUsbOTGDescriptor* TUsbOTGDescriptor::ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* /*aPreviousDesc*/)
	{
	TUsbOTGDescriptor* endDes = NULL;

	TUint8 descriptorLength = aUsbDes[KbLengthOffset];
	const TInt KMinOTGDesDecisionLength = 2;
	if(	aUsbDes.Length() >= KMinOTGDesDecisionLength &&
		aUsbDes[KbDescriptorTypeOffset] == EOTG &&
		TUsbOTGDescriptor::IsValidOTGDescriptorLength( descriptorLength ) )
		{
		// Robustness check - check the length field is valid, and that we have enough data.
		if(aUsbDes.Length() < descriptorLength)
			{
			User::Leave(KErrCorrupt);
			}

		// Looks ok to be an OTG descriptor.
		endDes = new(ELeave) TUsbOTGDescriptor;
		// Set the standard fields
		endDes->ibLength = descriptorLength;
		endDes->ibDescriptorType = EOTG;
		// Set the blob appropriately
		endDes->iBlob.Set(aUsbDes.Left(descriptorLength));

		// Null the pointers
		endDes->iFirstChild = NULL;
		endDes->iNextPeer = NULL;
		endDes->iParent = NULL;
		
		endDes->iRecognisedAndParsed = ERecognised;

		// Update the data-left-to-parse Symbian descriptor
		aUsbDes.Set(aUsbDes.Mid(descriptorLength));
		}

	return endDes;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbOTGDescriptor::IsParent(TUsbGenericDescriptor& aPotentialParent)
	{
	switch(aPotentialParent.ibDescriptorType)
		{
	case EConfiguration:    // we are part of a configuration descriptor, or standalone
		return ETrue;
	default:
		return EFalse;
		}
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbOTGDescriptor::IsPeer(TUsbGenericDescriptor& aPotentialPeer)
	{
    switch(aPotentialPeer.ibDescriptorType)
		{
	//case EInterfaceAssociation:
	//		We let the IAD descriptor handle the logic of how we bind to it.
	case EInterface:
		// If another interface descriptor then it is a peer not child.
		return ETrue;
	default:
		// Any other descriptors are ignored.
		return EFalse;
		}
	}

/**
@internalComponent
*/
/*static*/ TBool TUsbOTGDescriptor::IsValidOTGDescriptorLength(TUint8 aLength)
	{
    TBool ret = EFalse;
    const TUint8 KOTG13DescriptorLength = 3; //OTG1.3 
    const TUint8 KOTG20DescriptorLength = 5; //OTG2.0
    if ( ( aLength == KOTG13DescriptorLength ) || ( aLength == KOTG20DescriptorLength ) )
        {
        ret = ETrue;
        }
    return ret;
	}

// ----------------------
// TUsbStringDescriptor
// See section 9.6.7 of the USB 2.0 specification.
// ----------------------

// The length of the header in a string descriptor (i.e. the same as every other standard USB descriptor).
static const TInt KStringDescriptorHeaderFieldLength = 2;

EXPORT_C TUsbStringDescriptor::TUsbStringDescriptor()
	{
	}
	
EXPORT_C /*static*/ TUsbStringDescriptor* TUsbStringDescriptor::Cast(TUsbGenericDescriptor* aOriginal)
	{
	TUsbStringDescriptor* ret = NULL;
	// Only cast if correctly indentified as string descriptor
	if(	aOriginal &&
		aOriginal->ibDescriptorType == EString &&
		aOriginal->ibLength >= KStringDescriptorHeaderFieldLength &&
		aOriginal->iRecognisedAndParsed == ERecognised)
		{
		ret = static_cast<TUsbStringDescriptor*>(aOriginal);
		}
	return ret;
	}

/**
For string descriptor zero, this function allows a means to iterate through the list of supported languages
for strings on this device.

@param aIndex Index into language ID table.
@return The language ID at the requested index, or KErrNotFound if the end of the list has been reached.
Note that the language IDs are unsigned 16-bit numbers, while the return from this function is signed 32-bit.
*/
EXPORT_C TInt TUsbStringDescriptor::GetLangId(TInt aIndex) const
	{
	__ASSERT_ALWAYS(aIndex >= 0, UsbDescPanic(UsbdiPanics::EUsbDescNegativeIndexToLangId));
	const TUint8 KSizeOfLangIdField = 2;

	TInt offset = KStringDescriptorHeaderFieldLength + KSizeOfLangIdField * aIndex;
	if(offset >= ibLength)
		{
		return KErrNotFound;
		}
	return ParseTUint16(iBlob, offset);
	}

/**
Writes the string data into a Symbian descriptor of sufficient size.

@param aString The Symbian descriptor that will have the string data written into it.
*/
EXPORT_C void TUsbStringDescriptor::StringData(TDes16& aString) const
	{
	const TUint8 KUnicodeCharacterWidth = 2;
	aString.Zero();

	TInt index = KStringDescriptorHeaderFieldLength;
	while(index+KUnicodeCharacterWidth <= ibLength)
		{
		aString.Append(ParseTUint16(iBlob, index));
		index += KUnicodeCharacterWidth;
		}
	}


/*static*/ TUsbStringDescriptor* TUsbStringDescriptor::ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* /*aPreviousDesc*/)
	{
	TUsbStringDescriptor* stringDes = NULL;

	if(	aUsbDes.Length() >= KStringDescriptorHeaderFieldLength &&
		aUsbDes[KbDescriptorTypeOffset] == EString)
		{
		TUint8 stringDesLen = aUsbDes[KbLengthOffset];

		// Robustness check - check the length field is valid
		if(aUsbDes.Length() < stringDesLen || stringDesLen < KStringDescriptorHeaderFieldLength)
			{
			User::Leave(KErrCorrupt);
			}
		// Robustness check - check the length is a multiple of two.
		if(stringDesLen % 2 != 0)
			{
			User::Leave(KErrCorrupt);
			}

		// Looks ok to be a string descriptor.
		stringDes = new(ELeave) TUsbStringDescriptor;
		// Set the standard fields
		stringDes->ibLength = stringDesLen;
		stringDes->ibDescriptorType = EString;
		// Set the blob appropriately
		stringDes->iBlob.Set(aUsbDes.Left(stringDesLen));

		stringDes->iRecognisedAndParsed = ERecognised;

		// Update the data-left-to-parse Symbian descriptor
		aUsbDes.Set(aUsbDes.Mid(stringDesLen));
		}

	return stringDes;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbStringDescriptor::IsParent(TUsbGenericDescriptor& /*aPotentialParent*/)
	{
	// String descriptors have no parents - they are standalone.
	return EFalse;
	}

/**
@internalComponent
*/
/*virtual*/ TBool TUsbStringDescriptor::IsPeer(TUsbGenericDescriptor& /*aPotentialPeer*/)
	{
	// String descriptors have no peers - they are standalone.
	return EFalse;
	}

