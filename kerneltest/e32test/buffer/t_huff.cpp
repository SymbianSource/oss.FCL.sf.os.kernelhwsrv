// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/buffer/t_huff.cpp
// Overview:
// Test methods of the Huffman, TBitInput and TBitOutput classes.
// API Information:
// Huffman, TBitInput, TBitOutput
// Details:
// - Test and verify the results of TBitInput bit reading:
// - test and verify single bit reads, multiple bit reads and 32-bit reads
// - test and verify single bit reads and multiple bit reads from a 
// fractured input.
// - test and verify overrun reads
// - Test and verify the results of TBitOutput bit writing:
// - test and verify bitstream padding
// - test and verify single bit and multiple bit writes
// - test and verify overflow writes
// - Test and verify the results of a Huffman decoder using Huffman class 
// static methods, TBitOutput and TBitInput objects.
// - Test and verify the results of a Huffman generator for known distributions:
// flat, power-of-2 and Fibonacci.
// - Test and verify the results of a Huffman generator for random distributions:
// - generate random frequency distributions and verify:
// (a) the Huffman generator creates a mathematically 'optimal code'
// (b) the canonical encoding is canonical
// (c) the decoding tree correctly decodes each code
// (d) the encoding can be correctly externalised and internalised
// Platforms/Drives/Compatibility:
// All 
// Assumptions/Requirement/Pre-requisites:
// Failures and causes:
// Base Port information:
// 
//

#include <e32test.h>
#include <e32math.h>
#include <e32huffman.h>

RTest test(RProcess().FileName());

const Uint64 KTestData=UI64LIT(0x6f1b09a7e8c523d4);
const TUint8 KTestBuffer[] = {0x6f,0x1b,0x09,0xa7,0xe8,0xc5,0x23,0xd4};
const TInt KTestBytes=sizeof(KTestBuffer);
const TInt KTestBits=KTestBytes*8;

// Input stream: bit and multi-bit read tests with exhsautive buffer reload testing

typedef TBool (*TestFn)(TBitInput& aIn, Uint64 aBits, TInt aCount);

class TAlignedBitInput : public TBitInput
	{
public:
	TAlignedBitInput(const TUint8*,TInt,TInt);
private:
	void UnderflowL();
private:
	const TUint8* iRemainder;
	TInt iCount;
	};

TAlignedBitInput::TAlignedBitInput(const TUint8* aPtr,TInt aCount,TInt aOffset)
	:TBitInput(aPtr,32-aOffset,aOffset), iRemainder(aPtr+4), iCount(aOffset+aCount-32)
	{}

void TAlignedBitInput::UnderflowL()
	{
	if (!iRemainder)
		User::Leave(KErrUnderflow);
	else
		{
		Set(iRemainder,iCount);
		iRemainder=0;
		}
	}

class TSplitBitInput : public TBitInput
	{
public:
	TSplitBitInput(const TUint8*,TInt,TInt,TInt);
private:
	void UnderflowL();
private:
	const TUint8* iBase;
	TInt iBlockSize;
	TInt iOffset;
	TInt iAvail;
	};

TSplitBitInput::TSplitBitInput(const TUint8* aPtr,TInt aLength,TInt aOffset,TInt aSize)
	:TBitInput(aPtr,aSize,aOffset), iBase(aPtr), iBlockSize(aSize), iOffset(aOffset+aSize), iAvail(aLength-aSize)
	{}

void TSplitBitInput::UnderflowL()
	{
	TInt len=Min(iBlockSize,iAvail);
	if (len==0)
		User::Leave(KErrUnderflow);
	Set(iBase,len,iOffset);
	iOffset+=len;
	iAvail-=len;
	}

class TAlternateBitInput : public TBitInput
	{
public:
	TAlternateBitInput(const TUint8*,TInt,TInt);
private:
	void UnderflowL();
private:
	const TUint8* iBase;
	TInt iOffset;
	TInt iAvail;
	};

