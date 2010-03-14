/**
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL " http://www.eclipse.org/legal/epl-v10.html ".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: SHA256 implementation for Random number generation
* kernel\sha256.cpp
*
*/

#include "sha256.h"


/**
 * SHA256 Constants
 * 
 * SHA-256 uses a sequence of sixty-four constant 32-bit words. 
 * These words represent the first thirty-two bits of the fractional 
 * parts of the cube roots of the first sixtyfour prime numbers.
 * 
 * FIPS 180-2 Section 4.2.2
 */
const TUint K[64] = 
	{
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,	
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};


inline TUint SHA_Ch(TUint aX, TUint aY, TUint aZ)
	{
	return ((aX & aY) ^ ((~aX) & aZ));
	}

inline TUint SHA_Maj(TUint aX, TUint aY, TUint aZ)
	{
	return ((aX & aY) ^ (aX & aZ) ^ (aY & aZ));
	}

/**
 * SHA Rotate Right Operation: The rotate right (circular right shift) operation
 * ROTR^n(x), where x is a w-bit word and n is an integer with 0 <= n < w, 
 * is defined by ROTR n(x)=(x >> n) || (x << w - n).
 */
inline TUint SHA_ROTR(TUint aBits, TUint aWord)
	{
	TInt totalBits = sizeof(TUint) << 3;
	return ((aWord >> aBits) | (aWord << (totalBits-aBits)));
	}

	
/**
 * Define the SHA shift, and rotate right macro 
 * Defined in FIPS 180-2 Section 3.2
 */
/** 
 * SHA Right Shift operation: The right shift operation SHR^n(x), 
 * where x is a w-bit word and n is an integer with 0 <= n < w, 
 * is defined by  SHR^n(x) = x >> n.
 */
inline TUint SHA_SHR(TUint aBits, TUint aWord)
	{
	return (aWord >> aBits);
	}

/**
 * Define the SHA SIGMA and sigma macros 
 * 
 * FIPS 180-2 section 4.1.2
 */
// Equation 4.4
inline TUint SHA256_SIGMA0(TUint aWord)
	{
	return (SHA_ROTR(2,aWord) ^ SHA_ROTR(13,aWord) ^ SHA_ROTR(22,aWord));
	}
// Equation 4.5
inline TUint SHA256_SIGMA1(TUint aWord)
	{
	return (SHA_ROTR(6,aWord) ^ SHA_ROTR(11,aWord) ^ SHA_ROTR(25,aWord));
	}
// Equation 4.6
inline TUint SHA256_sigma0(TUint aWord)
	{
	return (SHA_ROTR(7,aWord) ^ SHA_ROTR(18,aWord) ^ SHA_SHR(3,aWord));
	}
// Equation 4.7
inline TUint SHA256_sigma1(TUint aWord)
	{
	return (SHA_ROTR(17,aWord) ^ SHA_ROTR(19,aWord) ^ SHA_SHR(10,aWord));
	}

// Macros
inline TUint MakeWord(const TUint8* aData)
	{
	return (aData[0] << 24 | aData[1] << 16 | aData[2] << 8 | aData[3]);
	}
													
// Constructor
SHA256::SHA256()
	{
	Reset();	
	}
			
 void SHA256::Reset()
	{
	/**
	 * Initial Hash Value
	 * 
	 * These words were obtained by taking the first thirty-two bits 
	 * of the fractional parts of the square roots of the first eight
	 * prime numbers.
	 * 
	 * FIPS 180-2 Section 5.3.2
	 */
	iA=0x6a09e667; 
    iB=0xbb67ae85; 
    iC=0x3c6ef372; 
    iD=0xa54ff53a; 
    iE=0x510e527f; 
    iF=0x9b05688c; 
    iG=0x1f83d9ab; 
	iH=0x5be0cd19; 
	iNh=0;
	iNl=0;
	}
	
// This assumes a big-endian architecture
 void SHA256::Update(const TUint8* aData,TUint aLength)
	{
	while((aLength / 4) > 0 && (iNl % 4 == 0))
		{
		iData[iNl>>2] = MakeWord(aData);
		iNl+=4;
		aData+=4;
		aLength-=4;
		if(iNl==KSHA256BlockSize) 
			{
			Block();
			AddLength(KSHA256BlockSize);
			}
		}

	while(aLength--)
		{
		if(!(iNl&0x03))
			{
			iData[iNl >> 2] = 0;
			}
		iData[iNl >> 2] |= *aData << ((3 - iNl&0x03) << 3) ;
		++aData;
		++iNl;
		if(iNl==KSHA256BlockSize) 
			{
			Block();
			AddLength(KSHA256BlockSize);
			}
		}
	}

