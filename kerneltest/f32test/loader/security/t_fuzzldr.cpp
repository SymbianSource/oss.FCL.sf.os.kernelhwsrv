// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\loader\security\t_fuzzldr.cpp
// 
//

#define __E32TEST_EXTENSION__
#include <e32test.h>
#include <e32svr.h>
#include <e32uid.h>
#include <f32file.h>
#include <f32image.h>
#include "t_hash.h"

// Fuzzing parameters

const TInt KFuzzImages = 5;
const TInt KRandomFieldIterations = 8;
const TTimeIntervalMicroSeconds32 KDllTimeout = 10 * 1000000;

#define FUZZFIELD(OBJ, NAME) { (const TText*)(L ## #OBJ L"." L ## #NAME), sizeof(((OBJ*)8)->NAME), _FOFF(OBJ, NAME) }
#define DUMBFIELD(NAME, SIZE, OFFSET) { (const TText*)L ## NAME, SIZE, OFFSET }
#define FUZZEND { NULL, 0, 0 }

struct SFuzzField
	{
	const TText* name;
	TInt size;
	TInt offset;
	};

const SFuzzField HeaderFields[] =
	{
	FUZZFIELD(E32ImageHeaderV, iUid1),
	FUZZFIELD(E32ImageHeaderV, iUid2),
	FUZZFIELD(E32ImageHeaderV, iUid3),
	FUZZFIELD(E32ImageHeaderV, iUidChecksum),
	FUZZFIELD(E32ImageHeaderV, iSignature),
	FUZZFIELD(E32ImageHeaderV, iHeaderCrc),
	FUZZFIELD(E32ImageHeaderV, iModuleVersion),
	FUZZFIELD(E32ImageHeaderV, iCompressionType),
	FUZZFIELD(E32ImageHeaderV, iToolsVersion.iMajor),
	FUZZFIELD(E32ImageHeaderV, iToolsVersion.iMinor),
	FUZZFIELD(E32ImageHeaderV, iToolsVersion.iBuild),
	FUZZFIELD(E32ImageHeaderV, iTimeLo),
	FUZZFIELD(E32ImageHeaderV, iTimeHi),
	FUZZFIELD(E32ImageHeaderV, iFlags),
	FUZZFIELD(E32ImageHeaderV, iCodeSize),
	FUZZFIELD(E32ImageHeaderV, iDataSize),
	FUZZFIELD(E32ImageHeaderV, iHeapSizeMin),
	FUZZFIELD(E32ImageHeaderV, iHeapSizeMax),
	FUZZFIELD(E32ImageHeaderV, iStackSize),
	FUZZFIELD(E32ImageHeaderV, iBssSize),
	FUZZFIELD(E32ImageHeaderV, iEntryPoint),
	FUZZFIELD(E32ImageHeaderV, iCodeBase),
	FUZZFIELD(E32ImageHeaderV, iDataBase),
	FUZZFIELD(E32ImageHeaderV, iDllRefTableCount),
	FUZZFIELD(E32ImageHeaderV, iExportDirOffset),
	FUZZFIELD(E32ImageHeaderV, iExportDirCount),
	FUZZFIELD(E32ImageHeaderV, iTextSize),
	FUZZFIELD(E32ImageHeaderV, iCodeOffset),
	FUZZFIELD(E32ImageHeaderV, iDataOffset),
	FUZZFIELD(E32ImageHeaderV, iImportOffset),
	FUZZFIELD(E32ImageHeaderV, iCodeRelocOffset),
	FUZZFIELD(E32ImageHeaderV, iDataRelocOffset),
	FUZZFIELD(E32ImageHeaderV, iProcessPriority),
	FUZZFIELD(E32ImageHeaderV, iCpuIdentifier),
	FUZZFIELD(E32ImageHeaderV, iUncompressedSize),
	FUZZFIELD(E32ImageHeaderV, iS.iSecureId),
	FUZZFIELD(E32ImageHeaderV, iS.iVendorId),
	FUZZFIELD(E32ImageHeaderV, iS.iCaps.iCaps[0]),
	FUZZFIELD(E32ImageHeaderV, iS.iCaps.iCaps[1]),
	FUZZFIELD(E32ImageHeaderV, iExceptionDescriptor),
	FUZZFIELD(E32ImageHeaderV, iSpare2),
	FUZZFIELD(E32ImageHeaderV, iExportDescSize),
	FUZZFIELD(E32ImageHeaderV, iExportDescType),
	FUZZFIELD(E32ImageHeaderV, iExportDesc[0]),
	FUZZEND
	};

