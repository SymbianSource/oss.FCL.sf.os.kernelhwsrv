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

/**
 @file
 @publishedAll
 @prototype
*/

#if !defined(__F32FILE64_H__)
#define __F32FILE64_H__


/**
@publishedAll
@prototype

Creates and opens a file, and performs all operations on a single open file.

This is equivalent to RFile class.
This class is meant for accessing files of size greater than or equal to 2GB also.

These include:

- reading from and writing to the file

- seeking to a position within the file

- locking and unlocking within the file

- setting file attributes

Before using any of these services, a connection to a file server session must
have been made, and the file must be open.

Opening Files:

-  use Open() to open an existing file for reading or writing; an error is
   returned if it does not already exist.
   To open an existing file for reading only, use Open() with an access mode of
   EFileRead, and a share mode of EFileShareReadersOnly.

-  use Create() to create and open a new file for writing; an error is returned
   if it already exists.

-  use Replace() to open a file for writing, replacing any existing file of
   the same name if one exists, or creating a new file if one does not exist.
   Note that if a file exists, its length is reset to zero.

-  use Temp() to create and open a temporary file with a unique name,
   for writing and reading.

When opening a file, you must specify the file server session to use for
operations with that file. If you do not close the file explicitly, it is
closed when the server session associated with it is closed.

Reading and Writing:

There are several variants of both Read() and Write().
The basic Read(TDes8& aDes) and Write(const TDesC8& aDes) are supplemented
by variants allowing the descriptor length to be overridden, or the seek
position of the first byte to be specified, or asynchronous completion,
or any combination.

Reading transfers data from a file to a descriptor, and writing transfers
data from a descriptor to a file. In all cases, the file data is treated
as binary and byte descriptors are used (TDes8, TDesC8).

@see RFile
*/
class RFile64 : public RFile
	{
public:
	EFSRV_IMPORT_C TInt Open(RFs& aFs,const TDesC& aName,TUint aFileMode);
	EFSRV_IMPORT_C TInt Create(RFs& aFs,const TDesC& aName,TUint aFileMode);
	EFSRV_IMPORT_C TInt Replace(RFs& aFs,const TDesC& aName,TUint aFileMode);
	EFSRV_IMPORT_C TInt Temp(RFs& aFs,const TDesC& aPath,TFileName& aName,TUint aFileMode);

	EFSRV_IMPORT_C TInt AdoptFromClient(const RMessage2& aMsg, TInt aFsIndex, TInt aFileIndex);
	EFSRV_IMPORT_C TInt AdoptFromServer(TInt aFsHandle, TInt aFileHandle);
	EFSRV_IMPORT_C TInt AdoptFromCreator(TInt aFsIndex, TInt aFileHandleIndex);
	
	inline TInt Read(TDes8& aDes) const;
	inline void Read(TDes8& aDes,TRequestStatus& aStatus) const;
	inline TInt Read(TDes8& aDes,TInt aLength) const;
	inline void Read(TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const;
	
	EFSRV_IMPORT_C TInt Read(TInt64 aPos, TDes8& aDes) const;
	EFSRV_IMPORT_C void Read(TInt64 aPos, TDes8& aDes, TRequestStatus& aStatus) const;
	EFSRV_IMPORT_C TInt Read(TInt64 aPos, TDes8& aDes, TInt aLength) const;
	EFSRV_IMPORT_C void Read(TInt64 aPos, TDes8& aDes, TInt aLength,TRequestStatus& aStatus) const;
	
	inline TInt Write(const TDesC8& aDes);
	inline void Write(const TDesC8& aDes,TRequestStatus& aStatus);
	inline TInt Write(const TDesC8& aDes,TInt aLength);
	inline void Write(const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus);
	
	EFSRV_IMPORT_C TInt Write(TInt64 aPos, const TDesC8& aDes);
	EFSRV_IMPORT_C void Write(TInt64 aPos, const TDesC8& aDes,TRequestStatus& aStatus);
	EFSRV_IMPORT_C TInt Write(TInt64 aPos, const TDesC8& aDes,TInt aLength);
	EFSRV_IMPORT_C void Write(TInt64 aPos, const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus);
	
	EFSRV_IMPORT_C TInt Seek(TSeek aMode, TInt64& aPos) const;
	EFSRV_IMPORT_C TInt Size(TInt64& aSize) const;
	EFSRV_IMPORT_C TInt SetSize(TInt64 aSize);
	EFSRV_IMPORT_C TInt Lock(TInt64 aPos, TInt64 aLength) const;
	EFSRV_IMPORT_C TInt UnLock(TInt64 aPos, TInt64 aLength) const;
	
#if defined(_F32_STRICT_64_BIT_MIGRATION)
//
// If _F32_STRICT_64_BIT_MIGRATION is defined, hide TUint overloads of RFile64::Read 
// and RFile64::Write APIs to force compiler errors when TUint positions are used.
//
private:
#endif
	
	EFSRV_IMPORT_C TInt Read(TUint aPos,TDes8& aDes) const;
	EFSRV_IMPORT_C void Read(TUint aPos,TDes8& aDes,TRequestStatus& aStatus) const;
	EFSRV_IMPORT_C TInt Read(TUint aPos,TDes8& aDes,TInt aLength) const;
	EFSRV_IMPORT_C void Read(TUint aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const;
	
	EFSRV_IMPORT_C TInt Write(TUint aPos,const TDesC8& aDes);
	EFSRV_IMPORT_C void Write(TUint aPos,const TDesC8& aDes,TRequestStatus& aStatus);
	EFSRV_IMPORT_C TInt Write(TUint aPos,const TDesC8& aDes,TInt aLength);
	EFSRV_IMPORT_C void Write(TUint aPos,const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus);
	
#if defined(_F32_STRICT_64_BIT_MIGRATION)
//
// If _F32_STRICT_64_BIT_MIGRATION is defined, create private overloads of legacy 32-bit 
// RFile Read/Write API's to force compiler errors when TInt positions are used.
//
private:
#endif
	
	inline TInt Read(TInt aPos,TDes8& aDes) const;
	inline void Read(TInt aPos,TDes8& aDes,TRequestStatus& aStatus) const;
	inline TInt Read(TInt aPos,TDes8& aDes,TInt aLength) const;
	inline void Read(TInt aPos,TDes8& aDes,TInt aLength,TRequestStatus& aStatus) const;
	
	inline TInt Write(TInt aPos,const TDesC8& aDes);
	inline void Write(TInt aPos,const TDesC8& aDes,TRequestStatus& aStatus);
	inline TInt Write(TInt aPos,const TDesC8& aDes,TInt aLength);
	inline void Write(TInt aPos,const TDesC8& aDes,TInt aLength,TRequestStatus& aStatus);
	
private:
	TInt Seek(TSeek aMode, TInt& aPos) const;	// This API is not supported for RFile64
	TInt Size(TInt& aSize) const;			 	// This API is not supported for RFile64
	friend class RFilePlugin;
	};

#ifndef  SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
#include <f32file64.inl>
#endif

#endif