inline void SHA256::AddLength(const TUint aLength)
	{
	iNh += aLength << 3;
	}


static inline void CSHA256_16(	const TUint aA, 
								const TUint aB, 
								const TUint aC,
								TUint& aD, 
								const TUint aE, 
								const TUint aF,
								const TUint aG, 
								TUint& aH,
								TUint aTemp1,
								TUint aTemp2,
								const TUint aK,
								const TUint aWord)
	{
	aTemp1 = aH + SHA256_SIGMA1(aE) + SHA_Ch(aE,aF,aG) + aK + aWord;
	aTemp2 = SHA256_SIGMA0(aA) + SHA_Maj(aA,aB,aC);
	aD = aD + aTemp1;
	aH = aTemp1 + aTemp2;
	}

static inline void CSHA256_48(	const TUint aA, 
								const TUint aB, 
								const TUint aC,
								TUint& aD, 
								const TUint aE, 
								const TUint aF,
								const TUint aG, 
								TUint& aH,
								TUint aTemp1,
								TUint aTemp2,
								const TUint aK,
								TUint& aWord0,
								const TUint aWord2,
								const TUint aWord7,
								const TUint aWord15,
								const TUint aWord16)
	{
	aWord0 = SHA256_sigma1(aWord2) + aWord7 + SHA256_sigma0(aWord15) + aWord16;
	CSHA256_16(aA, aB, aC, aD, aE, aF, aG, aH, aTemp1, aTemp2, aK, aWord0);
	}

/**
 * This function actually calculates the hash.
 * Function is defined in FIPS 180-2 section 6.2.2
 * 
 * This function is the expanded version of the following loop.
 *	for(TUint i = 0; i < 64; ++i)
 *		{
 *		if(i >= 16)
 *			{
 * 			iData[i] = SHA256_sigma1(iData[i-2]) + iData[i-7] + SHA256_sigma0(iData[i-15]) + iData[i-16];
 *			}
 *
 *		temp1 = tempH + SHA256_SIGMA1(tempE) + SHA_Ch(tempE,tempF,tempG) + K[i] + iData[i];
 *		temp2 = SHA256_SIGMA0(tempA) + SHA_Maj(tempA,tempB,tempC);
 *	    tempH = tempG;
 *	    tempG = tempF;
 *	    tempF = tempE;
 *	    tempE = tempD + temp1;
 *	    tempD = tempC;
 *	    tempC = tempB;
 *	    tempB = tempA;
 *	    tempA = temp1 + temp2;		
 *		}
 */
