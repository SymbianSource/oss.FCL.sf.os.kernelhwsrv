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
// e32\debug\crashMonitor\src\scmlockdata.cpp
// some utility classes for writing data to flash buffer
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
	 * TSCMLockData constructor
	 * @param none
	*/
	TSCMLockData::TSCMLockData()
	: iId(ESCMLocks)
	, iMutexHoldCount(-1)
	, iMutexThreadWaitCount(-1)		
		{	
		}

	/**
	 * TSCMLockData Serialize 
	 * @param aWriter byte stream to use
	 * @return N/A
	*/
	TInt TSCMLockData::Serialize(TByteStreamWriter& aWriter)
		{
		TInt startPos = aWriter.CurrentPosition();
			
		// ID saved first 
		if(iId != ESCMLocks)
			{
			CLTRACE("TSCMLockData::Serialize Corrupt ID");
			return KErrCorrupt;
			}
		
		// write id first
		aWriter.WriteInt(iId);		 				    // 4		
		// 2 counts written as shorts (should be enough range!)
		aWriter.WriteShort((TInt16) iMutexHoldCount);			// 2
		aWriter.WriteShort((TInt16) iMutexThreadWaitCount);      // 2
		aWriter.WriteShort((TInt16) iLockCount);      // 2
		
			
		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("TSCMLockData::Serialize serialization size error");	
			return KErrCorrupt;
			}
		
		return KErrNone;
		}
	
	/**
	 * Reads the classes data from the specified byte stream
	 * @param aReader Byte stream to use
	 * @return void
	 */
	TInt TSCMLockData::Deserialize(TByteStreamReader& aReader)
		{
		TInt startPos = aReader.CurrentPosition();
		
		iId = (SCMStructId)aReader.ReadInt();					// 4
		if(iId != ESCMLocks)
			{
			CLTRACE("TSCMLockData::Deserialize Corrupt ID read");
			return KErrCorrupt;
			}
		
		iMutexHoldCount = (TInt) aReader.ReadShort();
		iMutexThreadWaitCount = (TInt) aReader.ReadShort();
		iLockCount =  (TInt) aReader.ReadShort();
		
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{			
			CLTRACE("TSCMLockData::Deserialize size error");	
			return KErrCorrupt;
			}
		return KErrNone;
		}
	
	/**
	 * Returns the externalised size of this class
	 * @return TInt size
	 */
	TInt TSCMLockData::GetSize() const
		{
		return KSCMLockDataMaxSize;		
		}
		
	/**
	 * MutexHoldCount
	 * @param none
	 * @return mutex hold count
	*/
	TInt TSCMLockData::MutexHoldCount() const
		{
		return iMutexHoldCount;
		}
	
	/**
	 * SetMutexHoldCount
	 * @param 
	 * @return 
	*/
	void TSCMLockData::SetMutexHoldCount(TInt aMutexHoldCount)
		{
		iMutexHoldCount = aMutexHoldCount;
		}
	
	/**
	 * MutexThreadWaitCount
	 * @param none
	 * @return number of threads waiting on held mutex - will only be valid if
	 * MutexHoldCount > 0
	*/
	TInt TSCMLockData::MutexThreadWaitCount() const
		{
		return iMutexThreadWaitCount;
		}
	
	/**
	 * SetMutexThreadWaitCount
	 * @param TInt - number of threads waiting on held mutex(es)
	 * @return void
	*/
	void TSCMLockData::SetMutexThreadWaitCount(TInt aMutexThreadWaitCount)
		{
		iMutexThreadWaitCount = aMutexThreadWaitCount;
		}		
	
	/**
	 * LockCount
	 * @param none
	 * @return TIOnt - the lock count
	*/
	TInt TSCMLockData::LockCount() const
		{
		return iLockCount;
		}
		
	/**
	 * SetLockCount
	 * @param TInt - number of locks held
	 * @return void
	*/
	void TSCMLockData::SetLockCount(TInt aLockCount)
		{
		iLockCount = aLockCount;
		}

	TBool TSCMLockData::operator == (const TSCMLockData& aOther) const
		{
		return ( iId == aOther.iId &&  
				 iMutexHoldCount == aOther.iMutexHoldCount &&			 
				 iMutexThreadWaitCount == aOther.iMutexThreadWaitCount &&
				 iLockCount == aOther.iLockCount ); 	
		}
	
	TBool TSCMLockData::operator != (const TSCMLockData& aOther) const
		{
		return !(*this == aOther);
		}
	}


