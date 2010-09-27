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
// Contributors:
//
// Description:
//

#ifndef D32USBDESCRIPTORS_H
#define D32USBDESCRIPTORS_H

#include <e32base.h>


/*****************************************************************************/
/*                                                                           */
/* USB descriptors parser framework                                          */
/*                                                                           */
/*****************************************************************************/

class TUsbGenericDescriptor;

/**
The Symbian USB Descriptor Parsing Framework class.

This class is to aid users of USBDI by providing the facilities to parse the
raw descriptor data into wrapper classes that allow access to the fields in 
the descriptor bodies, and pointers to map the serial data blob into the tree 
structure that descriptors logically have.

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(UsbDescriptorParser)
	{
public:
	typedef TUsbGenericDescriptor* (*TUsbDescriptorParserL)(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);

public:
	// Main parse function.
	IMPORT_C static TInt Parse(const TDesC8& aUsbDes, TUsbGenericDescriptor*& aDesc);

	// Custom parsing framework.
	IMPORT_C static void RegisterCustomParserL(TUsbDescriptorParserL aParserFunc);
	IMPORT_C static void UnregisterCustomParser(TUsbDescriptorParserL aParserFunc);

private:
	static TUsbGenericDescriptor* FindParserAndParseAndCheckL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	static TUsbGenericDescriptor* FindParserAndParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	static void ParseDescriptorTreeL(TPtrC8& aUsbDes, TUsbGenericDescriptor& aPreviousDesc);
	static void BuildTreeL(TUsbGenericDescriptor& aNewDesc, TUsbGenericDescriptor& aPreviousDesc);
	static TUsbGenericDescriptor& FindSuitableParentL(TUsbGenericDescriptor& aNewDesc, TUsbGenericDescriptor& aTopParent);
	static TUsbGenericDescriptor* UnknownUsbDescriptorParserL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	};


/*****************************************************************************/
/*                                                                           */
/* USB standard descriptors                                                  */
/*                                                                           */
/*****************************************************************************/


/**
Base class for USB descriptors.
All USB descriptors contain type and length, and may have peers and children.

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
class TUsbGenericDescriptor
	{
public:
	IMPORT_C TUsbGenericDescriptor();
	
	IMPORT_C void DestroyTree();

	IMPORT_C TUint8 TUint8At(TInt aOffset) const;
	IMPORT_C TUint16 TUint16At(TInt aOffset) const;
	IMPORT_C TUint32 TUint32At(TInt aOffset) const;

	IMPORT_C TUsbGenericDescriptor& operator=(const TUsbGenericDescriptor& aDescriptor);

	/**
	Helper function to allow TUsbGenericDescriptor types to be placed on the cleanup stack.
	*/
	inline operator TCleanupItem() { return TCleanupItem(Cleanup,this); }

public:
	virtual TBool IsParent(TUsbGenericDescriptor& aPotentialParent);
	virtual TBool IsPeer(TUsbGenericDescriptor& aPotentialPeer);
	virtual TBool IsChild(TUsbGenericDescriptor& aPotentialChild);

private:
	IMPORT_C static void Cleanup(TAny* aPtr);
	static void WalkAndDelete(TUsbGenericDescriptor* aDesc);

public:		// USB standard fields
	/**
	The offset in a standard USB descriptor to the bLength field.
	*/
	static const TInt KbLengthOffset = 0;

	/**
	The offset in a standard USB descriptor to the bDescriptorType field.
	*/
	static const TInt KbDescriptorTypeOffset = 1;

	/**
	Standard Length field.
	*/
	TUint8	ibLength;
	
	/**
	Standard Type field.
	*/
	TUint8	ibDescriptorType;
	
public:
	/**
	The flag to indicate whether the USB descriptor has been recognised
	and parsed.
	*/
	enum TUsbGenericDescriptorFlags
		{
		EUnrecognised = 0x00,
		ERecognised = 0x01,
		};
	
