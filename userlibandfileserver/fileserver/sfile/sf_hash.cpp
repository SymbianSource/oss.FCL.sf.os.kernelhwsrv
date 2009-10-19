// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfile\sf_hash.cpp
// 
//

#include "sf_std.h"
#define EXPANDLOOP


static inline TUint CMD_R(TUint a,TUint s)
	{
	return (a<<s) | (a>>(32-s));
	}

CSHA1::CSHA1(void)
: iHash(SHA1_HASH)
	{
	}

CSHA1* CSHA1::NewL(void)
	{
	CSHA1* self=new(ELeave) CSHA1;
	self->Reset();
	return self;
	}

void CSHA1::Update(const TDesC8& aMessage)
	{
	DoUpdate(aMessage.Ptr(),aMessage.Size());
	}

TPtrC8 CSHA1::Final()
	{
	TPtrC8 ptr(KNullDesC8());
	DoFinal();
	ptr.Set(iHash);
	Reset();
	return ptr;
	}

CSHA1::~CSHA1(void)
	{
	}

void CSHA1::Reset(void)
	{
	iA=0x67452301;
	iB=0xefcdab89;
	iC=0x98badcfe;
	iD=0x10325476;
	iE=0xc3d2e1f0;
	iNh=0;
	iNl=0;
	}

// This assumes a big-endian architecture
void CSHA1::DoUpdate(const TUint8* aData,TUint aLength)
	{
	while((aLength / 4) > 0 && (iNl % 4 == 0))
		{
		iData[iNl>>2] = aData[0] << 24 | aData[1] << 16 | aData[2] << 8 | aData[3];
		iNl+=4;
		aData+=4;
		aLength-=4;
		if(iNl==64) 
			{
			Block();
			iNh+=64;
			iNl=0;
			}
		}

	while(aLength--)
		{
		switch (iNl&3) 
			{
			case 0:
				iData[iNl>>2]=((TUint)(*aData))<<24;
				break;
			case 1:
				iData[iNl>>2]|=((TUint)(*aData))<<16;
				break;
			case 2:
				iData[iNl>>2]|=((TUint)(*aData))<<8;
				break;
			case 3:
				iData[iNl>>2]|=((TUint)(*aData));
				break;
			default:
				break;
			};
			aData++;
			iNl++;
			if(iNl==64) 
				{
				Block();
				iNh+=64;
				iNl=0;
				}
		}
	}

static inline TUint CSHA1_F(const TUint x,const TUint y,const TUint z)
	{
	return (x&y) | (~x&z);
	}

static inline TUint CSHA1_G(const TUint x,const TUint y,const TUint z)
	{
	return x^y^z;
	}

static inline TUint CSHA1_H(const TUint x,const TUint y,const TUint z)
	{
	return (x&y) | (x&z) | (y&z);
	}

/*static inline TUint CSHA1_I(const TUint x,const TUint y,const TUint z)
	{
	return x^y^z;
	}*/

#ifdef EXPANDLOOP

#ifdef MACRO

#define CSHA1_16(x,y,z,u,t,v,w)					v=CMD_R(x,5)+CSHA1_F(y,z,u)+t+w+0x5a827999;\
												y=CMD_R(y,30);t=v;
#define CSHA1_20(x,y,z,u,t,v,w0,w3,w8,w14,w16)  v=w3^w8^w14^w16;w0=CMD_R(v,1);\
												CSHA1_16(x,y,z,u,t,v,w0);
#define CSHA1_40(x,y,z,u,t,v,w0,w3,w8,w14,w16)	v=w3^w8^w14^w16;w0=CMD_R(v,1);\
												v=CMD_R(x,5)+CSHA1_G(y,z,u)+t+w0+0x6ed9eba1;\
												y=CMD_R(y,30);t=v;
#define CSHA1_60(x,y,z,u,t,v,w0,w3,w8,w14,w16)	v=w3^w8^w14^w16;w0=CMD_R(v,1);\
												v=CMD_R(x,5)+CSHA1_H(y,z,u)+t+w0+0x8f1bbcdc;\
												y=CMD_R(y,30);t=v;