void SHA256::Block()
	{
	TUint tempA=iA;
	TUint tempB=iB;
	TUint tempC=iC;
	TUint tempD=iD;
	TUint tempE=iE;
	TUint tempF=iF;
	TUint tempG=iG;
	TUint tempH=iH;
	TUint temp1=0;
	TUint temp2=0;
	
	CSHA256_16(tempA,tempB,tempC,tempD,tempE,tempF,tempG,tempH,temp1,temp2,K[0],iData[0]);
	CSHA256_16(tempH,tempA,tempB,tempC,tempD,tempE,tempF,tempG,temp1,temp2,K[1],iData[1]);
	CSHA256_16(tempG,tempH,tempA,tempB,tempC,tempD,tempE,tempF,temp1,temp2,K[2],iData[2]);
	CSHA256_16(tempF,tempG,tempH,tempA,tempB,tempC,tempD,tempE,temp1,temp2,K[3],iData[3]);
	CSHA256_16(tempE,tempF,tempG,tempH,tempA,tempB,tempC,tempD,temp1,temp2,K[4],iData[4]);
	CSHA256_16(tempD,tempE,tempF,tempG,tempH,tempA,tempB,tempC,temp1,temp2,K[5],iData[5]);
	CSHA256_16(tempC,tempD,tempE,tempF,tempG,tempH,tempA,tempB,temp1,temp2,K[6],iData[6]);
	CSHA256_16(tempB,tempC,tempD,tempE,tempF,tempG,tempH,tempA,temp1,temp2,K[7],iData[7]);

	CSHA256_16(tempA,tempB,tempC,tempD,tempE,tempF,tempG,tempH,temp1,temp2,K[8],iData[8]);
	CSHA256_16(tempH,tempA,tempB,tempC,tempD,tempE,tempF,tempG,temp1,temp2,K[9],iData[9]);
	CSHA256_16(tempG,tempH,tempA,tempB,tempC,tempD,tempE,tempF,temp1,temp2,K[10],iData[10]);
	CSHA256_16(tempF,tempG,tempH,tempA,tempB,tempC,tempD,tempE,temp1,temp2,K[11],iData[11]);
	CSHA256_16(tempE,tempF,tempG,tempH,tempA,tempB,tempC,tempD,temp1,temp2,K[12],iData[12]);
	CSHA256_16(tempD,tempE,tempF,tempG,tempH,tempA,tempB,tempC,temp1,temp2,K[13],iData[13]);
	CSHA256_16(tempC,tempD,tempE,tempF,tempG,tempH,tempA,tempB,temp1,temp2,K[14],iData[14]);
	CSHA256_16(tempB,tempC,tempD,tempE,tempF,tempG,tempH,tempA,temp1,temp2,K[15],iData[15]);

	CSHA256_48(	tempA, tempB, tempC, tempD, tempE, tempF, tempG, tempH, temp1, temp2,
				K[16], iData[16], iData[14], iData[9], iData[1], iData[0]);
	CSHA256_48(	tempH, tempA, tempB, tempC, tempD, tempE, tempF, tempG, temp1, temp2,
				K[17], iData[17], iData[15], iData[10], iData[2], iData[1]);
	CSHA256_48(	tempG, tempH, tempA, tempB, tempC, tempD, tempE, tempF, temp1, temp2,
				K[18], iData[18], iData[16], iData[11], iData[3], iData[2]);
	CSHA256_48(	tempF, tempG, tempH, tempA, tempB, tempC, tempD, tempE, temp1, temp2,
				K[19], iData[19], iData[17], iData[12], iData[4], iData[3]);
	CSHA256_48(	tempE, tempF, tempG, tempH, tempA, tempB, tempC, tempD, temp1, temp2,
				K[20], iData[20], iData[18], iData[13], iData[5], iData[4]);
	CSHA256_48(	tempD, tempE, tempF, tempG, tempH, tempA, tempB, tempC, temp1, temp2,
				K[21], iData[21], iData[19], iData[14], iData[6], iData[5]);
	CSHA256_48(	tempC, tempD, tempE, tempF, tempG, tempH, tempA, tempB, temp1, temp2,
				K[22], iData[22], iData[20], iData[15], iData[7], iData[6]);
	CSHA256_48(	tempB, tempC, tempD, tempE, tempF, tempG, tempH, tempA, temp1, temp2,
				K[23], iData[23], iData[21], iData[16], iData[8], iData[7]);

	CSHA256_48(	tempA, tempB, tempC, tempD, tempE, tempF, tempG, tempH, temp1, temp2,
				K[24], iData[24], iData[22], iData[17], iData[9], iData[8]);
	CSHA256_48(	tempH, tempA, tempB, tempC, tempD, tempE, tempF, tempG, temp1, temp2,
				K[25], iData[25], iData[23], iData[18], iData[10], iData[9]);
	CSHA256_48(	tempG, tempH, tempA, tempB, tempC, tempD, tempE, tempF, temp1, temp2,
				K[26], iData[26], iData[24], iData[19], iData[11], iData[10]);
	CSHA256_48(	tempF, tempG, tempH, tempA, tempB, tempC, tempD, tempE, temp1, temp2,
				K[27], iData[27], iData[25], iData[20], iData[12], iData[11]);
	CSHA256_48(	tempE, tempF, tempG, tempH, tempA, tempB, tempC, tempD, temp1, temp2,
				K[28], iData[28], iData[26], iData[21], iData[13], iData[12]);
	CSHA256_48(	tempD, tempE, tempF, tempG, tempH, tempA, tempB, tempC, temp1, temp2,
				K[29], iData[29], iData[27], iData[22], iData[14], iData[13]);
	CSHA256_48(	tempC, tempD, tempE, tempF, tempG, tempH, tempA, tempB, temp1, temp2,
				K[30], iData[30], iData[28], iData[23], iData[15], iData[14]);
	CSHA256_48(	tempB, tempC, tempD, tempE, tempF, tempG, tempH, tempA, temp1, temp2,
				K[31], iData[31], iData[29], iData[24], iData[16], iData[15]);

	CSHA256_48(	tempA, tempB, tempC, tempD, tempE, tempF, tempG, tempH, temp1, temp2,
				K[32], iData[32], iData[30], iData[25], iData[17], iData[16]);
	CSHA256_48(	tempH, tempA, tempB, tempC, tempD, tempE, tempF, tempG, temp1, temp2,
				K[33], iData[33], iData[31], iData[26], iData[18], iData[17]);
	CSHA256_48(	tempG, tempH, tempA, tempB, tempC, tempD, tempE, tempF, temp1, temp2,
				K[34], iData[34], iData[32], iData[27], iData[19], iData[18]);
	CSHA256_48(	tempF, tempG, tempH, tempA, tempB, tempC, tempD, tempE, temp1, temp2,
				K[35], iData[35], iData[33], iData[28], iData[20], iData[19]);
	CSHA256_48(	tempE, tempF, tempG, tempH, tempA, tempB, tempC, tempD, temp1, temp2,
				K[36], iData[36], iData[34], iData[29], iData[21], iData[20]);
	CSHA256_48(	tempD, tempE, tempF, tempG, tempH, tempA, tempB, tempC, temp1, temp2,
				K[37], iData[37], iData[35], iData[30], iData[22], iData[21]);
	CSHA256_48(	tempC, tempD, tempE, tempF, tempG, tempH, tempA, tempB, temp1, temp2,
				K[38], iData[38], iData[36], iData[31], iData[23], iData[22]);
	CSHA256_48(	tempB, tempC, tempD, tempE, tempF, tempG, tempH, tempA, temp1, temp2,
				K[39], iData[39], iData[37], iData[32], iData[24], iData[23]);

	CSHA256_48(	tempA, tempB, tempC, tempD, tempE, tempF, tempG, tempH, temp1, temp2,
				K[40], iData[40], iData[38], iData[33], iData[25], iData[24]);
	CSHA256_48(	tempH, tempA, tempB, tempC, tempD, tempE, tempF, tempG, temp1, temp2,
				K[41], iData[41], iData[39], iData[34], iData[26], iData[25]);
	CSHA256_48(	tempG, tempH, tempA, tempB, tempC, tempD, tempE, tempF, temp1, temp2,
				K[42], iData[42], iData[40], iData[35], iData[27], iData[26]);
	CSHA256_48(	tempF, tempG, tempH, tempA, tempB, tempC, tempD, tempE, temp1, temp2,
				K[43], iData[43], iData[41], iData[36], iData[28], iData[27]);
	CSHA256_48(	tempE, tempF, tempG, tempH, tempA, tempB, tempC, tempD, temp1, temp2,
				K[44], iData[44], iData[42], iData[37], iData[29], iData[28]);
	CSHA256_48(	tempD, tempE, tempF, tempG, tempH, tempA, tempB, tempC, temp1, temp2,
				K[45], iData[45], iData[43], iData[38], iData[30], iData[29]);
	CSHA256_48(	tempC, tempD, tempE, tempF, tempG, tempH, tempA, tempB, temp1, temp2,
				K[46], iData[46], iData[44], iData[39], iData[31], iData[30]);
	CSHA256_48(	tempB, tempC, tempD, tempE, tempF, tempG, tempH, tempA, temp1, temp2,
				K[47], iData[47], iData[45], iData[40], iData[32], iData[31]);

	CSHA256_48(	tempA, tempB, tempC, tempD, tempE, tempF, tempG, tempH, temp1, temp2,
				K[48], iData[48], iData[46], iData[41], iData[33], iData[32]);
	CSHA256_48(	tempH, tempA, tempB, tempC, tempD, tempE, tempF, tempG, temp1, temp2,
				K[49], iData[49], iData[47], iData[42], iData[34], iData[33]);
	CSHA256_48(	tempG, tempH, tempA, tempB, tempC, tempD, tempE, tempF, temp1, temp2,
				K[50], iData[50], iData[48], iData[43], iData[35], iData[34]);
	CSHA256_48(	tempF, tempG, tempH, tempA, tempB, tempC, tempD, tempE, temp1, temp2,
				K[51], iData[51], iData[49], iData[44], iData[36], iData[35]);
	CSHA256_48(	tempE, tempF, tempG, tempH, tempA, tempB, tempC, tempD, temp1, temp2,
				K[52], iData[52], iData[50], iData[45], iData[37], iData[36]);
	CSHA256_48(	tempD, tempE, tempF, tempG, tempH, tempA, tempB, tempC, temp1, temp2,
				K[53], iData[53], iData[51], iData[46], iData[38], iData[37]);
	CSHA256_48(	tempC, tempD, tempE, tempF, tempG, tempH, tempA, tempB, temp1, temp2,
				K[54], iData[54], iData[52], iData[47], iData[39], iData[38]);
	CSHA256_48(	tempB, tempC, tempD, tempE, tempF, tempG, tempH, tempA, temp1, temp2,
				K[55], iData[55], iData[53], iData[48], iData[40], iData[39]);

	CSHA256_48(	tempA, tempB, tempC, tempD, tempE, tempF, tempG, tempH, temp1, temp2,
				K[56], iData[56], iData[54], iData[49], iData[41], iData[40]);
	CSHA256_48(	tempH, tempA, tempB, tempC, tempD, tempE, tempF, tempG, temp1, temp2,
				K[57], iData[57], iData[55], iData[50], iData[42], iData[41]);
	CSHA256_48(	tempG, tempH, tempA, tempB, tempC, tempD, tempE, tempF, temp1, temp2,
				K[58], iData[58], iData[56], iData[51], iData[43], iData[42]);
	CSHA256_48(	tempF, tempG, tempH, tempA, tempB, tempC, tempD, tempE, temp1, temp2,
				K[59], iData[59], iData[57], iData[52], iData[44], iData[43]);
	CSHA256_48(	tempE, tempF, tempG, tempH, tempA, tempB, tempC, tempD, temp1, temp2,
				K[60], iData[60], iData[58], iData[53], iData[45], iData[44]);
	CSHA256_48(	tempD, tempE, tempF, tempG, tempH, tempA, tempB, tempC, temp1, temp2,
				K[61], iData[61], iData[59], iData[54], iData[46], iData[45]);
	CSHA256_48(	tempC, tempD, tempE, tempF, tempG, tempH, tempA, tempB, temp1, temp2,
				K[62], iData[62], iData[60], iData[55], iData[47], iData[46]);
	CSHA256_48(	tempB, tempC, tempD, tempE, tempF, tempG, tempH, tempA, temp1, temp2,
				K[63], iData[63], iData[61], iData[56], iData[48], iData[47]);

	iA+=tempA;
	iB+=tempB;
	iC+=tempC;
	iD+=tempD;
	iE+=tempE;
	iF+=tempF;
	iG+=tempG;
	iH+=tempH;

	iNl=0;
	}

