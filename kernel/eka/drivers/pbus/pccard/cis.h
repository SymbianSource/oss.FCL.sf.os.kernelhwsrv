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
// e32\drivers\pbus\pccard\cis.h
// 
//

// General definitions, applicable to multiple tuple types
const TUint8 KCisTplExt=0x80;		// Extended tuple
const TUint8 KCisTplExponM=0x07; 	// Mask for exponent field.
const TUint8 KCisTplMantM=0x78;  	// Mask for mantisa field.
const TUint8 KCisTplMantFO=3;		// Offset for mantisa field.


// Function Identification Tuple (KCisTplFuncId) - Function codes
const TUint8 KCisTplFuncIdMultiFunc=0x00;
const TUint8 KCisTplFuncIdMemory=0x01;
const TUint8 KCisTplFuncIdSerial=0x02;
const TUint8 KCisTplFuncIdParallel=0x03;
const TUint8 KCisTplFuncIdFixed=0x04;
const TUint8 KCisTplFuncIdVideo=0x05;
const TUint8 KCisTplFuncIdNetwork=0x06;
const TUint8 KCisTplFuncIdAims=0x07;
const TUint8 KCisTplFuncIdScsi=0x08;
const TUint8 KCisTplFuncIdVendorSpecific=0xFE;
const TUint8 KCisTplFuncIdInvalid=0xFF;

// Configuration Tuple (KCisTplConfig)
const TUint8 KTpCcRaszM=0x03;		// Mask for size of TPCC_RADR field.
const TUint8 KTpCcRaszFO=0;			// Offset for size of TPCC_RADR field.
const TUint8 KTpCcRmszM=0x3C;		// Mask for size of TPCC_RMSK field.
const TUint8 KTpCcRmszFO=2;			// Offset for size of TPCC_RMSK field.

// Configuration Table Entry Tuple (KCisTplCfTableEntry)
const TUint8 KTpCeOptionM=0x3F;		// Mask for Config. Option field.
const TUint8 KTpCeIsDefaultM=0x40; 	// Mask for Is Default field.
const TUint8 KTpCeIntfPresM=0x80; 	// Mask for interface present field.
const TUint8 KTpCeIntfTypeM=0x0F; 	// Mask for interface type field.

const TUint8 KTpCeBvdM=0x10;	   	// Mask for BVD active field
const TUint8 KTpCeWpM=0x20;			// Mask for WP active field
const TUint8 KTpCeReadyM=0x40;		// Mask for READY active field
const TUint8 KTpCeWaitM=0x80;		// Mask for WAIT active field

const TUint8 KTpCePwrPresM=0x03;  	// Mask for Power present field.
const TUint8 KTpCePwrPresFO=0;		// Offset for Power present field.
const TUint8 KTpCeTimPresM=0x04;  	// Mask for Timing present field.
const TUint8 KTpCeTimPresFO=2;		// Offset for Timing present field.
const TUint8 KTpCeIoPresM=0x08;   	// Mask for IO space present field.
const TUint8 KTpCeIoPresFO=3;		// Offset for IO space present field.
const TUint8 KTpCeIrqPresM=0x10;  	// Mask for IRQ present field.
const TUint8 KTpCeIrqPresFO=4;		// Offset for IRQ present field.
const TUint8 KTpCeMemPresM=0x60;  	// Mask for Mem space present field.
const TUint8 KTpCeMemPresFO=5;		// Offset for Mem space present field.
const TUint8 KTpCeMiscPresM=0x80; 	// Mask for Misc present field.
const TUint8 KTpCeMiscPresFO=7;		// Offset for Misc present field.

const TUint8 KTpCeTimWaitM=0x03;  	// Mask for wait timing field.
const TUint8 KTpCeTimWaitFO=0;		// Offset for wait timing field.
const TUint8 KTpCeTimRdyM=0x1C;   	// Mask for ready timing field.
const TUint8 KTpCeTimRdyFO=2;		// Offset for ready timing field.
const TUint8 KTpCeTimResM=0xE0;		// Mask for reserved timing field.
const TUint8 KTpCeTimResFO=5;		// Offset for reserved timing field.

