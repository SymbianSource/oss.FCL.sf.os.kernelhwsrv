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
// e32\kernel\exstart.cpp
// 
//

#include "plat_priv.h"
#include <e32uid.h>

enum TStartFault
	{
	EIniCreateSecondary1=0,
	EIniCreateSecondary2=1,
	EIniCreateSecondary3=2,
	};

void Fault(TStartFault aFault)
	{
	Kern::Fault("EXSTART",aFault);
	}

void StartSecondary(TAny*);
GLDEF_D TDfc StartSecondaryDfc(&StartSecondary, NULL, Kern::SvMsgQue(), 0);

#ifdef __EPOC32__

_LIT(KSecondaryFullPathName,"z:\\sys\\bin\\efile.exe");

TInt SecondaryInfo(TProcessCreateInfo& aInfo)
//
// Provide loader information for the secondary process from the ROM
//
	{
    TRomEntry *pE=(TRomEntry*)Epoc::RomHeader().iSecondaryFile;
	TRomImageHeader& secondary=*(TRomImageHeader*)pE->iAddressLin;
	Epoc::RomProcessInfo(aInfo, secondary);
	aInfo.iFileName=KSecondaryFullPathName;
	aInfo.iRootNameOffset=11;
	aInfo.iRootNameLength=9;
	aInfo.iExtOffset=16;
	return KErrNone;
	}

#else

#include <property.h>
#include <emulator.h>

_LIT(KSecondaryFullPathName,"EFile.exe");

TInt SecondaryInfo(TProcessCreateInfo& aInfo)
//
// Provide loader information for the secondary process
// We know this is efile.exe, so interrogate this image file
//
	{
	memclr(&aInfo.iUids, sizeof(TProcessCreateInfo)-sizeof(aInfo.iFileName));
	aInfo.iFileName=KSecondaryFullPathName;
	aInfo.iRootNameOffset=0;
	aInfo.iRootNameLength=9;
	aInfo.iExtOffset=5;

	//TFileName name;
	TUint16 wName[KMaxFileName];
	TUint16 wFileName[KMaxFileName];
	TUint8* ptr8=(TUint8*)aInfo.iFileName.Ptr();
	for(int i=0;i<aInfo.iFileName.Length();i++)
		wFileName[i] = (TUint16)*ptr8++;
	TPtrC ptrFile((TUint8*)wFileName,aInfo.iFileName.Length()<<1);
	TPtr ptrName((TUint8*)wName,KMaxFileName<<1);
	Property::MapFilename(ptrName, ptrFile);

	//Property::MapFilename(name, aInfo.iFileName);
	wName[ptrName.Length()>>1] = '\0';
	Emulator::RImageFile pefile;
	
	TInt r = pefile.Open((LPCTSTR)wName);
	if (r == KErrNone)
		{
		pefile.GetInfo(aInfo);
		pefile.Close();
		}
	return r;
	}

#endif

void StartSecondary(TAny*)
	{
//
// First we need to setup the creation info for the secondary process.
//
	__KTRACE_OPT(KBOOT,Kern::Printf("Starting secondary"));

	TProcessCreateInfo info;
	TInt r = SecondaryInfo(info);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EIniCreateSecondary2));

	__KTRACE_OPT(KBOOT, Kern::Printf("FS entrypoint %08x",info.iFileEntryPoint));
	__KTRACE_OPT(KBOOT, Kern::Printf("FS data size %08x",info.iDataSize));
	__KTRACE_OPT(KBOOT, Kern::Printf("FS bss size %08x",info.iBssSize));

	DProcess* pP=NULL;
	HBuf* cmd=NULL;
	r=Kern::ProcessCreate(pP, info, cmd, NULL);
	__ASSERT_ALWAYS(r==KErrNone,Fault(EIniCreateSecondary3));
 	__KTRACE_OPT(KBOOT,Kern::Printf("Secondary created"));

	pP->Loaded(info);

	pP->iFlags = KProcessFlagSystemPermanent|KProcessFlagSystemCritical;
	pP->FirstThread()->iFlags |= KThreadFlagSystemPermanent;

	// Start the secondary
	NKern::LockSystem();
	pP->Resume();
	NKern::UnlockSystem();
	__KTRACE_OPT(KBOOT,Kern::Printf("Resumed secondary"));
	}

DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("Launching secondary"));
	StartSecondaryDfc.Enque();
	return KErrNone;
	}