#define CSHA1_80(x,y,z,u,t,v,w0,w3,w8,w14,w16)	v=w3^w8^w14^w16;w0=CMD_R(v,1);\
												v=CMD_R(x,5)+CSHA1_G(y,z,u)+t+w0+0xca62c1d6;\
												y=CMD_R(y,30);t=v;
#else

static inline void CSHA1_16(const TUint x, TUint& y, const TUint z,
							const TUint u, TUint& t, TUint& v, const TUint w)
	{
	v = CMD_R(x,5) + CSHA1_F(y,z,u) + t + w + 0x5a827999;
	y = CMD_R(y,30);
	t = v;
	}

static inline void CSHA1_20(const TUint x,TUint& y,const TUint z,
							const TUint u,TUint& t,TUint& v,
							TUint& w0,const TUint w3,const TUint w8,
							const TUint w14,const TUint w16)
	{
	v = w3 ^ w8 ^ w14 ^ w16;
	w0 = CMD_R(v,1);
	CSHA1_16(x,y,z,u,t,v,w0);
	}

static inline void CSHA1_40(const TUint x,TUint& y,const TUint z,
							const TUint u,TUint& t,TUint& v,
							TUint& w0,const TUint w3,const TUint w8,
							const TUint w14,const TUint w16)
	{
	v = w3 ^ w8 ^ w14 ^ w16;
	w0 = CMD_R(v,1);
	v = CMD_R(x,5) + CSHA1_G(y,z,u) + t + w0 + 0x6ed9eba1;
	y = CMD_R(y,30);
	t = v;
	}

static inline void CSHA1_60(const TUint x,TUint& y,const TUint z,
							const TUint u,TUint& t,TUint& v,
							TUint& w0,const TUint w3,const TUint w8,
							const TUint w14,const TUint w16)
	{
	v = w3 ^ w8 ^ w14 ^ w16;
	w0 = CMD_R(v,1);
	v = CMD_R(x,5) + CSHA1_H(y,z,u) + t + w0 + 0x8f1bbcdc;
	y = CMD_R(y,30);
	t = v;
	}

static inline void CSHA1_80(const TUint x,TUint& y,const TUint z,
							const TUint u,TUint& t,TUint& v,
							TUint& w0,const TUint w3,const TUint w8,
							const TUint w14,const TUint w16)
	{
	v = w3 ^ w8 ^ w14 ^ w16;
	w0 = CMD_R(v,1);
	v = CMD_R(x,5) + CSHA1_G(y,z,u) + t + w0 + 0xca62c1d6;
	y = CMD_R(y,30);
	t = v;
	}

#endif // MACRO
#endif // EXPANDLOOP

#ifdef WEIDAI

template <class T> inline T rotlFixed(T x, unsigned int y)
{
	ASSERT(y < sizeof(T)*8);
	return (x<<y) | (x>>(sizeof(T)*8-y));
}

template<> inline TUint32 rotlFixed<TUint32>(TUint32 x, unsigned int y)
{
	ASSERT(y < 32);
	return y ? CMD_R(x, y) : x;
}

#define blk0(i) (W[i] = iData[i])
#define blk1(i) (W[i&15] = rotlFixed(W[(i+13)&15]^W[(i+8)&15]^W[(i+2)&15]^W[i&15],1))

#define f1(x,y,z) (z^(x&(y^z)))
#define f2(x,y,z) (x^y^z)
#define f3(x,y,z) ((x&y)|(z&(x|y)))
#define f4(x,y,z) (x^y^z)

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=f1(w,x,y)+blk0(i)+0x5A827999+rotlFixed(v,5);w=rotlFixed(w,30);
#define R1(v,w,x,y,z,i) z+=f1(w,x,y)+blk1(i)+0x5A827999+rotlFixed(v,5);w=rotlFixed(w,30);
#define R2(v,w,x,y,z,i) z+=f2(w,x,y)+blk1(i)+0x6ED9EBA1+rotlFixed(v,5);w=rotlFixed(w,30);
#define R3(v,w,x,y,z,i) z+=f3(w,x,y)+blk1(i)+0x8F1BBCDC+rotlFixed(v,5);w=rotlFixed(w,30);
#define R4(v,w,x,y,z,i) z+=f4(w,x,y)+blk1(i)+0xCA62C1D6+rotlFixed(v,5);w=rotlFixed(w,30);

