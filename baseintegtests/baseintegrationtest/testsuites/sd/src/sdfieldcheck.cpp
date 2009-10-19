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
// Contributors:
//
// Description:
// Ensure the File System complies with the SD specification
// 
//

#include "sdfieldcheck.h"

const TUint32 KSectorSize = 512;

/*
Class constructor

@param None
@return None
*/
CBaseTestSDFieldCheck::CBaseTestSDFieldCheck(CBaseTestSDServer& aOurServer) : iServer(aOurServer)
	{
	SetTestStepName(KTestStepFieldCheck);
	}

/*
Test Step Preamble
 - Load device driver for direct disk access

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDFieldCheck::doTestStepPreambleL()
	{
	SetTestStepResult(EFail);
	
	if (!InitDeviceDriver())
		return TestStepResult();

	SetTestStepResult(EPass);
	return TestStepResult();
	}
	
/*
Test step

@param None
@return EPass if successful or EFail if not
@see TVerdict
*/
TVerdict CBaseTestSDFieldCheck::doTestStepL()
	{
	if (TestStepResult() == EPass)
		{
		TInt checktype = 0;
		_LIT(KFieldCheckType, "FieldCheckType");
		if (!GetIntFromConfig(ConfigSection(), KFieldCheckType, checktype))
			{
			ERR_PRINTF1(_L("INI file read error"));
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		switch (checktype)
			{
			case 1:
				FS1();
				break;
			case 2:
				if (iServer.iSharedData.iFsType < 0x0b)
				// FAT12/16
					{
					FS2();
					}
				else
				// FAT32
					{
					FS2Fat32();
					}
				break;
			case 3:
				if (iServer.iSharedData.iFsType < 0x0b)
					{
					FS3();
					}
				else
					{
					INFO_PRINTF1(_L("Test step skipped: not a FAT12/16 partition"));
					}
				break;
			case 4:
				if (iServer.iSharedData.iFsType < 0x0b)
					{
					FS4();
					}
				else
					{
					INFO_PRINTF1(_L("Test step skipped: not a FAT12/16 partition"));
					}
				break;
			case 5:
				if (iServer.iSharedData.iFsType < 0x0b)
					{
					FS5();
					}
				else
					{
					INFO_PRINTF1(_L("Test step skipped: not a FAT12/16 partition"));
					}
				break;
			case 6:
				if (iServer.iSharedData.iFsType >= 0x0b)
					{
					FSInfo();
					}
				else
					{
					INFO_PRINTF1(_L("Test step skipped: not a FAT32 partition"));
					}
				break;
			case 7:
				if (iServer.iSharedData.iFsType >= 0x0b)
					{
					FSBackupSectors();
					}
				else
					{
					INFO_PRINTF1(_L("Test step skipped: not a FAT32 partition"));
					}
				break;
			default:
				ERR_PRINTF2(_L("Invalid check type value: %d"), checktype);
				SetTestStepResult(EFail);
				return TestStepResult();
			}
		}
	else
		{
		INFO_PRINTF1(_L("Test preamble did not complete succesfully - Test Step skipped"));
		}
	return TestStepResult();
	}

	
/*
Field Check of MBR and Partition Table

@param None
@return None
*/
void CBaseTestSDFieldCheck::FS1()
	{
	INFO_PRINTF1(_L("FS1 - Field Check of MBR and Partition Table"));
	TBuf8<KSectorSize> sector;
	
	if (ReadSector(0, sector) != KErrNone)
		{
		SetTestStepResult(EFail);
		return;
		}
	
	// See section 3.1.3. Arrangement of the Partition Area (FAT12/16)
	// and section 4.1.3. Arrangement of the Partition Area (FAT32)
	// of the SD Specification for details
	
	// Also see SD Test Specification for File Systems
	
	TInt relativeSector = SYMBIAN_TEST_LE4(sector[446 + 8], sector[446 + 9], sector[446 + 10], sector[446 + 11]);
	
	// Save values for later use
	iServer.iSharedData.iTotalSector = SYMBIAN_TEST_LE4(sector[446 + 12], sector[446 + 13], sector[446 + 14], sector[446 + 15]);
	iServer.iSharedData.iPartitionBootSector = relativeSector;
	
	TInt noHeads = 0;
	if (iCardSizeInSectors <= 32768) { noHeads = 2; }			// <=16MB
	else if (iCardSizeInSectors <= 65536) { noHeads = 4; }		// <=32MB
	else if (iCardSizeInSectors <= 262144) { noHeads = 8; }		// <=128MB
	else if (iCardSizeInSectors <= 1032192) { noHeads = 16; }	// <=504MB
	else if (iCardSizeInSectors <= 2064384) { noHeads = 32; }	// <=1008MB
	else if (iCardSizeInSectors <= 4128768) { noHeads = 64; }	// <=2016MB
	else if (iCardSizeInSectors <= 8257536) { noHeads = 128; }	// <=4032GB
	else { noHeads = 255; }
	
	TInt sectorsPerTrack = 0;
	if (iCardSizeInSectors <= 4096) { sectorsPerTrack = 16; }		// <=2MB
	else if (iCardSizeInSectors <= 524288) { sectorsPerTrack = 32; }	// <=256MB
	else { sectorsPerTrack = 63; }
	
	TInt expected;
	
	// FS1-1	
	INFO_PRINTF2(_L("FS 1-1 Boot Indicator: %02xh"), sector[446]);
	SYMBIAN_TEST_TESTNOPANIC(sector[446] == 0x00 || sector[446] == 0x80);
	
	// FS1-2
	if (relativeSector <= 16450560) // FAT12/16
		{
		expected = (relativeSector % (noHeads * sectorsPerTrack)) / sectorsPerTrack;
		}
	else
		{
		expected = 0xfe;
		}
	INFO_PRINTF3(_L("FS 1-2 Starting Head: %02xh (expected: %02xh)"), sector[446 + 1], expected);
	SYMBIAN_TEST_TESTNOPANIC(sector[446 + 1] == expected);
	
	// FS1-3
	if (relativeSector <= 16450560) // FAT12/16
		{
		expected = relativeSector % sectorsPerTrack + 1;
		}
	else
		{
		expected = 0x3f;
		}
	INFO_PRINTF3(_L("FS 1-3 Starting Sector: %02xh (expected %02xh)"), sector[446 + 2] & 0x3f, expected);
	SYMBIAN_TEST_TESTNOPANIC((sector[446 + 2] & 0x3f) == expected);
	
	// FS1-4
	if (relativeSector <= 16450560) // FAT12/16
		{
		expected = relativeSector / (noHeads * sectorsPerTrack);
		}
	else
		{
		expected = 0x3ff;
		}
	INFO_PRINTF3(_L("FS 1-4 Starting Cylinder: %03xh (expected %03xh) "), ((sector[446 + 2] & 0xc0) << 2) + sector[446 + 3], expected);
	SYMBIAN_TEST_TESTNOPANIC(((sector[446 + 2] & 0xc0) << 2) + sector[446 + 3] == expected);
	
	// FS1-5	
	INFO_PRINTF2(_L("FS 1-5 System ID: %02xh"), sector[446 + 4]);
	if (iServer.iSharedData.iTotalSector < 32680)
		{
		SYMBIAN_TEST_TESTNOPANIC(sector[446 + 4] == 0x01);
		}
	else if (iServer.iSharedData.iTotalSector < 65536)
		{
		SYMBIAN_TEST_TESTNOPANIC(sector[446 + 4] == 0x04);
		}
	else if (iServer.iSharedData.iTotalSector <= 4194304) // FAT16 (<=2048MB)
		{
		SYMBIAN_TEST_TESTNOPANIC(sector[446 + 4] == 0x06);
		}
	else if (iServer.iSharedData.iTotalSector + relativeSector <= 16450560) // FAT32 and ending location of partition doesn't exceed 8032.5MB
		{
		SYMBIAN_TEST_TESTNOPANIC(sector[446 + 4] == 0x0b);
		}
	else
		{
		SYMBIAN_TEST_TESTNOPANIC(sector[446 + 4] == 0x0c);
		}
	iServer.iSharedData.iFsType = sector[446 + 4];
	
	// FS1-6
	if (iServer.iSharedData.iTotalSector + relativeSector <= 16450560) // FAT12/16
		{
		expected = ((relativeSector + iServer.iSharedData.iTotalSector - 1) % (noHeads * sectorsPerTrack)) / sectorsPerTrack;
		}
	else
		{
		expected = 0xfe;
		}
	INFO_PRINTF3(_L("FS 1-6 Ending Head: %02xh (expected: %02xh)"), sector[446 + 5], expected);
	SYMBIAN_TEST_TESTNOPANIC(sector[446 + 5] == expected);
	
	// FS1-7
	if (iServer.iSharedData.iTotalSector + relativeSector <= 16450560) // FAT12/16
		{
		expected = (relativeSector + iServer.iSharedData.iTotalSector - 1) % sectorsPerTrack + 1;
		}
	else
		{
		expected = 0x3f;
		}
	INFO_PRINTF3(_L("FS 1-7 Ending Sector: %02xh (expected %02xh)"), sector[446 + 6] & 0x3f, expected);
	SYMBIAN_TEST_TESTNOPANIC((sector[446 + 6] & 0x3f) == expected);
	
	// FS1-8
	if (iServer.iSharedData.iTotalSector + relativeSector <= 16450560) // FAT12/16
		{
		expected = (relativeSector + iServer.iSharedData.iTotalSector - 1) / (noHeads * sectorsPerTrack);
		}
	else
		{
		expected = 0x3ff;
		}
	INFO_PRINTF3(_L("FS 1-8 Ending Cylinder: %03xh (expected %03xh)"), ((sector[446 + 6] & 0xc0) << 2) + sector[446 + 7], expected);
	SYMBIAN_TEST_TESTNOPANIC(((sector[446 + 6] & 0xc0) << 2) + sector[446 + 7] == expected);
	
	// FS1-9
	INFO_PRINTF2(_L("FS 1-9 Relative Sector: %08xh"), relativeSector);
	SYMBIAN_TEST_TESTNOPANIC(relativeSector < iCardSizeInSectors);
	
	// FS1-10
	INFO_PRINTF2(_L("FS 1-10 Total Sector: %08xh"), iServer.iSharedData.iTotalSector);
	SYMBIAN_TEST_TESTNOPANIC(iServer.iSharedData.iTotalSector <= iCardSizeInSectors);
	
	// FS1-11
	INFO_PRINTF1(_L("FS 1-11 Partition Table 2"));
	for (TInt i = 0; i < 16; i++)
		{
		SYMBIAN_TEST_TESTNOPANIC(sector[462 + i] == 0);
		}
	
	// FS1-12
	INFO_PRINTF1(_L("FS 1-12 Partition Table 3"));
	for (TInt i = 0; i < 16; i++)
		{
		SYMBIAN_TEST_TESTNOPANIC(sector[478 + i] == 0);
		}
	
	// FS1-13
	INFO_PRINTF1(_L("FS 1-13 Partition Table 4"));
	for (TInt i = 0; i < 16; i++)
		{
		SYMBIAN_TEST_TESTNOPANIC(sector[494 + i] == 0);
		}
	
	// FS1-15
	INFO_PRINTF3(_L("FS 1-14 Signature Word %02xh %02xh"), sector[510], sector[511]);
	SYMBIAN_TEST_TESTNOPANIC(sector[510] == 0x55);
	SYMBIAN_TEST_TESTNOPANIC(sector[511] == 0xaa);
	
	for (TInt i = 0; i < KSectorSize / 16; i++)
		{
		TBuf<60> buffer;
		buffer.Format(_L("%08x:"), i * 16);
		for (TInt j = 0; j < 16; j++)
			{
			buffer.AppendFormat(_L(" %02x"), sector[i * 16 + j]);
			}
		INFO_PRINTF2(_L("%S"), &buffer);
		}
	}

/*
Field Check of Partition Boot Sector (FAT12/16)

@param None
@return None
*/
void CBaseTestSDFieldCheck::FS2()
	{
	INFO_PRINTF1(_L("FS2 - Field Check of Partition Boot Sector"));
	// See section 3.2.1. Partition Boot Sector (FAT12/16)
	// of the SD Specification for details
	
	// Also see SD Test Specification for File Systems
	TBuf8<KSectorSize> sector;
	
	if (ReadSector(iServer.iSharedData.iPartitionBootSector, sector) != KErrNone)
		{
		SetTestStepResult(EFail);
		return;
		}
	
	TInt sectorsPerCluster = 0;
	if (iCardSizeInSectors <= 16384) { sectorsPerCluster = 16; }			// <=8MB
	else if (iCardSizeInSectors <= 2097152) { sectorsPerCluster = 32; }	// <=1024MB
	else { sectorsPerCluster = 64; }
	
	iServer.iSharedData.iNumberOfClusters = (iServer.iSharedData.iTotalSector - (1 + SYMBIAN_TEST_LE2(sector[22], sector[23]) * 2 + (KSectorSize * 32 / KSectorSize))) / sectorsPerCluster;
	
	if (sector[38] != 0x29) // FDC
		{
		// FS2-1
		INFO_PRINTF4(_L("FS 2-1 Jump Command: %02xh %02xh %02xh"), sector[0], sector[1], sector[2]);
		SYMBIAN_TEST_TESTNOPANIC((sector[0] == 0xe9) ||
			((sector[0] == 0xeb) && (sector[2] == 0x90)));
		
		// FS2-2
		INFO_PRINTF2(_L("FS 2-2 Sector Size: %04xh"), SYMBIAN_TEST_LE2(sector[11], sector[12]));
		SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[11], sector[12]) == KSectorSize);
		
		// FS2-3
		INFO_PRINTF3(_L("FS 2-3 Sectors per Cluster: %02xh (expected %02xh)"), sector[13], sectorsPerCluster);
		SYMBIAN_TEST_TESTNOPANIC(sector[13] == sectorsPerCluster);
		
		// FS2-4
		INFO_PRINTF2(_L("FS 2-4 Reserved Sector Count: %04xh"), SYMBIAN_TEST_LE2(sector[14], sector[15]));
		SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[14], sector[15]) == 1);
		iServer.iSharedData.iReservedSectorCount = SYMBIAN_TEST_LE2(sector[14], sector[15]);
		
		// FS2-5
		INFO_PRINTF2(_L("FS 2-5 Number of FATs: %02xh"), sector[16]);
		SYMBIAN_TEST_TESTNOPANIC(sector[16] == 2);
		
		// FS2-6
		INFO_PRINTF2(_L("FS 2-6 Number of Root Directory entries: %04xh"), SYMBIAN_TEST_LE2(sector[17], sector[18]));
		SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[17], sector[18]) == KSectorSize);

		// FS2-7
		INFO_PRINTF2(_L("FS 2-7 Total Sectors: %04xh"), SYMBIAN_TEST_LE2(sector[19], sector[20]));
		SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[19], sector[20]) <= 65535);
		SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[19], sector[20]) == iServer.iSharedData.iTotalSector);
		
		// FS2-8
		INFO_PRINTF2(_L("FS 2-8 Medium Identifier: %02xh"), sector[21]);
		SYMBIAN_TEST_TESTNOPANIC(sector[21] == 0xf8);
		
		// FS2-9
		if (iServer.iSharedData.iNumberOfClusters < 4085)
			{
			iServer.iSharedData.iSectorsPerFat = 1 + (1 + ((iServer.iSharedData.iNumberOfClusters + 2) * 3 / 2)) / KSectorSize;
			}
		else
			{
			iServer.iSharedData.iSectorsPerFat = 1 + ((iServer.iSharedData.iNumberOfClusters + 2) * 2) / KSectorSize;
			}
		INFO_PRINTF3(_L("FS 2-9 Sector per FAT: %04xh (expected %04xh)"), SYMBIAN_TEST_LE2(sector[22], sector[23]), iServer.iSharedData.iSectorsPerFat);
		SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[22], sector[23]) == iServer.iSharedData.iSectorsPerFat);
		
		// FS2-10
		INFO_PRINTF2(_L("FS 2-10 Sectors per Track: %04xh"), SYMBIAN_TEST_LE2(sector[24], sector[25]));
		if (iCardSizeInSectors <= 4096)
			{
			SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[24], sector[25]) == 16);	// <=2MB
			}
		else if (iCardSizeInSectors <= 524288)
			{
			SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[24], sector[25]) == 32);	// <=256MB
			}
		else
			{
			SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[24], sector[25]) == 63);
			}
		
		// FS2-11
		INFO_PRINTF2(_L("FS 2-11 Number of Sides: %04xh"), SYMBIAN_TEST_LE2(sector[26], sector[27]));
		if (iCardSizeInSectors <= 32768)			{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 2); }		// <=16MB
		else if (iCardSizeInSectors <= 65536)		{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 4); }		// <=32MB
		else if (iCardSizeInSectors <= 262144)		{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 8); }		// <=128MB
		else if (iCardSizeInSectors <= 1032192)		{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 16); }	// <=504MB
		else if (iCardSizeInSectors <= 2064384)		{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 32); }	// <=1008MB
		else if (iCardSizeInSectors <= 4128768) 	{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 64); }	// <=2016MB
		else 										{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 128); }
		
		// FS2-12
		INFO_PRINTF3(_L("FS 2-12 Reserved for future standardisation: %02x %02x"), sector[28], sector[29]);
		SYMBIAN_TEST_TESTNOPANIC(sector[28] == 0 && sector[29] == 0);
		
		// FS2-13
		INFO_PRINTF3(_L("FS 2-12 Signature Word: %02xh %02xh"), sector[510], sector[511]);
		SYMBIAN_TEST_TESTNOPANIC(sector[510] == 0x55 && sector[511] == 0xaa);
		}
	else // Extended FDC
		{
		// FS2-14
		INFO_PRINTF4(_L("FS 2-14 Jump Command: %02xh %02xh %02xh"), sector[0], sector[1], sector[2]);
		SYMBIAN_TEST_TESTNOPANIC((sector[0] == 0xe9) ||
			((sector[0] == 0xeb) && (sector[2] == 0x90)));
		
		// FS2-15
		INFO_PRINTF2(_L("FS 2-15 Sector Size: %04xh"), SYMBIAN_TEST_LE2(sector[11], sector[12]));
		SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[11], sector[12]) == KSectorSize);
		
		// FS2-16
		INFO_PRINTF3(_L("FS 2-16 Sectors per Cluster: %02xh (expected %02xh)"), sector[13], sectorsPerCluster);
		SYMBIAN_TEST_TESTNOPANIC(sector[13] == sectorsPerCluster);
		
		// FS2-17
		INFO_PRINTF2(_L("FS 2-17 Reserved Sector Count: %04xh"), SYMBIAN_TEST_LE2(sector[14], sector[15]));
		SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[14], sector[15]) == 1);
		
		// FS2-18
		INFO_PRINTF2(_L("FS 2-18 Number of FATs: %02xh"), sector[16]);
		SYMBIAN_TEST_TESTNOPANIC(sector[16] == 2);
		
		// FS2-19
		INFO_PRINTF2(_L("FS 2-19 Number of Root Directory entries: %04xh"), SYMBIAN_TEST_LE2(sector[17], sector[18]));
		SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[17], sector[18]) == KSectorSize);
		
		// FS2-20
		INFO_PRINTF2(_L("FS 2-20 Total Sectors: %04xh"), SYMBIAN_TEST_LE2(sector[19], sector[20]));
		if (iServer.iSharedData.iTotalSector <= 65535)
			{
			SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[19], sector[20]) == iServer.iSharedData.iTotalSector);
			}
		else
			{
			SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[19], sector[20]) == 0);
			}
			
		// FS2-21
		INFO_PRINTF2(_L("FS 2-21 Medium Identifier: %02xh"), sector[21]);
		SYMBIAN_TEST_TESTNOPANIC(sector[21] == 0xf8);
		
		// FS2-22
		if (iServer.iSharedData.iNumberOfClusters < 4085)
			{
			iServer.iSharedData.iSectorsPerFat = 1 + (1 + ((iServer.iSharedData.iNumberOfClusters + 2) * 3 / 2)) / KSectorSize;
			}
		else
			{
			iServer.iSharedData.iSectorsPerFat = 1 + ((iServer.iSharedData.iNumberOfClusters + 2) * 2) / KSectorSize;
			}
		INFO_PRINTF3(_L("FS 2-22 Sector per FAT: %04xh (expected %04xh)"), SYMBIAN_TEST_LE2(sector[22], sector[23]), iServer.iSharedData.iSectorsPerFat);
		SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[22], sector[23]) == iServer.iSharedData.iSectorsPerFat);
		
		// FS2-23
		INFO_PRINTF2(_L("FS 2-23 Sectors per Track: %04xh"), SYMBIAN_TEST_LE2(sector[24], sector[25]));
		if (iCardSizeInSectors <= 4096)
			{
			SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[24], sector[25]) == 16);	// <=2MB
			}
		else if (iCardSizeInSectors <= 524288)
			{
			SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[24], sector[25]) == 32);	// <=256MB
			}
		else
			{
			SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[24], sector[25]) == 63);
			}
		
		// FS2-24
		INFO_PRINTF2(_L("FS 2-24 Number of Sides: %04xh"), SYMBIAN_TEST_LE2(sector[26], sector[27]));
		if (iCardSizeInSectors <= 32768)			{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 2); }		// <=16MB
		else if (iCardSizeInSectors <= 65536)	{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 4); }		// <=32MB
		else if (iCardSizeInSectors <= 262144)	{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 8); }		// <=128MB
		else if (iCardSizeInSectors <= 1032192)	{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 16); }	// <=504MB
		else if (iCardSizeInSectors <= 2064384)	{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 32); }	// <=1008MB
		else if (iCardSizeInSectors <= 4128768) 	{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 64); }	// <=2016MB
		else 									{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 128); }
		
		// FS2-25
		INFO_PRINTF2(_L("FS 2-25 Number of Hidden Sectors %08xh"), SYMBIAN_TEST_LE4(sector[28], sector[29], sector[30], sector[31]));
		SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE4(sector[28], sector[29], sector[30], sector[31]) == iServer.iSharedData.iPartitionBootSector);
		
		// FS2-26
		INFO_PRINTF2(_L("FS 2-26 Total Sectors %08xh"), SYMBIAN_TEST_LE4(sector[32], sector[33], sector[34], sector[35]));
		if (iServer.iSharedData.iTotalSector <= 65535)
			{
			SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE4(sector[32], sector[33], sector[34], sector[35]) == 0);
			}
		else
			{
			SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE4(sector[32], sector[33], sector[34], sector[35]) == iServer.iSharedData.iTotalSector);
			}
		
		// FS2-27
		INFO_PRINTF2(_L("FS 2-27 Physical Disk Number: %02xh"), sector[36]);
		SYMBIAN_TEST_TESTNOPANIC(sector[36] == 0x80);
		
		// FS2-28
		INFO_PRINTF2(_L("FS 2-28 Reserved: %02xh"), sector[37]);
		SYMBIAN_TEST_TESTNOPANIC(sector[37] == 0);
		
		// FS2-29
		INFO_PRINTF9(_L("FS 2-29 File System Type: '%c%c%c%c%c%c%c%c'"), sector[54], sector[55], sector[56], sector[57], sector[58], sector[59], sector[60], sector[61]);
		SYMBIAN_TEST_TESTNOPANIC((sector[54] == 'F') && (sector[55] == 'A') && (sector[56] == 'T') && (sector[57] == '1') && (sector[59] == ' ') && (sector[60] == ' ') && (sector[61] == ' '));
		if (iServer.iSharedData.iNumberOfClusters < 4085)
			{
			SYMBIAN_TEST_TESTNOPANIC(sector[58] == '2');
			}
		else
			{
			SYMBIAN_TEST_TESTNOPANIC(sector[58] == '6');
			}
		
		// FS2-30
		INFO_PRINTF3(_L("FS 2-30 Signature Word: %02xh %02xh"), sector[510], sector[511]);
		SYMBIAN_TEST_TESTNOPANIC(sector[510] == 0x55 && sector[511] == 0xaa);
		}
		
	for (TInt i = 0; i < KSectorSize / 16; i++)
		{
		TBuf<60> buffer;
		buffer.Format(_L("%08x:"), i * 16 + iServer.iSharedData.iPartitionBootSector);
		for (TInt j = 0; j < 16; j++)
			{
			buffer.AppendFormat(_L(" %02x"), sector[i * 16 + j]);
			}
		INFO_PRINTF2(_L("%S"), &buffer);
		}
	}

