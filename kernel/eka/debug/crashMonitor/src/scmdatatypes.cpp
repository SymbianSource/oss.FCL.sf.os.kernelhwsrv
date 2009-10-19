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
// e32\debug\crashMonitor\src\scmdatatypes.cpp
// Core dump server - Data Types for System Crash
// 
//

/**
 @file
 @internalTechnology
*/
#include <scmtrace.h>
#include <scmdatatypes.h>
#include <scmbytestreamutil.h>

namespace Debug 
	{	
	/**
	 * TCrashOffsetsHeader implementation
	 * @internal technology
	 */

	/**
	 * TCrashOffsetsHeader Constructor
	 */
	TCrashOffsetsHeader::TCrashOffsetsHeader():
		iId(ESCMOffsetsHeader),	
		iVersion(EChVersion1),	
		iCTFullRegOffset(0),
		iCTUsrStkOffset(0),
		iCTSvrStkOffset(0),
		iCPMetaOffset(0),
		iCTMetaOffset(0),
		iCPCodeSegOffset(0),
		iSysUsrStkOffset(0),
		iSysSvrStkOffset(0),
		iSysUsrRegOffset(0),
		iSysSvrRegOffset(0),
		iTLstOffset(0),
		iPLstOffset(0),	
		iSysCodeSegOffset(0),
		iExcStkOffset(0),		
		iTraceOffset(0),
		iScmLocksOffset(0),
		iKernelHeapOffset(0),
		iVarSpecInfOffset(0),
		iRomInfoOffset(0),	
		iSpare1(0),
		iSpare2(0),
		iSpare3(0),
		iSpare4(0),	
		iSpare5(0),	
		iSpare6(0)
		{	
		}	

	/**
	 * Writes this class to the specified byte stream
	 * @param aWriter Byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TCrashOffsetsHeader::Serialize(TByteStreamWriter& aWriter)
		{						
		TInt startPos = aWriter.CurrentPosition();
		
		if(iId != ESCMOffsetsHeader)
			{
			CLTRACE("TCrashOffsetsHeader::Serialize Corrupt ID");
			return KErrCorrupt;
			}

		// ID saved first 
		aWriter.WriteInt(iId);		 				// 4		
		aWriter.WriteShort((TUint16) iVersion);		// 2

		if(iVersion == EChVersion1)
			{
			// write data v1 format	
			aWriter.WriteInt(iCTFullRegOffset); 	// 4 
			aWriter.WriteInt(iCTUsrStkOffset);  	// 4
			aWriter.WriteInt(iCTSvrStkOffset);  	// 4		
			aWriter.WriteInt(iCPMetaOffset);	   	// 4
			aWriter.WriteInt(iCTMetaOffset);		// 4
			aWriter.WriteInt(iCPCodeSegOffset);	// 4
			aWriter.WriteInt(iSysUsrStkOffset);	// 4
			aWriter.WriteInt(iSysSvrStkOffset);	// 4
			aWriter.WriteInt(iSysUsrRegOffset);	// 4
			aWriter.WriteInt(iSysSvrRegOffset);	// 4
			aWriter.WriteInt(iTLstOffset);		// 4
			aWriter.WriteInt(iPLstOffset);		// 4
			aWriter.WriteInt(iSysCodeSegOffset);	// 4
			aWriter.WriteInt(iExcStkOffset);		// 4
			aWriter.WriteInt(iTraceOffset);		// 4
			aWriter.WriteInt(iScmLocksOffset);	// 4
			aWriter.WriteInt(iKernelHeapOffset);		// 4			
			aWriter.WriteInt(iVarSpecInfOffset);		// 4
			aWriter.WriteInt(iRomInfoOffset);		// 4	
			}
		else
			{
			CLTRACE("TCrashOffsetsHeader::Serialize Unsupported version");
			}
		
		TInt sizeWritten = aWriter.CurrentPosition() - startPos;
		if(sizeWritten != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TCrashOffsetsHeader serialization size error");	
			return KErrCorrupt;
			}

		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TCrashOffsetsHeader::Deserialize(TByteStreamReader& aReader)	
		{		
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();	// 4
		if(iId != ESCMOffsetsHeader)
			{
			CLTRACE("TCrashOffsetsHeader::Deserialize Corrupt ID");
			return KErrCorrupt;
			}
		
		iVersion = (TCrashHeaderVersion)aReader.ReadShort();  //2
		
		if(iVersion == EChVersion1)
			{	
			iCTFullRegOffset = aReader.ReadInt(); // 4
			iCTUsrStkOffset = aReader.ReadInt();	// 4
			iCTSvrStkOffset = aReader.ReadInt();	// 4
			iCPMetaOffset = aReader.ReadInt();	// 4		
			iCTMetaOffset = aReader.ReadInt();	// 4
			iCPCodeSegOffset = aReader.ReadInt();	// 4
			iSysUsrStkOffset = aReader.ReadInt(); // 4
			iSysSvrStkOffset = aReader.ReadInt(); // 4
			iSysUsrRegOffset = aReader.ReadInt(); // 4
			iSysSvrRegOffset = aReader.ReadInt(); // 4
			iTLstOffset = aReader.ReadInt();		// 4
			iPLstOffset = aReader.ReadInt();		// 4
			iSysCodeSegOffset = aReader.ReadInt();// 4
			iExcStkOffset = aReader.ReadInt();	// 4
			iTraceOffset = aReader.ReadInt();		// 4
			iScmLocksOffset = aReader.ReadInt();	// 4			
			iKernelHeapOffset = aReader.ReadInt();		// 4
			iVarSpecInfOffset = aReader.ReadInt();		// 4
			iRomInfoOffset = aReader.ReadInt();		// 4						
			}
		else
			{
			CLTRACE("TCrashOffsetsHeader::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TCrashOffsetsHeader deserialization size error");		
			return KErrCorrupt;
			}
		return KErrNone;
		}

	/**
	 * Get the externalised size of this class
	 * @return TInt the Size
	 */
	TInt TCrashOffsetsHeader::GetSize() const
		{
		// return the size in bytes that this struct uses to serialize - deserialize itself
		if(iVersion == EChVersion1)
			{
			return 82;
			}
		else
			{
			CLTRACE("TCrashOffsetsHeader::GetSize Unsupported version");			
			return KErrNotSupported;
			}
		}
	
	TBool TCrashOffsetsHeader::operator == (const TCrashOffsetsHeader& aOther) const
		{
		return (iId ==  aOther.iId && 	
				iVersion == aOther.iVersion &&	
				iCTFullRegOffset == aOther.iCTFullRegOffset &&			
				iCTUsrStkOffset == aOther.iCTUsrStkOffset &&
				iCTSvrStkOffset == aOther.iCTSvrStkOffset &&
				iCPMetaOffset == aOther.iCPMetaOffset &&
				iCTMetaOffset == aOther.iCTMetaOffset &&		
				iSysUsrStkOffset == aOther.iSysUsrStkOffset &&
				iSysSvrStkOffset == aOther.iSysSvrStkOffset &&
				iSysUsrRegOffset == aOther.iSysUsrRegOffset &&
				iSysSvrRegOffset == aOther.iSysSvrRegOffset &&
				iTLstOffset == aOther.iTLstOffset &&
				iPLstOffset == aOther.iPLstOffset &&	
				iExcStkOffset == aOther.iExcStkOffset &&	
				iCPCodeSegOffset == aOther.iCPCodeSegOffset &&
				iSysCodeSegOffset == aOther.iSysCodeSegOffset &&
				iTraceOffset == aOther.iTraceOffset &&
				iScmLocksOffset == aOther.iScmLocksOffset &&
				iVarSpecInfOffset == aOther.iVarSpecInfOffset &&
				iKernelHeapOffset == aOther.iKernelHeapOffset &&
				iRomInfoOffset == aOther.iRomInfoOffset);
		}
	
	
	/**
	 * TCrashInfoHeader implementation
	 * @internal technology
	 */
	
	/**
	 * TCrashInfoHeader constructor
	 */
	TCrashInfoHeader::TCrashInfoHeader():
		iId(ESCMTCrashInfo),
		iVersion(ECiVersion1),
		iLogSize(0),
		iFlashAlign(0),
		iCachedWriterSize(0),
		iPid(0),
		iTid(0),
		iExitType(0),
		iExitReason(0),
		iExcCode(0),	
		iCrashTime(0),
		iCrashId(0),
		iFlashBlockSize(0),
		iFlashPartitionSize(0),
		iCategorySize(0),
		iSpare1(0),
		iSpare2(0)
		{		
		TVersion ver(KSCMDataTypesMajorVersion, KSCMDataTypesMinorVersion, KSCMDataTypesBuildNumber);
		iSCMDataTypesVersion = ver;
		}

	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TCrashInfoHeader::Serialize(TByteStreamWriter& aWriter)
		{	
		
		//CLTRACE("TCrashInfoHeader::Serialize");
		TInt startPos = aWriter.CurrentPosition();
		
		// ID saved first 
		aWriter.WriteInt(iId);		 				// 4		
		if(iId != ESCMTCrashInfo)
			{
			CLTRACE("TCrashInfoHeader::Serialize Corrupt ID");
			return KErrCorrupt;
			}
		
		aWriter.WriteShort((TUint16) iVersion);			// 2

		if(iVersion == ECiVersion1)
			{
			// write data v1 format
			aWriter.WriteInt(iLogSize);					// 4
			aWriter.WriteByte((TUint8)iFlashAlign);  	// 1
			aWriter.WriteByte((TUint8)iCachedWriterSize);		// 1		
			aWriter.WriteInt64(iPid);					// 8
			aWriter.WriteInt64(iTid);					// 8
			aWriter.WriteInt(iExitType);				// 4
			aWriter.WriteInt(iExitReason);				// 4
			aWriter.WriteInt(iExcCode);					// 4
			aWriter.WriteInt64(iCrashTime);				// 8
			aWriter.WriteInt(iCrashId);					// 4
			aWriter.WriteInt(iFlashBlockSize);			// 4
			aWriter.WriteInt(iFlashPartitionSize);			// 4
			aWriter.WriteInt(iSCMDataTypesVersion.iMajor);  // 4
			aWriter.WriteInt(iSCMDataTypesVersion.iMinor);  // 4
			aWriter.WriteInt(iSCMDataTypesVersion.iBuild);  // 4
			
			if(iCategory.Ptr())
				{
				aWriter.WriteInt(iCategory.Length());	// 4		
				for(TInt cnt = 0; cnt < iCategory.Length(); cnt++)
					{
					aWriter.WriteByte(iCategory[cnt]);  //iCategorySize bytes
					}
				}
			else
				{
				aWriter.WriteInt(0);
				}
			}
		else
			{
			CLTRACE("TCrashInfoHeader::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE2("TCrashInfoHeader::Serialize serialization size error. Wrote [%d] but expected [%d]", endPos - startPos, GetSize());
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TCrashInfoHeader::Deserialize(TByteStreamReader& aReader)	
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();
		if(iId != ESCMTCrashInfo)
			{
			CLTRACE("TCrashInfoHeader::Deserialize Corrupt ID");
			return KErrCorrupt;
			}
		
		iVersion = (TCrashInfoHeaderVersion)aReader.ReadShort();			// 2

		if(iVersion == ECiVersion1)
			{
			// read data v1 format
			iLogSize = aReader.ReadInt(); 			 	    // 4
			iFlashAlign = aReader.ReadByte();  				// 1
			iCachedWriterSize = aReader.ReadByte();			// 1
			iPid = aReader.ReadInt64();						// 8
			iTid = aReader.ReadInt64();						// 8
			iExitType = aReader.ReadInt();					// 4
			iExitReason = aReader.ReadInt();				// 4
			iExcCode = aReader.ReadInt();					// 4
			iCrashTime = aReader.ReadInt64();				// 8
			iCrashId = aReader.ReadInt();					// 4
			iFlashBlockSize = aReader.ReadInt(); 			// 4	
			iFlashPartitionSize = aReader.ReadInt(); 		// 4
			iSCMDataTypesVersion.iMajor = aReader.ReadInt(); 		// 4
			iSCMDataTypesVersion.iMinor = aReader.ReadInt(); 		// 4
			iSCMDataTypesVersion.iBuild = aReader.ReadInt(); 		// 4			
			

			iCategorySize = aReader.ReadInt(); 				// 4
			
			if(iCategory.MaxLength() >= (TInt)iCategorySize)
				{
				iCategory.SetLength(0);
				
				for(TUint cnt = 0; cnt < iCategorySize; cnt++)
					{
					iCategory.Append(aReader.ReadByte());		//iCategorySize bytes
					} 
				}
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TCrashInfoHeader::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{					
			// error between actual size & real size in data
			CLTRACE2("TCrashInfoHeader::Deserialize serialization size error. Read [%d] but expected [%d]", endPos - startPos, GetSize());			
			iId = ESCMLast;	//unrecognised header
			
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TCrashInfoHeader::GetSize() const
		{
		if(iVersion == ECiVersion1)
			{
			return 76 + iCategory.Length();
			}
		else
			{
			CLTRACE("TCrashInfoHeader::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
		
	TBool TCrashInfoHeader::operator == (const TCrashInfoHeader& aOther) const
		{
		return (
			iId == aOther.iId &&
			iVersion == aOther.iVersion &&
			iLogSize == aOther.iLogSize &&	
			iFlashAlign == aOther.iFlashAlign &&
			iCachedWriterSize == aOther.iCachedWriterSize &&
			iPid == aOther.iPid &&
			iTid == aOther.iTid &&
			iExitType == aOther.iExitType &&
			iExitReason == aOther.iExitReason &&
			iCrashTime == aOther.iCrashTime &&
			iCategorySize == aOther.iCategorySize &&
			iCategory == aOther.iCategory &&
			iSCMDataTypesVersion.iMajor == aOther.iSCMDataTypesVersion.iMajor &&
			iSCMDataTypesVersion.iMinor == aOther.iSCMDataTypesVersion.iMinor &&
			iSCMDataTypesVersion.iBuild == aOther.iSCMDataTypesVersion.iBuild);
		}	

	/**
	 * TThreadStack implementation
	 * @internal technology
	 */
	
	
	/**
	 * TThreadStack Constructor
	 */
	TThreadStack::TThreadStack():
		iId(ESCMThreadStack),
		iVersion(EStackVersion1),
		iStackType(ELast),
		iThreadId(0),
		iStackSize(0),		
		iSpare1(0),
		iSpare2(0)
		{		
		}
	
	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TThreadStack::Serialize(TByteStreamWriter& aWriter)
		{
		TInt startPos = aWriter.CurrentPosition();
		
		if(iId != ESCMThreadStack)
			{
			CLTRACE("TThreadStack::Serialize failed - corrupt ID");
			return KErrCorrupt;
			}
		
		aWriter.WriteInt(iId);							// 4		
		aWriter.WriteShort((TUint16) iVersion);			// 2

		if(iVersion == EStackVersion1)
			{
			// write data v1 format	
			aWriter.WriteInt(iStackType);		 		// 4		
			aWriter.WriteInt64(iThreadId); 			 	// 8
			aWriter.WriteInt(iStackSize);  				// 4
			}
		else
			{
			CLTRACE("TThreadStack::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TThreadStack::Serialize serialization size error");
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TThreadStack::GetSize() const
		{
		if(iVersion == EStackVersion1)
			{
			return 22;
			}
		else
			{
			CLTRACE("TThreadStack::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TThreadStack::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();			// 4
		if(iId != ESCMThreadStack)
			{
			CLTRACE("TThreadStack::Deserialize failed Corrupt ID");
			return KErrCorrupt;
			}
		
		iVersion = (TThreadStackVersion)aReader.ReadShort();			// 2

		if(iVersion == EStackVersion1)
			{
			// read data v1 format	
			iStackType = (TThreadStackType)aReader.ReadInt();		// 4
			iThreadId = aReader.ReadInt64();	// 8
			iStackSize = aReader.ReadInt();		// 4

			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TThreadStack::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			iId = ESCMLast;	//unrecognised header
			
			// error between actual size & real size in data
			CLTRACE("TThreadStack::Deserialize serialization size error");	
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * TRegisterValue implementation
	 * @internal technology
	 */
	
	/**
	 * TRegisterValue Constructor
	 */
	TRegisterValue::TRegisterValue():
		iId(ESCMRegisterValue),
		iVersion(ERegisterValueVersion1),
		iOwnId(0),
		iType(0),
		iClass(0), //core reg by default
		iSubId(0),		
		iSize(2) //default for core registers
		{
		}
	
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */	 
	TInt TRegisterValue::GetSize() const
		{
		if(iVersion == ERegisterValueVersion1)
			{
			TInt baseSize = 22;
			
			//variant for union
			if(iSize == 0)
				{
				return baseSize + 1;
				}
			else if(iSize == 1)
				{
				return baseSize + 2;
				}
			else if(iSize == 2)
				{
				return baseSize + 4;
				}
			else if(iSize == 3)
				{
				return baseSize + 8;
				}
			else
				{
				CLTRACE("TRegisterValue::GetSize() Corrupt size");
				return 0;
				}
			}
		else
			{
			CLTRACE("TRegisterValue::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	/**
	 * Writes this classes data to the specified byte stream 
	 * @param aWriter byte stream to use
	 * @return One of the OS wide codes
	 */	 
	TInt TRegisterValue::Serialize(TByteStreamWriter& aWriter)
		{
		TInt startPos = aWriter.CurrentPosition();
				
		// ID saved first
		if(iId != ESCMRegisterValue)
			{
			CLTRACE("TRegisterValue::Serialize Error - Corrupt ID");
			return KErrCorrupt;
			}
		
		aWriter.WriteInt(iId);							// 4
		aWriter.WriteShort((TUint16) iVersion);			// 2

		if(iVersion == ERegisterValueVersion1)
			{
			// write data v1 format	
			aWriter.WriteInt64(iOwnId);		 				// 8
			aWriter.WriteInt(iType);						// 4
			aWriter.WriteByte(iClass);						// 1
			aWriter.WriteShort(iSubId);						// 2
			aWriter.WriteByte(iSize);						// 1
			
			//variant for union
			if(iSize == 0)
				{
				aWriter.WriteByte(iValue8);
				}
			else if(iSize == 1)
				{
				aWriter.WriteShort(iValue16);
				}
			else if(iSize == 2)
				{
				aWriter.WriteInt(iValue32);
				}
			else if(iSize == 3)
				{
				aWriter.WriteInt64(iValue64);
				}
			else
				{
				CLTRACE("TRegisterValue::Serialize() Corrupt size");
				return KErrCorrupt;
				}
			}
		else
			{
			CLTRACE("TRegisterValue::Serialize Unsupported version");
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TRegisterValue::Serialize serialization size error");	
			return KErrCorrupt;
			}
		
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return One of the OS wide codes
	 */	 
	TInt TRegisterValue::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();
		if(iId != ESCMRegisterValue)
			{
			CLTRACE("TRegisterValue::Deserialize() ERROR Corrupt ID");			
			return KErrCorrupt;
			}
		
		iVersion = (TRegisterValueVersion)aReader.ReadShort();			// 2

		if(iVersion == ERegisterValueVersion1)
			{
			// read data v1 format	
			iOwnId = aReader.ReadInt64();		 				// 8
			iType = aReader.ReadInt();  						// 4
			iClass = aReader.ReadByte();						// 1
			iSubId = aReader.ReadShort();						// 2
			iSize = aReader.ReadByte();							// 1
			
			//variant for union
			if(iSize == 0)
				{
				iValue8 = aReader.ReadByte();				
				}
			else if(iSize == 1)
				{
				iValue16 = aReader.ReadShort();				
				}
			else if(iSize == 2)
				{
				iValue32 = aReader.ReadInt();
				}
			else if(iSize == 3)
				{
				iValue64 = aReader.ReadInt64();
				}
			else
				{
				CLTRACE("TRegisterValue::Serialize() Corrupt size");
				return KErrCorrupt;
				}
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TRegisterValue::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			iId = ESCMLast;	//unrecognised header
			
			// error between actual size & real size in data
			CLTRACE("TRegisterValue::Deserialize serialization size error");	
			return KErrCorrupt;
			}
		
		return KErrNone;
		}
	
	/**
	 * TRegisterSet implementation
	 * @internal technology
	 */
	
	/**
	 * TRegisterSet Constructor
	 */
	TRegisterSet::TRegisterSet():
		iVersion(ETRegisterSetVersion1),
		iId(ESCMRegisterSet),		
		iNumRegisters(0)
		{

		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TRegisterSet::GetSize() const
		{
		if(iVersion == ETRegisterSetVersion1)
			{
			return 10;
			}
		else
			{
			CLTRACE("TRegisterSet::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TRegisterSet::Serialize(TByteStreamWriter& aWriter)
		{
		TInt startPos = aWriter.CurrentPosition();
				
		// ID saved first
		if(iId != ESCMRegisterSet)
			{
			CLTRACE("TRegisterSet::Serialize Error - Corrupt ID");
			return KErrCorrupt;
			}
		
		aWriter.WriteInt(iId);							// 4
		aWriter.WriteShort((TUint16) iVersion);			// 2

		if(iVersion == ETRegisterSetVersion1)
			{
			// write data v1 format
			aWriter.WriteInt(iNumRegisters);				// 4
			}
		else
			{
			CLTRACE("TRegisterSet::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TRegisterSet::Serialize serialization size error");
			return KErrCorrupt;
			}
		
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TRegisterSet::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();
		if(iId != ESCMRegisterSet)
			{
			CLTRACE("TRegisterSet::Deserialize() ERROR Corrupt ID");			
			return KErrCorrupt;
			}
		
		iVersion = (TRegisterSetVersion)aReader.ReadShort();			// 2

		if(iVersion == ETRegisterSetVersion1)
			{
			// read data v1 format	
			iNumRegisters = aReader.ReadInt();		 				// 8
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TRegisterSet::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			iId = ESCMLast;	//unrecognised header
			
			// error between actual size & real size in data
			CLTRACE("TRegisterSet::Deserialize serialization size error");	
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * TCodeSegmentSet implementation
	 * @internal technology
	 */
	
	/**
	 * TCodeSegmentSet Constructor
	 */
	TCodeSegmentSet::TCodeSegmentSet():
		iId(ESCMCodeSegSet),
		iVersion(ETCodeSegmentSetVersion1),
		iNumSegs(0),
		iPid(0)		
		{
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TCodeSegmentSet::GetSize() const
		{
		if(iVersion == ETCodeSegmentSetVersion1)
			{
			return 18;
			}
		else
			{
			CLTRACE("TCodeSegmentSet::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	/**
	 * This returns the largest size this class can be
	 */
	TInt TCodeSegmentSet::GetMaxSize() const
		{
		return 18;
		}
	
	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TCodeSegmentSet::Serialize(TByteStreamWriter& aWriter)
		{
		TInt startPos = aWriter.CurrentPosition();
				
		// ID saved first
		if(iId != ESCMCodeSegSet)
			{
			CLTRACE("TCodeSegmentSet::Serialize Error - Corrupt ID");
			return KErrCorrupt;
			}
		
		aWriter.WriteInt(iId);							// 4
		if(iId != ESCMCodeSegSet)
			{
			CLTRACE("TCodeSegmentSet::Serialize Corrupt ID");
			return KErrCorrupt;
			}
		
		aWriter.WriteShort((TUint16) iVersion);			// 2

		if(iVersion == ETCodeSegmentSetVersion1)
			{
			// write data v1 format
			aWriter.WriteInt(iNumSegs);				// 4
			aWriter.WriteInt64(iPid);				// 8
			}
		else
			{
			CLTRACE("TCodeSegmentSet::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TCodeSegmentSet::Serialize serialization size error");	
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TCodeSegmentSet::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();
		if(iId != ESCMCodeSegSet)
			{
			CLTRACE("TCodeSegmentSet::Deserialize() ERROR Corrupt ID");			
			return KErrCorrupt;
			}
		
		iVersion = (TCodeSegmentSetVersion)aReader.ReadShort();			// 2

		if(iVersion == ETCodeSegmentSetVersion1)
			{
			// read data v1 format	
			iNumSegs = aReader.ReadInt();    				// 4
			iPid = aReader.ReadInt64();		 				// 8
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TRegisterSet::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			iId = ESCMLast;	//unrecognised header
			
			// error between actual size & real size in data
			CLTRACE("TRegisterSet::Deserialize serialization size error");	
			return KErrCorrupt;
			}
		
		return KErrNone;
		}	
	
	
	/**
	 * TCodeSegment implementation
	 * @internal technology
	 */
	
	/**
	 * TCodeSegment constructor
	 */
	TCodeSegment::TCodeSegment():
		iId(ESCMCodeSeg),
		iVersion(ETCodeSegmentVersion1),			
		iCodeSegType(EUnknownCodeSegType),		
		iXip(0),					
		iNameLength(0)		
		{
		}

	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TCodeSegment::Serialize(TByteStreamWriter& aWriter)
		{	
		TInt startPos = aWriter.CurrentPosition();
		
		// ID saved first 
		aWriter.WriteInt(iId);		 				// 4		
		if(iId != ESCMCodeSeg)
			{
			CLTRACE("TCodeSegment::Serialize Error - Corrupt ID");
			return KErrCorrupt;
			}
		
		aWriter.WriteShort((TUint16) iVersion);			// 2

		if(iVersion == ETCodeSegmentVersion1)
			{
			// write data v1 format
			aWriter.WriteInt(iCodeSegType);
			aWriter.WriteInt(iCodeSegMemInfo.iCodeBase);
			aWriter.WriteInt(iCodeSegMemInfo.iCodeSize);
			aWriter.WriteInt(iCodeSegMemInfo.iConstDataBase);
			aWriter.WriteInt(iCodeSegMemInfo.iConstDataSize);
			aWriter.WriteInt(iCodeSegMemInfo.iInitialisedDataBase);
			aWriter.WriteInt(iCodeSegMemInfo.iInitialisedDataSize);
			aWriter.WriteInt(iCodeSegMemInfo.iUninitialisedDataBase);
			aWriter.WriteInt(iCodeSegMemInfo.iUninitialisedDataSize);
			aWriter.WriteInt(iXip);
			
			if(iName.Ptr())
				{
				aWriter.WriteInt(iName.Length());	// 4		
				for(TInt cnt = 0; cnt < iName.Length(); cnt++)
					{
					aWriter.WriteByte(iName[cnt]);  //iCategorySize bytes
					}
				}
			else
				{
				aWriter.WriteInt(0);
				}
			}
		else
			{
			CLTRACE("TCodeSegment::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE2("TCodeSegment::Serialize serialization size error. Wrote [%d] but expected [%d]", endPos - startPos, GetSize());
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TCodeSegment::Deserialize(TByteStreamReader& aReader)	
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();
		if(iId != ESCMCodeSeg)
			{
			CLTRACE("TCodeSegment::Deserialize Corrupt ID");
			return KErrCorrupt;
			}
		
		iVersion = (TCodeSegmentVersion)aReader.ReadShort();			// 2

		if(iVersion == ETCodeSegmentVersion1)
			{
			// read data v1 format			
			iCodeSegType = (TCodeSegType)aReader.ReadInt();
			iCodeSegMemInfo.iCodeBase = aReader.ReadInt();
			iCodeSegMemInfo.iCodeSize = aReader.ReadInt();
			iCodeSegMemInfo.iConstDataBase = aReader.ReadInt();
			iCodeSegMemInfo.iConstDataSize = aReader.ReadInt();
			iCodeSegMemInfo.iInitialisedDataBase = aReader.ReadInt();
			iCodeSegMemInfo.iInitialisedDataSize = aReader.ReadInt();
			iCodeSegMemInfo.iUninitialisedDataBase = aReader.ReadInt();
			iCodeSegMemInfo.iUninitialisedDataSize = aReader.ReadInt();
			iXip = aReader.ReadInt();
			iNameLength = aReader.ReadInt(); 				// 4
			
			if(iName.Ptr() && iName.MaxLength() >= iNameLength)
				{
				iName.SetLength(0);
				
				for(TInt cnt = 0; cnt < iNameLength; cnt++)
					{
					iName.Append(aReader.ReadByte());		//iCategorySize bytes
					} 
				}
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TCodeSegment::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{					
			// error between actual size & real size in data
			CLTRACE2("TCodeSegment::Deserialize serialization size error. Read [%d] but expected [%d]", endPos - startPos, GetSize());			
			iId = ESCMLast;	//unrecognised header
			
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TCodeSegment::GetSize() const
		{
		if(iVersion == ETCodeSegmentVersion1)
			{
			return 50 + iName.Length();
			}
		else
			{
			CLTRACE("TCodeSegment::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	/**
	 * This returns the largest size this class can be
	 */
	TInt TCodeSegment::GetMaxSize() const
		{
		return 50 + KMaxSegmentNameSize;
		}
	/**
	 * TRawData implementation
	 * @internal technology
	 */
	
	/**
	 * TRawData constructor
	 */
	TRawData::TRawData():		
		iId(ESCMRawData),
		iVersion(ETRawData1),	
		iLength(0),
		iData(NULL, 0)
		{	
		}

	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TRawData::Serialize(TByteStreamWriter& aWriter)
		{	
		TInt startPos = aWriter.CurrentPosition();
		
		// ID saved first 
		aWriter.WriteInt(iId);		 							// 4		
		if(iId != ESCMRawData)
			{
			CLTRACE("TRawData::Serialize Corrupt ID");
			return KErrCorrupt;
			}
		
		aWriter.WriteShort((TUint16) iVersion);					// 2

		if(iVersion == ETRawData1)
			{	
			if(iData.Ptr())
				{
				aWriter.WriteInt(iData.Length());				// 4
				iLength = iData.Length();
				
				for(TInt cnt = 0; cnt < iData.Length(); cnt++)
					{
					//Kern::Printf("cnt = %d \t\t\t0x%X", cnt, cnt);
					aWriter.WriteByte(iData[cnt]);  			//iLength bytes
					}
				}
			else
				{
				aWriter.WriteInt(0);
				}
			}
		else
			{
			CLTRACE("TRawData::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE2("TRawData::Serialize serialization size error. Wrote [%d] but expected [%d]", endPos - startPos, GetSize());
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TRawData::Deserialize(TByteStreamReader& aReader)	
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();						// 4
		if(iId != ESCMRawData)
			{
			CLTRACE("TRawData::Deserialize Corrupt ID");
			return KErrCorrupt;
			}
		
		iVersion = (TTRawDataVersion)aReader.ReadShort();			// 2

		if(iVersion == ETRawData1)
			{
			iLength = aReader.ReadInt(); 							// 4			
			
			if(iData.Ptr())
				{
				//Deserialise as much as we can into this buffer - it may be that the caller doesnt want all of it
				iData.SetLength(0);
				TUint amtToRead = (iData.MaxLength() >= iLength) ? iLength : iData.MaxLength();
				
				for(TUint cnt = 0; cnt < amtToRead; cnt++)
					{
					iData.Append(aReader.ReadByte());		//iCategorySize bytes
					} 
				
				//move reader along what we havent read
				aReader.SetPosition(aReader.CurrentPosition() + (iLength - amtToRead));
				}
			else
				{
				aReader.SetPosition(aReader.CurrentPosition() + iLength);
				}
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TRawData::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{					
			// error between actual size & real size in data
			CLTRACE2("TRawData::Deserialize serialization size error. Read [%d] but expected [%d]", endPos - startPos, GetSize());			
			iId = ESCMLast;	//unrecognised header
			
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Same as Deserialise except it only starts copying data into the buffer
	 * when it reaches aStartPos bytes into the data
	 * @param aStartPos
	 * @param aReader
	 * @return
	 */
	TInt TRawData::Deserialize(TInt aStartPos, TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();						// 4
		if(iId != ESCMRawData)
			{
			CLTRACE("TRawData::Deserialize Corrupt ID");
			return KErrCorrupt;
			}
		
		iVersion = (TTRawDataVersion)aReader.ReadShort();			// 2

		if(iVersion == ETRawData1)
			{
			iLength = aReader.ReadInt(); 							// 4			
			
			if(iData.Ptr())
				{
				//Deserialise as much as we can into this buffer - it may be that the caller doesnt want all of it
				iData.SetLength(0);
				aReader.SetPosition(aReader.CurrentPosition() + aStartPos);
				
				TUint amtToRead = (iData.MaxLength() >= iLength) ? iLength : iData.MaxLength();
				
				for(TUint cnt = 0; cnt < amtToRead; cnt++)
					{
					iData.Append(aReader.ReadByte());		//iCategorySize bytes
					} 
				
				//move reader along what we havent read
				aReader.SetPosition(aReader.CurrentPosition() + (iLength - (amtToRead + aStartPos)));
				}
			else
				{
				aReader.SetPosition(aReader.CurrentPosition() + iLength);
				}
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TRawData::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{					
			// error between actual size & real size in data
			CLTRACE2("TRawData::Deserialize serialization size error. Read [%d] but expected [%d]", endPos - startPos, GetSize());			
			iId = ESCMLast;	//unrecognised header
			
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TRawData::GetSize() const
		{
		if(iVersion == ETRawData1)
			{
			return 10 + iLength;
			}
		else
			{
			CLTRACE("TRawData::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	
	/**
	 * TMemoryDump implementation
	 * @internal technology
	 */
	
	/**
	 * TMemoryDump Constructor
	 */
	TMemoryDump::TMemoryDump():
		iId(ESCMMemory),
		iVersion(EMemDumpVersion1),		
		iStartAddress(0),
		iPid(0),
		iLength(0)
		{
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TMemoryDump::GetSize() const
		{
		if(iVersion == EMemDumpVersion1)
			{
			return 22;
			}
		else
			{
			CLTRACE("TMemoryDump::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TMemoryDump::Serialize(TByteStreamWriter& aWriter)
		{
		TInt startPos = aWriter.CurrentPosition();
				
		// ID saved first		
		aWriter.WriteInt(iId);								// 4
		if(iId != ESCMMemory)
			{
			CLTRACE("TMemoryDump::Serialize Error - Corrupt ID");
			return KErrCorrupt;
			}
		
		aWriter.WriteShort((TUint16) iVersion);				// 2

		if(iVersion == EMemDumpVersion1)
			{
			// write data v1 format
			aWriter.WriteInt(iStartAddress);				// 4
			aWriter.WriteInt64(iPid);						// 8
			aWriter.WriteInt(iLength);						// 4
			}
		else
			{
			CLTRACE("TMemoryDump::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TMemoryDump::Serialize serialization size error");	
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TMemoryDump::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();										// 4
		if(iId != ESCMMemory)
			{
			CLTRACE("TMemoryDump::Deserialize() ERROR Corrupt ID");			
			return KErrCorrupt;
			}
		
		iVersion = (TMemDumpVersionVersion)aReader.ReadShort();			// 2

		if(iVersion == EMemDumpVersion1)
			{
			// read data v1 format	
			iStartAddress = aReader.ReadInt();    						// 4
			iPid = aReader.ReadInt64();		 							// 8
			iLength = aReader.ReadInt();								// 4
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TMemoryDump::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			iId = ESCMLast;	//unrecognised header
			
			// error between actual size & real size in data
			CLTRACE("TMemoryDump::Deserialize serialization size error");	
			return KErrCorrupt;
			}
		
		return KErrNone;
		}	
	
	/**
	 * TTraceDump implementation
	 * @internal technology
	 */
	
	/**
	 * TTraceDump Constructor
	 */
	TTraceDump::TTraceDump():
		iId(ESCMTraceData),
		iVersion(ETraceDumpVersion1),
		iSizeOfMemory(0),
		iNumberOfParts(0)
		{
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TTraceDump::GetSize() const
		{
		if(iVersion == ETraceDumpVersion1)
			{
			return 14;
			}
		else
			{
			CLTRACE("TTraceDump::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TTraceDump::Serialize(TByteStreamWriter& aWriter)
		{
		TInt startPos = aWriter.CurrentPosition();
				
		// ID saved first		
		aWriter.WriteInt(iId);								// 4
		if(iId != ESCMTraceData)
			{
			CLTRACE("TTraceDump::Serialize Error - Corrupt ID");
			return KErrCorrupt;
			}
		
		aWriter.WriteShort((TUint16) iVersion);				// 2

		if(iVersion == ETraceDumpVersion1)
			{
			// write data v1 format
			aWriter.WriteInt(iSizeOfMemory);				// 4
			aWriter.WriteInt(iNumberOfParts);				// 4
			}
		else
			{
			CLTRACE("TTraceDump::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TTraceDump::Serialize serialization size error");	
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TTraceDump::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();										// 4
		if(iId != ESCMTraceData)
			{
			CLTRACE("TTraceDump::Deserialize() Error: Corrupt ID");			
			return KErrCorrupt;
			}
		
		iVersion = (TTraceDumpVersion)aReader.ReadShort();			// 2

		if(iVersion == ETraceDumpVersion1)
			{
			// read data v1 format	
			iSizeOfMemory = aReader.ReadInt();    						// 4
			iNumberOfParts = aReader.ReadInt();    						// 4
			
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TTraceDump::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			iId = ESCMLast;	//unrecognised header
			
			// error between actual size & real size in data
			CLTRACE("TTraceDump::Deserialize serialization size error");	
			return KErrCorrupt;
			}
		
		return KErrNone;
		}	
	}

//eof