const SFuzzField ImportSectionFields[] =
	{
	FUZZFIELD(E32ImportSection, iSize),
	FUZZEND
	};

const SFuzzField ImportBlockFields[] =
	{
	FUZZFIELD(E32ImportBlock, iOffsetOfDllName), // we should fuzz the string also
	FUZZFIELD(E32ImportBlock, iNumberOfImports),
	FUZZEND
	};

const SFuzzField ImportEntryFields[] =
	{
	DUMBFIELD("import", 4, 0),
	FUZZEND
	};

const SFuzzField RelocSectionFields[] =
	{
	FUZZFIELD(E32RelocSection, iSize),
	FUZZFIELD(E32RelocSection, iNumberOfRelocs),
	FUZZEND
	};

const SFuzzField RelocBlockFields[] =
	{
	FUZZFIELD(E32RelocBlock, iPageOffset),
	FUZZFIELD(E32RelocBlock, iBlockSize),
	FUZZEND
	};

const SFuzzField RelocEntryFields[] =
	{
	DUMBFIELD("reloc", 2, 0),
	FUZZEND
	};

const SFuzzField ExportEntryFields[] =
	{
	DUMBFIELD("export", 4, 0),
	FUZZEND
	};

const SFuzzField CompressedDataFields[] =
	{
	DUMBFIELD("data1", 4, 0),
	DUMBFIELD("data2", 4, 4),
	DUMBFIELD("data3", 4, 16),
	DUMBFIELD("data4", 4, 64),
	DUMBFIELD("data5", 4, 256),
	DUMBFIELD("data6", 4, 1024),
	DUMBFIELD("data7", 4, 4096),
	FUZZEND
	};

// Values to try for different sizes, signed here but can be interpreted as either
// each will also try the smaller sizes' values
const TInt8 Values8[] = { KMinTInt8, -100, -10, -2, -1, 0, 1, 2, 10, 100, KMaxTInt8 };
const TInt Values8Count = sizeof(Values8)/sizeof(TInt8);
const TInt16 Values16[] = { KMinTInt16, -10000, -256, -255, 128, 255, 256, 10000, KMaxTInt16 };
const TInt Values16Count = sizeof(Values16)/sizeof(TInt16);
const TInt32 Values32[] = { KMinTInt32, -1000000000, -65536, -65535, 32768, 65535, 65536, 268435455, 268435456, 1000000000, KMaxTInt32 };
const TInt Values32Count = sizeof(Values32)/sizeof(TInt32);
const TInt ValuesCount[] = { 0, Values8Count, Values8Count+Values16Count, 0, Values8Count+Values16Count+Values32Count };
const TInt Offsets[] = { 1, 2, 4, 16, -1, -2, -4, -16 };
const TInt OffsetsCount = sizeof(Offsets)/sizeof(TInt);

// Regular boring definitions and stuff

RTest test(_L("T_FUZZLDR"));
RFs TheFs;
CFileMan* FileMan;

_LIT(KOrigDir, "Z:\\sys\\bin\\");
_LIT(KSysBin, ":\\sys\\bin\\");
_LIT(KSysHash, ":\\sys\\hash\\");
_LIT(KImageName, "fuzzv");
_LIT(KExeExt, ".exe");
_LIT(KDllExt, ".dll");
_LIT(KMyself, "t_fuzzldr");
_LIT(KSlaveArg, "-l ");

