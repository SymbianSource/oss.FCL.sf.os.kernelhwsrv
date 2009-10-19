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
// base\omap_hrp\h4_bootloader\inflate2.cpp
// For inflate image which is compressed by Deflate algortihm instead of ZIP
// (The ROM header un-compressed, the rest part of the image is compressed.)
// 
//

#define FILE_ID	0x4C5A4955

#include <e32def.h>
#include <e32def_private.h>
#include <e32cmn.h>
#include <e32rom.h>

#include "inflate2.h"

#include <e32std.h>
#include <e32std_private.h>
#include "bootldr.h"
#include "unzip.h"

#include <f32file.h>
#include <e32svr.h>


#define PTRADD(T,p,x)	((T*)((char*)(p)+(x)))
#define MIN(a,b)		(((a)<(b))?(a):(b))


// bit-stream input class
inline TUint reverse(TUint aVal)
//
// Reverse the byte-order of a 32 bit value
// This generates optimal ARM code (4 instructions)
//
	{
	TUint v=(aVal<<16)|(aVal>>16);
	v^=aVal;
	v&=0xff00ffff;
	aVal=(aVal>>8)|(aVal<<24);
	return aVal^(v>>8);
	}



void HexDump(TUint8 * aStartAddress, TUint aLength)
    {
    TUint index;
    for( index = 0; index != aLength; ++index)
        {
        if( index % 16 == 0)
            {
            PrintToScreen(_L("\r\n0x%08x: "),aStartAddress + index);   
            }
            
        PrintToScreen(_L("%02x "), *(aStartAddress+index));
        
        }
    PrintToScreen(_L("\r\n\r\n"));
    }



/******************************************************************************************************
	Bit input Stream code
 *****************************************************************************************************/



/** Construct a bit stream input object

	Following construction the bit stream is ready for reading bits, but will
	immediately call UnderflowL() as the input buffer is empty.
*/
TBitInput::TBitInput()
    :iCount(0)
	,iRemain(0)
	{}

/** Construct a bit stream input object over a buffer

	Following construction the bit stream is ready for reading bits from
	the specified buffer.

	@param aPtr The address of the buffer containing the bit stream
	@param aLength The length of the bitstream in bits
	@param aOffset The bit offset from the start of the buffer to the bit stream (defaults to zero)
*/
TBitInput::TBitInput(const TUint8* aPtr, TInt aLength, TInt aOffset)
	{
	Set(aPtr,aLength,aOffset);
	}

/** Set the memory buffer to use for input

	Bits will be read from this buffer until it is empty, at which point
	UnderflowL() will be called.
	
	@param aPtr The address of the buffer containing the bit stream
	@param aLength The length of the bitstream in bits
	@param aOffset The bit offset from the start of the buffer to the bit stream (defaults to zero)
*/
void TBitInput::Set(const TUint8* aPtr, TInt aLength, TInt aOffset)
	{
	TUint p=(TUint)aPtr;
	p+=aOffset>>3;			// nearest byte to the specified bit offset
	aOffset&=7;				// bit offset within the byte
	const TUint32* ptr=(const TUint32*)(p&~3);	// word containing this byte
	aOffset+=(p&3)<<3;		// bit offset within the word
	if (aLength==0)
		iCount=0;
	else
		{
		// read the first few bits of the stream
		iBits=reverse(*ptr++)<<aOffset;
		aOffset=32-aOffset;
		aLength-=aOffset;
		if (aLength<0)
			aOffset+=aLength;
		iCount=aOffset;
		}
	iRemain=aLength;
	iPtr=ptr;
	}

//#define __HUFFMAN_MACHINE_CODED__

#ifndef __HUFFMAN_MACHINE_CODED__
/** Read a single bit from the input

	Return the next bit in the input stream. This will call UnderflowL() if
	there are no more bits available.

	@return The next bit in the stream

	@leave "UnderflowL()" It the bit stream is exhausted more UnderflowL is called
		to get more data
*/
TUint TBitInput::ReadL()
	{
	TInt c=iCount;
	TUint bits=iBits;
	if (--c<0)
		return ReadL(1);
	iCount=c;
	iBits=bits<<1;
	return bits>>31;
	}