TAlternateBitInput::TAlternateBitInput(const TUint8* aPtr,TInt aLength,TInt aOffset)
	:TBitInput(aPtr,1,aOffset), iBase(aPtr), iOffset(aOffset+2), iAvail(aLength-2)
	{}

void TAlternateBitInput::UnderflowL()
	{
	if (iAvail<=0)
		User::Leave(KErrUnderflow);
	Set(iBase,1,iOffset);
	iOffset+=2;
	iAvail-=2;
	}

void TestReader(TBitInput& aIn, TestFn aFunc, Uint64 aBits, TInt aCount)
	{
	TBool eof=EFalse;
	TRAPD(r,eof=aFunc(aIn,aBits,aCount));
	test (r==KErrNone);
	if (eof)
		{
		TRAP(r,aIn.ReadL());
		test (r==KErrUnderflow);
		}
	}

void TestBits(TInt aOffset, TInt aCount, TestFn aFunc)
	{
	Uint64 bits=KTestData;
	if (aOffset)
		bits<<=aOffset;
	if (aCount<64)
		bits&=~((Uint64(1)<<(64-aCount))-1);
	// test with direct input
	TBitInput in1(KTestBuffer,aCount,aOffset);
	TestReader(in1,aFunc,bits,aCount);
	// test with aligned input
	if (aOffset<32 && aOffset+aCount>32)
		{
		TAlignedBitInput in2(KTestBuffer,aCount,aOffset);
		TestReader(in2,aFunc,bits,aCount);
		}
	// test with blocked input
	for (TInt block=aCount;--block>0;)
		{
		TSplitBitInput in3(KTestBuffer,aCount,aOffset,block);
		TestReader(in3,aFunc,bits,aCount);
		}
	}

void TestAlternateBits(TInt aOffset, TInt aCount, TestFn aFunc)
	{
	Uint64 bits=0;
	TInt c=0;
	for (TInt ix=aOffset;ix<aOffset+aCount;ix+=2)
		{
		if (KTestData<<ix>>63)
			bits|=Uint64(1)<<(63-c);
		++c;
		}
	// test with alternate input
	TAlternateBitInput in1(KTestBuffer,aCount,aOffset);
	TestReader(in1,aFunc,bits,c);
	}

void PermBits(TestFn aFunc, TInt aMinCount=1, TInt aMaxCount=64)
	{
	for (TInt offset=0;offset<KTestBits;++offset)
		for (TInt count=Min(KTestBits-offset,aMaxCount);count>=aMinCount;--count)
			TestBits(offset,count,aFunc);
	}

void AlternateBits(TestFn aFunc, TInt aMinCount=1)
	{
	for (TInt offset=0;offset<KTestBits;++offset)
		for (TInt count=KTestBits-offset;count>=aMinCount;--count)
			TestAlternateBits(offset,count,aFunc);
	}

TBool SingleBitRead(TBitInput& aIn, Uint64 aBits, TInt aCount)
	{
	while (--aCount>=0)
		{
		test (aIn.ReadL() == (aBits>>63));
		aBits<<=1;
		}
	return ETrue;
	}

TBool MultiBitRead(TBitInput& aIn, Uint64 aBits, TInt aCount)
	{
	TInt c=aCount/2;
	TUint v=aIn.ReadL(c);
	if (c==0)
		test (v==0);
	else
		{
		test (v==TUint(aBits>>(64-c)));
		aBits<<=c;
		}
	c=aCount-c;
	v=aIn.ReadL(c);
	if (c==0)
		test (v==0);
	else
		test (v==TUint(aBits>>(64-c)));
	return ETrue;
	}

TBool LongShortRead(TBitInput& aIn, Uint64 aBits, TInt aCount)
	{
	TUint v=aIn.ReadL(32);
	test (v==TUint(aBits>>32));
	aBits<<=32;
	TInt c=aCount-32;
	v=aIn.ReadL(c);
	if (c==0)
		test (v==0);
	else
		test (v==TUint(aBits>>(64-c)));
	return ETrue;
	}