/*
Field Check of Partition Boot Sector (FAT32)

@param None
@return None
*/
void CBaseTestSDFieldCheck::FS2Fat32()
	{
	INFO_PRINTF1(_L("FS2 - Field Check of Partition Boot Sector"));
	// See section 4.2.1. Partition Boot Sector (FAT32)
	// of the SD Specification for details

	TBuf8<KSectorSize> sector;
	
	if (ReadSector(iServer.iSharedData.iPartitionBootSector, sector) != KErrNone)
		{
		SetTestStepResult(EFail);
		return;
		}
	
	TInt sectorsPerCluster = 0;
	if (iCardSizeInSectors <= 16384) { sectorsPerCluster = 16; }			// <=8MB
	else if (iCardSizeInSectors <= 2097152) { sectorsPerCluster = 32; }	// <=1024MB
	else { sectorsPerCluster = 64; }
	
	iServer.iSharedData.iNumberOfClusters = (iServer.iSharedData.iTotalSector - (1 + SYMBIAN_TEST_LE2(sector[22], sector[23]) * 2 + (KSectorSize * 32 / KSectorSize))) / sectorsPerCluster;
	
	// Calculations (SD Spec, Annex c.2.4)
	TUint32 KBoundaryUnit = 8192;
	TUint32 KSectorsPerCluster = 64;
	TUint32 KFatBits = 32;
	TUint32 KSectorsInMBR = KBoundaryUnit;
	
	// Caclculate number of sectors per FAT according to section C.2.4
	TUint32 sf = SYMBIAN_TEST_CEIL(KFatBits * iCardSizeInSectors / KSectorsPerCluster, KSectorSize * 8);
	TUint32 sfp;
	TUint32 rsc;
	TBool cond13 = ETrue;
	do
		{
		TUint32 n = 0;
		while (KBoundaryUnit * n < 2 * sf + 9)
			{
			n++;
			}
		rsc = KBoundaryUnit * n - 2 * sf;
		TInt ssa = rsc + 2 * sf;
		do {
			TInt max = SYMBIAN_TEST_IP(iCardSizeInSectors - KSectorsInMBR - ssa, KSectorsPerCluster) + 1;
			sfp = SYMBIAN_TEST_CEIL((2 + (max - 1)) * KFatBits, KSectorSize * 8);
			if (sfp > sf)
				{
				ssa += KBoundaryUnit;
				rsc += KBoundaryUnit;
				}
			} while (sfp > sf);
		if (sf != sfp)
			{
			sf--;
			cond13 = EFalse;
			}
		else
			{
			cond13 = ETrue;
			}
		} while (!cond13);

	// FS2-31
	INFO_PRINTF4(_L("FS 2-31 Jump Command: %02xh %02xh %02xh"), sector[0], sector[1], sector[2]);
	SYMBIAN_TEST_TESTNOPANIC((sector[0] == 0xe9) ||
		((sector[0] == 0xeb) && (sector[2] == 0x90)));
	
	// FS2-32
	INFO_PRINTF3(_L("FS 2-32 Sector Size: %04xh (expected %04xh)"), SYMBIAN_TEST_LE2(sector[11], sector[12]), KSectorSize);
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[11], sector[12]) == KSectorSize);
	
	// FS2-33
	INFO_PRINTF3(_L("FS 2-33 Sectors per Cluster: %02xh (expected %02xh)"), sector[13], KSectorsPerCluster);
	SYMBIAN_TEST_TESTNOPANIC(sector[13] == KSectorsPerCluster);
	
	// FS2-34
	INFO_PRINTF3(_L("FS 2-34 Reserved Sector Count: %04xh (expected %04xh)"), SYMBIAN_TEST_LE2(sector[14], sector[15]), rsc);
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[14], sector[15]) == rsc);
	
	// FS2-35
	INFO_PRINTF2(_L("FS 2-35 Number of FATs: %02xh"), sector[16]);
	SYMBIAN_TEST_TESTNOPANIC(sector[16] == 2);
	
	// FS2-36
	INFO_PRINTF2(_L("FS 2-36 Number of Root Directory entries: %04xh"), SYMBIAN_TEST_LE2(sector[17], sector[18]));
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[17], sector[18]) == 0); // FAT32 No max root dir entries

	// FS2-37
	INFO_PRINTF2(_L("FS 2-37 Total Sectors: %04xh"), SYMBIAN_TEST_LE2(sector[19], sector[20]));
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[19], sector[20]) == 0);
	
	// FS2-38
	INFO_PRINTF2(_L("FS 2-38 Medium Identifier: %02xh"), sector[21]);
	SYMBIAN_TEST_TESTNOPANIC(sector[21] == 0xf8);
	
	// FS2-39
	INFO_PRINTF2(_L("FS 2-39 Sector per FAT: %04xh"), SYMBIAN_TEST_LE2(sector[22], sector[23]));
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[22], sector[23]) == 0); // This field not used in the FAT32 spec
	
	// FS2-40
	INFO_PRINTF2(_L("FS 2-40 Sectors per Track: %04xh"), SYMBIAN_TEST_LE2(sector[24], sector[25]));
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[24], sector[25]) == 63);
	
	// FS2-41
	INFO_PRINTF2(_L("FS 2-41 Number of Sides: %04xh"), SYMBIAN_TEST_LE2(sector[26], sector[27]));
	if (iCardSizeInSectors <= 8257536)		{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 128); }		// <=4032MB
	else 									{ SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[26], sector[27]) == 255); }

	// FS2-42
	INFO_PRINTF2(_L("FS 2-42 Number of Hidden Sectors %08xh"), SYMBIAN_TEST_LE4(sector[28], sector[29], sector[30], sector[31]));
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE4(sector[28], sector[29], sector[30], sector[31]) == iServer.iSharedData.iPartitionBootSector);
			
	// FS2-43
	INFO_PRINTF2(_L("FS 2-43 Total Sectors %08xh"), SYMBIAN_TEST_LE4(sector[32], sector[33], sector[34], sector[35]));
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE4(sector[32], sector[33], sector[34], sector[35]) == iServer.iSharedData.iTotalSector);
	
	// FS2-44
	INFO_PRINTF3(_L("FS 2-44 Sectors per FAT for FAT32 %08xh (expected: %08xh)"), SYMBIAN_TEST_LE4(sector[36], sector[37], sector[38], sector[39]), sf);
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE4(sector[36], sector[37], sector[38], sector[39]) == sf);
	
	// FS2-45
	INFO_PRINTF2(_L("FS 2-45 Extension Flag %04xh"), SYMBIAN_TEST_LE2(sector[40], sector[41]));
	
	// FS2-46
	INFO_PRINTF2(_L("FS 2-46 FS Version %04xh"), SYMBIAN_TEST_LE2(sector[42], sector[43]));
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[42], sector[43]) == 0);
	
	// FS2-47
	INFO_PRINTF2(_L("FS 2-47 Root Cluster %08xh"), SYMBIAN_TEST_LE4(sector[44], sector[45], sector[46], sector[47]));
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE4(sector[44], sector[45], sector[46], sector[47]) >= 2);
	
	// FS2-48
	INFO_PRINTF2(_L("FS 2-48 FS Info %04xh"), SYMBIAN_TEST_LE2(sector[48], sector[49]));
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[48], sector[49]) == 1);
	
	// FS2-49
	INFO_PRINTF2(_L("FS 2-49 Backup Boot Sector %04xh"), SYMBIAN_TEST_LE2(sector[50], sector[51]));
	SYMBIAN_TEST_TESTNOPANIC(SYMBIAN_TEST_LE2(sector[50], sector[51]) == 6);
		
	// FS2-50
	INFO_PRINTF2(_L("FS 2-50 Physical Disk Number: %02xh"), sector[64]);
	SYMBIAN_TEST_TESTNOPANIC(sector[64] == 0x80);
	
	// FS2-51
	INFO_PRINTF2(_L("FS 2-51 Extended Boot Record Signature: %02xh"), sector[66]);
	SYMBIAN_TEST_TESTNOPANIC(sector[66] == 0x29);
	
	// FS2-52
	INFO_PRINTF9(_L("FS 2-52 File System Type: '%c%c%c%c%c%c%c%c'"), sector[82], sector[83], sector[84], sector[85], sector[86], sector[87], sector[88], sector[89]);
	SYMBIAN_TEST_TESTNOPANIC((sector[82] == 'F') && (sector[83] == 'A') && (sector[84] == 'T') && (sector[85] == '3') && (sector[86] == '2')&& (sector[87] == ' ') && (sector[88] == ' ') && (sector[89] == ' '));
	
	// FS2-53
	INFO_PRINTF3(_L("FS 2-30 Signature Word: %02xh %02xh"), sector[510], sector[511]);
	SYMBIAN_TEST_TESTNOPANIC(sector[510] == 0x55 && sector[511] == 0xaa);
		
	for (TInt i = 0; i < KSectorSize / 16; i++)
		{
		TBuf<60> buffer;
		buffer.Format(_L("%08x:"), i * 16 + iServer.iSharedData.iPartitionBootSector);
		for (TInt j = 0; j < 16; j++)
			{
			buffer.AppendFormat(_L(" %02x"), sector[i * 16 + j]);
			}
		INFO_PRINTF2(_L("%S"), &buffer);
		}
	}

