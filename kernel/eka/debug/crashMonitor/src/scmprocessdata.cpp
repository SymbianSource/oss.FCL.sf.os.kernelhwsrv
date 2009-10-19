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
// e32\debug\crashMonitor\src\scmprocessdata.cpp
// Core dump server - Process Data for System Crash
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
	 * TProcessData implementation
	 * @internal technology
	 */
	
	/**
	 * TProcessData constructor
	 */
	TProcessData::TProcessData()
		:iId(ESCMProcessData)
		,iVersion(EProcData1)				
		,iPid(0)
		,iNamesize(0)
		,iPriority(0)
		,iSpare1(0)
		,iSpare2(0)	
		,iSpare3(0)
		{
		}
	
	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return One of the OS wide codes
	 */	 
	TInt TProcessData::Serialize(TByteStreamWriter& aWriter)
		{
		TInt startPos = aWriter.CurrentPosition();
		
		// ID saved first 
		aWriter.WriteInt(iId);		 				// 4
		
		if(iId != ESCMProcessData)
			{
			CLTRACE("TProcessData::Serialize Corrupt ID");
			return KErrCorrupt;
			}
		
		aWriter.WriteShort((TUint16) iVersion);			// 2

		if(iVersion == EProcData1)
			{
			// write data v1 format				
			aWriter.WriteInt(iPriority); 			 	// 4
			aWriter.WriteInt64(iPid);  					// 8
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
			CLTRACE("TProcessData::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE2("TProcessData::Serialize serialization size error. Wrote [%d] but expected [%d]", endPos - startPos, GetSize());
			return KErrCorrupt;
			}
		return KErrNone;
		}				
		
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return One of the OS wide codes
	 */
	TInt TProcessData::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();		 				// 4
		if(iId != ESCMProcessData)
			{
			CLTRACE("TProcessData::Deserialize failed - Read corrupt ID");
			return KErrCorrupt;
			}
		
		iVersion = (TProcessDataVersion)aReader.ReadShort();			// 2

		if(iVersion == EProcData1)
			{
			// read data v1 format						
			iPriority = aReader.ReadInt(); 			 	    // 4		
			iPid = aReader.ReadInt64();						// 8
			
			iNamesize = aReader.ReadInt();					// 4			
			
			if(iName.Ptr() && iName.MaxLength() >= (TInt)iNamesize)
				{
				iName.SetLength(0);
				
				for(TUint cnt = 0; cnt < iNamesize; cnt++)
					{
					iName.Append(aReader.ReadByte());		//iCategorySize bytes
					} 
				}
			

			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TProcessData::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			iId = ESCMLast;	//unrecognised header
			
			// error between actual size & real size in data
			CLTRACE("TProcessData::Deserialize serialization size error");
			return KErrCorrupt;			
			}
		return KErrNone;
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TProcessData::GetSize() const
		{
		if(iVersion == EProcData1)
			{
			return 22 + iName.Length();
			}
		else
			{
			CLTRACE("TProcessData::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	}