TBool ShortLongRead(TBitInput& aIn, Uint64 aBits, TInt aCount)
	{
	TInt c=aCount-32;
	TUint v=aIn.ReadL(c);
	if (c==0)
		test (v==0);
	else
		{
		test (v==TUint(aBits>>(64-c)));
		aBits<<=c;
		}
	v=aIn.ReadL(32);
	test (v==TUint(aBits>>32));
	return ETrue;
	}

TBool EofRead(TBitInput& aIn, Uint64, TInt aCount)
	{
	TRAPD(r,aIn.ReadL(aCount+1));
	test(r==KErrUnderflow);
	return EFalse;
	}

void TestBitReading()
	{
	test.Start(_L("Test single bit reads"));
	PermBits(&SingleBitRead);
	test.Next(_L("Test multi bit reads"));
	PermBits(&MultiBitRead);
	test.Next(_L("Test 32-bit reads"));
	PermBits(&LongShortRead,32);
	PermBits(&ShortLongRead,32);
	test.Next(_L("Test single bit reads (fractured input)"));
	AlternateBits(&SingleBitRead);
	test.Next(_L("Test multi bit reads (fractured input)"));
	AlternateBits(&MultiBitRead);
	test.Next(_L("Test overrun reads"));
	PermBits(&EofRead,1,31);
	test.End();
	}

// Bit output testing (assumes bit input is correct)

void TestPadding()
	{
	TUint8 buffer[4];
	TBitOutput out(buffer,4);
	test(out.Ptr()==buffer);
	test(out.BufferedBits()==0);
	out.PadL(0);
	test(out.Ptr()==buffer);
	test(out.BufferedBits()==0);
	out.WriteL(0,0);
	out.PadL(0);
	test(out.Ptr()==buffer);
	test(out.BufferedBits()==0);

	TInt i;
	for (i=1;i<=8;++i)
		{
		out.Set(buffer,4);
		out.WriteL(0,i);
		test(out.BufferedBits()==(i%8));
		out.PadL(1);
		test(out.BufferedBits()==0);
		out.WriteL(0,i);
		test(out.BufferedBits()==(i%8));
		out.PadL(1);
		test(out.BufferedBits()==0);
		test (out.Ptr()==buffer+2);
		test (buffer[0]==(0xff>>i));
		test (buffer[1]==(0xff>>i));
		}

	for (i=1;i<=8;++i)
		{
		out.Set(buffer,4);
		out.WriteL(0xff,i);
		out.PadL(0);
		test (out.Ptr()==buffer+1);
		test (buffer[0]==(0xff^(0xff>>i)));
		}
	}

void TestBitWrites()
	{
	TUint8 buffer[KTestBytes];
	TBitOutput out(buffer,KTestBytes);
	TBitInput in(KTestBuffer,KTestBits);
	TInt i;
	for (i=KTestBits;--i>=0;)
		out.WriteL(in.ReadL(),1);
	test (Mem::Compare(buffer,KTestBytes,KTestBuffer,KTestBytes)==0);	

	Mem::FillZ(buffer,KTestBytes);
	out.Set(buffer,KTestBytes);
	Uint64 bits=KTestData;
	for (i=KTestBits;--i>=0;)
		out.WriteL(TUint(bits>>i),1);
	test (Mem::Compare(buffer,KTestBytes,KTestBuffer,KTestBytes)==0);
	}

void TestMultiBitWrites()
	{
	TInt i=0;
	for (TInt j=0;j<32;++j)
		for (TInt k=0;k<32;++k)
			{
			++i;
			if (i+j+k>KTestBits)
				i=0;
			TUint8 buffer[KTestBytes];
			TBitInput in(KTestBuffer,KTestBits);
			TBitOutput out(buffer,KTestBytes);
			in.ReadL(i);
			out.WriteL(in.ReadL(j),j);
			out.WriteL(in.ReadL(k),k);
			out.PadL(0);
			const TUint8* p=out.Ptr();
			test (p-buffer == (j+k+7)/8);
			Uint64 v=0;
			while (p>buffer)
				v=(v>>8) | Uint64(*--p)<<56;
			Uint64 res=KTestData;
			if (i+j+k<KTestBits)
				res>>=KTestBits-i-j-k;
			if (j+k<KTestBits)
				res<<=KTestBits-j-k;
			test (v==res);
			}
	}