/** Read a multi-bit value from the input

	Return the next few bits as an unsigned integer. The last bit read is
	the least significant bit of the returned value, and the value is
	zero extended to return a 32-bit result.

	A read of zero bits will always reaturn zero.
	
	This will call UnderflowL() if there are not enough bits available.

	@param aSize The number of bits to read

	@return The bits read from the stream

	@leave "UnderflowL()" It the bit stream is exhausted more UnderflowL is called
		to get more data
*/
TUint TBitInput::ReadL(TInt aSize)
	{
	if (!aSize)
		return 0;
	TUint val=0;
	TUint bits=iBits;
	iCount-=aSize;
	while (iCount<0)
		{
		// need more bits
		val|=bits>>(32-(iCount+aSize))<<(-iCount);	// scrub low order bits

		aSize=-iCount;	// bits still required
		if (iRemain>0)
			{
			bits=reverse(*iPtr++);
			iCount+=32;
			iRemain-=32;
			if (iRemain<0)
				iCount+=iRemain;
			}
		else
			{
			UnderflowL();
			bits=iBits;
			iCount-=aSize;
			}
		}
//#ifdef __CPU_X86
	// X86 does not allow shift-by-32
//	iBits=aSize==32?0:bits<<aSize;
//#else
	iBits=bits<<aSize;
//#endif
	return val|(bits>>(32-aSize));
	}

/** Read and decode a Huffman Code

	Interpret the next bits in the input as a Huffman code in the specified
	decoding. The decoding tree should be the output from Huffman::Decoding().

	@param aTree The huffman decoding tree

	@return The symbol that was decoded
	
	@leave "UnderflowL()" It the bit stream is exhausted more UnderflowL is called
		to get more data
*/
TUint TBitInput::HuffmanL(const TUint32* aTree)
	{
	TUint huff=0;
	do
		{
		aTree=PTRADD(TUint32,aTree,huff>>16);
		huff=*aTree;
		if (ReadL()==0)
			huff<<=16;
		} while ((huff&0x10000u)==0);
	return huff>>17;
	}

#endif


/** Handle an empty input buffer

	This virtual function is called when the input buffer is empty and
	more bits are required. It should reset the input buffer with more
	data using Set().

	A derived class can replace this to read the data from a file
	(for example) before reseting the input buffer.

	@leave KErrUnderflow The default implementation leaves
*/
void TBitInput::UnderflowL()
	{
	
	}



/******************************************************************************************************
	Huffman Code
 *****************************************************************************************************/

TUint32* HuffmanSubTree(TUint32* aPtr,const TUint32* aValue,TUint32** aLevel)
//
// write the subtree below aPtr and return the head
//
	{
	TUint32* l=*aLevel++;
	if (l>aValue)
		{
		TUint32* sub0=HuffmanSubTree(aPtr,aValue,aLevel);	// 0-tree first
		aPtr=HuffmanSubTree(sub0,aValue-(aPtr-sub0)-1,aLevel);			// 1-tree
		TInt branch0=(TUint8*)sub0-(TUint8*)(aPtr-1);
		*--aPtr=KBranch1|branch0;
		}
	else if (l==aValue)
		{
		TUint term0=*aValue--;						// 0-term
		aPtr=HuffmanSubTree(aPtr,aValue,aLevel);			// 1-tree
		*--aPtr=KBranch1|(term0>>16);
		}
	else	// l<iNext
		{
		TUint term0=*aValue--;						// 0-term
		TUint term1=*aValue--;
		*--aPtr=(term1>>16<<16)|(term0>>16);
		}
	return aPtr;
	}