TFileName Provided;
TFileName Original;
TFileName Current;
TFileName Hash;
TFileName HashDir;
TBool LoadDll;
RFile File;
RTimer Timer;
TUint8* Target;
E32ImageHeaderV* Header;
E32ImageHeaderV* CleanHeader;
CSHA1* Hasher;

TInt FileSize;
TInt OutFileSize;
TUint8* CleanFileData;
TPtr8 CleanFileDes(CleanFileData, 0);
TUint8* FileData;
TPtr8 FileDes(FileData, 0);
TUint8* EndOfFile;
TChar Drive = '?', InternalDrive = '?', RemovableDrive = '?';

TBool Verbose;
TBool Forever = EFalse;
TUint32 Seed = 0;
typedef void (*TFieldFuzzer)(const SFuzzField*, TInt);

enum SetMode {
	ESetLiteral,
	ESetOffset,
	ESetRandom,
	ESetXor,
};

enum ValueMode {
	EValLiteral,
	EValOffset,
	EValRandom,
	EValXor,
	EValList,
	EValOffsetList,
};


TUint32 Rand()
	{
	Seed *= 69069;
	Seed += 5;
	return Seed;
	}


TUint32 Rand(TUint32 aLimit)
	{
	TUint64 temp = (TUint64)Rand() * (TUint64)aLimit;
	return (TUint32)(temp>>32);
	}


void PrepareName(TInt aNum, TBool aDll)
	{
	Original = KOrigDir;
	Original += KImageName;
	Original.AppendNum(aNum);
	Original += aDll ? KDllExt : KExeExt;
	Current.Zero();
	Current.Append(Drive);
	Current += KSysBin;
	Current += KImageName;
	Current.AppendNum(aNum);
	Current += aDll ? KDllExt : KExeExt;
	Hash = HashDir;
	Hash += KImageName;
	Hash.AppendNum(aNum);
	Hash += aDll ? KDllExt : KExeExt;
	}


void PrepareProvidedName()
	{
	Original = KOrigDir;
	Original += Provided;
	Current.Zero();
	Current.Append(Drive);
	Current += KSysBin;
	Current += Provided;
	Hash = HashDir;
	Hash += Provided;
	}


void MakeCleanCopy()
	{
	Mem::Copy(FileData, CleanFileData, FileSize);
	}


void LoadCleanFile()
	{
	test_KErrNone(File.Open(TheFs, Original, EFileRead));
	test_KErrNone(File.Size(FileSize));
	OutFileSize = FileSize;
	CleanFileData = new TUint8[FileSize];
	test_NotNull(CleanFileData);
	FileData = new TUint8[FileSize];
	EndOfFile = FileData + FileSize;
	test_NotNull(FileData);
	CleanFileDes.Set(CleanFileData, 0, FileSize);
	test_KErrNone(File.Read(CleanFileDes));
	File.Close();
	Header = (E32ImageHeaderV*)FileData;
	CleanHeader = (E32ImageHeaderV*)CleanFileData;
	FileDes.Set(FileData, FileSize, FileSize);
	MakeCleanCopy();
	test(CleanHeader->iUid1==(TUint32)KExecutableImageUidValue || CleanHeader->iUid1==(TUint32)KDynamicLibraryUidValue);
	LoadDll = CleanHeader->iUid1==(TUint32)KDynamicLibraryUidValue;
	}


void DoneFile()
	{
	delete[] FileData;
	delete[] CleanFileData;
	}


void RecalcChecksums()
	{
	if (Header->iUidChecksum == CleanHeader->iUidChecksum)
		{
		TUidType uids = *(const TUidType*)Header;
		TCheckedUid chkuid(uids);
		const TUint32* pChkUid = (const TUint32*)&chkuid;
		Header->iUidChecksum = pChkUid[3];
		}
	if (Header->iHeaderCrc == CleanHeader->iHeaderCrc)
		{
		Header->iHeaderCrc = KImageCrcInitialiser;
		TUint32 crc = 0;
		Mem::Crc32(crc, Header, sizeof(E32ImageHeaderV));
		Header->iHeaderCrc = crc;
		}
	}