void TestAlternatingWrites()
	{
	const TInt KBufferSize=(1+32)*32;
	TUint8 buffer[(7+KBufferSize)/8];
	TBitOutput out(buffer,sizeof(buffer));
	TInt i;
	for (i=0;i<=32;++i)
		out.WriteL(i&1?0xffffffff:0,i);
	while (--i>=0)
		out.WriteL(i&1?0:0xffffffff,i);
	out.PadL(0);
	TBitInput in(buffer,KBufferSize);
	for (i=0;i<=32;++i)
		{
		TUint v=in.ReadL(i);
		if (i&1)
			test (v == (1u<<i)-1u);
		else
			test (v == 0);
		}
	while (--i>=0)
		{
		TUint v=in.ReadL(i);
		if (i&1)
			test (v == 0);
		else if (i==32)
			test (v == 0xffffffffu);
		else
			test (v == (1u<<i)-1u);
		}
	}

class TOverflowOutput : public TBitOutput
	{
public:
	TOverflowOutput();
private:
	void OverflowL();
private:
	TUint8 iBuf[1];
	TInt iIx;
	};

TOverflowOutput::TOverflowOutput()
	:iIx(0)
	{}

void TOverflowOutput::OverflowL()
	{
	if (Ptr()!=0)
		{
		test (Ptr()-iBuf == 1);
		test (iBuf[0] == KTestBuffer[iIx]);
		if (++iIx==KTestBytes)
			User::Leave(KErrOverflow);
		}
	Set(iBuf,1);
	}

void OverflowTestL(TBitOutput& out, TInt j)
	{
	for (;;) out.WriteL(0xffffffffu,j);
	}

void TestOverflow()
	{
	test.Start(_L("Test default constructed output"));
	TBitOutput out;
	TInt i;
	for (i=1;i<=8;++i)
		{
		TRAPD(r,out.WriteL(1,1));
		if (i<8)
			{
			test (out.BufferedBits() == i);
			test (r == KErrNone);
			}
		else
			test (r == KErrOverflow);
		}

	test.Next(_L("Test overflow does not overrun the buffer"));
	i=0;
	for (TInt j=1;j<=32;++j)
		{
		if (++i>KTestBytes)
			i=1;
		TUint8 buffer[KTestBytes+1];
		Mem::FillZ(buffer,sizeof(buffer));
		out.Set(buffer,i);
		TRAPD(r,OverflowTestL(out,j));
		test (r == KErrOverflow);
		TInt k=0;
		while (buffer[k]==0xff)
			{
			++k;
			test (k<TInt(sizeof(buffer)));
			}
		test (k <= i);
		test ((i-k)*8 < j);
		while (k<TInt(sizeof(buffer)))
			{
			test (buffer[k]==0);
			++k;
			}
		}

	test.Next(_L("Test overflow handler"));
	TOverflowOutput vout;
	TBitInput in(KTestBuffer,KTestBits);
	for (i=KTestBits;--i>=0;)
		vout.WriteL(in.ReadL(),1);
	test(vout.BufferedBits() == 0);
	TRAPD(r,vout.WriteL(0,1));
	test (r == KErrNone);
	TRAP(r,vout.PadL(0));
	test (r == KErrOverflow);
	test.End();
	}

void TestBitWriting()
	{
	test.Start(_L("Test padding"));
	TestPadding();
	test.Next(_L("Test bit writes"));
	TestBitWrites();
	test.Next(_L("Test multi-bit writes"));
	TestMultiBitWrites();
	TestAlternatingWrites();
	test.Next(_L("Test overflow writes"));
	TestOverflow();
	test.End();
	}