const TUint8 KTpCeIoLinesM=0x1F;   	// Mask for IOAddrLines field.
const TUint8 KTpCeIoLinesFO=0;		// Offset for IOAddrLines field.
const TUint8 KTpCeBus16_8M=0x60;   	// Mask for Bus16/8 field.
const TUint8 KTpCeBus16_8FO=5;		// Offset for Bus16/8 field.
const TUint8 KTpCeRangePresM=0x80;	// Mask for IO range field.
const TUint8 KTpCeRangePresFO=7;  	// Offset for IO range field.
const TUint8 KTpCeIoRangesM=0x0F;  	// Mask for number of IO ranges.
const TUint8 KTpCeIoRangesFO=0;		// Offset for number of IO ranges.
const TUint8 KTpCeIoAddrSzM=0x30; 	// Mask for size of IO addr.
const TUint8 KTpCeIoAddrSzFO=4;   	// Offset for size of IO addr.
const TUint8 KTpCeIoAddrLenM=0xC0;	// Mask for size of IO len.
const TUint8 KTpCeIoAddrLenFO=6;  	// Offset for size of IO len.

const TUint8 KTpCeIrqMaskM=0x10;   	// Mask for mask field in IRQ info.

const TUint8 KTpCeMemWindowsM=0x07; // Mask for number of memory windows.
const TUint8 KTpCeMemLenSzM=0x18;	// Mask for size of each window len.
const TUint8 KTpCeMemLenSzFO=3;  	// Offset for size of each window len.
const TUint8 KTpCeMemAddrSzM=0x60; 	// Mask for size of each window addr.
const TUint8 KTpCeMemAddrSzFO=5;   	// Offset for size of each window addr.
const TUint8 KTpCeMemHostAddrM=0x80;// Mask for host address field

const TUint8 KTpCePwrDownM=0x20;   	// Mask for power down in Misc info.

// Device Information Tuples (KCisTplDevice,KCisTplDeviceA)
const TUint8 KTpDiDSpeedM=0x07;		// Mask for Device Speed field.
const TUint8 KTpDiDSpeedNull=0;
const TUint8 KTpDiDSpeed250nS=1;
const TUint8 KTpDiDSpeed200nS=2;
const TUint8 KTpDiDSpeed150nS=3;
const TUint8 KTpDiDSpeed100nS=4;
const TUint8 KTpDiDSpeedExt=7;

const TUint8 KTpDiWpsM=0x08;		// Mask for Write Prot. field.
const TUint8 KTpDiDTypeM=0xF0;		// Mask for Device Type field.
const TUint8 KTpDiDTypeFO=4;		// Offset for Device Type field.
const TUint8 KTpDiDTypeNull=0;
const TUint8 KTpDiDTypeRom=1;
const TUint8 KTpDiDTypeOtprom=2;
const TUint8 KTpDiDTypeEprom=3;
const TUint8 KTpDiDTypeEeprom=4;
const TUint8 KTpDiDTypeFlash=5;
const TUint8 KTpDiDTypeSram=6;
const TUint8 KTpDiDTypeDram=7;
const TUint8 KTpDiDTypeFuncSpec=13;
const TUint8 KTpDiDTypeExtend=14;

const TUint8 KTpDiDSizeM=0x07;		// Mask for Size Code field.
const TUint8 KTpDiDSizeFO=0;		// Offset for Size Code field.
const TUint8 KTpDiDUnitsM=0xF8;		// Mask for Num of Units field.
const TUint8 KTpDiDUnitsFO=3;		// Offset for Num of Units field.
const TUint8 KTpDiDSize512=0;
const TUint8 KTpDiDSize2K=1;
const TUint8 KTpDiDSize8K=2;
const TUint8 KTpDiDSize32K=3;
const TUint8 KTpDiDSize128K=4;
const TUint8 KTpDiDSize512K=5;
const TUint8 KTpDiDSize2M=6;

// Other Conditions Device Info. Tuples (KCisTplDeviceOC,KCisTplDeviceOA)
const TUint8 KTpDoMWaitM=0x01;		// Mask for MWAIT field.
const TUint8 KTpDoVccUsedM=0x06;	// Mask for VccUsed field.
const TUint8 KTpDoVccUsedFO=1;		// Offset for VccUsed field.

const TInt KMaxTuplesPerCis=256;