/** Create a canonical Huffman decoding tree

	This generates the huffman decoding tree used by TBitInput::HuffmanL() to read huffman
	encoded data. The input is table of code lengths, as generated by Huffman::HuffmanL()
	and must represent a valid huffman code.
	
	@param aHuffman The table of code lengths as generated by Huffman::HuffmanL()
	@param aNumCodes The number of codes in the table
	@param aDecodeTree The space for the decoding tree. This must be the same
		size as the code-length table, and can safely be the same memory
	@param  aSymbolBase the base value for the output 'symbols' from the decoding tree, by default
		this is zero.

	@panic "USER ???" If the provided code is not a valid Huffman coding

	@see IsValid()
	@see HuffmanL()
*/
void Huffman::Decoding(const TUint32 aHuffman[],TInt aNumCodes,TUint32 aDecodeTree[],TInt aSymbolBase)
	{
#ifdef _DEBUG
	if(!IsValid(aHuffman,aNumCodes))
		{
#ifdef __LED__
		leds(0xBAD00006); 	
#endif

		}
#endif
	TInt counts[KMaxCodeLength];
	memset1(counts, 0, (sizeof(TInt)*KMaxCodeLength));
	
	TInt codes=0;
	TInt ii;
	for (ii=0;ii<aNumCodes;++ii)
		{
		TInt len=aHuffman[ii];
		aDecodeTree[ii]=len;
		if (--len>=0)
			{
			++counts[len];
			++codes;
			}
		}
		
	TUint32* level[KMaxCodeLength];

	TUint32* lit=aDecodeTree+codes;
	for (ii=0;ii<KMaxCodeLength;++ii)
		{
		level[ii]=lit;
		lit-=counts[ii];
		}
	aSymbolBase=(aSymbolBase<<17)+(KHuffTerminate<<16);
	for (ii=0;ii<aNumCodes;++ii)
		{
		TUint len=TUint8(aDecodeTree[ii]);
		if (len)
			*--level[len-1]|=(ii<<17)+aSymbolBase;
		}
	if (codes==1)	// codes==1 special case: incomplete tree
		{
		TUint term=aDecodeTree[0]>>16;
		aDecodeTree[0]=term|(term<<16); // 0- and 1-terminate at root
		}
	else if (codes>1)
		HuffmanSubTree(aDecodeTree+codes-1,aDecodeTree+codes-1,&level[0]);
	}

// The decoding tree for the externalised code
const TUint32 HuffmanDecoding[]=
	{
	0x0004006c,
	0x00040064,
	0x0004005c,
	0x00040050,
	0x00040044,
	0x0004003c,
	0x00040034,
	0x00040021,
	0x00040023,
	0x00040025,
	0x00040027,
	0x00040029,
	0x00040014,
	0x0004000c,
	0x00040035,
	0x00390037,
	0x00330031,
	0x0004002b,
	0x002f002d,
	0x001f001d,
	0x001b0019,
	0x00040013,
	0x00170015,
	0x0004000d,
	0x0011000f,
	0x000b0009,
	0x00070003,
	0x00050001
	};

/** Restore a canonical huffman encoding from a bit stream

	The encoding must have been stored using Huffman::ExternalizeL(). The resulting
	code-length table can be used to create an encoding table using Huffman::Encoding()
	or a decoding tree using Huffman::Decoding().
	
	@param aInput The input stream with the encoding
	@param aHuffman The internalized code-length table is placed here
	@param aNumCodes The number of huffman codes in the table

	@leave TBitInput::HuffmanL()

	@see ExternalizeL()
*/
void Huffman::InternalizeL(TBitInput& aInput,TUint32 aHuffman[],TInt aNumCodes)
// See ExternalizeL for a description of the format
	{

	// initialise move-to-front list
	TUint8 list[Huffman::KMetaCodes];
	for (TInt i=0;i<Huffman::KMetaCodes;++i)	
		list[i]=TUint8(i);

	TInt last=0;
	// extract codes, reverse rle-0 and mtf encoding in one pass
	TUint32* p=aHuffman;
	const TUint32* end=aHuffman+aNumCodes;
	TInt rl=0;
	while (p+rl<end)
		{
		TInt c=aInput.HuffmanL(HuffmanDecoding);
		if (c<2)
			{
			// one of the zero codes used by RLE-0
			// update he run-length
			rl+=rl+c+1;
			}
		else
			{
			while (rl>0)
				{
				*p++=last;
				--rl;
				}
			--c;
			list[0]=TUint8(last);
			last=list[c];
			memcpy1(&list[1],&list[0],c);
			*p++=last;
			}
		}
	while (rl>0)
		{
		*p++=last;
		--rl;
		}
	}