// Huffman decode testing
#ifdef __ARMCC__
#pragma Onoinline
#endif
void Dummy(volatile TInt & /*x*/)
        {
	}

void TestHuffmanL()
	{
	const TInt KTestBits=32*32;

	// build the huffman decoding tree for
	// 0: '0'
	// 1: '10'
	// 2: '110' etc
	TUint32 huffman[Huffman::KMaxCodeLength+1];
	TInt i;
	for (i=0;i<Huffman::KMaxCodeLength;++i)
		huffman[i]=i+1;
	huffman[Huffman::KMaxCodeLength]=Huffman::KMaxCodeLength;
	Huffman::Decoding(huffman,Huffman::KMaxCodeLength+1,huffman);

	TUint8 buffer[KTestBits/8];
	for (TInt sz=0;sz<Huffman::KMaxCodeLength;++sz)
		{
		const TInt rep=KTestBits/(sz+1);
		TBitOutput out(buffer,sizeof(buffer));
		for (i=0;i<rep;++i)
			{
			out.WriteL(0xffffffff,sz);
			out.WriteL(0,1);
			}
		out.PadL(1);
		for (TInt blk=1;blk<=64;++blk)
			{
			TSplitBitInput in(buffer,rep*(sz+1)-1,0,blk);
			for (i=0;i<rep-1;++i)
				{
				TInt v=-1;
				TRAPD(r,v=in.HuffmanL(huffman));
				test (r==KErrNone);
				test (sz==v);
				}
			volatile TInt v=-1;
		        Dummy(v);
			TRAPD(r, v=in.HuffmanL(huffman));
			test (v==-1);
			test (r==KErrUnderflow);
			}
		}
	}

// Huffman generator testing with known but atypical distributions

void FlatHuffman(TInt aMaxCount)
	{
	TUint32* tab=new TUint32[aMaxCount];
	test (tab!=NULL);

	// test empty distribution
	Mem::FillZ(tab,sizeof(TUint32)*aMaxCount);
	TRAPD(r, Huffman::HuffmanL(tab,aMaxCount,tab));
	test (r==KErrNone);
	TInt i;
	for (i=0;i<aMaxCount;++i)
		test (tab[i]==0);
	Huffman::Decoding(tab,aMaxCount,tab);

	// test single-symbol distribution
	Mem::FillZ(tab,sizeof(TUint32)*aMaxCount);
	tab[0]=100;
	TRAP(r, Huffman::HuffmanL(tab,aMaxCount,tab));
	test (r==KErrNone);
	test (tab[0]==1);
	for (i=1;i<aMaxCount;++i)
		test (tab[i]==0);
	Huffman::Decoding(tab,aMaxCount,tab,200);
	TUint8 bits=0;
	TBitInput in(&bits,1);
	test (in.HuffmanL(tab)==200);

	// test flat distributions with 2..aMaxCount symbols
	TInt len=0;
	for (TInt c=2;c<aMaxCount;++c)
		{
		if ((2<<len)==c)
			++len;
		Mem::FillZ(tab,sizeof(TUint32)*aMaxCount);
		for (i=0;i<c;++i)
			tab[i]=100;
		TRAP(r, Huffman::HuffmanL(tab,aMaxCount,tab));
		test (r==KErrNone);
		TInt small=0;
		for (i=0;i<c;++i)
			{
			if (TInt(tab[i])==len)
				++small;
			else
				test (TInt(tab[i])==len+1);
			}
		for (;i<aMaxCount;++i)
			test (tab[i]==0);
		test (small == (2<<len)-c);
		}

	delete [] tab;
	}