/*
Field Check of FAT32 FS Info Sector

@param None
@return None
*/
void CBaseTestSDFieldCheck::FSInfo()
	{
	// See section 4.2.2. FS Info Sector
	// of the SD Specification for details

	INFO_PRINTF1(_L("FSx - Field Check of FAT32's FS Info Sector"));
	TBuf8<KSectorSize> sector;
	
	if (ReadSector(iServer.iSharedData.iPartitionBootSector + 1, sector) != KErrNone)
		{
		SetTestStepResult(EFail);
		return;
		}
		
	// 
	INFO_PRINTF5(_L("- Lead Signature: %02xh %02xh %02xh %02xh"), sector[0], sector[1], sector[2], sector[3]);	
	SYMBIAN_TEST_TESTNOPANIC(sector[0] == 0x52);
	SYMBIAN_TEST_TESTNOPANIC(sector[1] == 0x52);
	SYMBIAN_TEST_TESTNOPANIC(sector[2] == 0x61);
	SYMBIAN_TEST_TESTNOPANIC(sector[3] == 0x41);
	
	//
	for (TInt i = 4; i < 484; i++)
		{
		SYMBIAN_TEST_TESTNOPANIC(sector[i] == 0);
		}
	
	// 
	INFO_PRINTF5(_L("- Structure Signature: %02xh %02xh %02xh %02xh"), sector[484], sector[485], sector[486], sector[487]);
	SYMBIAN_TEST_TESTNOPANIC(sector[484] == 0x72);
	SYMBIAN_TEST_TESTNOPANIC(sector[485] == 0x72);
	SYMBIAN_TEST_TESTNOPANIC(sector[486] == 0x41);
	SYMBIAN_TEST_TESTNOPANIC(sector[487] == 0x61);
	
	//
	INFO_PRINTF2(_L("- Free Cluster Count: %08xh"), SYMBIAN_TEST_LE4(sector[488], sector[489], sector[490], sector[491]));
	INFO_PRINTF2(_L("- Next Free Cluster: %08xh"), SYMBIAN_TEST_LE4(sector[492], sector[493], sector[494], sector[495]));
	
	INFO_PRINTF5(_L("- Trail Signature: %02xh %02xh %02xh %02xh"), sector[508], sector[509], sector[510], sector[511]);
	SYMBIAN_TEST_TESTNOPANIC(sector[508] == 0);
	SYMBIAN_TEST_TESTNOPANIC(sector[509] == 0);
	SYMBIAN_TEST_TESTNOPANIC(sector[510] == 0x55);
	SYMBIAN_TEST_TESTNOPANIC(sector[511] == 0xaa);
	
	for (TInt i = 0; i < KSectorSize / 16; i++)
		{
		TBuf<60> buffer;
		buffer.Format(_L("%08x:"), i * 16 + iServer.iSharedData.iPartitionBootSector + 1);
		for (TInt j = 0; j < 16; j++)
			{
			buffer.AppendFormat(_L(" %02x"), sector[i * 16 + j]);
			}
		INFO_PRINTF2(_L("%S"), &buffer);
		}
	}