void Load()
	{
	RecalcChecksums();
	test_KErrNone(File.Replace(TheFs, Current, EFileWrite));
	test_KErrNone(File.Write(FileDes, OutFileSize));
	test_KErrNone(File.Flush());
	File.Close();
	if (Drive == RemovableDrive)
		{
		TPtrC8 data(FileData, OutFileSize);
		Hasher->Reset();
		Hasher->Update(data);
		TBuf8<SHA1_HASH> hashVal = Hasher->Final();
		test_KErrNone(File.Replace(TheFs, Hash, EFileWrite));
		test_KErrNone(File.Write(hashVal));
		test_KErrNone(File.Flush());
		File.Close();
		}
	RProcess p;
	TInt r;
	if (LoadDll)
		{
		TFileName args;
		args.Copy(KSlaveArg);
		args.Append(Current);
		test_KErrNone(p.Create(KMyself, args));
		TRequestStatus logon, rendez, timeout;
		p.Logon(logon);
		p.Rendezvous(rendez);
		p.Resume();
		User::WaitForRequest(rendez);
		test(rendez==KErrNone);
		Timer.After(timeout, KDllTimeout);
		User::WaitForRequest(logon, timeout);
		if (logon == KRequestPending)
			{
			p.Kill(0);
			User::WaitForRequest(logon);
			}
		else
			{
			Timer.Cancel();
			User::WaitForRequest(timeout);
			}
		p.Close();
		// we don't check the return code as passing it back makes the log output
		// super spammy with KPANIC on - it prints for every nonzero return code.
		if (Verbose) test.Printf(_L("\n"));
		}
	else
		{
		r = p.Create(Current, KNullDesC);
		if (r==KErrNone)
			p.Kill(0);
		if (Verbose) test.Printf(_L("=> %d\n"), r);
		}
	p.Close();
	}


template <typename T> void SetFieldTo(const SFuzzField* aField, T aSetTo, SetMode aMode)
	{
	T* field = (T*)(Target + aField->offset);
	if ((TUint8*)field >= EndOfFile)
		{
		if (Verbose) test.Printf(_L("skipping, eof "));
		return;
		}
	if (aMode == ESetOffset)
		aSetTo += *field;
	else if (aMode == ESetRandom)
		aSetTo = (T)Rand();
	else if (aMode == ESetXor)
		aSetTo ^= *field;
	*field = aSetTo;
	if (Verbose) test.Printf(_L("%d "), aSetTo);
	}


void SetField(const SFuzzField* aField, TInt aValue, ValueMode aMode)
	{
	if (aMode < EValList)
		{
		switch(aField->size)
			{
		case 1:
			SetFieldTo<TInt8>(aField, aValue, (SetMode)aMode);
			break;
		case 2:
			SetFieldTo<TInt16>(aField, aValue, (SetMode)aMode);
			break;
		case 4:
			SetFieldTo<TInt32>(aField, aValue, (SetMode)aMode);
			break;
			}
		}
	else if (aMode == EValList)
		{
		switch(aField->size)
			{
		case 1:
			SetFieldTo<TInt8>(aField, Values8[aValue], ESetLiteral);
			break;
		case 2:
			if (aValue < ValuesCount[1])
				SetFieldTo<TInt16>(aField, Values8[aValue], ESetLiteral);
			else
				SetFieldTo<TInt16>(aField, Values16[aValue-ValuesCount[1]], ESetLiteral);
			break;
		case 4:
			if (aValue < ValuesCount[1])
				SetFieldTo<TInt32>(aField, Values8[aValue], ESetLiteral);
			else if (aValue < ValuesCount[2])
				SetFieldTo<TInt32>(aField, Values16[aValue-ValuesCount[1]], ESetLiteral);
			else
				SetFieldTo<TInt32>(aField, Values32[aValue-ValuesCount[2]], ESetLiteral);
			break;
			}
		}
	else if (aMode == EValOffsetList)
		{
		switch(aField->size)
			{
		case 1:
			SetFieldTo<TInt8>(aField, Offsets[aValue], ESetOffset);
			break;
		case 2:
			SetFieldTo<TInt16>(aField, Offsets[aValue], ESetOffset);
			break;
		case 4:
			SetFieldTo<TInt32>(aField, Offsets[aValue], ESetOffset);
			break;
			}
		}
	}


