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
// e32\kernel\scmvariantdata.cpp
// Core dump server - Variant Data for System Crash
// 
//

#include <scmdatatypes.h>

namespace Debug
	{
	/**
	 * TVariantSpecificData implementation
	 * @internal technology
	 */
	
	/**
	 * TVariantSpecificData constructor
	 */
	TVariantSpecificData::TVariantSpecificData() :
		iId(ESCMVariantData)
		,iVersion(EVariantSpecificDataVersion1)
		,iSize(0)
		{			
		}
	
	/**
	 * Writes this classes data to the specified byte stream
	 * @param aWriter byte stream to use
	 * @return void
	 */	
	TInt TVariantSpecificData::Serialize(TByteStreamWriter& aWriter)
		{
		TInt startPos = aWriter.CurrentPosition();
		
		if(iId != ESCMVariantData)
			{
			CLTRACE("TVariantSpecificData::Serialize Corrupt ID");
			return KErrCorrupt;
			}

		// ID saved first 
		aWriter.WriteInt(iId);		 				    // 4		
		
		aWriter.WriteShort((TUint16) iVersion);			// 2

		if(iVersion == EVariantSpecificDataVersion1)
			{
			// write data v1 format					
			aWriter.WriteInt(iSize); 			 	// 4
			}
		else
			{
			CLTRACE("TVariantSpecificData::Serialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TVariantSpecificData::Serialize serialization size error");	
			return KErrCorrupt;
			}
		
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return void
	 */
	TInt TVariantSpecificData::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();					// 4
		if(iId != ESCMVariantData)
			{
			CLTRACE("TVariantSpecificData::Deserialize Corrupt ID read");
			return KErrCorrupt;
			}
		
		iVersion = (TVariantSpecificDataVersion)aReader.ReadShort();			// 2

		if(iVersion == EVariantSpecificDataVersion1)
			{
			// read data v1 format	
			iSize = aReader.ReadInt();		 				// 4
			}
		else
			{
			iId = ESCMLast;	//unrecognised header
			CLTRACE("TVariantSpecificData::Deserialize Unsupported version");
			return KErrCorrupt;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			iId = ESCMLast;	//unrecognised header
			
			// error between actual size & real size in data
			CLTRACE("TVariantSpecificData::Deserialize serialization size error");	
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TVariantSpecificData::GetSize() const
		{
		if(iVersion == EVariantSpecificDataVersion1)
			{
			return 10;
			}
		else
			{
			CLTRACE("TThreadData::GetSize Unsupported version");			
			return KErrNotSupported;		
			}
		}
	
	}