/*
Field Check of the File Allocation Table

@param None
@return None
*/
void CBaseTestSDFieldCheck::FS3()
	{
	INFO_PRINTF1(_L("FS3 - Field Check of File Allocation Table"));
	// See section 3.2.2. File Allocation Table
	// of the SD Specification for details
	
	// Also see SD Test Specification for File Systems
	TBuf8<KSectorSize> sector;
		
	if (ReadSector(iServer.iSharedData.iPartitionBootSector + 1, sector) != KErrNone)
		{
		SetTestStepResult(EFail);
		return;
		}

	// FS3-1
	if (iServer.iSharedData.iNumberOfClusters < 4085)
		{
		INFO_PRINTF4(_L("FS 3-1 Head 3 bytes of First FAT: %02xh %02xh %02xh"), sector[0], sector[1], sector[2]);	
		SYMBIAN_TEST_TESTNOPANIC((sector[0] == 0xf8) && (sector[1] == 0xff) && (sector[2] == 0xff));
		}
	else
		{
		INFO_PRINTF5(_L("FS 3-1 Head 4 bytes of First FAT: %02xh %02xh %02xh %02xh"), sector[0], sector[1], sector[2], sector[3]);	
		SYMBIAN_TEST_TESTNOPANIC((sector[0] == 0xf8) && (sector[1] == 0xff) && (sector[2] == 0xff) && (sector[3] == 0xff));
		}
	
	// FS3-2
	INFO_PRINTF1(_L("FS 3-2 Read First FAT"));	
	for (TInt i = 0; i < iServer.iSharedData.iSectorsPerFat; i++)
		{
		if (ReadSector(iServer.iSharedData.iPartitionBootSector + 1 + i, sector) != KErrNone)
			{
			SetTestStepResult(EFail);
			return;
			}
		for (TInt j = 0; j < KSectorSize; j++)
			{
			if ((i > 0) || ((i == 0) && (j > 3)) ||	((i == 0) && (j == 3) && (iServer.iSharedData.iNumberOfClusters < 4085)))
				{
				SYMBIAN_TEST_TESTNOPANIC(sector[j] == 0);
				}
			}
		}
	
	if (ReadSector(iServer.iSharedData.iPartitionBootSector + 1 + iServer.iSharedData.iSectorsPerFat, sector) != KErrNone)
		{
		SetTestStepResult(EFail);
		return;
		}
	
	// FS3-3
	if (iServer.iSharedData.iNumberOfClusters < 4085)
		{
		INFO_PRINTF4(_L("FS 3-3 Head 3 bytes of Second FAT: %02xh %02xh %02xh"), sector[0], sector[1], sector[2]);	
		SYMBIAN_TEST_TESTNOPANIC((sector[0] == 0xf8) && (sector[1] == 0xff) && (sector[2] == 0xff));
		}
	else
		{
		INFO_PRINTF5(_L("FS 3-3 Head 4 bytes of Second FAT: %02xh %02xh %02xh"), sector[0], sector[1], sector[2], sector[3]);	
		SYMBIAN_TEST_TESTNOPANIC((sector[0] == 0xf8) && (sector[1] == 0xff) && (sector[2] == 0xff) && (sector[3] == 0xff));
		}
	
	// FS3-4
	INFO_PRINTF1(_L("FS 3-4 Read Second FAT"));	
	for (TInt i = 0; i < iServer.iSharedData.iSectorsPerFat; i++)
		{
		if (ReadSector(iServer.iSharedData.iPartitionBootSector + 1 + iServer.iSharedData.iSectorsPerFat + i, sector) != KErrNone)
			{
			SetTestStepResult(EFail);
			return;
			}
		for (TInt j = 0; j < KSectorSize; j++)
			{
			TInt sec = 0;
			if ((i > 0) || ((i == 0) && (j > 3)) ||	((i == 0) && (j == 3) && (iServer.iSharedData.iNumberOfClusters < 4085)))
				{
				sec = sec | sector[j];
				}
			SYMBIAN_TEST_TESTNOPANIC(sec == 0);
			}
		}
	}

