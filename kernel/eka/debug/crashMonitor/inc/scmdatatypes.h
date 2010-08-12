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
// Definitions for the data types the SCM stores to flash
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//


#ifndef __SCMDATATYPES_H_INCLUDED__
#define __SCMDATATYPES_H_INCLUDED__


#include <e32rom.h>

#include <scmbytestreamutil.h>
#include <scmtrace.h>

/**
 @file
 @internalComponent
 */

namespace Debug 
{
	/** SCM Data Types Major Number */
	static const TInt KSCMDataTypesMajorVersion = 1;
	
	/** SCM Data Types Minor Number */
	static const TInt KSCMDataTypesMinorVersion = 0;
	
	/** SCM Data Types Build Number */
	static const TInt KSCMDataTypesBuildNumber = 0;

	/**
	  Specifies the type of a code segment.
	  @see TCodeSegListEntry
	  */
	enum TCodeSegType
	    {
	    EUnknownCodeSegType = 0, /**< Signifies an unknown code segment type. */
	    EExeCodeSegType = 1,     /**< Signifies a code segment belonging to an executable. */
	    EDllCodeSegType = 2      /**< Signifies a code segment belonging to a library. */
	    };
	
	
	/**
	  Used for storing the contents of a 32 bit register
	  */
	typedef TUint32 TRegisterValue32;
	
	/**
	  Structure containing information about the state of the registers when a
	  hardware exception occurred
	  */
	class TRmdArmExcInfo
	    {
	public:
	    /** Enumeration detailing the types of exception which may occur. */
	    enum TExceptionType
	        {
	        /** Enumerator signifying that a prefetch abort error has occurred. */
	        EPrefetchAbort = 0,
	        /** Enumerator signifying that a data abort error has occurred. */
	        EDataAbort = 1,
	        /** Enumerator signifying that an undefined instruction error has occurred. */
	        EUndef =2
	        };

	    /** Value of CPSR. */
	    TRegisterValue32 iCpsr;
	    /** Type of exception which has occurred. */
	    TExceptionType iExcCode;
	    /** Value of R13 supervisor mode banked register. */
	    TRegisterValue32 iR13Svc;
	    /** Value of user mode register R4. */
	    TRegisterValue32 iR4;
	    /** Value of user mode register R5. */
	    TRegisterValue32 iR5;
	    /** Value of user mode register R6. */
	    TRegisterValue32 iR6;
	    /** Value of user mode register R7. */
	    TRegisterValue32 iR7;
	    /** Value of user mode register R8. */
	    TRegisterValue32 iR8;
	    /** Value of user mode register R9. */
	    TRegisterValue32 iR9;
	    /** Value of user mode register R10. */
	    TRegisterValue32 iR10;
	    /** Value of user mode register R11. */
	    TRegisterValue32 iR11;
	    /** Value of R14 supervisor mode banked register. */
	    TRegisterValue32 iR14Svc;
	    /** Address which caused exception (System Control Coprocessor Fault Address Register) */
	    TRegisterValue32 iFaultAddress;
	    /** Value of System Control Coprocessor Fault Status Register. */
	    TRegisterValue32 iFaultStatus;
	    /** Value of SPSR supervisor mode banked register. */
	    TRegisterValue32 iSpsrSvc;
	    /** Value of user mode register R13. */
	    TRegisterValue32 iR13;
	    /** Value of user mode register R14. */
	    TRegisterValue32 iR14;
	    /** Value of user mode register R0. */
	    TRegisterValue32 iR0;
	    /** Value of user mode register R1. */
	    TRegisterValue32 iR1;
	    /** Value of user mode register R2. */
	    TRegisterValue32 iR2;
	    /** Value of user mode register R3. */
	    TRegisterValue32 iR3;
	    /** Value of user mode register R12. */
	    TRegisterValue32 iR12;
	    /** Value of user mode register R15, points to instruction which caused exception. */
	    TRegisterValue32 iR15;
	    };
	/**
	 * This enum defines the type of struct we are dealing with when we
	 * are serialising/deserialising
	 */
	enum SCMStructId
		{
		ESCMFirst,		
		ESCMOffsetsHeader,
		ESCMTCrashInfo,
		ESCMProcessData,
		ESCMThreadData,
		ESCMThreadStack,
		ESCMRegisterValue,
		ESCMRegisterSet,
		ESCMMemory,
		ESCMCodeSegSet,
		ESCMCodeSeg,
		ESCMRawData,
		ESCMTraceData,
		ESCMLocks,
		ESCMKernelHeap,
		ESCMVariantData,
		ESCMRomHeader,
		ESCMLast
		};