void Power2Huffman()
//
// Test Huffman generator for the distribution 2^0,2^0,2^1,2^2,2^3,...
//
	{
	TUint32 tab[Huffman::KMaxCodeLength+2];

	for (TInt c=1;c<=Huffman::KMaxCodeLength+1;c++)
		{
		tab[c]=tab[c-1]=1;
		TInt i;
		for (i=c-1;--i>=0;)
			tab[i]=2*tab[i+1];

		TRAPD(r,Huffman::HuffmanL(tab,c+1,tab));
		if (c>Huffman::KMaxCodeLength)
			{
			test (r==KErrOverflow);
			continue;
			}

		test (TInt(tab[c]) == c);
		for (i=0;i<c;++i)
			test (TInt(tab[i]) == i+1);

		Huffman::Decoding(tab,c+1,tab);
		for (i=0;i<=c;++i)
			{
			TUint8 buf[4];
			TBitOutput out(buf,4);
			out.WriteL(0xffffffff,i);
			out.WriteL(0,1);
			out.PadL(1);
			TBitInput in(buf,Min(i+1,c));
			TInt ix=-1;
			TRAP(r, ix=in.HuffmanL(tab));
			test (r==KErrNone);
			test (ix==i);
			TRAP(r, in.HuffmanL(tab));
			test (r==KErrUnderflow);
			}
		}
	}

void FibonacciHuffman()
//
// Test Huffman generator for the distribution 1,1,2,3,5,8,13,21,...
//
	{
	TUint32 tab[Huffman::KMaxCodeLength+2];

	for (TInt c=1;c<=Huffman::KMaxCodeLength+1;c++)
		{
		tab[c]=tab[c-1]=1;
		TInt i;
		for (i=c-1;--i>=0;)
			tab[i]=tab[i+1]+tab[i+2];

		TRAPD(r,Huffman::HuffmanL(tab,c+1,tab));
		if (c>Huffman::KMaxCodeLength)
			{
			test (r==KErrOverflow);
			continue;
			}

		test (TInt(tab[c]) == c);
		for (i=0;i<c;++i)
			test (TInt(tab[i]) == i+1);

		Huffman::Decoding(tab,c+1,tab);
		for (i=0;i<=c;++i)
			{
			TUint8 buf[4];
			TBitOutput out(buf,4);
			out.WriteL(0xffffffff,i);
			out.WriteL(0,1);
			out.PadL(1);
			TBitInput in(buf,Min(i+1,c));
			TInt ix=-1;
			TRAP(r, ix=in.HuffmanL(tab));
			test (r==KErrNone);
			test (ix==i);
			TRAP(r, in.HuffmanL(tab));
			test (r==KErrUnderflow);
			}
		}
	}

void SpecificHuffman(TInt aMaxCount)
	{
	test.Start(_L("Flat distributions"));
	FlatHuffman(aMaxCount);
	test.Next(_L("Power-of-2 distributions"));
	Power2Huffman();
	test.Next(_L("Fibonacci distributions"));
	FibonacciHuffman();
	test.End();
	}

// Huffman generator validity testing. Checking code properties for a sequence of random
// frequency distributions.

TInt64 RSeed(KTestData);

inline TInt Random(TInt aLimit)
	{return aLimit>0 ? (Math::Rand(RSeed)%aLimit) : 0;}

void GenerateFreq(TUint32* aTable, TInt aCount, TInt aTotalFreq, TInt aVariance, TInt aZeros)
//
// Generate a random frequency table
//
	{
	for (TInt i=0;i<aCount;++i)
		{
		if (aZeros && Random(aCount-i)<aZeros)
			{
			aTable[i]=0;
			--aZeros;
			}
		else if (aCount-aZeros-i == 1)
			{
			aTable[i]=aTotalFreq;
			aTotalFreq=0;
			}
		else
			{
			TInt ave=aTotalFreq/(aCount-aZeros-i);
			if (aVariance==0)
				{
				aTable[i]=ave;
				aTotalFreq-=ave;
				}
			else
				{
				TInt var=I64INT(TInt64(ave)<<aVariance>>8);
				TInt min=Max(1,ave-var);
				TInt max=Min(1+aTotalFreq-(aCount-aZeros-i),ave+var);
				TInt f = max<=min ? ave : min+Random(max-min);
				aTable[i] = f;
				aTotalFreq-=f;
				}
			}
		}
	}

