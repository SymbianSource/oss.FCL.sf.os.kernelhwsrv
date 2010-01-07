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
// e32\include\partitions.h
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __PARTITIONS_H__
#define __PARTITIONS_H__
#include <e32def.h>

const TUint KPartitionTypeEmpty=0x00;
const TUint KPartitionTypeFAT12=0x01;
const TUint KPartitionTypeXENIXroot=0x02;
const TUint KPartitionTypeXENIXusr=0x03;
const TUint KPartitionTypeFAT16small=0x04;
const TUint KPartitionTypeExtended=0x05;
const TUint KPartitionTypeFAT16=0x06;
const TUint KPartitionTypeNTFS=0x07;
const TUint KPartitionTypeAIX=0x08;
const TUint KPartitionTypeAIXboot=0x09;
const TUint KPartitionTypeOS2BootManager=0x0a;
const TUint KPartitionTypeWin95FAT32=0x0b;
const TUint KPartitionTypeWin95FAT32LBA=0x0c;
const TUint KPartitionTypeWin95FAT16LBA=0x0e;
const TUint KPartitionTypeWin95ExtdLBA=0x0f;
const TUint KPartitionTypeOPUS=0x10;
const TUint KPartitionTypeHiddenFAT12=0x11;
const TUint KPartitionTypeCompaqDiag=0x12;
const TUint KPartitionTypeHiddenFAT16small=0x14;
const TUint KPartitionTypeHiddenFAT16=0x16;
const TUint KPartitionTypeHiddenNTFS=0x17;
const TUint KPartitionTypeASTSmartSleep=0x18;
const TUint KPartitionTypeHiddenWin95FAT32=0x1b;
const TUint KPartitionTypeHiddenWin95FAT32LBA=0x1c;
const TUint KPartitionTypeHiddenWin95FAT16LBA=0x1e;
const TUint KPartitionTypeNECDOS=0x24;
const TUint KPartitionTypePlan9=0x39;
const TUint KPartitionTypePartitionMagic=0x3c;
const TUint KPartitionTypeVenix80286=0x40;
const TUint KPartitionTypePPCPRePBoot=0x41;
const TUint KPartitionTypeSFS=0x42;
const TUint KPartitionTypeQNX4x=0x4d;
const TUint KPartitionTypeQNX4x_2=0x4e;
const TUint KPartitionTypeQNX4x_3=0x4f;
const TUint KPartitionTypeOnTrackDM=0x50;
const TUint KPartitionTypeOnTrackDM6Aux=0x51;
const TUint KPartitionTypeCPM=0x52;
const TUint KPartitionTypeOnTrackDM6Aux_2=0x53;
const TUint KPartitionTypeOnTrackDM6=0x54;
const TUint KPartitionTypeEZDrive=0x55;
const TUint KPartitionTypeGoldenBow=0x56;
const TUint KPartitionTypePriamEdisk=0x5c;
const TUint KPartitionTypeSpeedStor=0x61;
const TUint KPartitionTypeGNU_HURD=0x63;
const TUint KPartitionTypeNovellNetware=0x64;
const TUint KPartitionTypeNovellNetware_2=0x65;
const TUint KPartitionTypeDiskSecure=0x70;
const TUint KPartitionTypePCIX=0x75;
const TUint KPartitionTypeOldMinix=0x80;
const TUint KPartitionTypeMinixOldLinux=0x81;
const TUint KPartitionTypeLinuxSwap=0x82;
const TUint KPartitionTypeLinux=0x83;
const TUint KPartitionTypeOS2Hidden=0x84;
const TUint KPartitionTypeLinuxExtended=0x85;
const TUint KPartitionTypeNTFSvolset=0x86;
const TUint KPartitionTypeNTFSvolset_2=0x87;
const TUint KPartitionTypeLinuxLVM=0x8e;
const TUint KPartitionTypeAmoeba=0x93;
const TUint KPartitionTypeAmoebaBBT=0x94;
const TUint KPartitionTypeBSD_OS=0x9f;
const TUint KPartitionTypeIBMThinkpad=0xa0;
const TUint KPartitionTypeFreeBSD=0xa5;
const TUint KPartitionTypeOpenBSD=0xa6;
const TUint KPartitionTypeNeXTSTEP=0xa7;
const TUint KPartitionTypeNetBSD=0xa9;
const TUint KPartitionTypeBSDIfs=0xb7;
const TUint KPartitionTypeBSDIswap=0xb8;
const TUint KPartitionTypeBootWizardHidden=0xbb;
const TUint KPartitionTypeDRDOS=0xc1;
const TUint KPartitionTypeDRDOS_2=0xc4;
const TUint KPartitionTypeDRDOS_3=0xc6;
const TUint KPartitionTypeSyrinx=0xc7;
const TUint KPartitionTypeNonFSData=0xda;
const TUint KPartitionTypeCPM_CTOS=0xdb;
const TUint KPartitionTypeDellUtility=0xde;
const TUint KPartitionTypeBootIt=0xdf;
const TUint KPartitionTypeDOSaccess=0xe1;
const TUint KPartitionTypeDOS_RO=0xe3;
const TUint KPartitionTypeSymbianCrashLog=0xf0;
const TUint KPartitionTypeSpeedStor_2=0xf1;
const TUint KPartitionTypeDOSsecondary=0xf2;
const TUint KPartitionTypeSpeedStor_3=0xf4;
const TUint	KPartitionTypePagedData=0xf8;		// Symbian defined 
const TUint	KPartitionTypeROM=0xf9;				// Symbian defined 
const TUint	KPartitionTypeRofs=0xfa;			// Symbian defined 
const TUint KPartitionTypeIso9660=0xfb;			// Symbian defined 
const TUint KPartitionTypeEneaLFFS=0xfc;		// Symbian defined 
const TUint KPartitionTypeLinuxRaidAuto=0xfd;
const TUint KPartitionTypeLANStep=0xfe;
const TUint KPartitionTypeBBT=0xff;

