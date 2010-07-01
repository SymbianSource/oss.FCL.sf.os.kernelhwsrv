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
// Symbian USBDI Descriptor Parsing Framework.
// 
//

/**
 @file
 @internalComponent
*/

#include <d32usbdescriptors.h>
#include "usbdescutils.h"


// ---------------------
// UsbDescriptorParser
// ---------------------

/**
The main parsing function of the USB descriptor parsing framework.

This will perform a best effort parse of a USB descriptor tree.  It is best effort in the
fact that upon encountering a form of syntatic corruption in the source data it will error
the parse attempt, but also return the incomplete descriptor tree up to the parsing error.

@param aUsbDes The source data that will be parsed.
@param aDesc The pointer that will be updated to the top-level descriptor.

@return KErrNone if successful, a system-wide error code otherwise.

@publishedPartner
@prototype
*/
EXPORT_C /*static*/ TInt UsbDescriptorParser::Parse(const TDesC8& aUsbDes, TUsbGenericDescriptor*& aDesc)
	{
	TInt ret = KErrNone;
	aDesc = NULL;
	TPtrC8 des(aUsbDes);

	// First we must find the top level descriptor (the one we will return to the caller).
	TRAP(ret, aDesc = FindParserAndParseAndCheckL(des, NULL));
	if(ret == KErrNone)
		{
		if(!aDesc)
			{
			ret = KErrNotFound;
			}
		else
			{
			// Now we have a top level descriptor - we now try to build up the descriptor
			// tree if there are more descriptors available.
			TRAP(ret, ParseDescriptorTreeL(des, *aDesc));
			}
		}

	// Ensure that all the data has been parsed if successful.
	if(ret == KErrNone && des.Length() > 0)
		{
		// If no parser was found for some data then we should have been errored with KErrNotFound.
		__ASSERT_DEBUG(EFalse, UsbDescFault(UsbdiFaults::EUsbDescSuccessButDataLeftUnparsed));
		ret = KErrUnknown;
		}

	// release the allocated descriptor if there was an error
	if(ret != KErrNone && aDesc)
		{
		delete aDesc;
		aDesc = NULL;
		}

	return ret;
	}

/**
The function to register a custom parsing routine in the USB descriptor parser framework.

The routine is registered locally to the current thread, and so if an application wishes
to perform the same custom parsing in multiple threads, it must call this function with
the appropriate routine in each thread context.

If the custom routine becomes unapplicable after being registered, the application may 
unregister it using the UsbDescriptorParser::UnregisterCustomParser function.
@see UsbDescriptorParser::UnregisterCustomParser

@param aParserFunc The routine which will be added to the USB descriptor parsing framework.

@publishedPartner
@prototype
*/
EXPORT_C /*static*/ void UsbDescriptorParser::RegisterCustomParserL(TUsbDescriptorParserL aParserFunc)
	{
	TBool newlyCreatedList = EFalse;
	CUsbCustomDescriptorParserList* parserList = static_cast<CUsbCustomDescriptorParserList*>(Dll::Tls());
	if(!parserList)
		{
		parserList = CUsbCustomDescriptorParserList::NewL();
		newlyCreatedList = ETrue;
		CleanupStack::PushL(parserList);
		}
	
	parserList->RegisterParserL(aParserFunc);

	if(newlyCreatedList)
		{
		Dll::SetTls(parserList);
		CleanupStack::Pop(parserList);
		}
	}

/**
The function to unregister a custom parsing routine in the USB descriptor parser framework.

This routine will only unregister the routine from the current thread context.  If the routine
is registered in multiple threads and it is no longer wanted in any thread, an application 
must call this function in each thread context that the routine is registered.

It is safe to call this function even if RegisterCustomParserL has never been called successfully.

@see UsbDescriptorParser::RegisterCustomParserL

@param aParserFunc The routine which will be removed from the USB descriptor parsing framework.

@publishedPartner
@prototype
*/
EXPORT_C /*static*/ void UsbDescriptorParser::UnregisterCustomParser(TUsbDescriptorParserL aParserFunc)
	{
	CUsbCustomDescriptorParserList* parserList = static_cast<CUsbCustomDescriptorParserList*>(Dll::Tls());
	if(parserList)
		{
		parserList->UnregisterParser(aParserFunc);
		if(parserList->NumOfRegisteredParsers() <= 0)
			{
			Dll::FreeTls();
			delete parserList;
			}
		}
	}

/*static*/ TUsbGenericDescriptor* UsbDescriptorParser::FindParserAndParseAndCheckL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc)
	{
	TUsbGenericDescriptor* ret = FindParserAndParseL(aUsbDes, aPreviousDesc);
	// We need to ensure that the parsers have correctly initialised the USB descriptor objects.
	// It is important that we check as it is possible that a custom parser did the parsing.
	__ASSERT_ALWAYS(!ret || (!ret->iParent && !ret->iFirstChild && !ret->iNextPeer),
		UsbDescPanic(UsbdiPanics::EUsbDescNonNullPointersAfterParsing));
	return ret;
	}

