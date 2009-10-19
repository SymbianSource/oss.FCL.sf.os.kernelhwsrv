// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32utils\setcap\setcap.cpp
// 
//

#include "setcap.h"

#include <e32std.h>
#include <e32std_private.h>

GLDEF_D TBool CapabilitySet;
GLDEF_D SCapabilitySet Capability;
GLDEF_D TBool SecureIdSet;
GLDEF_D TSecureId SecureId;
GLDEF_D TBool VendorIdSet;
GLDEF_D TVendorId VendorId;

#ifdef __TOOLS__
#define		Mem		HMem
#include "h_utl.h"
#endif //__TOOLS__

#ifndef __EPOC32__

GLDEF_C TInt SetCap(HANDLE hFile)
	{
	DWORD ret;
	TText8 checkedUidBuf[sizeof(TCheckedUid)];
	ReadFile(hFile,checkedUidBuf,sizeof(TCheckedUid),&ret,NULL);
	if (ret!=sizeof(TCheckedUid))
		goto close;

	//Look at PE file for UID section
	{
	const TInt KPeHeaderAddrAddr=0x3c;
	const TInt KPeHeaderAddrSize=0x01;
	const TInt KNumberOfSectionsOffset=0x06;
	const TInt KNumberOfSectionsSize=0x02;
	const TInt KSectionTableOffset=0xf8;
	const TInt KSectionHeaderSize=0x28;
	const TInt KSectionNameLength=0x08;
	const TInt KPtrToRawDataOffset=0x14;
	const TInt KPtrToRawDataSize=0x04;
	const TText8 peText[4]={'P','E',0,0};
	const TText8 uidText[8]={'.','S','Y','M','B','I','A','N'};
		
	//Read address of start of PE header
	if (SetFilePointer(hFile,KPeHeaderAddrAddr,0,FILE_BEGIN)==0xFFFFFFFF)
		goto close;
	TInt peAddr=0;
	ReadFile(hFile,&peAddr,KPeHeaderAddrSize,&ret,NULL);
	if (ret!=KPeHeaderAddrSize)
		goto close;
		
	//Check it really is the start of PE header
	if (SetFilePointer(hFile,peAddr,0,FILE_BEGIN)==0xFFFFFFFF)
		goto close;
	TText8 text[4];
	ReadFile(hFile,text,4,&ret,NULL);
	if (*(TInt32*)text!=*(TInt32*)peText)
		goto close;
		
	//Read number of sections
	if (SetFilePointer(hFile,peAddr+KNumberOfSectionsOffset,0,FILE_BEGIN)==0xFFFFFFFF)
		goto close;
	TInt sections=0;
	ReadFile(hFile,&sections,KNumberOfSectionsSize,&ret,NULL);
	if (ret!=KNumberOfSectionsSize)
		goto close;

	//Go through section headers looking for UID section
	if (SetFilePointer(hFile,peAddr+KSectionTableOffset,0,FILE_BEGIN)==0xFFFFFFFF)
		goto close;
	TInt i=0;
	for(;i<sections;i++)
		{
		TText8 name[KSectionNameLength];
		ReadFile(hFile,name,KSectionNameLength,&ret,NULL);
		if (ret!=KSectionNameLength)
			goto close;
		if (*(TInt64*)name==*(TInt64*)uidText)
			break;
		if (SetFilePointer(hFile,KSectionHeaderSize-KSectionNameLength,0,FILE_CURRENT)==0xFFFFFFFF)
			goto close;
		}
	if (i==sections)
		goto close;

	//Read RVA/Offset
	if (SetFilePointer(hFile,KPtrToRawDataOffset-KSectionNameLength,0,FILE_CURRENT)==0xFFFFFFFF)
		goto close;
	TInt uidOffset;
	ReadFile(hFile,&uidOffset,KPtrToRawDataSize,&ret,NULL);
	if (ret!=KPtrToRawDataSize)
		goto close;

	//SYMBIAN Header
	if (SetFilePointer(hFile,uidOffset,0,FILE_BEGIN)==0xFFFFFFFF)
		goto close;

	TEmulatorImageHeader header;

	ReadFile(hFile,&header,sizeof(header),&ret,NULL);
	if (ret!=sizeof(header))
		goto close;

	// set new capabilities
	if (CapabilitySet)
		{
		header.iS.iCaps = Capability;
		}

	// set new secure id and vendor id if specified
	if (SecureIdSet)
		{
		header.iS.iSecureId = SecureId.iId;
		}
	if (VendorIdSet)
		{
		header.iS.iVendorId = VendorId.iId;
		}

	// save header values
	Capability = header.iS.iCaps;
	SecureId.iId = header.iS.iSecureId;
	VendorId.iId = header.iS.iVendorId;

	if (SetFilePointer(hFile,uidOffset,0,FILE_BEGIN)==0xFFFFFFFF)
		goto close;

	BOOL b = WriteFile(hFile,&header,sizeof(header),&ret,NULL);
	if (b==FALSE)
		goto close;

	CloseHandle(hFile);
	return KErrNone;
	}
close:
	CloseHandle(hFile);
	return KErrGeneral;
	}

#endif //__EPOC32__

#ifndef __WINS__

GLDEF_C TInt SetCap(E32ImageHeader* h)
	{
	TUint hdrfmt = h->HeaderFormat();
	if (hdrfmt < KImageHdrFmt_V)
		return KErrNotSupported;
	E32ImageHeaderV* v = (E32ImageHeaderV*)h;
	if (CapabilitySet)
		v->iS.iCaps = Capability;
	if (SecureIdSet)
		v->iS.iSecureId = SecureId.iId;
	if (VendorIdSet)
		v->iS.iVendorId = VendorId.iId;	

	// save header values
	Capability = v->iS.iCaps;
	SecureId.iId = v->iS.iSecureId;
	VendorId.iId = v->iS.iVendorId;

	// regenerate CRC
	v->iHeaderCrc = KImageCrcInitialiser;
	TUint32 crc = 0;
	Mem::Crc32(crc, v, v->TotalSize());
	v->iHeaderCrc = crc;
	return KErrNone;
	}

#endif //__WINS__