inline TBool PartitionIsFAT(TUint a)
	{
	return (
		a==KPartitionTypeFAT12						||
		a==KPartitionTypeFAT16small					||
		a==KPartitionTypeFAT16						||
		a==KPartitionTypeFAT16						||
		a==KPartitionTypeWin95FAT16LBA				||
		a==KPartitionTypeHiddenFAT12				||
		a==KPartitionTypeHiddenFAT16small			||
		a==KPartitionTypeHiddenFAT16				||
		a==KPartitionTypeHiddenWin95FAT16LBA
		);
	}

inline TBool PartitionIsFAT32(TUint a)
	{
	return (
		a==KPartitionTypeWin95FAT32					||
		a==KPartitionTypeWin95FAT32LBA				||
		a==KPartitionTypeHiddenWin95FAT32			||
		a==KPartitionTypeHiddenWin95FAT32LBA
		);
	}

inline TBool PartitionIsNTFS(TUint a)
	{
	return (
		a==KPartitionTypeNTFS						||
		a==KPartitionTypeHiddenNTFS
		);
	}

//
// Entry in MBR partition table
//
const TUint KBootIndicatorBootable=0x80;

class TMBRPartitionEntry
	{
public:
	TBool IsValidPartition()
		{ return (iNumSectors>0 && iPartitionType!=KPartitionTypeEmpty); }
	TBool IsValidDosPartition()
		{ return (iNumSectors>0 && PartitionIsFAT(iPartitionType)); }
	TBool IsDefaultBootPartition()
		{ return(iX86BootIndicator==KBootIndicatorBootable && (IsValidDosPartition() || IsValidFAT32Partition())); }
	TBool IsValidFAT32Partition()
		{ return (iNumSectors>0 && PartitionIsFAT32(iPartitionType)); }
	TBool IsValidExFATPartition()
		{ return (iNumSectors>0 && PartitionIsNTFS(iPartitionType)); }
public:
	TUint8 iX86BootIndicator;
	TUint8 iStartHead;
	TUint8 iStartSector;
	TUint8 iStartCylinder;
	TUint8 iPartitionType;
	TUint8 iEndHead;
	TUint8 iEndSector;
	TUint8 iEndCylinder;
	TUint32 iFirstSector;
	TUint32 iNumSectors;
	};

//
// Defines for Master boot record
//
const TUint KMBRFirstPartitionOffset=0x1BE;
const TUint KMBRSignatureOffset=0x1FE;
const TUint KMBRSignature=0xAA55;
const TInt KMBRMaxPrimaryPartitions=4;

class TMasterBootRecord
	{
public:
	TUint8 iBootCode[KMBRFirstPartitionOffset];
	TMBRPartitionEntry iPartitionEntry[KMBRMaxPrimaryPartitions];
	TUint16 iSignature;
	};



#endif