TInt NumericalSort(const TUint32& aLeft, const TUint32& aRight)
	{
	return aLeft-aRight;
	}

TInt64 VerifyOptimalCode(const TUint32* aFreq, const TUint32* aCode, TInt aCount, TInt aTotalFreqLog2)
//
// We can show tht the expected code length is at least as short as a Shannon-Fano encoding
//
	{
	TInt64 totalHuff=0;
	TInt64 totalSF=0;
	TInt i;
	for (i=0;i<aCount;++i)
		{
		TInt f=aFreq[i];
		TInt l=aCode[i];
		if (f == 0)
			{
			test (l == 0);
			continue;
			}
		totalHuff+=f*l;
		TInt s=1;
		while ((f<<s>>aTotalFreqLog2)!=1)
			++s;
		totalSF+=f*s;
		}
	test (totalHuff<=totalSF);

	RPointerArray<TUint32> index(aCount);
	CleanupClosePushL(index);
	for (i=0;i<aCount;++i)
		{
		if (aFreq[i] != 0)
			User::LeaveIfError(index.InsertInOrderAllowRepeats(aFreq+i,&NumericalSort));
		}

	TInt smin,smax;
	smin=smax=aCode[index[0]-aFreq];
	for (i=1;i<index.Count();++i)
		{
		TInt pix=index[i-1]-aFreq;
		TInt nix=index[i]-aFreq;
		TInt pf=aFreq[pix];
		TInt nf=aFreq[nix];
		TInt ps=aCode[pix];
		TInt ns=aCode[nix];

		if (nf==pf)
			{
			smin=Min(smin,ns);
			smax=Max(smax,ns);
			test (smin==smax || smin+1==smax);
			}
		else
			{
			test (nf>pf);
			test (ns<=ps);
			smin=smax=ns;
			}
		}
	CleanupStack::PopAndDestroy();

	return totalHuff;
	}

TInt LexicalSort(const TUint32& aLeft, const TUint32& aRight)
	{
	const TUint32 KCodeMask=(1<<Huffman::KMaxCodeLength)-1;
	return (aLeft&KCodeMask)-(aRight&KCodeMask);
	}

void VerifyCanonicalEncodingL(const TUint32* aCode, const TUint32* aEncode, TInt aCount)
//
// A canonical encoding assigns codes from '0' in increasing code length order, and
// in increasing index in the table for equal code length.
//
// Huffman is also a 'prefix-free' code, so we check this property of the encoding
//
	{
	TInt i;
	for (i=0;i<aCount;++i)
		test (aCode[i] == aEncode[i]>>Huffman::KMaxCodeLength);

	RPointerArray<TUint32> index(aCount);
	CleanupClosePushL(index);
	for (i=0;i<aCount;++i)
		{
		if (aCode[i] != 0)
			User::LeaveIfError(index.InsertInOrder(aEncode+i,&LexicalSort));
		}

	for (i=1;i<index.Count();++i)
		{
		TInt pix=index[i-1]-aEncode;
		TInt nix=index[i]-aEncode;
		test (aCode[pix]<=aCode[nix]);				// code lengths are always increasing
		test (aCode[pix]<aCode[nix] || pix<nix);	// same code length => index order preserved

		// check that a code is not a prefix of the next one. This is sufficent for checking the
		// prefix condition as we have already sorted the codes in lexicographical order
		TUint32 pc=aEncode[pix]<<(32-Huffman::KMaxCodeLength);
		TUint32 nc=aEncode[nix]<<(32-Huffman::KMaxCodeLength);
		TInt plen=aCode[pix];
		test ((pc>>(32-plen)) != (nc>>(32-plen)));	// pc is not a prefix for nc
		}
	CleanupStack::PopAndDestroy(&index);
	}

