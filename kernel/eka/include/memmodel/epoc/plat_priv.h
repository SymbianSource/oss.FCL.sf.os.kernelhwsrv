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
// e32\include\memmodel\epoc\plat_priv.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __M32KERN_H__
#define __M32KERN_H__
#include <kernel/kern_priv.h>
#include <platform.h>
#include <e32def_private.h>

/** Hardware Variant Discriminator

@internalTechnology
*/
class THardwareVariant
	{
public:
	inline THardwareVariant();
	inline THardwareVariant(TUint aVariant);
	inline operator TUint();
	inline TBool IsIndependent();
	inline TBool IsCpu();
	inline TBool IsCompatibleWith(TUint aCpu, TUint aAsic, TUint aVMask);
private:
	inline TUint Layer();
	inline TUint Parent();
	inline TUint VMask();
private:
	TUint iVariant;
	};

/**
@internalTechnology
*/
inline THardwareVariant::THardwareVariant()
	{iVariant=0x01000000;}

/**
@internalTechnology
*/
inline THardwareVariant::THardwareVariant(TUint aVariant)
	{iVariant=aVariant;}

/**
@internalTechnology
*/
inline THardwareVariant::operator TUint()
	{return iVariant;}

/**
@internalTechnology
*/
inline TUint THardwareVariant::Layer()
	{return iVariant>>24;}

/**
@internalTechnology
*/
inline TUint THardwareVariant::Parent()
	{return (iVariant>>16)&0xff;}

/**
@internalTechnology
*/
inline TUint THardwareVariant::VMask()
	{return iVariant&0xffff;}

/**
@internalTechnology
*/
inline TBool THardwareVariant::IsIndependent()
	{return (Layer()<=3);}

/**
@internalTechnology
*/
inline TBool THardwareVariant::IsCpu()
	{return (Parent()==3);}

/**
@internalTechnology
*/
inline TBool THardwareVariant::IsCompatibleWith(TUint aCpu, TUint aAsic, TUint aVMask)
	{ return(Layer()<=3 || (Parent()==3 && Layer()==aCpu) ||
		(Layer()==aAsic && (VMask() & aVMask)!=0) );}


/** Functions/Data defined in layer 2 or below of the kernel and not available to layer 1.

@internalComponent
*/
class PP
	{
public:
	enum TPlatPanic
		{
		EInitialSystemTimeInvalid=0,
		EInvalidStartupReason=1,
		EIncorrectDllDataAddress=2,
		ENoDllDataChunk=3,
		EUnsupportedOldBinary=4,
    	};

	static void Panic(TPlatPanic aPanic);
	static void InitSuperPageFromRom(TLinAddr aRomHeader, TLinAddr aSuperPage);
public:
	static TInt RamDriveMaxSize;
	static TInt RamDriveRange;
	static TUint32 NanoWaitCal;
	static DChunk* TheRamDriveChunk;
	static TLinAddr RamDriveStartAddress;
	static TInt MaxUserThreadStack;
	static TInt UserThreadStackGuard;
	static TInt MaxStackSpacePerProcess;
	static TInt SupervisorThreadStackGuard;
	static TUint32 MonitorEntryPoint[3];
	static TLinAddr RomRootDirAddress;
public:
	};

extern "C" {
extern TLinAddr RomHeaderAddress;
}

/********************************************
 * Code segment
 ********************************************/

/**
@internalComponent
*/
struct SRamCodeInfo
	{
	TInt iCodeSize;
	TInt iTextSize;
	TLinAddr iCodeRunAddr;
	TLinAddr iCodeLoadAddr;
	TInt iDataSize;
	TInt iBssSize;
	TLinAddr iDataRunAddr;
	TLinAddr iDataLoadAddr;
	TInt iConstOffset; // not used
	TLinAddr iExportDir;
	TInt iExportDirCount;
	TLinAddr iExceptionDescriptor;
	};


class DEpocCodeSeg;

/**
@internalComponent
*/
class DEpocCodeSegMemory : public DBase
	{
public:
	static DEpocCodeSegMemory* New(DEpocCodeSeg* aCodeSeg);
	TInt Open();
	TInt Close();
protected:
	DEpocCodeSegMemory(DEpocCodeSeg* aCodeSeg);
public:
	TInt iAccessCount;
	SRamCodeInfo iRamInfo;
	DEpocCodeSeg* iCodeSeg;
	};


/**
@internalComponent
*/
class DEpocCodeSeg : public DCodeSeg
	{
public:
	virtual ~DEpocCodeSeg();
	void Destruct();
public:
	virtual void Info(TCodeSegCreateInfo& aInfo);
	virtual TLibraryFunction Lookup(TInt aOrdinal);
	virtual TInt GetMemoryInfo(TModuleMemoryInfo& aInfo, DProcess* aProcess);
	virtual TInt DoCreate(TCodeSegCreateInfo& aInfo, DProcess* aProcess);
	virtual void InitData();
	virtual TInt Loaded(TCodeSegCreateInfo& aInfo);
	virtual TInt DoCreateRam(TCodeSegCreateInfo& aInfo, DProcess* aProcess)=0;
	virtual TInt DoCreateXIP(DProcess* aProcess)=0;
public:
	inline SRamCodeInfo& RamInfo()
		{return *(SRamCodeInfo*)iInfo;}
	inline const TRomImageHeader& RomInfo()
		{return *(const TRomImageHeader*)iInfo;}
	void GetDataSizeAndBase(TInt& aTotalDataSizeOut, TLinAddr& aDataBaseOut);
public:
	TUint8 iXIP;		// TRUE for XIP ROM code
	const TAny* iInfo;	// pointer to TRomImageHeader or SRamCodeInfo
	DEpocCodeSegMemory* iMemory;
	TCodeSegLoaderCookieList* iLoaderCookie;
	};

/********************************************
 * Process control block
 ********************************************/

/**
@internalComponent
*/
class DEpocProcess : public DProcess
	{
public:
	virtual TInt AttachExistingCodeSeg(TProcessCreateInfo& aInfo);
	};

/**
@internalComponent
*/
inline const TRomHeader& TheRomHeader()
	{return *((const TRomHeader *)RomHeaderAddress);}

#endif