/** Validate a Huffman encoding

	This verifies that a Huffman coding described by the code lengths is valid.
	In particular, it ensures that no code exceeds the maximum length and
	that it is possible to generate a canonical coding for the specified lengths.
	
	@param aHuffman The table of code lengths as generated by Huffman::HuffmanL()
	@param aNumCodes The number of codes in the table

	@return True if the code is valid, otherwise false
*/
TBool Huffman::IsValid(const TUint32 aHuffman[],TInt aNumCodes)
	{
	// The code is valid if one of the following holds:
	// (a) the code exactly fills the 'code space'
	// (b) there is only a single symbol with code length 1
	// (c) there are no encoded symbols
	//
	TUint remain=1<<KMaxCodeLength;
	TInt totlen=0;
	for (const TUint32* p=aHuffman+aNumCodes; p>aHuffman;)
		{
		TInt len=*--p;
		if (len>0)
			{
			totlen+=len;
			if (len>KMaxCodeLength)
				return 0;
			TUint c=1<<(KMaxCodeLength-len);
			if (c>remain)
				return 0;
			remain-=c;
			}
		}

	return remain==0 || totlen<=1;
	}



TInt Inflater::Inflate(TBitInput& aBits, TUint8* aBuffer, TInt aSize)
	{
	TEncoding encoding;
	TInt r = Init(aBits, encoding);
	if (r==KErrNone)
		r = DoInflate(aBits, encoding, aBuffer, aSize);
	return r;
	}

TInt Inflater::Init(TBitInput& aBits, TEncoding& aEncoding)
	{
// read the encoding
	Huffman::InternalizeL(aBits,aEncoding.iLitLen,KDeflationCodes);
// validate the encoding
	if (!Huffman::IsValid(aEncoding.iLitLen,TEncoding::ELitLens) ||
		!Huffman::IsValid(aEncoding.iDistance,TEncoding::EDistances))
		return KErrCorrupt;
// convert the length tables into huffman decoding trees
	Huffman::Decoding(aEncoding.iLitLen,TEncoding::ELitLens,aEncoding.iLitLen);
	Huffman::Decoding(aEncoding.iDistance,TEncoding::EDistances,aEncoding.iDistance);
	return KErrNone;
	}


 
TInt Inflater::DoInflate(TBitInput& aBits, TEncoding& aEncoding, TUint8* aBuffer, TInt aSize)
	{
	TUint8* out=aBuffer;
	TUint8* const end=out+aSize;
//
	while (out<end)
		{
		// get a huffman code
		TInt code=aBits.HuffmanL(aEncoding.iLitLen)-TEncoding::ELiterals;
		if (code<0)
			{
			*out++=TUint8(code);
			continue;			// another literal/length combo
			}
		if (code==TEncoding::EEos-TEncoding::ELiterals)
			{	// eos marker. we're done
			break;
			}
		// get the extra bits for the length code
		if (code>=8)
			{
			TInt xtra=(code>>2)-1;
			code-=xtra<<2;
			code<<=xtra;
			code|=aBits.ReadL(xtra);
			}
		TInt len=code+KDeflateMinLength;
		// get the distance code
		code=aBits.HuffmanL(aEncoding.iDistance);
		if (code>=8)
			{
			TInt xtra=(code>>2)-1;
			code-=xtra<<2;
			code<<=xtra;
			code|=aBits.ReadL(xtra);
			}
		TUint8* dptr = out-(code+1);
		TInt wlen = MIN(end-out,len);
		for(TInt i=0;i<wlen;i++)	//this byte by byte copy is required in stead of a memcpy as over lap required. memcopy does
		    {
		    *out++=*dptr++;			//not do much better as the length of copies are short ie over the 16 byte threshold    
		    }
			
		};
	return out-aBuffer;
	}


