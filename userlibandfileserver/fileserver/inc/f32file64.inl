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
// f32\inc\f32file64.inl
// 
//

/**
 
 Reads from the file at the current position.
 
 This is a synchronous function.
 
 Note that when an attempt is made to read beyond the end of the file,
 no error is returned. 
 The descriptor's length is set to the number of bytes read into 
 it. Therefore, when reading through a file,the end of file has been reached 
 when the descriptor length, as returned by TDesC8::Length(), is zero.
 
 @param aDes Descriptor into which binary data is read. Any existing contents 
 are overwritten. On return, its length is set to the number of
 bytes read.
 @return KErrNone if successful, otherwise one of the other system-wide error 
 codes.
 
 @see TDesC8::Length
*/
inline TInt RFile64::Read(TDes8& aDes) const
	{return RFile::Read(aDes);}

/**
Reads from the file at the current position.

This is an asynchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into 
it. Therefore, when reading through a file,the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.

@param aDes    Descriptor into which binary data is read. Any existing contents 
               are overwritten. On return, its length is set to the number of
               bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.
               
@param aStatus Request status. On completion contains:
       KErrNone, if successful, otherwise one of the other system-wide error codes.

@see TDesC8::Length       
*/
inline void RFile64::Read(TDes8& aDes,TRequestStatus& aStatus) const
	{RFile::Read(aDes, aStatus);}

/**
Reads the specified number of bytes of binary data from the file at the current position.

This is a synchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into 
it. Therefore, when reading through a file,the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only circumstance 
in which Read() can return fewer bytes than requested, is when the end of 
file is reached or if an error occurs.

@param aDes    Descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to
               the number of bytes read.
            
@param aLength The number of bytes to be read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's 
               maximum length, the function returns KErrOverflow.
               This value must not be negative, otherwise the function
               returns KErrArgument.
               
@return KErrNone if successful, otherwise one of the other system-wide error
        codes.
*/
inline TInt RFile64::Read(TDes8& aDes,TInt aLength) const
	{return RFile::Read(aDes, aLength);}

