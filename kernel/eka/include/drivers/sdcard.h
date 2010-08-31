// Copyright (c) 1999-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// SD-specific extensions of generic MMC classes adhering to SD Physical layer simplified spec v2.0
// 
//

/**
 @file
 @internalComponent
*/

#ifndef __SDCARD_H__
#define __SDCARD_H__

#include <drivers/mmc.h>

class DSDStack;

enum TSDSessionTypeEnum
	{
	// empty.
	};

const TUint KSDMinCustomSession = KMMCMinCustomSession + 1024;

const TUint32 KSDBusWidth1				 = 0x00;	
const TUint32 KSDBusWidth4				 = 0x02;
const TUint32 KSDStatusBlockLength		 = 0x40;

const TUint32 KSDSCRLength				 = 0x08;
const TUint32 KSDSwitchFuncLength		 = 0x40;
const TUint32 KSDCheckFunctionHighSpeed	 = 0x00FFFF01;
const TUint32 KSDSwitchFunctionHighSpeed = 0x80FFFF01;

const TUint32 KSDCardIsSDCard			 = KBit16;	// KMMCardFirstCustomFlag
const TUint32 KSDCardIsCorrupt           = KBit17;
const TUint32 KSDCardFirstCustomFlag	 = KBit24;

const TUint   KSDDTClk25MHz				 = 25000; //25000KHz
const TUint   KSDDTClk50MHz				 = 50000; //50000KHz

class TSDCSD : public TCSD
	{
public:
	inline TSDCSD(const TCSD& aCSD);

	inline TBool SDEraseBlkEn() const;
	inline TBool SDSectorSize() const;
	inline TBool SDWPGrpSize() const;
	};

class TSDCard : public TMMCard
	{
public:
	TSDCard();
	inline TBool IsSDCard() const;
	inline TUint32 ProtectedAreaSize() const;
	inline void SetProtectedAreaSize(TUint32 aPAS);
	inline TUint8 GetAUSize() const;
	inline void SetAUSize(TUint8 aAU);
	inline TInt DeviceSize() const;
	virtual TUint32 PreferredWriteGroupLength() const;
	virtual TInt GetFormatInfo(TLDFormatInfo& aFormatInfo) const;
	virtual TUint32 MinEraseSectorSize() const;
	virtual TUint32 EraseSectorSize() const;
	virtual TInt GetEraseInfo(TMMCEraseInfo& anEraseInfo) const;
	virtual TInt MaxReadBlLen() const;
	virtual TInt MaxWriteBlLen() const;
	virtual TInt64 DeviceSize64() const;
	enum {KPARootDirEndUnknown = 0};
	inline TUint32 PARootDirEnd() const;
	inline void SetPARootDirEnd(TUint32 aPARootDirEnd);
	virtual TUint MaxTranSpeedInKilohertz() const;
	inline void RegisterClient();
	inline void DeregisterClient();
	inline TBool ClientsRegistered();
private:
	TUint32 iProtectedAreaSize;
	TUint32 iPARootDirEnd;
	TUint8	iAUSize;
	TUint8 iPad[3];
	TUint32 iClientCountSD;
	TUint32 iSpare[3];
	};

class TSDCardArray : public TMMCardArray
	{
public:
	inline TSDCardArray(DSDStack* aOwningStack);
	IMPORT_C virtual TInt AllocCards();

	inline TSDCard& Card(TUint aCardNumber) const;
	inline TSDCard& NewCard(TUint aCardNumber) const;
	void AddCardSDMode(TUint aCardNumber,const TUint8* aCID,TRCA* aNewRCA);
	TInt StoreRCAIfUnique(TUint aCardNumber,TRCA& anRCA);
	IMPORT_C virtual void DeclareCardAsGone(TUint aCardNumber);
	};

enum TSDAppCmd
	{
	ESDACmdSetBusWidth = 6,
	ESDACmdSDStatus = 13,
	ESDACmdSendNumWrBlocks = 22,
	ESDACmdSetWrBlkEraseCount = 23,
	ESDACmdSDAppOpCond = 41,
	ESDACmdSetClrCardDetect = 42,
	ESDACmdSendSCR = 51
	};