/*
Field Check of Root Directory

@param None
@return None
*/
void CBaseTestSDFieldCheck::FS4()
	{
	INFO_PRINTF1(_L("FS4 - Field Check of Root Directory"));
	// See SD Test Specification for File Systems for details
	TBuf8<KSectorSize> sector;
	
	INFO_PRINTF1(_L("FS 4-1 Read Root Directory"));
	for (TInt i = 0; i < KSectorSize / 16; i++)
		{
		TInt sec = 0;
		if (ReadSector(iServer.iSharedData.iPartitionBootSector + 1 + i + iServer.iSharedData.iSectorsPerFat * 2, sector) != KErrNone)
			{
			SetTestStepResult(EFail);
			return;
			}
		for (TInt j = 0; j < KSectorSize; j++)
			{
			sec = sec | sector[j];
			}
		SYMBIAN_TEST_TESTNOPANIC(sec == 0);
		}
	}

/*
File Systen Layout Check

@param None
@return None
*/
void CBaseTestSDFieldCheck::FS5()
	{
	INFO_PRINTF1(_L("FS5 - File System Layout Check"));
	// See SD Test Specification for File Systems for details

	TInt boundaryUnit = 0;
	if (iCardSizeInSectors <= 16384)			{ boundaryUnit = 16; }	// <= 8MB
	else if (iCardSizeInSectors <= 131072)		{ boundaryUnit = 32; }	// <= 64MB
	else if (iCardSizeInSectors <= 1048576)		{ boundaryUnit = 64; }	// <= 256MB
	else { boundaryUnit = 128; }
	
	INFO_PRINTF2(_L("Boundary Unit: %d"), boundaryUnit);
	INFO_PRINTF2(_L("Relative Sector: %d"), iServer.iSharedData.iPartitionBootSector);
	INFO_PRINTF2(_L("Reserved Sector Count: %d"), iServer.iSharedData.iReservedSectorCount);
	INFO_PRINTF2(_L("Sectors per FAT: %d"), iServer.iSharedData.iSectorsPerFat);
	SYMBIAN_TEST_TESTNOPANIC((iServer.iSharedData.iPartitionBootSector + iServer.iSharedData.iReservedSectorCount + iServer.iSharedData.iSectorsPerFat * 2 + 33) % boundaryUnit == 0);
	SYMBIAN_TEST_TESTNOPANIC(iServer.iSharedData.iPartitionBootSector >= boundaryUnit);
	}