	/**
	 * This class represents the header at the start of a crash log
	 * describing the size of the crash log and minimal location 
	 * information
	 */
	class TCrashOffsetsHeader : public MByteStreamSerializable
		{
		public:
			
			static const TInt KSCMCrashOffsetsMaxSize = 20 * sizeof(TUint32) + sizeof(TUint16);
			
			enum TCrashHeaderVersion 
				{ 
				EChVersion1 = 1 
				};
			
			TCrashOffsetsHeader();
			
			//From MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;
			
			TBool operator == (const TCrashOffsetsHeader& aOther) const;
				
			SCMStructId iId;
			TCrashHeaderVersion iVersion;		
			
			//These next members are offsets to the crash data in the log
			TUint32 iCTFullRegOffset; 
			TUint32 iCTUsrStkOffset;
			TUint32 iCTSvrStkOffset;
			TUint32 iCPMetaOffset;
			TUint32 iCTMetaOffset;		
			TUint32 iCPCodeSegOffset;
			TUint32 iSysUsrStkOffset;
			TUint32 iSysSvrStkOffset;
			TUint32 iSysUsrRegOffset;
			TUint32 iSysSvrRegOffset;
			TUint32 iTLstOffset;
			TUint32 iPLstOffset;
			TUint32 iSysCodeSegOffset;
			TUint32 iExcStkOffset;
			TUint32 iTraceOffset;
			TUint32 iScmLocksOffset;
			TUint32 iKernelHeapOffset;
			TUint32 iVarSpecInfOffset;
			TUint32 iRomInfoOffset;
			
			TUint32 iSpare1;
			TUint32 iSpare2;
			TUint32 iSpare3;
			TUint32 iSpare4;
			TUint32 iSpare5;
			TUint32 iSpare6;				
			
		};

	/**
	 * This class stores meta data for a given crash
	 */
	class TCrashInfoHeader : public MByteStreamSerializable
		{
		public:				
			
			static const TInt KMaxCatSize = 80;
			static const TInt KSCMCrashInfoMaxSize = 76 + KMaxCatSize;
			
			enum TCrashInfoHeaderVersion { ECiVersion1 = 1 };
			TCrashInfoHeader();
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;
			
			TBool operator == (const TCrashInfoHeader& aOther) const;	
		
			
			SCMStructId iId;
			TCrashInfoHeaderVersion iVersion;
			TInt32 iLogSize;	
			TInt32 iFlashAlign;
			TInt32 iCachedWriterSize;
			TUint64 iPid;
			TUint64 iTid;
			TInt32 iExitType;
			TInt32 iExitReason;	
			TInt32 iExcCode;
			TInt64 iCrashTime;	
			TInt32 iCrashId;
			TInt32 iFlashBlockSize;
			TInt32 iFlashPartitionSize;			
			TVersion iSCMDataTypesVersion;
			TUint32 iCategorySize;
			TBuf8<KMaxCatSize> iCategory;	
			
			TInt32 iSpare1;
			TInt32 iSpare2;
		};
	
	/**
	 * This class is used for a raw memory dump. It will always be preceded by a TMemoryDump.
	 * 
	 * Note: This class contains a TPtr8 to store the data
	 * name. Due to us not being able to allocate memory when the system
	 * is down, we assume that the memory to which this points is owned 
	 * by someone else. It is constructed to point to NULL. To make use
	 * of this, before serialising the data, ensure to set it to point to
	 * the location required. Equally, when derserialising, ensure to allocate
	 * a descriptor of required bytes (determined from TMemoryDump) and set it to iData, otherwise
	 * the name will be ignored upon reading. 
	 */
	class TRawData : public MByteStreamSerializable
		{		
		public:
			
