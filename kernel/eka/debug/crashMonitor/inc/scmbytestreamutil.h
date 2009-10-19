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
// some utility classes for writing data to buffer
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/
#ifndef __SCMBYTESTREAMUTIL_H_
#define __SCMBYTESTREAMUTIL_H_

#include <e32cmn.h> 
#include <e32def.h>
#include <e32const.h>


namespace Debug 
	{ 
	/**
	 * Base class for byte stream write - simply deals with the supplied buffer & position 
	 */
	class TByteStreamBase
		{
	public:
		TByteStreamBase(TUint8* aBuffer);
		virtual void SetPosition(TInt aPosition);
		virtual TInt CurrentPosition() const;
	
	protected:	
		
		/**
		 * Pointer to the buffer we will use to write/read to 
		 */
		TUint8* iBuffer;
		
		/**
		 * Current position in buffer
		 */
		TInt iPos;	
		};
	
	/**
	 * Class for reading byte stream
	 */
	class TByteStreamReader : public TByteStreamBase		
		{
	public:
		TByteStreamReader(TUint8* aBuffer);
		inline virtual TUint8 ReadByte();
		inline TUint16 ReadShort();
		inline TUint32 ReadInt();
		inline TUint64 ReadInt64();		
		};	

	/**
	 * Class for writing byte stream
	 */
	class TByteStreamWriter : public TByteStreamBase		
		{		
	public:
		TByteStreamWriter(TUint8* aBuffer, TBool aPhsEnabled = ETrue);	
		virtual void WriteByte(TUint8 aValue);
		inline void WriteShort(TUint16 aValue);
		inline void WriteInt(TUint32 aValue);
		inline void WriteInt64(TUint64 aValue);
		inline virtual void EnablePhysicalWriting();
		inline virtual void DisablePhysicalWriting();
		inline virtual TBool PhysicalWritingEnabled() const {return iPhysEnabled;};
		inline TInt GetBytesWritten() const {return iBytesWritten;};	
		void ResetBytesWritten();
		
	protected:
		
		/** 
		 * This records whether or not physical writing via DoPhysical write from set writer
		 */
		TBool iPhysEnabled;
		
		/**
		 * Records the number of bytes we have written to our buffer
		 */
		TInt iBytesWritten;
		};	
		
	/**
	 * This is the interface to write to flash
	 */
	class MPhysicalWriterImpl 
		{
		public:			
			virtual void DoPhysicalWrite(TAny* aData,TInt aPos, TInt aLen) = 0;
		};
	
	
	/**
	 *Class for writing byte stream via cache 
	 */
	class TCachedByteStreamWriter : public TByteStreamWriter		
		{		
	public:
			
		TCachedByteStreamWriter(TUint8* aCacheBuffer, TInt aCacheSize,  TBool aPhysEnabled = ETrue);
		virtual TInt CurrentPosition() const;
		virtual void WriteByte(TUint8 aValue);
		virtual TInt FlushCache();
		void SetWriterImpl(MPhysicalWriterImpl* aPhysicalWriter);
		TInt GetCacheSize() const  {return iCacheSize; };
		
	protected:		
		TInt iCacheSize;
		TUint8* iCacheBuffer;  			
		MPhysicalWriterImpl* iPhysicalWriter;
		};
	
	/**
	 * Serialization implementation interface
	 */
	class MByteStreamSerializable
		{
	public:
		virtual TInt Serialize(TByteStreamWriter& aWriter) = 0;
		virtual TInt Deserialize(TByteStreamReader& aReader) = 0;
 		virtual TInt GetSize() const = 0;
		};
	}


#include <scmbytestreamutil.inl>



#endif /*BYTESTREAMUTIL_H_*/