#endif // WEIDAI

void CSHA1::Block()
	{
#ifdef WEIDAI
	TUint32 W[16];
    /* Copy context->state[] to working vars */
    TUint32 a = iA;
    TUint32 b = iB;
    TUint32 c = iC;
    TUint32 d = iD;
    TUint32 e = iE;
    
	/* 4 rounds of 20 operations each. Loop unrolled. */
    
	R0(a,b,c,d,e, 0); 
	R0(e,a,b,c,d, 1); 
	R0(d,e,a,b,c, 2); 
	R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); 
	R0(a,b,c,d,e, 5); 
	R0(e,a,b,c,d, 6); 
	R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); 
	R0(b,c,d,e,a, 9); 
	R0(a,b,c,d,e,10); 
	R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); 
	R0(c,d,e,a,b,13); 
	R0(b,c,d,e,a,14); 
	R0(a,b,c,d,e,15);

    R1(e,a,b,c,d,16); 
	R1(d,e,a,b,c,17); 
	R1(c,d,e,a,b,18); 
	R1(b,c,d,e,a,19);

    R2(a,b,c,d,e,20); 
	R2(e,a,b,c,d,21); 
	R2(d,e,a,b,c,22); 
	R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); 
	R2(a,b,c,d,e,25); 
	R2(e,a,b,c,d,26); 
	R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); 
	R2(b,c,d,e,a,29); 
	R2(a,b,c,d,e,30); 
	R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); 
	R2(c,d,e,a,b,33); 
	R2(b,c,d,e,a,34); 
	R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); 
	R2(d,e,a,b,c,37); 
	R2(c,d,e,a,b,38); 
	R2(b,c,d,e,a,39);

    R3(a,b,c,d,e,40); 
	R3(e,a,b,c,d,41); 
	R3(d,e,a,b,c,42); 
	R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); 
	R3(a,b,c,d,e,45); 
	R3(e,a,b,c,d,46); 
	R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); 
	R3(b,c,d,e,a,49); 
	R3(a,b,c,d,e,50); 
	R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); 
	R3(c,d,e,a,b,53); 
	R3(b,c,d,e,a,54); 
	R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); 
	R3(d,e,a,b,c,57); 
	R3(c,d,e,a,b,58); 
	R3(b,c,d,e,a,59);

    R4(a,b,c,d,e,60); 
	R4(e,a,b,c,d,61); 
	R4(d,e,a,b,c,62); 
	R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); 
	R4(a,b,c,d,e,65); 
	R4(e,a,b,c,d,66); 
	R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); 
	R4(b,c,d,e,a,69); 
	R4(a,b,c,d,e,70); 
	R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); 
	R4(c,d,e,a,b,73); 
	R4(b,c,d,e,a,74); 
	R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); 
	R4(d,e,a,b,c,77); 
	R4(c,d,e,a,b,78); 
	R4(b,c,d,e,a,79);
    
	/* Add the working vars back into context.state[] */
    iA += a;
    iB += b;
    iC += c;
    iD += d;
    iE += e;
    /* Wipe variables */
    a = b = c = d = e = 0;
	Mem::FillZ(W, sizeof(W));
#else
	TUint tempA=iA;
	TUint tempB=iB;
	TUint tempC=iC;
	TUint tempD=iD;
	TUint tempE=iE;
	TUint temp=0;