			//Note this doesnt include the data, as this number is not determinable
			static const TInt KSCMRawDataMaxSize = 2 * sizeof(TUint32) + sizeof(TUint16);    
			
			enum TTRawDataVersion { ETRawData1 = 1 };
			TRawData();
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;
			
			TInt Deserialize(TInt aStartPos, TByteStreamReader& aReader);
		
			SCMStructId iId;
			TTRawDataVersion iVersion;
			
			TInt32 iLength;
			TPtr8 iData;
		
		};
		
	/**
	 * This class stores meta data for a given process
	 */
	class TProcessData : public MByteStreamSerializable
		{
		public:
			
			static const TInt KSCMProcessDataMaxSize = sizeof(TUint16) + sizeof(TUint64) + 2 * sizeof(TUint32) + KMaxProcessName;
			
			enum TProcessDataVersion { EProcData1 = 1 };
			TProcessData();
						
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;
			
			SCMStructId iId;
			TProcessDataVersion iVersion;				
					
			TUint64 iPid;			
			TUint32 iNamesize; //Length of process filename in bytes
			TInt32 iPriority;
			
			TBuf8<KMaxProcessName> iName;
			
			TInt32 iSpare1;
			TInt32 iSpare2;	
			TInt32 iSpare3;	
		};
	
	/**
	 * This class stores meta data for a given thread
	 */
	class TThreadData : public MByteStreamSerializable
		{
		public:
			
			static const TInt KMaxThreadName = KMaxProcessName;					
			static const TInt KSCMThreadDataMaxSize = sizeof(TUint32) + sizeof(TUint16) + 11 * sizeof(TUint32) + 2 * sizeof(TUint64) + KMaxThreadName;
			
			enum TThreadDataVersion { EThreadData1 = 1 };
			TThreadData();
						
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;
			
			SCMStructId iId;
			TInt32 iVersion;
			TInt32 iPriority;
			TUint64 iTid;
			TUint64 iOwnerId;
			TInt32 iSvcSP;
			TInt32 iSvcStack;
			TInt32 iSvcStacksize;
			TInt32 iUsrSP;
			TInt32 iUsrStack;
			TInt32 iUsrStacksize;
			TUint32 iNamesize;
			TUint32 iLastCpu;
			TInt32 iSvcHeap;
			TInt32 iSvcHeapSize;
			
			TBuf8<KMaxThreadName> iName;
			
			TInt32 iSpare1;
			TInt32 iSpare2;		
		};
	
	/**
	 * This class stores the stack for a given thread. The type (user or supervisor) is given
	 * by iStackType. It will be followed by a TMemoryDump containing the stack
	 */
	class TThreadStack : public MByteStreamSerializable	
		{
		public:
						
			static const TInt KSCMThreadStackMaxSize = sizeof(TUint32) + sizeof(TUint16) + 2 * sizeof(TUint32) + sizeof(TUint64);
			
			enum TThreadStackVersion { EStackVersion1 = 1 };
			
			enum TThreadStackType
				{
				EUsrStack,
				ESvrStack,
				EIRQStack,
				EFIQStack,
				ELast
				};
			
			TThreadStack();
						
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const; 
			
			SCMStructId iId;
			TThreadStackVersion iVersion;
			TThreadStackType iStackType; 
			TUint64 iThreadId;				
			TUint32 iStackSize;
			
			TInt32 iSpare1;
			TInt32 iSpare2;
		};
	
	/**
	 * Stores the value of a given register and tells you its type
	 */
	class TRegisterValue : public MByteStreamSerializable 
		{
		public:
			
			static const TInt KSCMRegisterValueMaxSize = sizeof(TUint32) + sizeof(TUint16) + 2 * sizeof(TUint8) + sizeof(TUint16) + sizeof(TUint32) + 2 * sizeof(TUint64);
			
			TRegisterValue();
			