// Utility macro to tidy up the parsing routine.
#define RETURN_IF_PARSEDL(aRet, aParserL, aUsbDes, aPreviousDesc)\
	{\
	aRet = aParserL(aUsbDes, aPreviousDesc);\
	if(aRet)\
		{\
		return aRet;\
		}\
	}

/*static*/ TUsbGenericDescriptor* UsbDescriptorParser::FindParserAndParseL(TPtrC8& aUsbDes, TUsbGenericDescriptor* aPreviousDesc)
	{
	// Special termination case.
	if(aUsbDes.Length() == 0)
		{
		return NULL;
		}

	TUsbGenericDescriptor* des;

	// Try the default parsing routines.
	RETURN_IF_PARSEDL(des, TUsbDeviceDescriptor::ParseL, aUsbDes, aPreviousDesc);
	RETURN_IF_PARSEDL(des, TUsbDeviceQualifierDescriptor::ParseL, aUsbDes, aPreviousDesc);
	RETURN_IF_PARSEDL(des, TUsbConfigurationDescriptor::ParseL, aUsbDes, aPreviousDesc);
	RETURN_IF_PARSEDL(des, TUsbOtherSpeedDescriptor::ParseL, aUsbDes, aPreviousDesc);
	RETURN_IF_PARSEDL(des, TUsbInterfaceAssociationDescriptor::ParseL, aUsbDes, aPreviousDesc);
	RETURN_IF_PARSEDL(des, TUsbInterfaceDescriptor::ParseL, aUsbDes, aPreviousDesc);
	RETURN_IF_PARSEDL(des, TUsbEndpointDescriptor::ParseL, aUsbDes, aPreviousDesc);
	RETURN_IF_PARSEDL(des, TUsbOTGDescriptor::ParseL, aUsbDes, aPreviousDesc);
	RETURN_IF_PARSEDL(des, TUsbStringDescriptor::ParseL, aUsbDes, aPreviousDesc);

	// Then we try the custom parsers that have been registered.
	const CUsbCustomDescriptorParserList* parserList = static_cast<const CUsbCustomDescriptorParserList*>(Dll::Tls());
	if(parserList)
		{
		TInt numOfParsers = parserList->NumOfRegisteredParsers()-1;
		for(TInt index=0; index<numOfParsers; ++index)
			{
			TUsbDescriptorParserL parserL = parserList->RegisteredParser(index);
			RETURN_IF_PARSEDL(des, parserL, aUsbDes, aPreviousDesc);
			}
		}

	// Then we try the unknown descriptor parser.
	RETURN_IF_PARSEDL(des, UnknownUsbDescriptorParserL, aUsbDes, aPreviousDesc);

	// Otherwise we haven't found anybody to parse the binary data.
	User::Leave(KErrNotFound); // inform caller that there is no parser for the data.
	return NULL;
	}
	
/*static*/ void UsbDescriptorParser::ParseDescriptorTreeL(TPtrC8& aUsbDes, TUsbGenericDescriptor& aPreviousDesc)
	{
	TUsbGenericDescriptor* desc = &aPreviousDesc;
	while(desc)
		{
		TUsbGenericDescriptor* preDesc = desc;
		desc = FindParserAndParseAndCheckL(aUsbDes, desc);
		if(desc)
			{
			CleanupStack::PushL(desc);
			BuildTreeL(*desc, *preDesc);
			CleanupStack::Pop(desc);
			}
		}
	}