/**
Reads a specified number of bytes of binary data from the file at the current position.

This is an asynchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only
circumstances in which Read() can return fewer bytes than requested is when
the end of file is reached or if an error has occurred.

@param aDes    Descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to the
               number of bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.
               
@param aLength The number of bytes to be read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's
               maximum length, then the function updates aStatus parameter with KErrOverflow.
               It must not be negative otherwise the function updates aStatus with KErrArgument.
               
@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.
*/
inline void RFile64::Read(TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const
	{ RFile::Read( aDes, aLength, aStatus);}

/**
Writes to the file at the current offset within the file.

This is a synchronous function.

@param aDes The descriptor from which binary data is written.
            The function writes the entire contents of aDes to the file.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
inline TInt RFile64::Write(const TDesC8& aDes)
	{return RFile::Write(aDes);}


/** 
Writes to the file at the current offset within the file.

This is an asynchronous function.

@param aDes    The descriptor from which binary data is written.
               The function writes the entire contents of aDes to the file.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.
            
@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.
*/
inline void RFile64::Write(const TDesC8& aDes,TRequestStatus& aStatus)
	{RFile::Write(aDes, aStatus);}


/**
Writes a portion of a descriptor to the file at the current offset within
the file.

This is a synchronous function.

@param aDes    The descriptor from which binary data is written.
@param aLength The number of bytes to be written from the descriptor.
               This must not be greater than the length of the descriptor.
               It must not be negative.

@return KErrNone if successful; KErrArgument if aLength is negative;
		otherwise one of the other system-wide error codes.
        
@panic FSCLIENT 27 in debug mode, if aLength is greater than the length
       of the descriptor aDes.  
*/
inline TInt RFile64::Write(const TDesC8& aDes,TInt aLength)
	{return RFile::Write(aDes, aLength);}


/**
Writes a portion of a descriptor to the file at the current offset
within the file.

This is an asynchronous function.

@param aDes    The descriptor from which binary data is written.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aLength The number of bytes to be written from the descriptor.
               This must not be greater than the length of the descriptor.
               It must not be negative.

@param aStatus Request status. On completion contains KErrNone if successful; 
			   KErrArgument if aLength is negative; 
			   otherwise one of the other system-wide error codes.
*/
inline void RFile64::Write(const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus)
	{RFile::Write(aDes, aLength, aStatus);}

/**
Reads from the file at the specified offset within the file

This is a synchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.

Note:
1. This function over-rides the base class function RFile::Read 
   and inlines the base class RFile::Read.
2. The difference is that this function can read beyond 2GB - 1 when
   aPos + length of aDes is beyond 2GB - 1.
3. This function is protected using _F32_STRICT_64_BIT_MIGRATION macro 
   to help migration to 64 bit file addressing. When the macro is defined, 
   this function becomes a private overload and hence use of 
   TInt RFile64::Read(TInt64 aPos,TDes8& aDes) const is recommended.

@see TInt RFile::Read(TInt aPos,TDes8& aDes) const
@see TInt RFile64::Read(TInt64 aPos,TDes8& aDes) const

@param aPos Position of first byte to be read.  This is an offset from
            the start of the file. If no position is specified, reading
            begins at the current file position. 
            If aPos is beyond the end of the file, the function returns
            a zero length descriptor.
            
@param aDes The descriptor into which binary data is read. Any existing content
            is overwritten. On return, its length is set to the number of
            bytes read.
            
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.

@panic FSCLIENT 19 if aPos is negative.
*/

inline TInt RFile64::Read(TInt aPos,TDes8& aDes) const
	{return RFile::Read(aPos, aDes);}

/**
Reads from the file at the specified offset within the file.

This is an asynchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.

Note:
1. This function over-rides the base class function RFile::Read 
   and inlines the base class RFile::Read.
2. The difference is that this function can read beyond 2GB - 1 when
   aPos + length of aDes is beyond 2GB - 1.
3. This function is protected using _F32_STRICT_64_BIT_MIGRATION macro 
   to help migration to 64 bit file addressing. When the macro is defined, 
   this function becomes a private overload and hence use of 
   void RFile64::Read(TInt64 aPos,TDes8& aDes,TRequestStatus& aStatus) const is recommended.

@see void RFile::Read(TInt aPos,TDes8& aDes,TRequestStatus& aStatus) const
@see void RFile64::Read(TInt64 aPos,TDes8& aDes,TRequestStatus& aStatus) const

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               content is overwritten. On return, its length is set to
               the number of bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.
               
@param aStatus The request status. On completion, contains an error code of KErrNone 
               if successful, otherwise one of the other system-wide error codes.

@panic FSCLIENT 19 if aPos is negative.        
*/
inline void RFile64::Read(TInt aPos,TDes8& aDes,TRequestStatus& aStatus) const
	{RFile::Read(aPos, aDes, aStatus);}

/**
Reads the specified number of bytes of binary data from the file at a specified 
offset within the file.

This is a synchronous function.

Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only
circumstances in which Read() can return fewer bytes than requested is when
the end of file is reached or if an error has occurred.

Note:
1. This function over-rides the base class function RFile::Read 
   and inlines the base class RFile::Read.
2. The difference is that this function can read beyond 2GB - 1 when
   aPos + aLength is beyond 2GB - 1.
3. This function is protected using _F32_STRICT_64_BIT_MIGRATION macro 
   to help migration to 64 bit file addressing. When the macro is defined, 
   this function becomes a private overload and hence use of 
   TInt RFile64::Read(TInt64 aPos,TDes8& aDes,TInt aLength) const is recommended.

@see TInt RFile::Read(TInt aPos,TDes8& aDes,TInt aLength) const
@see TInt RFile64::Read(TInt64 aPos,TDes8& aDes,TInt aLength) const

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to
               the number of bytes read.
@param aLength The number of bytes to read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's
               maximum length, then the function updates aStatus parameter with KErrOverflow.
               It must not be negative otherwise the function updates aStatus with KErrArgument.
               
@return KErrNone if successful, otherwise one of the other system-wide
        error codes.

@panic FSCLIENT 19 if aPos is negative.        
*/
inline TInt RFile64::Read(TInt aPos,TDes8& aDes,TInt aLength) const
	{return RFile::Read(aPos, aDes, aLength);}

/**
Reads the specified number of bytes of binary data from the file at a specified 
offset within the file.

This is an asynchronous function.


Note that when an attempt is made to read beyond the end of the file,
no error is returned. 
The descriptor's length is set to the number of bytes read into it.
Therefore, when reading through a file, the end of file has been reached 
when the descriptor length, as returned by TDesC8::Length(), is zero.
Assuming aLength is less than the maximum length of the descriptor, the only
circumstances in which Read() can return fewer bytes than requested is when
the end of file is reached or if an error has occurred.

Note:
1. This function over-rides the base class function RFile::Read 
   and inlines the base class RFile::Read.
2. The difference is that this function can read beyond 2GB - 1 when
   aPos + aLength is beyond 2GB - 1.
3. This function is protected using _F32_STRICT_64_BIT_MIGRATION macro 
   to help migration to 64 bit file addressing. When the macro is defined, 
   this function becomes a private overload and hence use of 
   void RFile64::Read(TInt64 aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const is recommended.

@see void RFile::Read(TInt aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const
@see void RFile64::Read(TInt64 aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const

@param aPos    Position of first byte to be read. This is an offset from
               the start of the file. If no position is specified, 
               reading begins at the current file position.
               If aPos is beyond the end of the file, the function returns
               a zero length descriptor.
               
@param aDes    The descriptor into which binary data is read. Any existing
               contents are overwritten. On return, its length is set to
               the number of bytes read.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aLength The number of bytes to read from the file into the descriptor. 
               If an attempt is made to read more bytes than the descriptor's
               maximum length, then the function returns KErrOverflow.
               It must not be negative otherwise the function returns KErrArgument.

@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.
               
@panic FSCLIENT 19 if aPos is negative.                       
*/
inline void RFile64::Read(TInt aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const
	{ RFile::Read(aPos, aDes, aLength, aStatus);}

/**
Writes to the file at the specified offset within the file

This is a synchronous function.

Note:
1. This function over-rides the base class function RFile::Write 
   and inlines the base class RFile::Write.
2. The difference is that this function can write beyond 2GB - 1 when
   aPos + length of aDes is beyond 2GB - 1.
3. This function is protected using _F32_STRICT_64_BIT_MIGRATION macro 
   to help migration to 64 bit file addressing. When the macro is defined, 
   this function becomes a private overload and hence use of 
   TInt RFile64::Write(TInt64 aPos,const TDesC8& aDes) is recommended.

@see TInt RFile::Write(TInt aPos,const TDesC8& aDes)
@see TInt RFile64::Write(TInt64 aPos,const TDesC8& aDes) 

@param aPos The offset from the start of the file at which the first
            byte is written. 
            If a position beyond the end of the file is specified, then
            the write operation begins at the end of the file.
            If the position has been locked, then the write fails.
            
@param aDes The descriptor from which binary data is written. The function writes 
            the entire contents of aDes to the file.
            
@return KErrNone if successful, otherwise one of the other system-wide error
        codes.

@panic FSCLIENT 19 if aPos is negative.                       
*/
inline TInt RFile64::Write(TInt aPos,const TDesC8& aDes)
	{return RFile::Write( aPos, aDes);}

/**
Writes to the file at the specified offset within the file

This is an asynchronous function.

Note:
1. This function over-rides the base class function RFile::Write 
   and inlines the base class RFile::Write.
2. The difference is that this function can write beyond 2GB - 1 when
   aPos + length of aDes is beyond 2GB - 1.
3. This function is protected using _F32_STRICT_64_BIT_MIGRATION macro 
   to help migration to 64 bit file addressing. When the macro is defined, 
   this function becomes a private overload and hence use of 
   void RFile64::Write(TInt64 aPos,const TDesC8& aDes,TRequestStatus& aStatus) is recommended.

@see void RFile::Write(TInt aPos,const TDesC8& aDes,TRequestStatus& aStatus)
@see void RFile64::Write(TInt64 aPos,const TDesC8& aDes,TRequestStatus& aStatus)

@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
               
@param aDes    The descriptor from which binary data is written. The function
               writes the entire contents of aDes to the file.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aStatus Request status. On completion contains KErrNone if successful, 
               otherwise one of the other system-wide error codes.

@panic FSCLIENT 19 if aPos is negative.                       
*/
inline void RFile64::Write(TInt aPos,const TDesC8& aDes,TRequestStatus& aStatus)
	{RFile::Write(aPos, aDes, aStatus);}

/**
Writes the specified number of bytes to the file at the specified offset within the file.

This is a synchronous function.

Note:
1. This function over-rides the base class function RFile::Write 
   and inlines the base class RFile::Write.
2. The difference is that this function can write beyond 2GB - 1 when
   aPos + aLength is beyond 2GB - 1.
3. This function is protected using _F32_STRICT_64_BIT_MIGRATION macro 
   to help migration to 64 bit file addressing. When the macro is defined, 
   this function becomes a private overload and hence use of 
   TInt RFile64::Write(TInt64 aPos,const TDesC8& aDes,TInt aLength) is recommended.

@see TInt RFile::Write(TInt aPos,const TDesC8& aDes,TInt aLength)
@see TInt RFile64::Write(TInt64 aPos,const TDesC8& aDes,TInt aLength)

@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
                             
@param aDes    The descriptor from which binary data is written.
@param aLength The number of bytes to be written from aDes .
			   It must not be negative.

@return KErrNone if successful; KErrArgument if aLength is negative;
		otherwise one of the other system-wide error codes.
        
@panic FSCLIENT 19 if aPos is negative.                       
*/
inline TInt RFile64::Write(TInt aPos,const TDesC8& aDes,TInt aLength)
	{return RFile::Write(aPos, aDes, aLength);}

/**
Writes the specified number of bytes to the file at the specified offset within the file.

This is an asynchronous function.

Note:
1. This function over-rides the base class function RFile::Write 
   and inlines the base class RFile::Write.
2. The difference is that this function can write beyond 2GB - 1 when
   aPos + aLength is beyond 2GB - 1.
3. This function is protected using _F32_STRICT_64_BIT_MIGRATION macro 
   to help migration to 64 bit file addressing. When the macro is defined, 
   this function becomes a private overload and hence use of 
   void RFile64::Write(TInt64 aPos,const TDesC8& aDes,TRequestStatus& aStatus) is recommended.

@see void RFile::Write(TInt aPos,const TDesC8& aDes,TRequestStatus& aStatus)
@see void RFile64::Write(TInt64 aPos,const TDesC8& aDes,TRequestStatus& aStatus)

@param aPos    The offset from the start of the file at which the first
               byte is written. 
               If a position beyond the end of the file is specified, then
               the write operation begins at the end of the file.
               If the position has been locked, then the write fails.
              
@param aDes    The descriptor from which binary data is written.
               NB: this function is asynchronous and the request that it
               represents may not complete until some time after the call
               to the function has returned. It is important, therefore, that
               this descriptor remain valid, or remain in scope, until you have
               been notified that the request is complete.

@param aLength The number of bytes to be written from aDes.
			   It must not be negative.
			   
@param aStatus Request status. On completion contains KErrNone if successful; 
			   KErrArgument if aLength is negative; 
			   otherwise one of the other system-wide error codes.

@panic FSCLIENT 19 if aPos is negative.                       
*/
inline void RFile64::Write(TInt aPos,const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus)
	{RFile::Write(aPos, aDes, aLength, aStatus);}