#ifdef EXPANDLOOP
	CSHA1_16(tempA,tempB,tempC,tempD,tempE,temp,iData[0]);
	CSHA1_16(temp,tempA,tempB,tempC,tempD,tempE,iData[1]);
	CSHA1_16(tempE,temp,tempA,tempB,tempC,tempD,iData[2]);
	CSHA1_16(tempD,tempE,temp,tempA,tempB,tempC,iData[3]);
	CSHA1_16(tempC,tempD,tempE,temp,tempA,tempB,iData[4]);
	CSHA1_16(tempB,tempC,tempD,tempE,temp,tempA,iData[5]);
	CSHA1_16(tempA,tempB,tempC,tempD,tempE,temp,iData[6]);
	CSHA1_16(temp,tempA,tempB,tempC,tempD,tempE,iData[7]);
	CSHA1_16(tempE,temp,tempA,tempB,tempC,tempD,iData[8]);
	CSHA1_16(tempD,tempE,temp,tempA,tempB,tempC,iData[9]);
	CSHA1_16(tempC,tempD,tempE,temp,tempA,tempB,iData[10]);
	CSHA1_16(tempB,tempC,tempD,tempE,temp,tempA,iData[11]);
	CSHA1_16(tempA,tempB,tempC,tempD,tempE,temp,iData[12]);
	CSHA1_16(temp,tempA,tempB,tempC,tempD,tempE,iData[13]);
	CSHA1_16(tempE,temp,tempA,tempB,tempC,tempD,iData[14]);
	CSHA1_16(tempD,tempE,temp,tempA,tempB,tempC,iData[15]);
	/*
	i = 16;
	TUint temp1 = tempA;
	tempA = 
	*/
#else
    TUint i=0;
	while (i<16) 
		{
		temp = CMD_R(tempA,5) + CSHA1_F(tempB,tempC,tempD) + tempE + iData[i++] + 0x5a827999;
		tempE = tempD;
		tempD = tempC;
		tempC = CMD_R(tempB,30);
		tempB = tempA;
		tempA = temp;
		}
#endif

#ifdef EXPANDLOOP
	CSHA1_20(tempC,tempD,tempE,temp,tempA,tempB,iData[16],iData[13],iData[8],iData[2],iData[0]);
	CSHA1_20(tempB,tempC,tempD,tempE,temp,tempA,iData[17],iData[14],iData[9],iData[3],iData[1]);
	CSHA1_20(tempA,tempB,tempC,tempD,tempE,temp,iData[18],iData[15],iData[10],iData[4],iData[2]);
	CSHA1_20(temp,tempA,tempB,tempC,tempD,tempE,iData[19],iData[16],iData[11],iData[5],iData[3]);
	//i = 20;
#else
	while (i<20) 
		{
		temp=iData[i-3] ^ iData[i-8] ^ iData[i-14] ^ iData[i-16];
		iData[i]=CMD_R(temp,1);
		temp = CMD_R(tempA,5) + CSHA1_F(tempB,tempC,tempD) + tempE + iData[i++] + 0x5a827999; 
		tempE = tempD;
		tempD = tempC; 
		tempC = CMD_R(tempB,30); 
		tempB = tempA; 
		tempA = temp;
		}
#endif