void VerifyCanonicalDecoding(const TUint32* aEncode, const TUint32* aDecode, TInt aCount, TInt aBase)
//
// We've checked the encoding is valid, so now we check that the decoding can correctly
// decode every code
//
	{
	TUint8 buffer[(Huffman::KMaxCodeLength+7)/8];
	TBitInput in;
	TBitOutput out;

	while (--aCount>=0)
		{
		if (aEncode[aCount])
			{
			out.Set(buffer,sizeof(buffer));
			out.HuffmanL(aEncode[aCount]);
			out.PadL(0);
			in.Set(buffer,aEncode[aCount]>>Huffman::KMaxCodeLength);
			TInt v=-1;
			TRAPD(r,v=in.HuffmanL(aDecode));
			test (r==KErrNone);
			test (v==aCount+aBase);
			TRAP(r,in.ReadL());
			test (r==KErrUnderflow);
			}
		}
	}

TInt TestExternalizeL(const TUint32* aCode, TUint8* aExtern, TUint32* aIntern, TInt aCount)
	{
	TBitOutput out(aExtern,aCount*4);
	Huffman::ExternalizeL(out,aCode,aCount);
	TInt bits=out.BufferedBits()+8*(out.Ptr()-aExtern);
	out.PadL(0);
	TBitInput in(aExtern,bits);
	TRAPD(r,Huffman::InternalizeL(in,aIntern,aCount));
	test (r == KErrNone);
	test (Mem::Compare((TUint8*)aCode,aCount*sizeof(TUint32),(TUint8*)aIntern,aCount*sizeof(TUint32)) == 0);
	TRAP(r,in.ReadL());
	test (r == KErrUnderflow);
	return bits;
	}

void RandomHuffmanL(TInt aIter, TInt aMaxSymbols)
//
// generate random frequency distributions and verify
// (a) the Huffman generator creates a mathematically 'optimal code'
// (b) the canonical encoding is the canonical encoding
// (c) the decoding tree correctly decodes each code.
// (d) the encoding can be correctly externalised and internalised
//
	{
	TReal KLog2;
	Math::Ln(KLog2,2);
	const TInt KTotalFreqLog2=24;
	const TInt KTotalFreq=1<<KTotalFreqLog2;

	while (--aIter >= 0)
		{
		TInt num=2+Random(aMaxSymbols-1);

		TUint32* const freq = new(ELeave) TUint32[num*3];
		CleanupArrayDeletePushL(freq);
		TUint32* const code = freq+num;
		TUint32* const encoding = code+num;
		TUint32* const decoding = freq;
		TUint8* const exter = (TUint8*)encoding;
		TUint32* const intern = freq;

		TInt var=Random(24);
		TInt zero=Random(num-2);
		GenerateFreq(freq,num,KTotalFreq,var,zero);

		Huffman::HuffmanL(freq,num,code);
		VerifyOptimalCode(freq,code,num,KTotalFreqLog2);

		Huffman::Encoding(code,num,encoding);
		VerifyCanonicalEncodingL(code,encoding,num);

		TInt base=Random(Huffman::KMaxCodes-num);
		Huffman::Decoding(code,num,decoding,base);
		VerifyCanonicalDecoding(encoding,decoding,num,base);

		TestExternalizeL(code,exter,intern,num);
		CleanupStack::PopAndDestroy();
		}
	}

///

void MainL()
	{
	test.Start(_L("Test Bit reader"));
	TestBitReading();
	test.Next(_L("Test Bit writer"));
	TestBitWriting();
	test.Next(_L("Test Huffman decoder"));
	TestHuffmanL();
	test.Next(_L("Test Huffman generator for known distributions"));
	SpecificHuffman(800);
	test.Next(_L("Test Huffman generator for random distributions"));
	TRAPD(r,RandomHuffmanL(1000,800));
	test (r==KErrNone);
	test.End();
	}

TInt E32Main()
	{
	test.Title();
	CTrapCleanup* c=CTrapCleanup::New();
	test (c!=0);
	TRAPD(r,MainL());
	test (r==KErrNone);
	delete c;
	test.Close();
	return r;
	}
