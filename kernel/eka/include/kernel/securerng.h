// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
//
// Description:
// eka\include\kernel\securerng.h
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __SECURERNG_H__
#define __SECURERNG_H__

#include <kernel/kern_priv.h>
#include <assp.h>
#include "sha256.h"

//Constants required for Secure RNG 

// Specifies the maximum number of random requests that can be served with a single secure internal state or seed.
// NIST recommends this value to be significantly less than 2^48 for all practical usage. The factors affecting the chosen value are
// 1> This value should not be too high as it may weaken the strength of the random numbers generated after certain requests.
// 2> Should be able to expose if any problems in the entropy Accumulation and Reseed unit at the earliest.
// 3> Also depends on the number of entropy sources contributing the entropy data.
const TUint32 KReseedInterval = 16777216;     //is equal to 2^24
// 8 (for 01) + 440 (for V) + 256 (for the hashed entropy input) + 64 (for personilazation string) => 768 / 8 = 96.
const TUint KMaxSeedMaterialLength = 96;
// Entropy pool threshold for reseed = HASH_DRBG security strength(256)
const TUint KReseedThreshold = 256;
// Entropy pool threshold for instantiation = HASH_DRBG security strength(256) + 1/2 of HASH_DRBG security strength(128) => 384
const TUint KInstantiationThreshold = 384;
// Maximum number of random bytes that can be served by Secure RNG. Should be less than or equal to 2^19 = 524288 bits as per HASH_DRBG algorithm
const TInt KMaxNoOfRequestedBytes = 65536; // in bytes

// SeedLength for Hash_DRBG is fixed to be 55 bytes as per Hash_DRBG Algorithm.
const TInt KSeedLength = 55;

// SHA256 hash size in bytes
const TInt KSHA256OutLengthInBytes = 32;

/**
 *Utility functions for generating secure random numbers.
*/
class DSecureRNG: public DBase
	{
public:
	DSecureRNG();
	
	//Generates the requested number of random bits
	TInt GenerateRandomNumber(TDes8& aRandomValue);
	void AddEntropy(const TUint8* aEntropy, TInt aLength, TInt aEstimation);
	inline TBool SecureRNGIdle() {return iSecureRNGIdle;}
	void SetReseedHook(void (*aHookFn)(TAny*), TAny* aHookArg);
	
private:
	void HashGen(TDes8& aRandomValue);
	void Reseed(const TDesC8& aEntropyInput);
	void AddBigNumberToInternalStateV(const TUint8* aInteger2, TInt aLength);
	void HashDf(const TDesC8& aInputData, TUint8* aOutputData);

	inline void IncrementData(TDes8& aData);
	inline TUint32 ConvertToBigEndian(TUint32 aTempCounter);
	inline const TUint8* HashDataAndCompare(TDes8& aData);

private:
	// Mutex to enforce concurrency control over the system internal states and other related members
	DMutex* iSecureRNGMutex;
	
	// Mutex to enforce concurrency control over the entropy pool and entropy estimation
	DMutex* iEntropyMutex;

	SHA256 iSha256;
	
	// Object acts as a entropy pool to hold the entropy inputs in hash context.
	SHA256 iEntropyPool;
	
	// Internal_State_V and Internal_State_C are secret values of Hash DRBG mechanism
	TUint8 iInternalStateV[KSeedLength];
	TUint8 iInternalStateC[KSeedLength];
	
	// Required for comparision during continous random number generation test
	TBuf8<KSHA256OutLengthInBytes> iCompareBuffer;
	
	// Counts how many random requests are served since last reseed.
	TUint32 iReseedCounter;
	
	// Cumulative estimation of the so far collected entropy inputs in the pool.
	TUint32 iEntropyEstimation;
	
	// Holds system's current operational status (Idle / Active).
	volatile TBool iSecureRNGIdle;

	// Says whether the system is secure or not at the moment.
	TBool iRNGSecure;
	
	// Estimation threshold limit, decides the reseed invocation point.
	TUint32 iEntropyThreshold;

	// Hook to call on reseed
	void (*iReseedHookFn)(TAny*);
	TAny* iReseedHookArg;
	};

// The secure random number generator - global object.
extern DSecureRNG *SecureRNG;

#endif