#ifdef EXPANDLOOP
	CSHA1_40(tempE,temp,tempA,tempB,tempC,tempD,iData[20],iData[17],iData[12],iData[6],iData[4]);
	CSHA1_40(tempD,tempE,temp,tempA,tempB,tempC,iData[21],iData[18],iData[13],iData[7],iData[5]);
	CSHA1_40(tempC,tempD,tempE,temp,tempA,tempB,iData[22],iData[19],iData[14],iData[8],iData[6]);
	CSHA1_40(tempB,tempC,tempD,tempE,temp,tempA,iData[23],iData[20],iData[15],iData[9],iData[7]);
	CSHA1_40(tempA,tempB,tempC,tempD,tempE,temp,iData[24],iData[21],iData[16],iData[10],iData[8]);
	CSHA1_40(temp,tempA,tempB,tempC,tempD,tempE,iData[25],iData[22],iData[17],iData[11],iData[9]);
	CSHA1_40(tempE,temp,tempA,tempB,tempC,tempD,iData[26],iData[23],iData[18],iData[12],iData[10]);
	CSHA1_40(tempD,tempE,temp,tempA,tempB,tempC,iData[27],iData[24],iData[19],iData[13],iData[11]);
	CSHA1_40(tempC,tempD,tempE,temp,tempA,tempB,iData[28],iData[25],iData[20],iData[14],iData[12]);
	CSHA1_40(tempB,tempC,tempD,tempE,temp,tempA,iData[29],iData[26],iData[21],iData[15],iData[13]);
	CSHA1_40(tempA,tempB,tempC,tempD,tempE,temp,iData[30],iData[27],iData[22],iData[16],iData[14]);
	CSHA1_40(temp,tempA,tempB,tempC,tempD,tempE,iData[31],iData[28],iData[23],iData[17],iData[15]);
	CSHA1_40(tempE,temp,tempA,tempB,tempC,tempD,iData[32],iData[29],iData[24],iData[18],iData[16]);
	CSHA1_40(tempD,tempE,temp,tempA,tempB,tempC,iData[33],iData[30],iData[25],iData[19],iData[17]);
	CSHA1_40(tempC,tempD,tempE,temp,tempA,tempB,iData[34],iData[31],iData[26],iData[20],iData[18]);
	CSHA1_40(tempB,tempC,tempD,tempE,temp,tempA,iData[35],iData[32],iData[27],iData[21],iData[19]);
	CSHA1_40(tempA,tempB,tempC,tempD,tempE,temp,iData[36],iData[33],iData[28],iData[22],iData[20]);
	CSHA1_40(temp,tempA,tempB,tempC,tempD,tempE,iData[37],iData[34],iData[29],iData[23],iData[21]);
	CSHA1_40(tempE,temp,tempA,tempB,tempC,tempD,iData[38],iData[35],iData[30],iData[24],iData[22]);
	CSHA1_40(tempD,tempE,temp,tempA,tempB,tempC,iData[39],iData[36],iData[31],iData[25],iData[23]);
	//i = 40;
#else
	while (i<40) 
		{
		temp = iData[i-3] ^ iData[i-8] ^ iData[i-14] ^ iData[i-16];
		iData[i] = CMD_R(temp,1);

		temp = CMD_R(tempA,5) + CSHA1_G(tempB,tempC,tempD) + tempE + iData[i++] + 0x6ed9eba1; 
		tempE = tempD; 
		tempD = tempC; 
		tempC = CMD_R(tempB,30); 
		tempB = tempA; 
		tempA = temp;
		}
#endif

#ifdef EXPANDLOOP
	CSHA1_60(tempC,tempD,tempE,temp,tempA,tempB,iData[40],iData[37],iData[32],iData[26],iData[24]);
	CSHA1_60(tempB,tempC,tempD,tempE,temp,tempA,iData[41],iData[38],iData[33],iData[27],iData[25]);
	CSHA1_60(tempA,tempB,tempC,tempD,tempE,temp,iData[42],iData[39],iData[34],iData[28],iData[26]);
	CSHA1_60(temp,tempA,tempB,tempC,tempD,tempE,iData[43],iData[40],iData[35],iData[29],iData[27]);
	CSHA1_60(tempE,temp,tempA,tempB,tempC,tempD,iData[44],iData[41],iData[36],iData[30],iData[28]);
	CSHA1_60(tempD,tempE,temp,tempA,tempB,tempC,iData[45],iData[42],iData[37],iData[31],iData[29]);
	CSHA1_60(tempC,tempD,tempE,temp,tempA,tempB,iData[46],iData[43],iData[38],iData[32],iData[30]);
	CSHA1_60(tempB,tempC,tempD,tempE,temp,tempA,iData[47],iData[44],iData[39],iData[33],iData[31]);
	CSHA1_60(tempA,tempB,tempC,tempD,tempE,temp,iData[48],iData[45],iData[40],iData[34],iData[32]);
	CSHA1_60(temp,tempA,tempB,tempC,tempD,tempE,iData[49],iData[46],iData[41],iData[35],iData[33]);
	CSHA1_60(tempE,temp,tempA,tempB,tempC,tempD,iData[50],iData[47],iData[42],iData[36],iData[34]);
	CSHA1_60(tempD,tempE,temp,tempA,tempB,tempC,iData[51],iData[48],iData[43],iData[37],iData[35]);
	CSHA1_60(tempC,tempD,tempE,temp,tempA,tempB,iData[52],iData[49],iData[44],iData[38],iData[36]);
	CSHA1_60(tempB,tempC,tempD,tempE,temp,tempA,iData[53],iData[50],iData[45],iData[39],iData[37]);
	CSHA1_60(tempA,tempB,tempC,tempD,tempE,temp,iData[54],iData[51],iData[46],iData[40],iData[38]);
	CSHA1_60(temp,tempA,tempB,tempC,tempD,tempE,iData[55],iData[52],iData[47],iData[41],iData[39]);
	CSHA1_60(tempE,temp,tempA,tempB,tempC,tempD,iData[56],iData[53],iData[48],iData[42],iData[40]);
	CSHA1_60(tempD,tempE,temp,tempA,tempB,tempC,iData[57],iData[54],iData[49],iData[43],iData[41]);
	CSHA1_60(tempC,tempD,tempE,temp,tempA,tempB,iData[58],iData[55],iData[50],iData[44],iData[42]);
	CSHA1_60(tempB,tempC,tempD,tempE,temp,tempA,iData[59],iData[56],iData[51],iData[45],iData[43]);
	//i = 60;
