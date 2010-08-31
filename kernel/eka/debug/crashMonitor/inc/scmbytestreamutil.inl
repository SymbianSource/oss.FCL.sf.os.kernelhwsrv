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
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#include <e32des8.h>

namespace Debug 
	{
	/**
	 * TByteStreamReader implementation
	 */
	
	/**
	 * Returns the next byte from the stream
	 * @return TUint8 byte requested
	 */
	inline TUint8 TByteStreamReader::ReadByte()
		{
		return iBuffer[iPos++];		
		}
	
	/**
	 * Returns the next short from the stream
	 * @return TUint16 short requested
	 */	
	inline TUint16 TByteStreamReader::ReadShort()
		{
		TUint8 b1 = ReadByte();
		TUint8 b2 = ReadByte();	
		return (TUint16)(b1 + (b2 << 8));	
		}
		
	/**
	 * Returns the next TUInt32 from the stream
	 * @return TUInt32 TUInt32 requested
	 */		
	inline TUint32 TByteStreamReader::ReadInt()
		{
		TUint16 s1 = ReadShort();
		TUint16 s2 = ReadShort();	
		return s1 + (s2 << 16);		
		}

	/**
	 * Returns the next TUInt64 from the stream
	 * @return TUInt64 TUInt64 requested
	 */		
	inline TUint64 TByteStreamReader::ReadInt64()
		{
		TUint32 high = ReadInt();
		TUint32 low = ReadInt();
		return  MAKE_TUINT64(high, low) ;
		}
	
	/**
	 * TByteStreamWriter implementation
	 */	

	/**
	 * Writes a short to the stream
	 * @param aValue Value to write to stream
	 */	
	inline void TByteStreamWriter::WriteShort(TUint16 aValue)
		{
		WriteByte((TUint8) aValue);
		WriteByte((TUint8) (aValue >> 8));		
		}
	
	/**
	 * Writes an int to the stream
	 * @param aValue Value to write to stream
	 */	
	inline void TByteStreamWriter::WriteInt(TUint32 aValue)
		{
		WriteByte((TUint8)aValue);
		WriteByte((TUint8) (aValue >> 8));		
		WriteByte((TUint8) (aValue >> 16));		
		WriteByte((TUint8) (aValue >> 24));				
		}
	
	/**
	 * Writes a 64 bit int to the stream
	 * @param aValue Value to write to stream
	 */		
	inline void TByteStreamWriter::WriteInt64(TUint64 aValue)
		{
		WriteInt(I64HIGH(aValue));
		WriteInt(I64LOW(aValue));			
		}
	
	/**
	 * Enables physical writing such that the physical writers DoPhysicalWrite
	 * method will be invoked upon a write. This may write to a given media
	 * as defined by the implementation of this method 
	 */		
	inline void TByteStreamWriter::EnablePhysicalWriting()
		{
		iPhysEnabled = ETrue;
		}

	/**
	 * Disables physical writing such that the physical writers DoPhysicalWrite
	 * method will not be invoked upon a write. 
	 */	
	inline void TByteStreamWriter::DisablePhysicalWriting()
		{
		iPhysEnabled = EFalse;
		}
	}


//eof