public:		// Symbian generated fields
	/**
	Flag to show if the descriptor has been recognised and parsed, or if its data can only be represented as a
	binary blob.  This field should particularly be checked if writing code which may run on older versions of
	the operating system, where a (now) known descriptor may not have been parsed, or before parsing a new
	descriptor from a blob, where later versions of the operating system may have already extracted the fields.
	*/
	TUint8	iRecognisedAndParsed;

	/**
	A pointer to the next peer of this descriptor, or NULL.
	As an example, an endpoint descriptor will contain pointers to any other endpoint descriptors on the same
	interface.
	*/
	TUsbGenericDescriptor* iNextPeer;

	/**
	A pointer to the first child of this descriptor, or NULL.
	As an example, an interface descriptor will contain a pointer to the first endpoint descriptor on the
	interface. The iNextPeer member can then be used to examine other endpoints on the interface.
	*/
	TUsbGenericDescriptor* iFirstChild;
	
	/**
	A pointer to the parent to this descriptor, or NULL.
	As an example an endpoint descriptor from a configuration bundle will have the interface that it
	is a member of as it's parent.
	*/
	TUsbGenericDescriptor* iParent;

	/**
	The binary blob that contains this descriptor
	*/
	TPtrC8			iBlob;
	};

enum TUsbDescriptorType
	{
	EDevice						= 1,
	EConfiguration				= 2,
	EString						= 3,
	EInterface					= 4,
	EEndpoint					= 5,
	EDeviceQualifier			= 6,
	EOtherSpeedConfiguration	= 7,
	EInterfacePower				= 8,
	EOTG						= 9,
	EDebug						= 10,
	EInterfaceAssociation		= 11,
	};

