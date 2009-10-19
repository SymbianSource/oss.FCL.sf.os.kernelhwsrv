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
// e32\debug\crashMonitor\src\scmthreaddata.cpp
// Core dump server - Thread Data for System Crash
// 
//

/**
 @file
 @internalTechnology
*/

#include <scmdatatypes.h>

namespace Debug
	{
	/**
	 * TThreadData implementation
	 * @internal technology
	 */
	
	/**
	 * TThreadData constructor
	 */
	TThreadData::TThreadData()
	: iId(ESCMThreadData)
		,iVersion(EThreadData1)
		,iPriority(0)
		,iTid(0)
		,iOwnerId(0)
		,iSvcSP(0)
		,iSvcStack(0)
		,iSvcStacksize(0)
		,iUsrSP(0)
		,iUsrStack(0)
		,iUsrStacksize(0)
		,iNamesize(0)
		,iLastCpu(0)
		,iSvcHeap(0)
		,iSvcHeapSize(0)
		{			
		}
	
	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return void
	 */	
	TInt TThreadData::Serialize(TByteStreamWriter& aWriter)
		{
		TInt startPos = aWriter.CurrentPosition();
		
		if(iId != ESCMThreadData)
			{
			CLTRACE("TThreadData::Serialize Corrupt ID");
			return KErrCorrupt;
			}

		// ID saved first 
		aWriter.WriteInt(iId);		 				    // 4		
		
		aWriter.WriteShort((TUint16) iVersion);			// 2

		if(iVersion == EThreadData1)
			{
			// write data v1 format					
			aWriter.WriteInt(iPriority); 			 	// 4
			aWriter.WriteInt64(iTid);  					// 8
			aWriter.WriteInt64(iOwnerId);				// 8
			aWriter.WriteInt(iSvcSP); 			 		// 4
			aWriter.WriteInt(iSvcStack); 			 	// 4
			aWriter.WriteInt(iSvcStacksize); 			 	// 4
			aWriter.WriteInt(iUsrSP); 			 		// 4
			aWriter.WriteInt(iUsrStack); 			 	// 4
			aWriter.WriteInt(iUsrStacksize); 			 	// 4			
			aWriter.WriteInt(iLastCpu); 			 	// 4
			aWriter.WriteInt(iSvcHeap); 			 		// 4
			aWriter.WriteInt(iSvcHeapSize); 			 	// 4
			if(iName.Ptr())
				{
				aWriter.WriteInt(iName.Length());		// 4		
				for(TInt cnt = 0; cnt < iName.Length(); cnt++)
					{
					aWriter.WriteByte(iName[cnt]);
					}
				}
			else
				{
				aWriter.WriteInt(0);
				}
			}
		else
			{
			CLTRACE("TThreadData::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TThreadData::Serialize serialization size error");	
			return KErrCorrupt;
			}
		
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return void
	 */
	TInt TThreadData::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();					// 4
		if(iId != ESCMThreadData)
			{
			CLTRACE("TThreadData::Deserialize Corrupt ID read");
			return KErrCorrupt;
			}
		
		iVersion = (TThreadDataVersion)aReader.ReadShort();			// 2

		if(iVersion == EThreadData1)
			{
			// read data v1 format	
			iPriority = aReader.ReadInt();		 				// 4		
			iTid = aReader.ReadInt64(); 			 	    	// 8		
			iOwnerId = aReader.ReadInt64();						// 8
			iSvcSP = aReader.ReadInt();							// 4
			iSvcStack = aReader.ReadInt();						// 4
			iSvcStacksize = aReader.ReadInt();					// 4
			iUsrSP = aReader.ReadInt();							// 4
			iUsrStack = aReader.ReadInt();						// 4
			iUsrStacksize = aReader.ReadInt();					// 4
			iLastCpu = aReader.ReadInt();						// 4
			iSvcHeap = aReader.ReadInt();	 			 		// 4
			iSvcHeapSize = aReader.ReadInt();	 			 		// 4
			
			iNamesize = aReader.ReadInt();						// 4
						
			if(iName.Ptr() && iName.MaxLength() >= (TInt)iNamesize)
				{
				iName.SetLength(0);
				
				for(TUint cnt = 0; cnt < iNamesize; cnt++)
					{
					iName.Append(aReader.ReadByte());		//iNamesize bytes
					} 
				}	
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TThreadData::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			iId = ESCMLast;	//unrecognised header
			
			// error between actual size & real size in data
			CLTRACE("TThreadData::Deserialize serialization size error");	
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TThreadData::GetSize() const
		{
		if(iVersion == EThreadData1)
			{
			return 66 + iName.Length();
			}
		else
			{
			CLTRACE("TThreadData::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	}
