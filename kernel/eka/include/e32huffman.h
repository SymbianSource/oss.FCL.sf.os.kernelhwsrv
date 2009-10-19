// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32huffman.h
// 
//

#include <e32std.h>

/** @file
	@internalTechnology
*/

/** Bit output stream.
	Good for writing bit streams for packed, compressed or huffman data algorithms.

	This class must be derived from and OverflowL() reimplemented if the bitstream data
	cannot be generated into a single memory buffer.
*/
class TBitOutput
	{
public:
	IMPORT_C TBitOutput();
	IMPORT_C TBitOutput(TUint8* aBuf,TInt aSize);
	inline void Set(TUint8* aBuf,TInt aSize);
	inline const TUint8* Ptr() const;
	inline TInt BufferedBits() const;
//
	IMPORT_C void WriteL(TUint aValue, TInt aLength);
	IMPORT_C void HuffmanL(TUint aHuffCode);
	IMPORT_C void PadL(TUint aPadding);
private:
	void DoWriteL(TUint aBits, TInt aSize);
	virtual void OverflowL();
private:
	TUint iCode;		// code in production
	TInt iBits;
	TUint8* iPtr;
	TUint8* iEnd;
	};

/** Set the memory buffer to use for output

	Data will be written to this buffer until it is full, at which point OverflowL() will
	be called. This should handle the data and then can Set() again to reset the buffer
	for further output.
	
	@param aBuf The buffer for output
	@param aSize The size of the buffer in bytes
*/
inline void TBitOutput::Set(TUint8* aBuf,TInt aSize)
	{iPtr=aBuf;iEnd=aBuf+aSize;}
	
/** Get the current write position in the output buffer

	In conjunction with the address of the buffer, which should be known to the
	caller, this describes the data in the bitstream.
*/
inline const TUint8* TBitOutput::Ptr() const
	{return iPtr;}
	
/** Get the number of bits that are buffered

	This reports the number of bits that have not yet been written into the
	output buffer. It will always lie in the range 0..7. Use PadL() to
	pad the data out to the next byte and write it to the buffer.
*/
inline TInt TBitOutput::BufferedBits() const
	{return iBits+8;}


/** Bit input stream. Good for reading bit streams for packed, compressed or huffman
	data algorithms.
*/
class TBitInput
	{
public:
	IMPORT_C TBitInput();
	IMPORT_C TBitInput(const TUint8* aPtr, TInt aLength, TInt aOffset=0);
	IMPORT_C void Set(const TUint8* aPtr, TInt aLength, TInt aOffset=0);
//
	IMPORT_C TUint ReadL();
	IMPORT_C TUint ReadL(TInt aSize);
	IMPORT_C TUint HuffmanL(const TUint32* aTree);
private:
	virtual void UnderflowL();
private:
	TInt iCount;
	TUint iBits;
	TInt iRemain;
	const TUint32* iPtr;
	};

/** Huffman code toolkit.

	This class builds a huffman encoding from a frequency table and builds
	a decoding tree from a code-lengths table

	The encoding generated is based on the rule that given two symbols s1 and s2, with 
	code length l1 and l2, and huffman codes h1 and h2:

		if l1<l2 then h1<h2 when compared lexicographically
		if l1==l2 and s1<s2 then h1<h2 ditto

	This allows the encoding to be stored compactly as a table of code lengths
*/
class Huffman
	{
public:
	enum {KMaxCodeLength=27};
	enum {KMetaCodes=KMaxCodeLength+1};
	enum {KMaxCodes=0x8000};
public:
	IMPORT_C static void HuffmanL(const TUint32 aFrequency[],TInt aNumCodes,TUint32 aHuffman[]);
	IMPORT_C static void Encoding(const TUint32 aHuffman[],TInt aNumCodes,TUint32 aEncodeTable[]);
	IMPORT_C static void Decoding(const TUint32 aHuffman[],TInt aNumCodes,TUint32 aDecodeTree[],TInt aSymbolBase=0);
	IMPORT_C static TBool IsValid(const TUint32 aHuffman[],TInt aNumCodes);
//
	IMPORT_C static void ExternalizeL(TBitOutput& aOutput,const TUint32 aHuffman[],TInt aNumCodes);
	IMPORT_C static void InternalizeL(TBitInput& aInput,TUint32 aHuffman[],TInt aNumCodes);
	};