/*static*/ void UsbDescriptorParser::BuildTreeL(TUsbGenericDescriptor& aNewDesc, TUsbGenericDescriptor& aPreviousDesc)
	{
	// We assume that the new descriptor has been properly initialised with NULL pointers.
	__ASSERT_DEBUG(!aNewDesc.iFirstChild && !aNewDesc.iNextPeer && !aNewDesc.iParent,
		UsbDescFault(UsbdiFaults::EUsbDescTreePointersAlreadySet));

	// Find first "top" parent claiming this new descriptor as a child.
	TUsbGenericDescriptor* parent = &aPreviousDesc;
	TUsbGenericDescriptor* topLevel = &aPreviousDesc;
	while(parent)
		{
		if(aNewDesc.IsParent(*parent) || parent->IsChild(aNewDesc))
			{
			break; // we have found a parent.
			}
		topLevel = parent; // Save the current one for use if we cannot find a parent
		parent = parent->iParent; // Scroll back up the tree.
		}
	__ASSERT_DEBUG(topLevel, UsbDescFault(UsbdiFaults::EUsbDescNoTopLevelDescriptorFound));

	if(parent)
		{
		// We should be able to place the descriptor directly as a child of this descriptor,
		// however it is not that simple because of IADs (Interface Association Descriptors).
		// The ECN states "All of the interface numbers in the set of associated interfaces must be
		// contiguous" meaning that if an IAD has two interfaces starting at 1 then the configuration
		// bundle may have interface descriptors in '1 then 3 then 2' order. As such we need to be able
		// to go backwards to find the most suitable binding.  The general way for doing this is to
		// find the right-most, lowest descriptor that descriptor considers a parent.
        // Where the tree is arranged with peers horizontally linked left to
        // right, with children linked vertically top to bottom.
		TUsbGenericDescriptor& suitableParent = FindSuitableParentL(aNewDesc, *parent);

		TUsbGenericDescriptor* peer = suitableParent.iFirstChild;
		if(peer)
			{
			TUsbGenericDescriptor* lastPeer;
			do
				{
				lastPeer = peer;
				peer = peer->iNextPeer;
				}
			while(peer);
			lastPeer->iNextPeer = &aNewDesc;
			}
		else
			{
			// we are the first child so just update.
			suitableParent.iFirstChild = &aNewDesc;
			}
		aNewDesc.iParent = &suitableParent;
		}
	else if(aNewDesc.IsPeer(*topLevel) || topLevel->IsPeer(aNewDesc))
		{
		// There is no explicit parent in the tree so, we may just have a group of top-level peers
		// in the bundle.  If the previous descriptor is a peer then we shall just tag on its tier.
		TUsbGenericDescriptor* lastPeer;
		TUsbGenericDescriptor* peer = topLevel;
		do
			{
			lastPeer = peer;
			peer = peer->iNextPeer;
			}
		while(peer);
		lastPeer->iNextPeer = &aNewDesc;
		}
	else
		{
		// The descriptor could not be bound into the tree, indicating that the bundle of descriptors
		// is unvalid.
		User::Leave(KErrUsbBadDescriptorTopology);
		}
	}
	
/*static*/ TUsbGenericDescriptor& UsbDescriptorParser::FindSuitableParentL(TUsbGenericDescriptor& aNewDesc, TUsbGenericDescriptor& aTopParent)
	{
	// This implements the algorithm to search down from the top parent found in the tree to the right most, lowest descriptor
	// that will accept the new descriptor as a child.

	TUsbGenericDescriptor* bestMatch = &aTopParent;

	TUsbGenericDescriptor* desc = aTopParent.iFirstChild;
	if(desc)
		{
		// Do a depth first search.
		FOREVER
			{
			// First see if the descriptor is suitable.
			__ASSERT_DEBUG(desc, UsbDescFault(UsbdiFaults::EUsbDescRunOffTree));
			if(aNewDesc.IsParent(*desc) || desc->IsChild(aNewDesc))
				{
				bestMatch = desc;
				}
			// Now walk to the next point in the tree.
			if(desc->iFirstChild)
				{
				desc = desc->iFirstChild;
				}
			else if(desc->iNextPeer)
				{
				desc = desc->iNextPeer;
				}
			else
				{
				// We've run to the end of a bottom tier, so go back up.
				do
					{
					__ASSERT_DEBUG(desc->iParent, UsbDescFault(UsbdiFaults::EUsbDescTreeMemberHasNoParent));
					desc = desc->iParent;
					}
				while(!desc->iNextPeer && desc != &aTopParent);
				if(desc == &aTopParent)
					{
					// This means that we must have got back to the original
					// parent.  So we don't do any more.
					break;
					}
				desc = desc->iNextPeer;
				}
			}
		}
	return *bestMatch;
	}

/*static*/ TUsbGenericDescriptor* UsbDescriptorParser::UnknownUsbDescriptorParserL(TPtrC8& aUsbDes, TUsbGenericDescriptor* /*aPreviousDesc*/)
	{
	TUsbGenericDescriptor* unknownDes = NULL;

	const TInt KMinUnknownDesLength = 2; // Length and type fields
	if(	aUsbDes.Length() >= KMinUnknownDesLength)
		{
		// We require unknown descriptors to have at least the length and type fields.
		// Any more exotic descriptors should have a custom parser for the framework to use.
		TUint8 unknownDesLen = aUsbDes[TUsbGenericDescriptor::KbLengthOffset];

		// Robustness check - check the length field is valid.
		if(aUsbDes.Length() < unknownDesLen || unknownDesLen < KMinUnknownDesLength)
			{
			User::Leave(KErrCorrupt);
			}

		unknownDes = new(ELeave) TUsbGenericDescriptor;
		// Set the standard fields
		unknownDes->ibLength = unknownDesLen;
		unknownDes->ibDescriptorType = aUsbDes[TUsbGenericDescriptor::KbDescriptorTypeOffset] ;
		// Set the blob appropriately
		unknownDes->iBlob.Set(aUsbDes.Left(unknownDesLen));
		// Update the data-left-to-parse Symbian descriptor
		aUsbDes.Set(aUsbDes.Mid(unknownDesLen));
		}

	return unknownDes;
	}