TFileInput::TFileInput(TInt aBlockLen, TInt aFileSize)
    :iReadBuf(iBuf1)
    ,iPtr(iBuf1,KBufSize)
    ,iBlockLen(aBlockLen)
    ,iFileSize(aFileSize)
    ,iImageReadProgress(0)
	{
	// Avoid buffer overrrun
	if( aBlockLen > KBufSize)
	    iBlockLen = KBufSize;
	
	// issue first read
	iState=ReadInputData(iReadBuf,iBlockLen);
	iImageReadProgress += iBlockLen;
	}

void TFileInput::Init()
    {
    Set(iReadBuf, iBlockLen*8);
    InitProgressBar(0,(TUint)iFileSize,_L("LOAD"));        
    }

void TFileInput::UnderflowL()
	{
	TUint8* b=iReadBuf;
	ASSERT(b!=NULL);
	Set(b, iBlockLen*8);
	
	// start reading to the next buffer
	b = iBuf1;
	iState=ReadInputData(b,iBlockLen);
	Set(b, iBlockLen*8);
	iReadBuf=b;

	// Update progress
	iImageReadProgress += iBlockLen;
	UpdateProgressBar(0,(TUint)iImageReadProgress);

#ifdef __SUPPORT_FLASH_REPRO__	
	NotifyDataAvailable(iImageReadProgress);
#endif	
	}


void memcpy1(TAny* aTrg, const TAny* aSrc, unsigned int aLength)
//
// Copy from the aSrc to aTrg for aLength bytes.
//
	{
	TInt aLen32=0;
	TUint32* pT32=(TUint32*)aTrg;
	const TUint32* pS32=(TUint32 *)aSrc;
	TInt aLen8;
	TUint32* pE32;
	TUint8* pT;
    TUint8* pE;
    TUint8* pS;
	
	if (aLength==0)
		return;//((TUint8*)aTrg);
	
	if (((TInt)pT32&3)==0 && ((TInt)pS32&3)==0)
		aLen32=aLength>>2;
	aLen8=aLength-(aLen32<<2);
	pE32=pT32+aLen32;
	if (aTrg<aSrc)
		{
		pS32=(TUint32*)aSrc;
		while (pT32<pE32)
			*pT32++=(*pS32++);
		pT=(TUint8*)pT32;
		pS=(TUint8*)pS32;
		pE=(TUint8*)aTrg+aLength;
		while (pT<pE)
			*pT++=(*pS++);
		}
	else if (aTrg>aSrc)
		{
		pT=(TUint8*)(pT32+aLen32);
		pE=pT+aLen8;
		pS=(TUint8*)aSrc+aLength;
		while (pE>pT)
			*--pE=(*--pS);
		pS32=(TUint32*)pS;
		while (pE32>pT32)
			*--pE32=(*--pS32);
		}
	}


void memset1(void* aTrg, int aValue, unsigned int aLength)
//
// Fill memory with aLength aChars.
//
	{
	TInt aLen32=0;
	TUint32 *pM32=(TUint32 *)aTrg;
	TUint32 *pE32;
	TUint c;
	TUint32 fillChar;
	TInt aLen8;
	TUint8 *pM;
	TUint8 *pE;
	
	if (((TInt)aTrg&3)==0)
		{
		aLen32=aLength>>2;
		pE32=pM32+aLen32;
		c = aValue & 0xff;
		fillChar=c+(c<<8)+(c<<16)+(c<<24);
		while (pM32<pE32)
			*pM32++=fillChar;
		}
	aLen8=aLength-(aLen32<<2);
	pM=(TUint8 *)pM32;
	pE=pM+aLen8;
	while (pM<pE)
		*pM++=(TUint8)(aValue);
	}


TInt memcmp1(const TUint8* aTrg, const TUint8* aSrc, TInt aLength)
//
// Compare aSrc with aTrg
//
	{
	for (TInt n=0; n<aLength; n++)
		{
		if (aTrg[n] != aSrc[n])
			return -1;
		}
	return 0;
	}