#else
	while (i<60) 
		{
		temp = iData[i-3] ^ iData[i-8] ^ iData[i-14] ^ iData[i-16];
		iData[i] = CMD_R(temp,1);

		temp = CMD_R(tempA,5) + CSHA1_H(tempB,tempC,tempD) + tempE + iData[i++] + 0x8f1bbcdc; 
		tempE = tempD; 
		tempD = tempC; 
		tempC = CMD_R(tempB,30); 
		tempB = tempA; 
		tempA = temp;
		}
#endif

#ifdef EXPANDLOOP
	CSHA1_80(tempA,tempB,tempC,tempD,tempE,temp,iData[60],iData[57],iData[52],iData[46],iData[44]);
	CSHA1_80(temp,tempA,tempB,tempC,tempD,tempE,iData[61],iData[58],iData[53],iData[47],iData[45]);
	CSHA1_80(tempE,temp,tempA,tempB,tempC,tempD,iData[62],iData[59],iData[54],iData[48],iData[46]);
	CSHA1_80(tempD,tempE,temp,tempA,tempB,tempC,iData[63],iData[60],iData[55],iData[49],iData[47]);
	CSHA1_80(tempC,tempD,tempE,temp,tempA,tempB,iData[64],iData[61],iData[56],iData[50],iData[48]);
	CSHA1_80(tempB,tempC,tempD,tempE,temp,tempA,iData[65],iData[62],iData[57],iData[51],iData[49]);
	CSHA1_80(tempA,tempB,tempC,tempD,tempE,temp,iData[66],iData[63],iData[58],iData[52],iData[50]);
	CSHA1_80(temp,tempA,tempB,tempC,tempD,tempE,iData[67],iData[64],iData[59],iData[53],iData[51]);
	CSHA1_80(tempE,temp,tempA,tempB,tempC,tempD,iData[68],iData[65],iData[60],iData[54],iData[52]);
	CSHA1_80(tempD,tempE,temp,tempA,tempB,tempC,iData[69],iData[66],iData[61],iData[55],iData[53]);
	CSHA1_80(tempC,tempD,tempE,temp,tempA,tempB,iData[70],iData[67],iData[62],iData[56],iData[54]);
	CSHA1_80(tempB,tempC,tempD,tempE,temp,tempA,iData[71],iData[68],iData[63],iData[57],iData[55]);
	CSHA1_80(tempA,tempB,tempC,tempD,tempE,temp,iData[72],iData[69],iData[64],iData[58],iData[56]);
	CSHA1_80(temp,tempA,tempB,tempC,tempD,tempE,iData[73],iData[70],iData[65],iData[59],iData[57]);
	CSHA1_80(tempE,temp,tempA,tempB,tempC,tempD,iData[74],iData[71],iData[66],iData[60],iData[58]);
	CSHA1_80(tempD,tempE,temp,tempA,tempB,tempC,iData[75],iData[72],iData[67],iData[61],iData[59]);
	CSHA1_80(tempC,tempD,tempE,temp,tempA,tempB,iData[76],iData[73],iData[68],iData[62],iData[60]);
	CSHA1_80(tempB,tempC,tempD,tempE,temp,tempA,iData[77],iData[74],iData[69],iData[63],iData[61]);
	CSHA1_80(tempA,tempB,tempC,tempD,tempE,temp,iData[78],iData[75],iData[70],iData[64],iData[62]);
	CSHA1_80(temp,tempA,tempB,tempC,tempD,tempE,iData[79],iData[76],iData[71],iData[65],iData[63]);