			enum TRegisterValueVersion { ERegisterValueVersion1 = 1 };
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;
			
			SCMStructId iId;
			TRegisterValueVersion iVersion;		
			TInt64 iOwnId;  
			
	        /** 
	         * Same as Sym32_reginfod::rd_id
	         *  if rid_class == ESYM_REG_CORE
	         *  	rd_id is one of rm_debug_api.h::TFunctionalityRegister
	         *	else
	         *		rd_id is CoProcessor number, eg 15 for ARM CP15  
	         */	           	      	     	       
			TUint32 iType;
			
			TUint8 iClass; //Same as Sym32_reginfod::rid_class
			TUint16	iSubId; //used for coprocessors
			
			/** 
			 * Same as Sym32_reginfod::rd_repre
			 * 		ESYM_REG_8 == 0, 
			 * 		ESYM_REG_16 == 1, 
			 * 		ESYM_REG_32 == 2, 
			 * 		ESYM_REG_64 == 3 
			 */				
			TUint8	iSize; //register size
			
			// Register value			   
			union
			{
				// Value of an 8 bit register 
				TUint8		iValue8;
	
				// Value of a 16 bit register  
				TUint16		iValue16;
	
				// Value of a 32 bit register 
				TUint32		iValue32;
	
				// Value of a 64 bit register 
				TUint64		iValue64;
			};
		

		};
	
	/**
	 * This class is a header for our register set. 
	 */
	class TRegisterSet : public MByteStreamSerializable	
		{
		public:
			
			static const TInt KSCMRegisterSetMaxSize = sizeof(TUint32) + sizeof(TUint16) + sizeof(TUint32);
			
			TRegisterSet();
			
			enum TRegisterSetVersion { ETRegisterSetVersion1 = 1 };
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;
			
			TRegisterSetVersion iVersion;
			SCMStructId iId;
			TInt32 iNumRegisters;
		};
	
	/**
	 * This class describes a memory dump and will be followed by a TRawData 
	 */
	class TMemoryDump : public MByteStreamSerializable
		{
		public:			
						
			static const TInt KSCMMemDumpMaxSize = sizeof(TUint32) + sizeof(TUint16) + 2 * sizeof(TUint32) + sizeof(TUint64);
			
			enum TMemDumpVersionVersion {	EMemDumpVersion1 = 1	};
			
			TMemoryDump();
			
			//From MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;
			
			SCMStructId iId;	
			TMemDumpVersionVersion iVersion; 
			TUint32 iStartAddress;
			
			TInt64 iPid;			
			TInt32 iLength;
		};
		
	/**
	 * class to represent a set of code segs corrosponding to a given process
	 */
	class TCodeSegmentSet : public MByteStreamSerializable
		{
		public:
			
			static const TInt KSCMCodeSegSetMaxSize = KMaxProcessName;
			
			TCodeSegmentSet();
			
			enum TCodeSegmentSetVersion { ETCodeSegmentSetVersion1 = 1 };
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;		
		
			TInt GetMaxSize() const;
			
			SCMStructId iId;								/**< Id that uniquely identifies this data */
			TCodeSegmentSetVersion iVersion;		/**< Version of this data */
			TInt32 iNumSegs;   						/**< The number of code segments following this struct that relate to this process ID */			
			TInt64 iPid;							/**< Process Id that owns the following code segments */		
		};
	
	/**
	 * class to represent a code segment in the SCM Log
	 */
	class TCodeSegment : public MByteStreamSerializable
		{
		public:			
			
			static const TInt KMaxSegmentNameSize = KMaxProcessName;
			
			//50 is the sum of the size of members that get serialised
			static const TInt KSCMCodeSegMaxSize = 50 + KMaxSegmentNameSize;
			
			TCodeSegment();
			
			enum TCodeSegmentVersion { ETCodeSegmentVersion1 = 1 };
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;	
			
			TInt GetMaxSize() const;			
		