enum TSDSpecificCmd
	{
	ESDCmdSendRelativeAddress = 3,
	ESDCmdSwitchFunction = 6,
	ESDCmdSendIfCond = 8
	};

class DSDSession : public DMMCSession
	{
public:
	inline DSDSession(const TMMCCallBack& aCallBack);

	static void FillAppCommandDesc(TMMCCommandDesc& aDesc, TSDAppCmd aCmd);
	static void FillAppCommandDesc(TMMCCommandDesc& aDesc, TSDAppCmd aCmd, TMMCArgument aArg);

	static void FillSdSpecificCommandDesc(TMMCCommandDesc& aDesc, TSDSpecificCmd aCmd);
	static void FillSdSpecificCommandDesc(TMMCCommandDesc& aDesc, TSDSpecificCmd aCmd, TMMCArgument aArg);

protected:
	// not implemented.  No SD specific macros
//	virtual TMMCSMSTFunc GetMacro(TInt aSessNum) const;

private:
	static void FillAppCommandDesc(TMMCCommandDesc& aDesc);
	static void FillSdSpecificCommandDesc(TMMCCommandDesc& aDesc);
	};

const TInt KSDMaxMBWRetries = 1;
const TUint32 KSDACMD22BlockLen = 4;

NONSHARABLE_CLASS(DAddressCard) : public DMMCStack::MAddressCard
	{
public:
	DAddressCard(DSDStack& aStack);
	virtual void AddressCard(TInt aCardNumber);
private:
	DSDStack& iStack;
	};


class DSDStack : public DMMCStack
	{
public:
	inline DSDStack(TInt aBus, DMMCSocket* aSocket);
	
	IMPORT_C virtual TInt Init();
	IMPORT_C virtual TMMCErr AcquireStackSM();
	IMPORT_C virtual TMMCErr CIMReadWriteBlocksSM();
	IMPORT_C virtual DMMCSession* AllocSession(const TMMCCallBack& aCallBack) const;
	
	virtual void AddressCard(TInt aCardNumber) = 0;

	inline TSDCardArray& CardArray() const;

protected:
	IMPORT_C virtual TMMCErr InitStackAfterUnlockSM();

	static TMMCErr InitialiseMemoryCardSMST(TAny* aStackP);
	TMMCErr InitialiseMemoryCardSM();

	static TMMCErr ConfigureMemoryCardSMST(TAny* aStackP);
	TMMCErr ConfigureMemoryCardSM();

	static TMMCErr CIMReadWriteMemoryBlocksSMST(TAny* aStackP);

	static TMMCErr BaseModifyCardCapabilitySMST(TAny* aStackP);
    IMPORT_C virtual TMMCErr ModifyCardCapabilitySM();

	static TMMCErr SwitchToHighSpeedModeSMST(TAny* aStackP);
    TMMCErr SwitchToHighSpeedModeSM();

private:
	TInt iSpare;
protected:
	enum TSDCardType {ESDCardTypeUnknown, ESDCardTypeIsMMC, ESDCardTypeIsSD};
	TSDCardType iCxCardType; 			// Used when detecting whether an SD Memory card is present.
private:
	TUint8 iACMD22[KSDACMD22BlockLen];

private:
    //
    // Dummy functions to maintain binary compatibility
    IMPORT_C virtual void Dummy1();
    IMPORT_C virtual void Dummy2();

protected:
	IMPORT_C virtual void GetInterface(TInterfaceId aInterfaceId, MInterface*& aInterfacePtr);

public: 
    IMPORT_C virtual DSDStack::TSDCardType CardType(TInt aSocket, TInt aCardNumber);

private:    
	DAddressCard* iAddressCard;
    //
    // Reserved members to maintain binary compatibility

    TInt iReserved[67];
	};

#include <drivers/sdcard.inl>

#endif	// #ifndef __SDCARD_H__
