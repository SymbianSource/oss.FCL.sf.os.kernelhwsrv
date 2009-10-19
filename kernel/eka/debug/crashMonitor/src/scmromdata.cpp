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
// e32\debug\crashMonitor\src\scmromdata.cpp
// Core dump server - ROM Data for System Crash
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
	 * TRomHeaderData implementation
	 * @internal technology
	 */
	
	/**
	 * TRomHeaderData constructor
	 */
	TRomHeaderData::TRomHeaderData():
		iId(ESCMRomHeader),
		iVersion(ERomHeaderDataVersion1),
		iMajorVersion(0),
		iMinorVersion(0),
		iBuildNumber(0),
		iTime(0)
		{			
		}
	
	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return void
	 */	
	TInt TRomHeaderData::Serialize(TByteStreamWriter& aWriter)
		{
		TInt startPos = aWriter.CurrentPosition();
		
		if(iId != ESCMRomHeader)
			{
			CLTRACE("TRomHeaderData::Serialize Corrupt ID");
			return KErrCorrupt;
			}

		// ID saved first 
		aWriter.WriteInt(iId);		 				    // 4		
		
		aWriter.WriteShort((TUint16) iVersion);			// 2

		if(iVersion == ERomHeaderDataVersion1)
			{
			//ROM time
			aWriter.WriteInt64(iTime);					// 8
			
			//Now the ROM version
			aWriter.WriteByte(iMajorVersion);				// 1
			aWriter.WriteByte(iMinorVersion);				// 1
			aWriter.WriteShort(iBuildNumber);				// 2			
			}
		else
			{
			CLTRACE("TRomHeaderData::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt pos1 = aWriter.CurrentPosition();
		if( pos1 - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TRomHeaderData::Serialize serialization size error");	
			return KErrCorrupt;
			}
		
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return void
	 */
	TInt TRomHeaderData::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();					// 4
		if(iId != ESCMRomHeader)
			{
			CLTRACE("TRomHeaderData::Deserialize Corrupt ID read");
			return KErrCorrupt;
			}
		
		iVersion = (TRomHeaderDataVersion)aReader.ReadShort();			// 2

		if(iVersion == ERomHeaderDataVersion1)
			{			
			//ROM time
			iTime = aReader.ReadInt64();					// 8
			
			//Now the ROM version
			iMajorVersion = aReader.ReadByte();				// 1
			iMinorVersion = aReader.ReadByte();				// 1
			iBuildNumber = aReader.ReadShort();				// 2
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TRomHeaderData::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt pos1 = aReader.CurrentPosition();
		if( pos1 - startPos != GetSize())
			{
			iId = ESCMLast;	//unrecognised header
			
			// error between actual size & real size in data
			CLTRACE("TRomHeaderData::Deserialize serialization size error");	
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TRomHeaderData::GetSize() const
		{
		if(iVersion == ERomHeaderDataVersion1)
			{
			return 18;
			}
		else
			{
			CLTRACE("ERomHeaderDataVersion1::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	}
