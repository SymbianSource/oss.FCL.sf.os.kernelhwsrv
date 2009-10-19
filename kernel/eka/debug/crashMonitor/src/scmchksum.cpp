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
// e32\debug\crashMonitor\src\scmchksum.cpp
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
	 * Constructor
	 */
	TScmChecksum::TScmChecksum()
	: iLength(0)	
	, iSum(0)
	, iZeroCount(0)
		{	
		}
	
	/**
	 * ChecksumBlock - calculate checksum values for given data
	 * @param aData - the data to checksum
	 * @param aLen - the length of the data to checksum
	 * @return void
	 */
	void TScmChecksum::ChecksumBlock(const TUint8* aData, TUint aLen)
		{		
		/**
		 * 
		 * Note there is Symbian CRC implementation to be found in the following
		 * \src\cedar\generic\tools\e32tools\elf2e32\source\checksum.h
		 * \src\cedar\generic\tools\e32tools\elf2e32\source\checksum.cpp
		 * this however may be no good to us
		 * as we need to produce a single checksum even though the entire data may be
		 * read in different size blocks - and the entire data may not be available
		 * (the comm port requirement is for read only )
		 * If we do however want to use the CRC then this is the place to insert the code
		 */		
		if(!aData)
			{
			return;
			}
			
		for(TUint i=0;i<aLen;i++)
			{
			TUint8 val = *(aData+i);
			
			iLength++;
			if(val == 0)
				{
				iZeroCount++;
				}
			else
				{
				iSum += val;
				}			
			}		
		}
	
	/**
	 * ChecksumBlock - calculate checksum values for given data
	 * @param aData - descriptor containing the data to checksum
	 * @return void
	 */
	void TScmChecksum::ChecksumBlock(const TDesC8& aDes)
		{
		ChecksumBlock(aDes.Ptr(), aDes.Length());
		}
	
	/**
	 * ChecksumBlock - operator ==
	 * @param aOther - the TScmChecksum to compare too
	 * @return ETrue is objects match - otherwise EFalse
	 */
	TBool TScmChecksum::operator == (const TScmChecksum& aOther) const
		{
		return (iLength == aOther.iLength && iSum == aOther.iSum && iZeroCount == aOther.iZeroCount);
		}

	/**
	 * ChecksumBlock - operator !=
	 * @param aOther - the TScmChecksum to compare too
	 * @return EFalse if objects match - otherwise ETrue
	 */
	TBool TScmChecksum::operator != (const TScmChecksum& aOther) const
		{
		return !(*this == aOther);
		}
	
	/**
	 * GetSize 
	 * @return size of this object when streamed in bytes
	 */
	TInt TScmChecksum::GetSize() const
 		{
 		return 12;
 		}
	
	/**
	 * Serialize - writes this object to the supplied byte stream
	 * @param aItem -  aWriter - the TByteStreamWriter that will be written to
	 * @return One of the OS wide codes
	 */
	TInt TScmChecksum::Serialize(TByteStreamWriter& aWriter)
		{	
		TInt startPos = aWriter.CurrentPosition();
		aWriter.WriteInt(iLength);
		aWriter.WriteInt(iSum);
		aWriter.WriteInt(iZeroCount);
		TInt sizeWritten = aWriter.CurrentPosition() - startPos;
		if(sizeWritten != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TScmChecksum serialization size error");	
			return KErrCorrupt;
			}
		return KErrNone;
		}
	

	/**
	 * Deserialize - read this objects state from the supplied byte stream
	 * @param aItem -  aReader - the TByteStreamReader that will be read from
	 * @return One of the OS wide codes
	 */
	TInt TScmChecksum::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		// do we need a version check here - will it change ?
		iLength = aReader.ReadInt();	
		iSum = aReader.ReadInt();
		iZeroCount = aReader.ReadInt();	

		TInt sizeRead = aReader.CurrentPosition() - startPos;
		if(sizeRead != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TScmChecksum Deserialization size error");	
			return KErrCorrupt;
			}
		return KErrCorrupt;
		}
	
	
	void TScmChecksum::Reset()
		{
		iLength = iSum = iZeroCount = 0;	
		}		
	}
//eof