#ifdef SYMBIAN_CHECK_ROM_CHECKSUM
TUint Check(const TUint32* aPtr, TInt aSize)
	{
	TUint sum=0;
	aSize/=4;
	while (aSize-->0)
		sum+=*aPtr++;
	return sum;
	}

TInt CheckRomChecksum(TRomHeader& aRomHeader)
	{

	TInt size = aRomHeader.iUnpagedUncompressedSize;
	const TUint32* addr = (TUint32*) &aRomHeader;
#ifdef _DEBUG_CORELDR_
	PrintVal("ROM addr = ", (TUint32) addr);
	PrintVal("ROM size = ", (TUint32) size);
#endif

	TUint checkSum = Check(addr, size);

	// modify the checksum because ROMBUILD is broken...
	checkSum -= (aRomHeader.iRomSize-size)/4; // adjust for missing 0xffffffff
	checkSum -= aRomHeader.iCompressionType;
	checkSum -= aRomHeader.iUnpagedCompressedSize;
	checkSum -= aRomHeader.iUnpagedUncompressedSize;

	TUint expectedChecksum = 0x12345678;
#ifdef _DEBUG_CORELDR_
	PrintVal("Checksum = ", checkSum);
	PrintVal("expectedChecksum = ", expectedChecksum);
#endif

	return (checkSum==expectedChecksum)?0:-2;
	}
#endif 


