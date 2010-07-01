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

#include "usbdescutils.h"

/**
@file
@internalComponent
*/

/*static*/ CUsbCustomDescriptorParserList* CUsbCustomDescriptorParserList::NewL()
	{
    CUsbCustomDescriptorParserList* self = new(ELeave) CUsbCustomDescriptorParserList;
    return self;
	}

CUsbCustomDescriptorParserList::~CUsbCustomDescriptorParserList()
	{
	iParserList.Close();
	}

void CUsbCustomDescriptorParserList::RegisterParserL(UsbDescriptorParser::TUsbDescriptorParserL aParserFunc)
	{
	iParserList.AppendL(aParserFunc);
	}

void CUsbCustomDescriptorParserList::UnregisterParser(UsbDescriptorParser::TUsbDescriptorParserL aParserFunc)
	{
	TInt res = iParserList.Find(aParserFunc);
	if(res != KErrNotFound)
		{
		iParserList.Remove(res);
		}
	}

TInt CUsbCustomDescriptorParserList::NumOfRegisteredParsers() const
	{
	return iParserList.Count();
	}
	
UsbDescriptorParser::TUsbDescriptorParserL CUsbCustomDescriptorParserList::RegisteredParser(TInt aIndex) const
	{
	return iParserList[aIndex];
	}

