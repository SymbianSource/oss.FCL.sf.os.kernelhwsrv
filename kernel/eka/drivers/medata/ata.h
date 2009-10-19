// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\medata\ata.h
// 
//

#include <partitions.h>

#if defined (SELECT_PRIMARY_IO_CONFIG)
//
// ATA Register addresses (for primary)
// 
const TUint KAtaDataRdWr16=0x000001F0;
const TUint KAtaDataRdWr8=0x000001F0;

const TUint KAtaErrorRd8=0x000001F1;
const TUint KAtaFeaturesWr8=0x000001F1;

const TUint KAtaSectorCountRdWr8=0x000001F2;

const TUint KAtaSectorNoRdWr8=0x000001F3;
const TUint KAtaLba7_0RdWr8=0x000001F3;

const TUint KAtaCylinderLowRdWr8=0x000001F4;
const TUint KAtaLba15_8RdWr8=0x000001F4;

const TUint KAtaCylinderHighRdWr8=0x000001F5;
const TUint KAtaLba23_16RdWr8=0x000001F5;

const TUint KAtaSelectDriveHeadRdWr8=0x000001F6;
const TUint KAtaDriveLba27_24RdWr8=0x000001F6;

const TUint KAtaStatusRd8=0x000001F7;
const TUint KAtaCommandWr8=0x000001F7;

#else
//
// ATA Register addresses (for either contiguous I/O or memory mapped)
// 
const TUint KAtaDataRdWr16=0x00000000;
const TUint KAtaDataRdWr8=0x00000000;
const TUint KAtaDataRdWrWinBase16=0x00000400;	// Memory mapped only
const TUint KAtaDataRdWrWinBase8=0x00000400;	// Memory mapped only

const TUint KAtaErrorRd8=0x00000001;
const TUint KAtaFeaturesWr8=0x00000001;

const TUint KAtaSectorCountRdWr8=0x00000002;

const TUint KAtaSectorNoRdWr8=0x00000003;
const TUint KAtaLba7_0RdWr8=0x00000003;

const TUint KAtaCylinderLowRdWr8=0x00000004;
const TUint KAtaLba15_8RdWr8=0x00000004;

const TUint KAtaCylinderHighRdWr8=0x00000005;
const TUint KAtaLba23_16RdWr8=0x00000005;

const TUint KAtaSelectDriveHeadRdWr8=0x00000006;
const TUint KAtaDriveLba27_24RdWr8=0x00000006;

const TUint KAtaStatusRd8=0x00000007;
const TUint KAtaCommandWr8=0x00000007;
#endif

//
// ATA Register addresses (for contiguous I/O or memory mapped)
// 
const TUint KAtaDupEvenDataRdWr8=0x00000008;
const TUint KAtaDupOddDataRdWr8=0x00000009;
const TUint KAtaDupErrorRd8=0x0000000D;
const TUint KAtaDupFeaturesWr8=0x0000000D;
const TUint KAtaAltStatusRd8=0x0000000E;
const TUint KAtaDeviceCtlWr8=0x0000000E;
const TUint KAtaDriveAddressRd8=0x0000000F;

//
// ATA Register bit fields 
// 
// KAtaErrorRd8
const TUint8	KAtaErrorAmnf=0x01;
const TUint8	KAtaErrorAbort=0x04;
const TUint8	KAtaErrorIdnf=0x10;
const TUint8	KAtaErrorUnc=0x40;
const TUint8	KAtaErrorBbk=0x80;

// KAtaSelectDriveHeadRdWr8
// KAtaDriveLba27_24RdWr8
const TUint8	KAtaDrvHeadLba27_24=0x0F;
const TUint8	KAtaDrvHeadDrive1=0x10;
const TUint8	KAtaDrvHeadLbaOn=0x40;

// KAtaStatusRd8
const TUint8	KAtaStatusErr=0x01;
const TUint8	KAtaStatusCorr=0x04;
const TUint8	KAtaStatusDrq=0x08;
const TUint8	KAtaStatusDsc=0x10;
const TUint8	KAtaStatusDwf=0x20;
const TUint8	KAtaStatusRdy=0x40;
const TUint8	KAtaStatusBusy=0x80;

// KAtaDeviceCtlWr8
const TUint8	KAtaDeviceCtlIntDis=0x02;
const TUint8	KAtaDeviceCtlSwRes=0x04;

//
// ATA commands
// 
const TUint8 KAtaCmdIdentifyDrive=0xEC;
const TUint8 KAtaCmdReadSectors=0x20;
const TUint8 KAtaCmdWriteSectors=0x30;
const TUint8 KAtaCmdSetFeatures=0xEF;
const TUint8 KAtaCmdRequestSense=0x03;
const TUint8 KAtaCmdFormatTrack=0x50;
const TUint8 KAtaCmdIdle=0xE3;

//
// Defines for Identify Drive Command
//
const TInt KAtaIdDefaultCylinders=2;			// Offset in bytes
const TInt KAtaIdDefaultHeads=6;			
const TInt KAtaIdDefaultSectorsPerTrack=12;			
const TInt KAtaIdCapabilities=98;			
const TUint16	KAtaIdCapLbaSupported=0x0200;
const TInt KAtaIdTranslationParams=106;		
const TUint16 	KAtaIdTrParamsValid=0x0001;		
const TInt KAtaIdCurrentCylinders=108;		
const TInt KAtaIdCurrentHeads=110;			
const TInt KAtaIdCurrentSectorsPerTrack=112;			
const TInt KAtaIdCurrentTotalSectors=114;			
const TInt KAtaIdTotalSectorsInLba=120;		

//
// Drive parameters
//
class TDriveParameters
	{
public:
	TInt iCylinders;
	TInt iHeads;
	TInt iSectorsPerTrack;
	TInt iSectorsPerCylinder;
	TBool iSupportsLba;
	TInt iTotalSectorsInLba;
	};

const TInt KAtaSectorSize=512;
const TInt KAtaSectorSizeMinusOne=(KAtaSectorSize-1);
const TInt KAtaSectorShift=9;
const TUint KAtaSectorMask=0xFFFFFE00;

const TInt KMaxSectorsPerCmd=256; // Maximum sectors that can be transfered in single command

enum TAtaDriveSelect
	{
	ESelectDrive1=KAtaDrvHeadDrive1,
	ESelectDrive0=0x00
	};