void FuzzFieldsDeterministically(const SFuzzField* aFields, TInt aOffset)
	{
	Target = FileData + aOffset;

	TInt f = -1;
   	while (aFields[++f].name)
		{
		test.Printf(_L("FIELD: %s ...\n"), aFields[f].name);
		TInt v;
		if (Verbose) test.Next(_L("Using preset values"));
		for (v = 0; v < ValuesCount[aFields[f].size]; ++v)
			{
			MakeCleanCopy();
			SetField(&aFields[f], v, EValList);
			Load();
			}
		if (Verbose) test.Next(_L("Using preset offsets"));
		for (v = 0; v < OffsetsCount; ++v)
			{
			MakeCleanCopy();
			SetField(&aFields[f], v, EValOffsetList);
			Load();
			}
		if (Verbose) test.Next(_L("Flipping single bits"));
		for (v = 0; v < aFields[f].size*8; ++v)
			{
			MakeCleanCopy();
			SetField(&aFields[f], 1<<v, EValXor);
			Load();
			}
		if (Verbose) test.Next(_L("Inverting"));
		MakeCleanCopy();
		SetField(&aFields[f], 0xffffffffu, EValXor);
		Load();

		// things that are offsets all go below, pointless on
		// narrow fields
		if (aFields[f].size == 4)
			{
			if (Verbose) test.Next(_L("Using filesize relative values"));
			for (v = FileSize-4; v <= FileSize+4; ++v)
				{
				MakeCleanCopy();
				SetField(&aFields[f], v, EValLiteral);
				Load();
				}
			if (Verbose) test.Next(_L("Using code-end relative values"));
			TInt codeend = CleanHeader->iCodeSize + CleanHeader->iCodeOffset;
			for (v = codeend-4; v <= codeend+4; ++v)
				{
				MakeCleanCopy();
				SetField(&aFields[f], v, EValLiteral);
				Load();
				}
			}
		}
	}


void FuzzFieldsRandomly(const SFuzzField* aFields, TInt aOffset)
	{
	Target = FileData + aOffset;

	TInt f = 0;
   	while (aFields[f].name)
		{
		test.Printf(_L("FIELD: %s ... (random)\n"), aFields[f].name);
		TInt v;
		for (v = 0; v < KRandomFieldIterations; ++v)
			{
			MakeCleanCopy();
			SetField(&aFields[f], 0, EValRandom);
			Load();
			}
		f++;
		}
	}


void FuzzBlockRandomly(TInt aOffset, TInt aSize)
	{
	SFuzzField field;
	field.size = 1;
	Target = FileData + aOffset;
	
	test.Printf(_L("FIELD: random words in data\n"));
	TInt v;
	for (v = 0; v < KRandomFieldIterations * 4; ++v)
		{
		MakeCleanCopy();
		field.offset = Rand(aSize);
		if (Verbose) test.Printf(_L("@ %d, "), field.offset);
		SetField(&field, 0, EValRandom);
		Load();
		}
	}


