// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Description: This is the header file for the PCI driver test , so far implemented 
//				only on the  Naviengine platform

#ifndef __TPCI_TEST_H
#define __TPCI_TEST_H

#ifndef __KERNEL_MODE__
#define __E32TEST_EXTENSION__
#include <e32test.h>
 #include <e32def_private.h>
#endif // __KERNEL_MODE__

_LIT(KPciLdd, "d_pci.ldd");
_LIT(KPciLddFactory, "PCI_test_factory");
_LIT(KPciTest, "PCI Test LDD");

/**
Test driver op-codes
*/
enum TPciTestCmd
	{
	EGetTestInfo,
	EAccessConfigSpace,
	EAccessMemorySpace,
	EOpenPciDChunk,
	EOpenPciPlatHwChunk,
	EOpenPciMappedChunk,
	EOpenPciWindowChunk,
	ERunUnitTests
	};

/**
Identifies a PCI Function (device) on the system
*/
struct TPciDevice
	{
	TPciDevice()
		:iVendorId(0xFFFFFFFF), iDeviceId(0xFFFFFFFF), iInstance(0) {}

	TPciDevice(TUint aVendorId, TUint aDeviceId, TInt aInstance=0)
		:iVendorId(aVendorId), iDeviceId(aDeviceId), iInstance(aInstance) {}

	TUint iVendorId;
	TUint iDeviceId;
	TInt iInstance; ///< Unit to open (there could be multiple devices on system)
	};

/**
Used to send chunk size and recieve
PCI address
*/
struct TPciChunkCreateInfo
	{
	TPciChunkCreateInfo()
		:iSize(0), iPciAddress(NULL)
		{
		}

	TPciChunkCreateInfo(TInt aSize, TUint& aPciAddress, TRequestStatus* aStatus=NULL)
		:iSize(aSize), iPciAddress(&aPciAddress), iStatus(aStatus)
		{
		}
	TInt iSize;
	TUint* iPciAddress;
	TRequestStatus* iStatus;
	};	

/**
Information about the PSL required by the
user side test
*/
struct TPciTestInfo
	{
	TPciDevice iDevice; ///< Probe for this

	/**
	Supplies the necessary information to test Read, Write, and
	Modify for a word of PCI memory or configuration space
	*/
	struct TAddrSpaceTest
		{
		TAddrSpaceTest()
			:iOffset(0), iExpectedValue(0), iReadOnlyMask(0)
			{}

		TAddrSpaceTest(TUint aOffset, TUint aExpectedValue, TUint aReadOnlyMask)
			:iOffset(aOffset), iExpectedValue(aExpectedValue), iReadOnlyMask(aReadOnlyMask)
			{}

		/**
		Returns a specified sub byte, or word from the whole dword
		*/
		inline TUint Expected(TInt aBitWidth, TInt aExtraOffset) const
			{
			//the right shift required to get field to bit 0
			const TInt shift = 8 *((aExtraOffset + iOffset) % 4);
			
			const TUint mask = 0xFFFFFFFF >> (32-aBitWidth);
			return (iExpectedValue >> shift) & mask;
			}

		const TUint iOffset;
		const TUint iExpectedValue; ///< The initial value of word
		const TUint iReadOnlyMask; ///< Mask of unwritable bits
		//Future work, memory spaces should state a bar index
		};


	TAddrSpaceTest iCfgSpaceRead;
	TAddrSpaceTest iCfgSpaceWrite;

	TUint iMemSpaceIndex; ///< Memory space to select
	TAddrSpaceTest iMemSpaceRead;
	TAddrSpaceTest iMemSpaceWrite;

	TInt iNumberOfBars; ///< Number of simultaneous mappings into PCI space
	};

class RPci;
class TAddrSpace;
/**
This class encapsulates all the various read/write/and modify commands
that can be carried out on a PCI memory space. The command is stored user
side, and then executed on kernel side when KRun() is called.
*/
class TUserPciSpace
	{
public:
	TUserPciSpace()
		:iPci(NULL), iOperation(EInvalid), iBitWidth(0), iOffset(0),
		iWriteValue(0), iClearMask(0), iSetMask(0)
	{}
	TUserPciSpace(RPci& aPci);
	
	/**
	Perform the encapsulated read/write/or modify
	@note Only run on kernel side
	*/
	TUint KRun(TAddrSpace& aAddrSpace);
	
	/**
	Clone method is required so that multiple threads may
	have their own copy of a TUserPciSpace (without knowing
	its runtime type)
	*/
	virtual TUserPciSpace* Clone() const = 0;

	TUint Read(TInt aBitWidth, TUint aOffset)
		{
		iOffset = aOffset;
		iOperation = ERead;
		iBitWidth = aBitWidth;

		return Call();
		}

	void Write(TInt aBitWidth, TUint aOffset, TUint aValue)
		{
		iOffset = aOffset;
		iOperation = EWrite;
		iBitWidth = aBitWidth;
		
		iWriteValue = aValue;
		Call();
		}

	void Modify(TInt aBitWidth, TUint aOffset, TUint aClearMask, TUint aSetMask)
		{
		iOffset = aOffset;
		iOperation = EModify;
		iBitWidth = aBitWidth;

		iClearMask = aClearMask;
		iSetMask = aSetMask;
		Call();
		}

protected:
	/**
	Makes a request to iPci and passes a copy of this object to
	the kernel side.
	*/
	virtual TUint Call() =0;

	enum TOperation {EInvalid, ERead, EWrite, EModify};

	/**
	Pointer to a PCI device handle
	*/
	RPci* iPci;

	TOperation iOperation; //!< Type of access to perform
	TInt iBitWidth;
	
	TUint iOffset;
	TUint32 iWriteValue;
	TUint32 iClearMask;
	TUint32 iSetMask;
	};

/**
Grants access to a PCI device's (identified
by aPci) config space from user side
*/
class TUserConfigSpace : public TUserPciSpace
	{
public:
	TUserConfigSpace()
		:TUserPciSpace()
		{}
	TUserConfigSpace(RPci& aPci);

	virtual TUserPciSpace* Clone() const;
private:
	TUint Call();
	};

/**
Grants access to some region of a PCI
device's memory space. A PCI device(or function)
may have up to 8 distinct memory spaces
*/
class TUserMemorySpace : public TUserPciSpace
	{
public:
	TUserMemorySpace()
		:TUserPciSpace(), iBarIndex(-1)
		{}

	TUserMemorySpace(RPci& aPci, TInt aBarIndex);	

	virtual TUserPciSpace* Clone() const;
	
	inline TInt BarIndex() {return iBarIndex;}

private:
	TUint Call();

	TInt iBarIndex; ///< Each PCI function may have up to 8 memory spaces
	};

#endif //__TPCI_TEST_H