/**
Device descriptor.

See section 9.6.1 of the USB 2.0 specification.

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(TUsbDeviceDescriptor) : public TUsbGenericDescriptor
	{
public:
	IMPORT_C TUsbDeviceDescriptor();
	IMPORT_C static TUsbDeviceDescriptor* Cast(TUsbGenericDescriptor* aOriginal);

public:
	static const TInt KSizeInOctets = 18;
	enum TFieldOffsets
		{
		EbcdUSB				= 2,
		EbDeviceClass		= 4,
		EbDeviceSubClass	= 5,
		EbDeviceProtocol	= 6,
		EbMaxPacketSize0	= 7,
		EidVendor			= 8,
		EidProduct			= 10,
		EbcdDevice			= 12,
		EiManufacturer		= 14,
		EiProduct			= 15,
		EiSerialNumber		= 16,
		EbNumConfigurations	= 17
		};

public:
	IMPORT_C TUint16 USBBcd() const;
	IMPORT_C TUint8 DeviceClass() const;
	IMPORT_C TUint8 DeviceSubClass() const;
	IMPORT_C TUint8 DeviceProtocol() const;
	IMPORT_C TUint8 MaxPacketSize0() const;
	IMPORT_C TUint16 VendorId() const;
	IMPORT_C TUint16 ProductId() const;
	IMPORT_C TUint16 DeviceBcd() const;
	IMPORT_C TUint8 ManufacturerIndex() const;
	IMPORT_C TUint8 ProductIndex() const;
	IMPORT_C TUint8 SerialNumberIndex() const;
	IMPORT_C TUint8 NumConfigurations() const;

public:
	static TUsbDeviceDescriptor* ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	virtual TBool IsParent(TUsbGenericDescriptor& aPotentialParent);
	virtual TBool IsPeer(TUsbGenericDescriptor& aPotentialPeer);
	};


/**
Device Qualifier descriptor.

See section 9.6.2 of the USB 2.0 specification.

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(TUsbDeviceQualifierDescriptor) : public TUsbGenericDescriptor
	{
public:
	IMPORT_C TUsbDeviceQualifierDescriptor();
	IMPORT_C static TUsbDeviceQualifierDescriptor* Cast(TUsbGenericDescriptor* aOriginal);
	
public:
	static const TInt KSizeInOctets = 10;
	enum TFieldOffsets
		{
		EbcdUSB				= 2,
		EbDeviceClass		= 4,
		EbDeviceSubClass	= 5,
		EbDeviceProtocol	= 6,
		EbMaxPacketSize0	= 7,
		EbNumConfigurations	= 8,
		EbReserved			= 9
		};

public:
	IMPORT_C TUint16 USBBcd() const;
	IMPORT_C TUint8 DeviceClass() const;
	IMPORT_C TUint8 DeviceSubClass() const;
	IMPORT_C TUint8 DeviceProtocol() const;
	IMPORT_C TUint8 MaxPacketSize0() const;
	IMPORT_C TUint8 NumConfigurations() const;
	IMPORT_C TUint8 Reserved() const;
	
public:
	static TUsbDeviceQualifierDescriptor* ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	virtual TBool IsParent(TUsbGenericDescriptor& aPotentialParent);
	virtual TBool IsPeer(TUsbGenericDescriptor& aPotentialPeer);
	};


/**
Configuration descriptor.

See section 9.6.3 of the USB 2.0 specification.

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(TUsbConfigurationDescriptor) : public TUsbGenericDescriptor
	{
public:
	IMPORT_C TUsbConfigurationDescriptor();
	IMPORT_C static TUsbConfigurationDescriptor* Cast(TUsbGenericDescriptor* aOriginal);

public:
	static const TInt KSizeInOctets = 9;
	enum TFieldOffsets
		{
		EwTotalLength			= 2,
		EbNumInterfaces			= 4,
		EbConfigurationValue	= 5,
		EiConfiguration			= 6,
		EbmAttributes			= 7,
		EbMaxPower				= 8
		};

public:
	IMPORT_C TUint16 TotalLength() const;
	IMPORT_C TUint8 NumInterfaces() const;
	IMPORT_C TUint8 ConfigurationValue() const;
	IMPORT_C TUint8 ConfigurationIndex() const;
	IMPORT_C TUint8 Attributes() const;
	IMPORT_C TUint8 MaxPower() const;

public:
	static TUsbConfigurationDescriptor* ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	virtual TBool IsParent(TUsbGenericDescriptor& aPotentialParent);
	virtual TBool IsPeer(TUsbGenericDescriptor& aPotentialPeer);
	};


/**
Other Speed descriptor.

See section 9.6.4 of the USB 2.0 specification.

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(TUsbOtherSpeedDescriptor) : public TUsbGenericDescriptor
	{
public:
	IMPORT_C TUsbOtherSpeedDescriptor();
	IMPORT_C static TUsbOtherSpeedDescriptor* Cast(TUsbGenericDescriptor* aOriginal);

public:
	static const TInt KSizeInOctets = 9;
	enum TFieldOffsets
		{
		EwTotalLength 			= 2,
		EbNumInterfaces			= 4,
		EbConfigurationValue	= 5,
		EiConfiguration			= 6,
		EbmAttributes			= 7,
		EbMaxPower				= 8
		};

public:
	IMPORT_C TUint16 TotalLength() const;
	IMPORT_C TUint8 NumInterfaces() const;
	IMPORT_C TUint8 ConfigurationValue() const;
	IMPORT_C TUint8 ConfigurationIndex() const;
	IMPORT_C TUint8 Attributes() const;
	IMPORT_C TUint8 MaxPower() const;
	
public:
	static TUsbOtherSpeedDescriptor* ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	virtual TBool IsParent(TUsbGenericDescriptor& aPotentialParent);
	virtual TBool IsPeer(TUsbGenericDescriptor& aPotentialPeer);
	};


/**
Interface Association Descriptor

See the USB IAD ECN.

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(TUsbInterfaceAssociationDescriptor) : public TUsbGenericDescriptor
	{
public:
	IMPORT_C TUsbInterfaceAssociationDescriptor();
	IMPORT_C static TUsbInterfaceAssociationDescriptor* Cast(TUsbGenericDescriptor* aOriginal);

public:
	static const TInt KSizeInOctets = 8;
	enum TFieldOffsets
		{
		EbFirstInterface	= 2,
		EbInterfaceCount	= 3,
		EbFunctionClass		= 4,
		EbFunctionSubClass	= 5,
		EbFunctionProtocol	= 6,
		EiFunction			= 7
		};
	
public:
	IMPORT_C TUint8 FirstInterface() const;
	IMPORT_C TUint8 InterfaceCount() const;
	IMPORT_C TUint8 FunctionClass() const;
	IMPORT_C TUint8 FunctionSubClass() const;
	IMPORT_C TUint8 FunctionProtocol() const;
	IMPORT_C TUint8 FunctionIndex() const;
	
public:
	static TUsbInterfaceAssociationDescriptor* ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	virtual TBool IsParent(TUsbGenericDescriptor& aPotentialParent);
	virtual TBool IsPeer(TUsbGenericDescriptor& aPotentialPeer);
	virtual TBool IsChild(TUsbGenericDescriptor& aPotentialChild);
	};

/**
Interface descriptor.

See section 9.6.5 of the USB 2.0 specification.

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(TUsbInterfaceDescriptor) : public TUsbGenericDescriptor
	{
public:
	IMPORT_C TUsbInterfaceDescriptor();
	IMPORT_C static TUsbInterfaceDescriptor* Cast(TUsbGenericDescriptor* aOriginal);

public:
	static const TInt KSizeInOctets = 9;
	enum TFieldOffsets
		{
		EbInterfaceNumber	= 2,
		EbAlternateSetting	= 3,
		EbNumEndpoints		= 4,
		EbInterfaceClass	= 5,
		EbInterfaceSubClass	= 6,
		EbInterfaceProtocol	= 7,
		EiInterface			= 8
		};
	
public:
	IMPORT_C TUint8 InterfaceNumber() const;
	IMPORT_C TUint8 AlternateSetting() const;
	IMPORT_C TUint8 NumEndpoints() const;
	IMPORT_C TUint8 InterfaceClass() const;
	IMPORT_C TUint8 InterfaceSubClass() const;
	IMPORT_C TUint8 InterfaceProtocol() const;
	IMPORT_C TUint8 Interface() const;
	
public:
	static TUsbInterfaceDescriptor* ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	virtual TBool IsParent(TUsbGenericDescriptor& aPotentialParent);
	virtual TBool IsPeer(TUsbGenericDescriptor& aPotentialPeer);
	};

/**
Endpoint descriptor.

See section 9.6.6 of the USB 2.0 specification.
Note these exclude support support for:
'Standard AC Interrupt Endpoint Descriptor'
'Standard AS Isochronous Synch Endpoint Descriptor'
'Standard AS Isochronous Audio Data Endpoint Descriptor'
as defined in USB Audio Device Class Spec v1.0 which are all 9 bytes in size.
To support these custom descriptors may be registered with the
parser.

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(TUsbEndpointDescriptor) : public TUsbGenericDescriptor
	{
public:
	IMPORT_C TUsbEndpointDescriptor();
	IMPORT_C static TUsbEndpointDescriptor* Cast(TUsbGenericDescriptor* aOriginal);

public:
	static const TInt KSizeInOctets = 7;
	enum TFieldOffsets
		{
		EbEndpointAddress	= 2,
		EbmAttributes		= 3,
		EwMaxPacketSize		= 4,
		EbInterval			= 6
		};

public:
	IMPORT_C TUint8 EndpointAddress() const;
	IMPORT_C TUint8 Attributes() const;
	IMPORT_C TUint16 MaxPacketSize() const;
	IMPORT_C TUint8 Interval() const;
	
public:
	static TUsbEndpointDescriptor* ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	virtual TBool IsParent(TUsbGenericDescriptor& aPotentialParent);
	virtual TBool IsPeer(TUsbGenericDescriptor& aPotentialPeer);
	};

/**
String descriptor

See section 9.6.7 of the USB 2.0 specification.

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(TUsbStringDescriptor) : public TUsbGenericDescriptor
	{
public:
	IMPORT_C TUsbStringDescriptor();
	IMPORT_C static TUsbStringDescriptor* Cast(TUsbGenericDescriptor* aOriginal);

public:
	IMPORT_C TInt GetLangId(TInt aIndex) const;
	IMPORT_C void StringData(TDes16& aString) const;

public:
	static TUsbStringDescriptor* ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	virtual TBool IsParent(TUsbGenericDescriptor& aPotentialParent);
	virtual TBool IsPeer(TUsbGenericDescriptor& aPotentialPeer);
	};

/**
OTG descriptor.

See section 6.4 of the USB 2.0 On-The-Go Supplement Revision 1.3

@publishedPartner Intended to be available to 3rd parties later
@prototype
*/
NONSHARABLE_CLASS(TUsbOTGDescriptor) : public TUsbGenericDescriptor
	{
public:
	IMPORT_C TUsbOTGDescriptor();
	IMPORT_C static TUsbOTGDescriptor* Cast(TUsbGenericDescriptor* aOriginal);

public:
	static const TInt KSizeInOctets = 3; //OTG1.3 otg descriptor length
	enum TFieldOffsets
		{
	  EbmLength       = 0,
		EbmAttributes		= 2,
		EbcdOTG         = 3
		};

public:
	IMPORT_C TUint8 Attributes() const;
    IMPORT_C TBool HNPSupported() const;
    IMPORT_C TBool SRPSupported() const;
    IMPORT_C TUint16 BcdOTG() const;
public:
	static TUsbOTGDescriptor* ParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc);
	virtual TBool IsParent(TUsbGenericDescriptor& aPotentialParent);
	virtual TBool IsPeer(TUsbGenericDescriptor& aPotentialPeer);
	static TBool IsValidOTGDescriptorLength(TUint8 aLength);
	};


#endif	// D32USBDESCRIPTORS_H