void FuzzFile(TBool aRandom)
	{
	TTime before, after;
	before.UniversalTime();
	LoadCleanFile();
	
	TFieldFuzzer FuzzFields = aRandom ? FuzzFieldsRandomly : FuzzFieldsDeterministically;

	// E32ImageHeader
	FuzzFields(HeaderFields, 0);

	if (CleanHeader->iCompressionType == KFormatNotCompressed)
		{
		// import table
		TInt offset = CleanHeader->iImportOffset;
		if (offset != 0)
			{
			FuzzFields(ImportSectionFields, offset);
			offset += sizeof(E32ImportSection);
			FuzzFields(ImportBlockFields, offset);
			offset += sizeof(E32ImportBlock);
			FuzzFields(ImportEntryFields, offset);
			}

		// code relocations
		offset = CleanHeader->iCodeRelocOffset;
		if (offset != 0)
			{
			FuzzFields(RelocSectionFields, offset);
			offset += sizeof(E32RelocSection);
			FuzzFields(RelocBlockFields, offset);
			offset += sizeof(E32RelocBlock);
			FuzzFields(RelocEntryFields, offset);
			}

		// data relocations
		offset = CleanHeader->iDataRelocOffset;
		if (offset != 0)
			{
			FuzzFields(RelocSectionFields, offset);
			offset += sizeof(E32RelocSection);
			FuzzFields(RelocBlockFields, offset);
			offset += sizeof(E32RelocBlock);
			FuzzFields(RelocEntryFields, offset);
			}

		// export table
		offset = CleanHeader->iExportDirOffset;
		if (offset != 0)
			{
			FuzzFields(ExportEntryFields, offset);
			}
		}
	else
		{
		if (aRandom)
			{
			// random bits of the compressed data
			FuzzBlockRandomly(CleanHeader->iCodeOffset, FileSize - CleanHeader->iCodeOffset);
			}
		else
			{
			// arbitrary bits of the compressed data
			FuzzFields(CompressedDataFields, CleanHeader->iCodeOffset);
			}
		}

	DoneFile();
	after.UniversalTime();
	TTimeIntervalSeconds interval;
	after.SecondsFrom(before, interval);
	test.Printf(_L("Took %d seconds\n"), interval.Int());
	}


void FuzzTruncateTo(TInt size)
	{
	OutFileSize = size - 4;
	if (Verbose) test.Printf(_L("%d "), OutFileSize);
	Load();
	OutFileSize = size - 1;
	if (Verbose) test.Printf(_L("%d "), OutFileSize);
	Load();
	if (size == FileSize)
		return;
	OutFileSize = size;
	if (Verbose) test.Printf(_L("%d "), OutFileSize);
	Load();
	OutFileSize = size + 1;
	if (Verbose) test.Printf(_L("%d "), OutFileSize);
	Load();
	OutFileSize = size + 4;
	if (Verbose) test.Printf(_L("%d "), OutFileSize);
	Load();
	}


void FuzzTruncate()
	{
	TTime before, after;
	before.UniversalTime();
	LoadCleanFile();

	FuzzTruncateTo(CleanHeader->iCodeOffset);
	if (CleanHeader->iCompressionType == KFormatNotCompressed)
		FuzzTruncateTo(CleanHeader->iCodeOffset+CleanHeader->iCodeSize);
	FuzzTruncateTo(FileSize);

	DoneFile();
	after.UniversalTime();
	TTimeIntervalSeconds interval;
	after.SecondsFrom(before, interval);
	test.Printf(_L("Took %d seconds\n"), interval.Int());
	}