int DoDeflateDownload()
    {    
    // Read ROM Loader Header
    TInt r = KErrNone;
    TInt headerSize = TROM_LOADER_HEADER_SIZE;
    
    if(RomLoaderHeaderExists)
        {
        TUint8 romLoaderHeader[TROM_LOADER_HEADER_SIZE];
    	FileSize -= headerSize;
    	r = ReadInputData((TUint8*)&romLoaderHeader, headerSize);
    	if( KErrNone!=r)
    		{
    		PrintToScreen(_L("Unable to read loader header... (size:%d)\r\n"), headerSize);
    		BOOT_FAULT();    
    		}
        }
    
	
	// Read ROM Header
	TRomHeader* romHeader;
	romHeader = (TRomHeader*)DestinationAddress();
	
	headerSize = sizeof(TRomHeader);
	r = ReadInputData((TUint8*)romHeader, headerSize);
	if( KErrNone!=r)
		{
        PrintToScreen(_L("Unable to read ROM header... (size:%d)\r\n"), headerSize);
		BOOT_FAULT();
		}
		
	DEBUG_PRINT((_L("headerSize       :%d\r\n"), headerSize));	
	DEBUG_PRINT((_L("iRomHeaderSize   :0x%08x\r\n"), romHeader->iRomHeaderSize));
	DEBUG_PRINT((_L("iDebugPort       :0x%08x\r\n"), romHeader->iDebugPort));
	DEBUG_PRINT((_L("iVersion         :%d.%d %d\r\n"), romHeader->iVersion.iMajor, romHeader->iVersion.iMinor, romHeader->iVersion.iBuild));
	DEBUG_PRINT((_L("iCompressionType :0x%08x\r\n"), romHeader->iCompressionType));
	DEBUG_PRINT((_L("iCompressedSize  :0x%08x\r\n"), romHeader->iCompressedSize));
	DEBUG_PRINT((_L("iUncompressedSize:0x%08x\r\n"), romHeader->iUncompressedSize));
	
	DEBUG_PRINT((_L("iCompressedUnpagedStart:0x%08x\r\n"), romHeader->iCompressedUnpagedStart));
	DEBUG_PRINT((_L("iUnpagedCompressedSize:0x%08x\r\n"), romHeader->iUnpagedCompressedSize));
	DEBUG_PRINT((_L("iUnpagedUncompressedSize:0x%08x\r\n"), romHeader->iUnpagedUncompressedSize));
	
	if( romHeader->iCompressionType != KUidCompressionDeflate )
		{
		PrintToScreen(_L("Not supported compression method:0x%08x\r\n"), romHeader->iCompressionType);
	    BOOT_FAULT();   
    	}

    TUint8 * pScr = (TUint8 *)DestinationAddress();

    DEBUG_PRINT((_L("Load address:0x%08x.\r\n"), pScr));

    if( romHeader->iCompressedUnpagedStart > (TUint)headerSize )
    	{
       	// Copy uncompressed un-paged part (bootstrap + Page Index Table) to the proper place if it longer than the romHeader
    	TInt unpagedSize = (romHeader->iCompressedUnpagedStart - headerSize);
    	
       	DEBUG_PRINT((_L("Copy uncompressed un-paged part ...\r\n")));         
       	DEBUG_PRINT((_L("to   :0x%08x.\r\n"),((TUint8 *)DestinationAddress()+headerSize) ));
       	DEBUG_PRINT((_L("len  :0x%08x.\r\n"),unpagedSize ));
       	
   		r = ReadInputData(((TUint8 *)DestinationAddress()+headerSize), unpagedSize);
   	  	// Modify header size to include the un-paged part such that the inflate code will not need to be modified
       	headerSize = unpagedSize;
   		if( KErrNone!=r)
   			{
   			PrintToScreen(_L("uncompressed un-paged part... (size:%d)\r\n"), headerSize);
   			BOOT_FAULT();
   			}
       }
    
    pScr += (headerSize + romHeader->iUnpagedUncompressedSize);    
    DEBUG_PRINT((_L("Compressed image load address:0x%08x.\r\n"), pScr));
    
    FileSize = romHeader->iUnpagedCompressedSize;
    
#ifdef __SUPPORT_FLASH_REPRO__
	if (LoadToFlash)
		ImageSize = ((romHeader->iUnpagedUncompressedSize | 0x3ff) + 1);  // Round it to 0x400 for flashing
#endif
        
    ImageReadProgress=0;
    TInt block_size = Max(0x1000,FileSize>>8);
    
	DEBUG_PRINT((_L("Compressed image loaded into the RAM for decompress.\r\n")));		
   
    pScr = (TUint8 *)DestinationAddress();
    pScr += (headerSize + romHeader->iUnpagedUncompressedSize);
    
    TFileInput image(block_size, FileSize);
    image.Init();
    
#ifdef __SUPPORT_FLASH_REPRO__
	if (LoadToFlash)
		{

		DEBUG_PRINT((_L("InitFlashWrite. ImageSize:%d (0x%08x).\r\n"), ImageSize, ImageSize));	

		r=InitFlashWrite();
		if (r!=KErrNone)
			{
			PrintToScreen(_L("FAULT due to InitFlashWrite return %d\r\n"), r);
			BOOT_FAULT();
			}
		}
#endif  // __SUPPORT_FLASH_REPRO__
    
    
	DEBUG_PRINT((_L("(TUint8 *)(DestinationAddress() + headerSize):0x%08x, size:%d.\r\n"),(TUint8 *)(DestinationAddress() + headerSize), romHeader->iUnpagedUncompressedSize));

    
    TUint nChars = Inflater::Inflate(
        image,
        (TUint8 *)(DestinationAddress() + headerSize),
        romHeader->iUnpagedUncompressedSize
        );
    
    
    DEBUG_PRINT((_L("Decompressed %d bytes.\r\n"), nChars));	


    if( 0 > (TInt)nChars)
        {
        PrintToScreen(_L("Error in decompression, return code: %d.\r\n"), nChars);
        BOOT_FAULT();
        }
    
#ifdef __SUPPORT_FLASH_REPRO__
	if (LoadToFlash)
        {

	    DEBUG_PRINT((_L("NotifyDataAvailable. ImageSize:%d (0x%08x).\r\n"), ImageSize, ImageSize));	
	    
		NotifyDataAvailable(ImageSize);

    	DEBUG_PRINT((_L("NotifyDownloadComplete.\r\n")));

    	NotifyDownloadComplete();
        }
#else

	DELAY(20000);

#endif   //  __SUPPORT_FLASH_REPRO__
    
    return KErrNone;
    }

