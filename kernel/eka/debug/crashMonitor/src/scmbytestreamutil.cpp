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
// e32\debug\crashMonitor\src\scmbytestreamutil.cpp
// some utility classes for writing data to flash buffer
// 
//

/**
 @file
 @internalTechnology
*/


#include "scmbytestreamutil.h"
#include "scmtrace.h"



namespace Debug 
	{
	/**
	 * TByteStreamBase Constructor 
	 * @param aBuffer - pointer to buffer that this utility will use
	 */	
	TByteStreamBase::TByteStreamBase(TUint8* aBuffer) 
		: iBuffer(aBuffer)
		, iPos(0) 
		{
		}
			
	/**
	 * SetPosition
	 * @param aBuffer - Sets iPos
	 * @return void
	 */	
	void TByteStreamBase::SetPosition(TInt aPos)
		{
		iPos = aPos;
		}
	
	/**
	 * CurrentPosition
	 * @param aBuffer - Returns the current value of iPos
	 * @return Tint
	 */	
	TInt TByteStreamBase::CurrentPosition() const
		{
		return iPos;
		}
	
	/**
	 * TByteStreamReader Constructor 
	 * @param aBuffer - pointer to buffer that this utility will use
	 */	
	TByteStreamReader::TByteStreamReader(TUint8* aBuffer) 
		: TByteStreamBase(aBuffer)
		{	
		}
	
	
	/**
	 * Constructor for TByteStreamWriter
	 * @param aBuffer buffer for writing
	 * @param aPhysEnabled whether or not physical writing to another media is enabled
	 */
	TByteStreamWriter::TByteStreamWriter(TUint8* aBuffer, TBool aPhysEnabled) 
		: TByteStreamBase(aBuffer)
		, iPhysEnabled(aPhysEnabled)
		, iBytesWritten(0)
		{			
		}

	/**
	 * Writes a byte to the buffer
	 * @param aValue Byte to write
	 */
	void TByteStreamWriter::WriteByte(TUint8 aValue)
		{
		if(iBuffer)
			{
			iBuffer[iPos++] = aValue;
			++iBytesWritten;
			}
		}
	
	/**
	 * Resets the byte counter back to zero
	 */
	void TByteStreamWriter::ResetBytesWritten()
		{
		iBytesWritten = 0;
		}
	
	/**
	 * TCachedByteStreamWriter Constructor 
	 * @param aBuffer - pointer to buffer that this utility will use
	 * @param aCacheSize - suggested length of cache to use if greater than EMaxCache
	 * 					cache length of EMaxCache will be used
	 */
	TCachedByteStreamWriter::TCachedByteStreamWriter(TUint8* aCacheBuffer, TInt aCacheSize, TBool aPhysEnabled) 
		: TByteStreamWriter(NULL, aPhysEnabled)
		, iCacheSize(aCacheSize)
		, iCacheBuffer(aCacheBuffer)
		, iPhysicalWriter(NULL)
		{
		}
	
	
	/**
	 * FlushCache 
	 * Writes the contents of the cache to buffer amd/or to physical writer implementation
	 * if one is currently set
	 * @return void
	 */	
	TInt TCachedByteStreamWriter::FlushCache()
		{			
		TInt padCount = iCacheSize - iPos;
		if(padCount > 0)
			{		
			for(TInt i=0;i<padCount;i++)
				{
				iCacheBuffer[iPos++] = 0;
				}		
			}
				
		if(iPhysEnabled)
			{
			if(iPhysicalWriter) // do we have a writer to send the cache data to
				{
				iPhysicalWriter->DoPhysicalWrite(iCacheBuffer, iBytesWritten, iPos);
				}
			}
		
		iPos = 0;
		return KErrNone;
		}
	
	/**
	 * Writes a byte to the cached buffer
	 * @param aValue Byte to write
	 */
	void TCachedByteStreamWriter::WriteByte(TUint8 aValue)
		{
		if(iPos == iCacheSize)
			{
			FlushCache();
			}		
		iCacheBuffer[iPos++] = aValue;	
		++iBytesWritten;
		}
	
	/**
	 * CurrentPosition
	 * @param aBuffer -  need to return the position in buffer plus the write pos into cache
	 * @return Tint
	 */	
	TInt TCachedByteStreamWriter::CurrentPosition() const
		{
		return iBytesWritten;	
		}

	/**
	 * SetWriterImpl
	 * @param aPhysicalWriter - sets the physical writer implementation to be used when the cache is flushed
	 * 							pass NULL to remove a previous writer implementation
	 * @return void
	 */	
	void TCachedByteStreamWriter::SetWriterImpl(MPhysicalWriterImpl* aPhysicalWriter)
		{
		iPhysicalWriter = aPhysicalWriter;
		}	
	}

//eof