void FuzzAllTestImages()
	{
	TInt i;
	Drive = InternalDrive;
	test.Next(_L("Fuzzing deterministically"));
	for (i=1; i<=KFuzzImages; ++i)
		{
		test.Next(_L("Next binary..."));
		test.Printf(_L("Fuzzing exe %d\n"), i);
		PrepareName(i, EFalse);
		FuzzFile(EFalse);
		if(i==5)
			continue; // DLL 5 doesn't exist because toolchain doesn't like DLLs with no exports
		test.Next(_L("Next binary..."));
		test.Printf(_L("Fuzzing dll %d\n"), i);
		PrepareName(i, ETrue);
		FuzzFile(EFalse);
		}
	Drive = RemovableDrive;
	test.Next(_L("Fuzzing deterministically on removable media"));
	for (i=1; i<=KFuzzImages; ++i)
		{
		test.Next(_L("Next binary..."));
		test.Printf(_L("Fuzzing exe %d\n"), i);
		PrepareName(i, EFalse);
		FuzzFile(EFalse);
		if(i==5)
			continue; // DLL 5 doesn't exist because toolchain doesn't like DLLs with no exports
		test.Next(_L("Next binary..."));
		test.Printf(_L("Fuzzing dll %d\n"), i);
		PrepareName(i, ETrue);
		FuzzFile(EFalse);
		}
	Drive = InternalDrive;
	test.Next(_L("Fuzzing by truncation"));
	for (i=1; i<=KFuzzImages; ++i)
		{
		test.Next(_L("Next binary..."));
		test.Printf(_L("Fuzzing exe %d\n"), i);
		PrepareName(i, EFalse);
		FuzzTruncate();
		if(i==5)
			continue; // DLL 5 doesn't exist because toolchain doesn't like DLLs with no exports
		test.Next(_L("Next binary..."));
		test.Printf(_L("Fuzzing dll %d\n"), i);
		PrepareName(i, ETrue);
		FuzzTruncate();
		}	
	Drive = RemovableDrive;
	test.Next(_L("Fuzzing by truncation on removable media"));
	for (i=1; i<=KFuzzImages; ++i)
		{
		test.Next(_L("Next binary..."));
		test.Printf(_L("Fuzzing exe %d\n"), i);
		PrepareName(i, EFalse);
		FuzzTruncate();
		if(i==5)
			continue; // DLL 5 doesn't exist because toolchain doesn't like DLLs with no exports
		test.Next(_L("Next binary..."));
		test.Printf(_L("Fuzzing dll %d\n"), i);
		PrepareName(i, ETrue);
		FuzzTruncate();
		}	
	test.Next(_L("Fuzzing randomly"));
	do
		{
		for (i=1; i<=KFuzzImages; ++i)
			{
			Drive = InternalDrive;
			test.Next(_L("Next binary..."));
			test.Printf(_L("Fuzzing exe %d\n"), i);
			PrepareName(i, EFalse);
			FuzzFile(ETrue);
			if(i==5)
				continue; // DLL 5 doesn't exist because toolchain doesn't like DLLs with no exports
			test.Next(_L("Next binary..."));
			test.Printf(_L("Fuzzing dll %d\n"), i);
			PrepareName(i, ETrue);
			FuzzFile(ETrue);
			Drive = RemovableDrive;
			test.Next(_L("Next binary..."));
			test.Printf(_L("Fuzzing exe %d on removable media\n"), i);
			PrepareName(i, EFalse);
			FuzzFile(ETrue);
			if(i==5)
				continue; // DLL 5 doesn't exist because toolchain doesn't like DLLs with no exports
			test.Next(_L("Next binary..."));
			test.Printf(_L("Fuzzing dll %d on removable media\n"), i);
			PrepareName(i, ETrue);
			FuzzFile(ETrue);
			}
		}
	while (Forever);
	}


void FuzzProvidedImage()
	{
	test.Printf(_L("Fuzzing file %S\n"), &Provided);
	PrepareProvidedName();
	Drive = InternalDrive;
	test.Next(_L("Fuzzing deterministically"));
	FuzzFile(EFalse);
	Drive = RemovableDrive;
	test.Next(_L("Fuzzing deterministically on removable media"));
	FuzzFile(EFalse);
	test.Next(_L("Fuzzing by truncation"));
	FuzzTruncate();
	Drive = RemovableDrive;
	test.Next(_L("Fuzzing by truncation on removable media"));
	FuzzTruncate();
	test.Next(_L("Fuzzing randomly"));
	do
		{
		Drive = InternalDrive;
		test.Next(_L("Internal drive"));
		FuzzFile(ETrue);
		Drive = RemovableDrive;
		test.Next(_L("Removable drive"));
		FuzzFile(ETrue);
		}
	while (Forever);
	}