#else
	const TUint total=SHA1_LBLOCK*5; // 16 * 5 = 80
	while (i<total) 
		{
		temp = iData[i-3] ^ iData[i-8] ^ iData[i-14] ^ iData[i-16];
		iData[i] = CMD_R(temp,1);

		temp = CMD_R(tempA,5) + CSHA1_I(tempB,tempC,tempD) + tempE + iData[i++] + 0xca62c1d6; 
		tempE = tempD; 
		tempD = tempC; 
		tempC = CMD_R(tempB,30); 
		tempB = tempA; 
		tempA = temp;
		}
#endif

#ifdef EXPANDLOOP
	iA+=tempE;
	iB+=temp;
	iC+=tempA;
	iD+=tempB;
	iE+=tempC;
#else
	iA+=tempA;
	iB+=tempB;
	iC+=tempC;
	iD+=tempD;
	iE+=tempE;
#endif // EXPANDLOOP
#endif // WEIDAI
	}

void CSHA1::DoFinal()
	{
	iNh += iNl;
	const TUint ul128=128;
	switch (iNl&3) 
		{
		case 0:
			iData[iNl>>2] = ul128<<24;
			break;
		case 1:
			iData[iNl>>2] += ul128<<16;
			break;
		case 2:
			iData[iNl>>2] += ul128<<8;
			break;
		case 3:
			iData[iNl>>2] += ul128;
			break;
		default:
			break;
		};
	if (iNl>=56) 
		{
		if (iNl<60)
			iData[15]=0;		
		Block();
		Mem::FillZ(iData,14*sizeof(TUint));
		} 
	else
		{
		const TUint offset=(iNl+4)>>2; //+4 to account for the word added in the
		//switch statement above
		Mem::FillZ(iData+offset,(14-offset)*sizeof(TUint));
		}

	//TODO: this will fail if the total input length is longer than 2^32 in bits
	//(2^31 in bytes) which is roughly half a gig.
	iData[14]=0;
	iData[15]=iNh<<3;//number in bits
	Block();
	
	//
	// Generate hash value into iHash
	//
	TUint tmp=iA;
	iHash[3]=(TUint8)(tmp & 255);
	iHash[2]=(TUint8)((tmp >>= 8) & 255);
	iHash[1]=(TUint8)((tmp >>= 8) & 255);
	iHash[0]=(TUint8)((tmp >>= 8) & 255);
	
	tmp=iB;
	iHash[7]=(TUint8)(tmp & 255);
	iHash[6]=(TUint8)((tmp >>= 8) & 255);
	iHash[5]=(TUint8)((tmp >>= 8) & 255);
	iHash[4]=(TUint8)((tmp >>= 8) & 255);
	
	tmp=iC;
	iHash[11]=(TUint8)(tmp & 255);
	iHash[10]=(TUint8)((tmp >>= 8) & 255);
	iHash[9]=(TUint8)((tmp >>= 8) & 255);
	iHash[8]=(TUint8)((tmp >>= 8) & 255);
	
	tmp=iD;
	iHash[15]=(TUint8)(tmp & 255);
	iHash[14]=(TUint8)((tmp >>= 8) & 255);
	iHash[13]=(TUint8)((tmp >>= 8) & 255);
	iHash[12]=(TUint8)((tmp >>= 8) & 255);
	
	tmp=iE;
	iHash[19]=(TUint8)(tmp & 255);
	iHash[18]=(TUint8)((tmp >>= 8) & 255);
	iHash[17]=(TUint8)((tmp >>= 8) & 255);
	iHash[16]=(TUint8)((tmp >>= 8) & 255);
	}
