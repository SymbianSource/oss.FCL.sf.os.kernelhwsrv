// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfsrv\cl_ftext.cpp
// 
//

#include "cl_std.h"




EXPORT_C TFileText::TFileText()
/**
Default constructor.
*/
	{}




EXPORT_C void TFileText::Set(RFile& aFile)
/**
Sets the Unicode file to be read from, or written to.

This function must be called before 
Read(), Write() or Seek() can be used.

@param aFile The file to be used. Must be open.

@see TFileText::Read
@see TFileText::Write
@see TFileText::Seek
*/
	{
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	iFile=aFile;
#else
	iFile=(RFile64&)aFile;
#endif
	iReadBuf.Zero();
	iNext=(TText*)iReadBuf.Ptr();
	iEnd=iNext;
#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	TInt pos = 0;
#else
	TInt64 pos = 0;
#endif
	iFile.Seek(ESeekCurrent,pos);
 	if (pos == 0)
 		iState = EStartOfFile;
 	else
 		iState = ENormal;
	}




EXPORT_C TInt TFileText::Read(TDes& aDes)
/**
Reads single line text record from a Unicode file into the specified descriptor.

The read operation begins at the current file position, and ends when
a line delimiter character is read.

If the maximum length of the descriptor is insufficient to hold the record, 
the function returns KErrTooBig and the descriptor is filled to its maximum 
length.

If Read() is called when the current position is the end of the file (that 
is, after the last line delimiter in the file), KErrEof is returned, and the 
length of the buffer is set to zero.

@param aDes On return, contains the single record read from the file. Any 
            previous contents are overwritten.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
	{

	TText* pD=(TText*)aDes.Ptr();
	TInt len=aDes.MaxLength();
	TInt newLen=0;
	while (newLen<len)
		{
		if (iNext>=iEnd)
			{
			TInt r=FillBuffer();
			if (r!=KErrNone && r!=KErrEof)
				return(r);
			if (r==KErrEof)
				{
				aDes.SetLength(newLen);
				return(newLen ? KErrNone : KErrEof);
				}
			continue;
			}
		TBool terminate=newLen;
		TInt r=CheckForTerminator(terminate);
		if (r!=KErrNone || terminate)
			{
			aDes.SetLength(newLen);
			return(r);
			}
		*pD++=(*iNext++);
		newLen++;
		}
	aDes.SetLength(newLen);
	TBool terminate=newLen;
	TInt r=CheckForTerminator(terminate);
	if (r!=KErrNone || terminate)
		return(r);
	NextRecord();
	return(KErrTooBig);
	}

void TFileText::NextRecord()
//
// Move to the start of the next record
//
	{

	FOREVER
		{
		TBool terminate=EFalse;
		TInt r=CheckForTerminator(terminate);
		if (r!=KErrNone || terminate)
			return;
		iNext++;
		}
	}

static void SwapWords(TText* aStart,TInt aCount)
 	{
 	TUint8* p = (TUint8*)aStart;
 	while (aCount-- > 0)
 		{
 		TUint8 temp = *p;
 		*p = p[1];
 		p[1] = temp;
 		p += 2;
   		}
   	}

TInt TFileText::FillBuffer()
//
// Read the new data from the file
//
	{
	
	TInt r=iFile.Read(iReadBuf);
	if (r!=KErrNone)
		return(r);
	if (iReadBuf.Length()==0)
		return(KErrEof);
	iNext=(const TText*)iReadBuf.Ptr();
	iEnd=iNext+iReadBuf.Length()/sizeof(TText);
	 
 	// Use any leading byte order marker to determine endianness.
 	if (iState == EStartOfFile)
 		{
 		iState = ENormal;

 		// Ignore an ordinary byte order marker.
 		if (*iNext == 0xFEFF)
 			iNext++;

 		// Set the endianness state to 'reverse' if a reversed byte order marker is found.
 		else if (*iNext == 0xFFFE)
 			{
 			iNext++;
 			iState = EReverse;
 			}
 
 		if (iNext == iEnd)
 			return KErrEof;
 		}
 
 	if (iState == EReverse)
		SwapWords((TText*)iNext,(iEnd - iNext));

	return(KErrNone);
	}

TInt TFileText::CheckForTerminator(TBool& anAnswer)
//
// Return ETrue if the next char is a record terminator: PARAGRAPH SEPARATOR (U+2029), LINE SEPARATOR (U+2028),
// CR-LF (U+000D, U+000A), or LF (U+000A)
//
	{

	if (iNext>=iEnd)
		{
		TInt r=FillBuffer();
		if (r!=KErrNone)
			{
			if (r==KErrEof && anAnswer)
				return(KErrNone);
			return(r);
			}
		}

	anAnswer=EFalse;
	const TText* oldNext=iNext;
	TInt oldBufferLength=iReadBuf.Length();
	TText c=(*iNext);
	TBool peek=EFalse;

	// Check for unambiguous paragraph or line separator.
 	if (c == 0x2029 || c == 0x2028)
 		{
 		iNext++;
 		anAnswer = ETrue;
		return KErrNone;
 		}
 
 	// Check for CR-LF or LF.
 	if (c == 0x000D)
		{
		iNext++;
		if (iNext<iEnd)
			c=(*iNext);
		else
			{
			peek=ETrue;
			TInt r=FillBuffer();
			if (r!=KErrNone && r!=KErrEof)
				return(r);
			if (r==KErrNone)
				c=(*iNext);
			}
		}

	if (c == 0x000A)
		{
		iNext++;
		anAnswer=ETrue;
		return(KErrNone);
		}

	iNext=oldNext;
	if (!peek)
		return(KErrNone);

#ifndef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	TInt pos=(-1)*(oldBufferLength+iReadBuf.Length());
#else
	TInt64 pos=(-1)*(oldBufferLength+iReadBuf.Length());
#endif
	TInt r=iFile.Seek(ESeekCurrent,pos);
	if (r==KErrNone)
		r=FillBuffer();
	if (r!=KErrNone)
		return(r);
	iNext=oldNext;
	return(KErrNone);
	}




EXPORT_C TInt TFileText::Write(const TDesC& aDes)
/**
Writes the contents of a descriptor to the end of a Unicode file.

A line delimiter is appended to the descriptor, and the current file position
is set to the new end of file.

If the descriptor contains one or more paragraph delimiters, Read() will treat 
the contents of the descriptor as more than one record.

@param aDes The descriptor content to be appended to the file.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
        
@see TFileText::Read
*/
	{

	TInt r=Seek(ESeekEnd);
	if (r!=KErrNone)
		return(r);
	TPtrC8 writeBuf((const TUint8*)aDes.Ptr(),aDes.Size());
	r=iFile.Write(writeBuf);
	if (r!=KErrNone)
		return(r);
 	TText lf = 0x000A;
 	TPtrC8 lf8((const TUint8*)&lf,sizeof(TText));
 	r=iFile.Write(lf8);
	return(r);
	}




EXPORT_C TInt TFileText::Seek(TSeek aMode)
/**
Seeks to start or end of file.

It is only necessary to call this function before 
using Read() because Write() always seeks to the end of the file
before writing.

@param aMode ESeekStart to seek to the start of the file;
             ESeekEnd to seek to the end.
             
@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.

@panic FSCLIENT 5 if aMode is neither ESeekStart nor ESeekEnd.

@see TFileText::Read
@see TFileText::Write
*/
	{

	__ASSERT_ALWAYS(aMode==ESeekStart || aMode==ESeekEnd,Panic(EFTextIllegalSeekMode));
#ifndef	SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
	TInt pos=0;
#else
	TInt64 pos = 0;
#endif
	TInt ret=iFile.Seek(aMode,pos);
 	if (ret == 0 && aMode == ESeekStart)
 		iState = EStartOfFile;
	iNext = (TText*)iReadBuf.Ptr();
 	iEnd = iNext;
	return ret;
	}
	