			SCMStructId iId;								/**< Id that uniquely identifies this data */
			TCodeSegmentVersion iVersion;			/**< Version of this data */		
			TCodeSegType iCodeSegType;				/**< @see TCodeSegType */
			TModuleMemoryInfo iCodeSegMemInfo;      /**< holds the memory info for this code segment (8 TUint32's)*/			
			TBool  iXip;							/**< If this code segment is XIP */
			TInt32 iNameLength;						/**< Length of the name of this code segment name */		
			TBuf8<KMaxSegmentNameSize> iName;				/**< Name of this code segment */
		
		};
	
	/**
	 * This class represents a trace dump in the crash log. It will be immediately followed
	 * in the crash log by a TRawData structure
	 */
	class TTraceDump : public MByteStreamSerializable
		{
		public:
					
			static const TInt KSCMTraceDumpMaxSize = sizeof(TUint32) + sizeof(TUint16) + 2 * sizeof(TUint32);
			
			TTraceDump();
			
			enum TTraceDumpVersion { ETraceDumpVersion1 = 1 };
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;
			
			SCMStructId iId;						/**< Id that uniquely identifies this data */
			TTraceDumpVersion iVersion;		/**< Version of this data */	
			TInt32 iSizeOfMemory;			/**< Size of the trace data that will be dumped in the following Memory Dump */
			TInt32 iNumberOfParts;			/**< Number of TRawData structs that will follow */
			
		};

	/**
	 * This represents variant specific data in the crash log. It will be followed
	 * immediately by a TRawData that contains the data
	 */
	class TVariantSpecificData : public MByteStreamSerializable
		{
		public:
			
			//sizeof(TUint32) + sizeof(TUint16) + sizeof(TUint32)
			static const TInt KSCMVarSpecMaxSize = 10;
			
			TVariantSpecificData();
			
			enum TVariantSpecificDataVersion { EVariantSpecificDataVersion1 = 1 };
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;
			
			SCMStructId iId;						/**< Id that uniquely identifies this data */
			TVariantSpecificDataVersion iVersion;		/**< Version of this data */
			TUint32 iSize;				/**< Size of the raw data (ie the var spec info) that will follow */
		};
	
	/**
	 * This represents the Rom Header Data in the crash Log
	 */
	class TRomHeaderData : public MByteStreamSerializable
		{
		public:
						
			static const TInt KSCMRomHdrMaxSize = sizeof(TUint32) + sizeof(TUint16) + sizeof(TUint64) + 2 * sizeof(TUint8) +sizeof(TUint16);
			
			TRomHeaderData();
			
			enum TRomHeaderDataVersion { ERomHeaderDataVersion1 = 1 };
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
			virtual TInt GetSize() const;
			
			SCMStructId iId;							/**< Id that uniquely identifies this data */
			TRomHeaderDataVersion iVersion;			/**< Version of this data */
			TUint8 iMajorVersion;					/**< Major Version of ROM build */
			TUint8 iMinorVersion;					/**< Minor Version of ROM build */			
			TUint16 iBuildNumber;					/**< ROM build number */
			TUint64 iTime;							/**< Build time of ROM in miliseconds */
		};	

	/**
	 * This represents the kernel lock info in the crash log
	 */
	class TSCMLockData : public MByteStreamSerializable
		{
		public:
			
			static const TInt KSCMLockDataMaxSize = sizeof(TUint32) + 3 * sizeof(TUint16);
			
			TSCMLockData();
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);			
			virtual TInt GetSize() const;
						
			TInt MutexHoldCount() const;
			void SetMutexHoldCount(TInt aMutexHoldCount);
			
			TInt MutexThreadWaitCount() const;			
			void SetMutexThreadWaitCount(TInt aMutexThreadWaitCount);
			
			TInt LockCount() const;
			void SetLockCount(TInt aLockCount);
			
			TBool operator == (const TSCMLockData& aOther) const;
			TBool operator != (const TSCMLockData& aOther) const;				