/*
Compare Boot Sectors and Backup Boot Sectors

@param None
@return None
*/
void CBaseTestSDFieldCheck::FSBackupSectors()
	{
	INFO_PRINTF1(_L("FSx - Compare Boot Sectors and Backup Boot Sectors"));
	// Backup sectors must start on sector 6 (relative)
	TBuf8<KSectorSize> originalsector;
	TBuf8<KSectorSize> backupsector;
	
	for (TInt i = 0; i < 3; i++)
		{
		INFO_PRINTF2(_L("Boot sector comparison %d/3"), i + 1);
		if (ReadSector(iServer.iSharedData.iPartitionBootSector + i, originalsector) != KErrNone)
			{
			SetTestStepResult(EFail);
			return;
			}
		if (ReadSector(iServer.iSharedData.iPartitionBootSector + i + 6, backupsector) != KErrNone)
			{
			SetTestStepResult(EFail);
			return;
			}
		TBool areIdentical = ETrue;
		for (TInt j = 0; j < KSectorSize; j++)
			{
			if (originalsector[j] != backupsector[j])
				{
				areIdentical = EFalse;
				}
			}
		if (areIdentical)
			{
			INFO_PRINTF1(_L("Sectors are identical"));
			}
		else
			{
			ERR_PRINTF1(_L("Sectors are not identical"));
			SetTestStepResult(EFail);
			}
		}
	}