/**
 * According to the standard, the message must be padded to an
 * even 512 bits. The first padding bit must be a '1'. The last
 * 64 bits represent the length of the original message. All bits 
 * in between should be 0. This helper function will pad the 
 * message according to those rules by filling the iData array 
 * accordingly. 
 */ 
void SHA256::PadMessage()
	{
	const TUint padByte = 0x80;
	
	if(!(iNl&0x03))
		{
		iData[iNl >> 2] = 0;
		}
	iData[iNl >> 2] |= padByte << ((3 - iNl&0x03) << 3) ;

	if (iNl >= (KSHA256BlockSize - 2*sizeof(TUint))) 
		{
		if (iNl < (KSHA256BlockSize - sizeof(TUint)))
			iData[(KSHA256BlockSize >> 2) - 1]=0;		
		Block();
		memset(iData , 0 ,KSHA256BlockSize*sizeof(TUint));
		} 
	else
		{
		const TUint offset=(iNl+4)>>2; //+4 to account for the word added in the
		//switch statement above
		memset((iData+offset), 0, ((KSHA256BlockSize - offset*sizeof(TUint))*sizeof(TUint)));
		}
	
	//Length in bits
	TUint64 msgLength = iNh;

	iData[(KSHA256BlockSize >> 2) - 2] = static_cast<TUint>((msgLength) >> 32);
	iData[(KSHA256BlockSize >> 2) - 1] = static_cast<TUint>((msgLength & 0xFFFFFFFF));	
	}


inline void SHA256::CopyWordToHash(TUint aVal, TInt aIndex)
	{
	TUint value = MakeWord(reinterpret_cast<TUint8*>(&aVal));
	memmove(const_cast<TUint8*>(iHash.Ptr())+ (4*aIndex), &value, sizeof(aVal));
	}
	

 const TDesC8& SHA256::Final()
	{
	AddLength(iNl);
	PadMessage();
	Block();
	//
	// Generate hash value into iHash
	//
	CopyWordToHash(iA,0);
	CopyWordToHash(iB,1);
	CopyWordToHash(iC,2);
	CopyWordToHash(iD,3);
	CopyWordToHash(iE,4);
	CopyWordToHash(iF,5);
	CopyWordToHash(iG,6);
	CopyWordToHash(iH,7);
	Reset();
	return iHash;
	}
 
 