GLDEF_C TInt E32Main()
	{
	// default to verbose unless the fasttest trace flag is on
	Verbose = (UserSvr::DebugMask(2)&0x00000002) == 0;

	TFileName cmd;
	User::CommandLine(cmd);
	TLex lex(cmd);
	FOREVER
		{
		lex.SkipSpace();
		if (lex.Eos())
			break;
		TChar next = lex.Peek();
		if (next == '-' || next == '/')
			{
			// option
			lex.Inc();
			switch(lex.Get())
				{
			case 'v':
				Verbose = ETrue;
				break;
			case 'q':
				Verbose = EFalse;
				break;
			case 'l':
				{
				// being used as a slave to load a DLL
				TPtrC libname(lex.NextToken());
				RLibrary l;
				RProcess::Rendezvous(KErrNone);
				l.Load(libname);
				return KErrNone;
				}
			case 's':
				// random seed
				lex.SkipSpace();
				test_KErrNone(lex.Val(Seed, EHex));
				test.Printf(_L("Using supplied random seed %08x\n"), Seed);
				break;
			case 'f':
				// run forever
				Forever = ETrue;
				break;
				}
			}
		else
			{
			// filename, at least i assume it is :)
			Provided.Copy(lex.NextToken());
			}
		}

	test.Title();
	test.Next(_L("Setup"));
	__UHEAP_MARK;
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();

	if (Seed == 0)
		{
		TTime time;
		time.UniversalTime();
		Seed = (TUint32)time.Int64();
		test.Printf(_L("Random seed is %08x\n"), Seed);
		}

	test_KErrNone(TheFs.Connect());
	test_TRAP(FileMan=CFileMan::NewL(TheFs));
	test_KErrNone(Timer.CreateLocal());
	test_TRAP(Hasher=CSHA1::NewL());
	HashDir.Append(TheFs.GetSystemDriveChar());
	HashDir.Append(KSysHash);
	TInt r = TheFs.MkDirAll(HashDir);
	test(r == KErrNone || r == KErrAlreadyExists);	

	// Find some interesting drives
	for (TInt driveno = EDriveA; driveno <= EDriveZ; ++driveno)
		{
		TDriveInfo di;
		test_KErrNone(TheFs.Drive(di, driveno));
		if (di.iType == EMediaNotPresent)
			continue;
		TChar drivechar;
		test_KErrNone(TheFs.DriveToChar(driveno, drivechar));
		if ((di.iDriveAtt & KDriveAttInternal) && InternalDrive == '?')
			InternalDrive = drivechar;
		else if ((di.iDriveAtt & KDriveAttRemovable) && RemovableDrive == '?')
			RemovableDrive = drivechar;
		else
			continue;

		TFileName fn;
		fn.Append(drivechar);
		fn.Append(KSysBin);
		TheFs.MkDirAll(fn);
		test(r == KErrNone || r == KErrAlreadyExists);	
		}
	test.Printf(_L("Using %c as internal drive, %c as removable\n"), (TUint)InternalDrive, (TUint)RemovableDrive);

	// Turn off evil lazy dll unloading
	RLoader l;
	test_KErrNone(l.Connect());
	test_KErrNone(l.CancelLazyDllUnload());
	l.Close();

	test.Start(_L("Fuzzing loader"));
	if (Provided.Length() == 0)
		FuzzAllTestImages();
	else
		FuzzProvidedImage();
	test.End();

	delete Hasher;
	Timer.Close();
	delete FileMan;
	TheFs.Close();
	test.Close();
	delete cleanup;
	__UHEAP_MARKEND;
	return KErrNone;
	}