		private:		
			SCMStructId iId;  
			TInt iMutexHoldCount;			// if mutex is valid number of holds on the mutex from current thread
			TInt iMutexThreadWaitCount;		// if mutex is valid number of threads waiting on the mutex
			TInt iLockCount; 	
			
		};
		
	/**
	 * TScmChecksum class is used to provide a level of sanity checking for the data it processes
	 * the check sum produced is not intended to be computationally unique
	 * This implementation has been chosen as there are restrictions in the data may only be available 
	 * in small chunks and the entire data may not be unable to be read (ie comm port implementation)
	 * These restrictions rule out the use of more sophisticated checksums that produce a checksum value for 
	 * an entire block of data 
	 */
	class TScmChecksum : MByteStreamSerializable
		{
	public:
		TScmChecksum();
		
		void ChecksumBlock(const TUint8* aData, TUint aLen);
		void ChecksumBlock(const TDesC8& aDes);		
		TBool operator == (const TScmChecksum& aOther) const;
		TBool operator != (const TScmChecksum& aOther) const;
		void Reset();
		
		// from MByteStreamSerializable
		TInt Serialize(TByteStreamWriter& aWriter);
		TInt Deserialize(TByteStreamReader& aReader);
	 	TInt GetSize() const;
		
	private:
		/** Total length of all data in bytes*/
		TUint32 iLength;	
		/** Sum of all bytes*/
		TUint32 iSum;
		/** Count of Bytes with value 0*/
		TUint32 iZeroCount;
		
		};
	
	/** class to describe a flash block */
	class SCMCrashBlockEntry
		{
	public:
		
		SCMCrashBlockEntry()
			: iBlockNumber(0)
			, iBlockOffset(0)
			, iBlockSize(0) 
			, iNext(NULL)
			{
		
			}
		
		SCMCrashBlockEntry(TInt aBlockNumber, TInt aBlockOffset, TInt aBlockSize)
			: iBlockNumber(aBlockNumber)
			, iBlockOffset(aBlockOffset)
			, iBlockSize(aBlockSize) 
			, iNext(NULL)
			{
		
			}

		/** The offset in bytes to this block from start of flash*/
		TInt iBlockNumber;
		/** The offset in bytes to this block from start of flash*/
		TInt iBlockOffset;
		/** the size of the flash block in bytes */
		TInt iBlockSize;	
		/** pointer to next in list*/
		SCMCrashBlockEntry* iNext;
		
		};

	/** Because of limitations in flash memory driver available to the 
	 *  system crash monitor - this class is used to describe the locations
	 *  on flash where crashes will be stored
	 *  we store 1 crash per block of flash ! This eliminates the 
	 *  need to for the scmonitor to hold memory required for bufering write data to flash
	 * 	class used to describe locations (typically in flash) 
	 * 	where
	 * 	holds a linked list of SCMCrashBlockEntry which describe an area we can write to
	 */
	class SCMMultiCrashInfo
		{
	public:

		/** constructor */
		SCMMultiCrashInfo();
		
		/** destructor */
		~SCMMultiCrashInfo();
		
		/** add a pointer to a block to the list - takes ownership of block */ 
		void AddBlock(SCMCrashBlockEntry* aBlockEntry);
		
		/** GetNextBlock returns NULL when no more blocks */
		SCMCrashBlockEntry* GetNextBlock();
		
		/** sets current block to first in list */
		void Reset();
		
		/** clear all entries in the list */
		void ClearList();
		
		
	private:
		SCMCrashBlockEntry* iFirstBlock; 
		SCMCrashBlockEntry* iCurrentBlock;
		TInt iSpare;
		};
	
	/**
	 * This constant gives us the maximum size of the Core Crash Header which consists of the Crash Info, the Offsets
	 * Header and the Core Registers
	 */
	static const TInt KMaxCoreHeaderSize = TCrashInfoHeader::KSCMCrashInfoMaxSize 					//Crash Info - always there
										+ TCrashOffsetsHeader::KSCMCrashOffsetsMaxSize				//offsets header - optional
										+ TRegisterSet::KSCMRegisterSetMaxSize					//The crash context - always there
										+ TRegisterValue::KSCMRegisterValueMaxSize * 37;			//could be up to 37 register values

	}

#endif		//__SCMDATATYPES_H_INCLUDED__

//eof scmdatatypes.h
