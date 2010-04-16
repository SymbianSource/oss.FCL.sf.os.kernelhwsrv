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
// Description:
// eka\kernel\securerng.cpp
//
//

#include "kernel/securerng.h"
#include "sha256.h"

/**
 * Default Constructor.
 * Initialize the Secure RNG with values for iInternalStateV, iInternalStateC, Reseed_Counter
 * Entropy Pool threshold and Secure RNG status
 */
DSecureRNG::DSecureRNG() :
			iSha256(),
			iEntropyPool(),
			iReseedCounter(0),
			iEntropyEstimation(0),
			iSecureRNGIdle(EFalse),
			iRNGSecure(EFalse),
			iEntropyThreshold(KInstantiationThreshold)
	{
	// Create the Mutex objects before initializing the internal state.
	_LIT(KRNGMutexName,"SecureRNGMutex");
	_LIT(KEntropyMutexName,"EntropyMutex");
	NKern::ThreadEnterCS();
	__ASSERT_ALWAYS(Kern::MutexCreate(iSecureRNGMutex, KRNGMutexName, KMutexOrdRandNumGeneration) == KErrNone,
					K::Fault(K::ESecureRNGInitializationFailed));
	__ASSERT_ALWAYS(Kern::MutexCreate(iEntropyMutex, KEntropyMutexName, KMutexOrdEntropyPool) == KErrNone,
					K::Fault(K::ESecureRNGInitializationFailed));
	NKern::ThreadLeaveCS();

	// Initially feed the internal state with the current system time tick
	TUint64 tick = Kern::SystemTimeSecure();
	TBuf8<sizeof(TUint64)> entropyInput;
	entropyInput.Copy((TUint8*)(&tick), sizeof(TUint64));

	// Initialize the system internal state with the time tick as entropy input.
	Reseed(entropyInput);
	}

/**
 * Random Number Generation Algorithm:
 * 1.If reseed_counter > reseed_interval, then return an indication that a reseed is required.
 * 2.(returned_bits) = Hashgen (requested_number_of_bits, V).
 * 3. H = Hash (0x03 || V). Where 0x03 is represented in one byte
 * 4. V = (V + H + C + reseed_counter) mod 2 ^seedlen.
 * 5. reseed_counter = reseed_counter + 1.
 * 6. Return SUCCESS, and update the new values of V, C, and reseed_counter
 *	for the new_working_state.
 * @param aRandomValue	   on return will contain the generated random numbers.The length of random number
 *						   generated will be equal to the current length of the descriptor(aRandomValue)
 *						   passed as argument to the function call
   @panic					Panics the kernel with KErrNotReady when the number of requests that can be served has crossed it maximum limit
							 of KReseedInterval. The panic will indicate that something has badly gone wrong with the Entropy accumulation and
							 Reseed unit.
 * @return					 KErrNone if everything is fine or KErrNotReady when the system is not ready yet.
 */
TInt DSecureRNG::GenerateRandomNumber(TDes8& aRandomValue)
	{
	// If requested for random numbers greater than the max limit specified by HASH_DRBG (2^19 bits = 65536 bytes), return error
	if(aRandomValue.Length() > KMaxNoOfRequestedBytes)
		{
		return KErrArgument;
		}

	// Secure the random number generation operation through mutex
	NKern::ThreadEnterCS();
	Kern::MutexWait(*iSecureRNGMutex);
	if(iReseedCounter >= KReseedInterval && iRNGSecure)
		{
		// If we got reseeded before but haven't been reseeded inside the interval, then something must
		// have happened to the entropy collection mechanism (the interval is quite large) - possibly we
		// are under attack. There is no way to return to a secure state, so fault the system.
		K::Fault(K::ESecureRNGInternalStateNotSecure);
		}
	iSecureRNGIdle = EFalse;

	// Generate the random numbers
	HashGen(aRandomValue);

	const TUint8 KRngConstant = 0x03;
	iSha256.Update((TUint8*)&KRngConstant, sizeof(TUint8));
	iSha256.Update(iInternalStateV , KSeedLength);

	// Update the secret value of the Internal State V so that back tracking can be avoided.
	// V = (V+C+H+reseed_counter) mod 2 ^seedlength
	AddBigNumberToInternalStateV(iInternalStateC, KSeedLength);
	AddBigNumberToInternalStateV(iSha256.Final().Ptr(),KSHA256OutLengthInBytes );

	// Converts iReseedCounter value from little endian to big endian format and stores in tempcounter.
	TUint32 tempCounter = ConvertToBigEndian(iReseedCounter);
	AddBigNumberToInternalStateV((TUint8*)&tempCounter,sizeof(tempCounter));

	++iReseedCounter;
	TBool rngSecure = iRNGSecure;
	Kern::MutexSignal(*iSecureRNGMutex);
	NKern::ThreadLeaveCS();

	if(rngSecure)
		{
		return KErrNone;
		}
	return KErrNotReady;
	}

/**
 * Converts the aTempCounter from little endian format to big endian format. This conversion is required
 * for performing addition in the function AddBigNumbers().
 * Works fine for 32 bits data only.
 * @ return TUint32, big endian format 32 bit value of the iReseedCounter
 */
inline TUint32 DSecureRNG::ConvertToBigEndian(TUint32 aTempCounter)
	{
	return ((aTempCounter >> 24 & 0x000000ff)| (aTempCounter<< 8 & 0x00ff0000)
					  |(aTempCounter>>8 & 0x0000ff00) | (aTempCounter << 24 &0xff000000));
	}

/**
 * This function performs addition of an integer(aInteger) passed to the function with iInternalStateV and
 * the result is stored in iInternalStateV. The function is required to update the internal state of
 * V(iInternalStateV) after each random number generation request.
 * @param aInteger  this is the input paramenter which is added with iInternalStateV and the result
 *				  is stored in iInternalStateV
 * @param aLength   length of the aInteger( integer that needs to be added to iInternalStateV) passed to the function
 */
void DSecureRNG::AddBigNumberToInternalStateV(const TUint8* aInteger, TInt aLength)
	{
	TUint8 sum[2]= {0};
	TInt index = KSeedLength;
	while(--index >= 0 && (--aLength >= 0 || sum[1] == 1))
		{
		TUint8 integer = (aLength >= 0) ? aInteger[aLength] : (TUint8)0 ;
		// sum[0]will hold the value of addition operation and sum[1] will hold carry (if any)
		*(TUint16*)&sum = TUint16(iInternalStateV[index] + integer + sum[1]);
		iInternalStateV[index] = sum[0];
		}
	}

/**
 *Generate the next random number bits to be returned.
 * HashGen Algorith:
 * 1. m =  requested_number_of_bits/ KSHA256OutLengthInBytes.
 * 2. data = V.
 * 3. W = the Null string.
 * 4. For i = 1 to m
 * 4.1 wi = Hash (data).
 * 4.2 W = W || wi.
 * 4.3 data = (data + 1) mod 2^seedlen.
 * 5. returned_bits = Leftmost (requested_no_of_bits) bits of W.
 *
 * Continuous Random number generation test: This is FIPS recommended (FIPS 140-2)test. Definition as per FIPS:-
 *	1.	If each call to a RNG produces blocks of n bits (where n > 15), the first n-bit block generated
 *		after power-up, initialization, or reset shall not be used, but shall be saved for comparison with
 *		the next n-bit block to be generated. Each subsequent generation of an n-bit block shall be compared
 *		with the previously generated block. The test shall fail if any two compared n-bit blocks are equal.
 *	2.	If each call to a RNG produces fewer than 16 bits, the first n bits generated after power-up,
 *		initialization, or reset (for some n > 15) shall not be used, but shall be saved for comparison
 *		with the next n generated bits. Each subsequent generation of n bits shall be compared with the
 *		previously generated n bits. The test fails if any two compared n-bit sequences are equal.
 *
 * @param aRandomBuffer on return, the descriptor will contain the generated random bytes.
 *
 */
void DSecureRNG::HashGen(TDes8& aRandomBuffer)
	{
	TBuf8<KSeedLength> data;
	// data = V(iInternalStateV)
	data.Copy(iInternalStateV, KSeedLength);

	TInt noOfBytesToCopy = aRandomBuffer.Length();
	TInt newLength = noOfBytesToCopy;
	//set the length to zero
	aRandomBuffer.Zero();
	while(noOfBytesToCopy > 0)
		{
		newLength = noOfBytesToCopy > KSHA256OutLengthInBytes ? KSHA256OutLengthInBytes: noOfBytesToCopy;
		// Append Hashed Data to buffer
		aRandomBuffer.Append(HashDataAndCompare(data), newLength);
		IncrementData(data);
		noOfBytesToCopy = noOfBytesToCopy - KSHA256OutLengthInBytes;
		}
	}

/*
 * Generates Hash of the aData and compares it with previous generated n(256) random bits to verify for
 * Continuous Random number generation test.
 * @param aData, the latest copy of iInternalStateV value to be used hash operation
 * @return ptr, pointer to the generated hash value using sha256
 */
inline const TUint8* DSecureRNG::HashDataAndCompare(TDes8& aData)
	{
	iSha256.Update(aData.Ptr(), aData.Length());
	const TDesC8& ptr = iSha256.Final();
	if(iCompareBuffer.Length()!= 0)
		{
		// According to FIPS 140-2: if the n-bit random number is same as the previously generated
		// n-bit random number then fault the system
		__ASSERT_ALWAYS(iCompareBuffer.Compare(ptr)!= 0,K::Fault(K::ESecureRNGOutputsInBadState));
		}
	iCompareBuffer.Copy(ptr);
	return ptr.Ptr();
	}

/**
 * Calculates (data+1) modulus of 2^440.
 * @return aData, on return contains aData value incremented by one
 */
inline void DSecureRNG::IncrementData(TDes8& aData)
	{
	TInt i = KSeedLength-1;
	aData[i] += 1;
	while( i > 0 && aData[i] == 0)
		{
		aData[--i] +=1;
		}
	}

/**
 * The hash-based derivation function hashes the given input string and
 * returns the required no. of bits of hash value on the second parameter.
 * Algorithm:
 * 1. temp = the Null string.
 * 2. len = no_of_bits_to_return / out_len.
 * 3. counter = an 8-bit binary value representing the integer "1".
 * 4. For i = 1 to len do //Comment : In step 4.1, no_of_bits_to_return is used as a 32-bit string.
 *  4.1 temp = temp || Hash (counter || no_of_bits_to_return || input_string).
 *  4.2 counter = counter + 1.
 * 5. requested_bits = Leftmost (no_of_bits_to_return) of temp.
 * 6. Return SUCCESS and requested_bits.
 *
 *  In our case, the no_of_bits_to_return and out_len are constants 440, 256 respectively
 *  and hence the KLoopLength too becomes constant 2.
 *  @param, aInputData, holds the input data(state) which needs to be updated
 *  @param aOutputData, on return will have the updated value of internal State
*/
void DSecureRNG::HashDf(const TDesC8& aInputData, TUint8* aOutputData)
	{
	// Seed Length is 440 and SHA256 output block length 256. So required iterations is 2 for 440 bits
	const TUint8 KLoopLength = 2;
	const TUint32 KNumOfBitsToReturn = KSeedLength * 8;
	TInt length = KSHA256OutLengthInBytes;
	// Note: The 'length' in memcpy works fine only for curent seed length (440 bits => two iterations)
	// In future, if the seed length changes, this too should be modified accoringly.
	for (TUint8 counter = 1; counter<= KLoopLength; ++counter)
		{
		iSha256.Update(&counter, sizeof(TUint8));
		iSha256.Update((TUint8*)&KNumOfBitsToReturn, sizeof(TUint32));
		iSha256.Update(aInputData.Ptr(), aInputData.Length());
		memmove((aOutputData + (counter-1) * KSHA256OutLengthInBytes), iSha256.Final().Ptr(), length);
		length = KSeedLength - KSHA256OutLengthInBytes;
		}
	}

 /**
  * This method would generate the new seed with the entropy passed in and update the internal state
  * based on the new seed generated. Seed Generation should happen in the following two cases.
  * Instantiattion: Internal state updation for the first time with sufficient entropy.
  * 1. construct the seed material : seed_material = entropy_input
  * 2. seed = Hash_df (seed_material, seedlen).
  * 3. V = seed.
  * 4. C = Hash_df ((0x00 || V), seedlen). // Precede with a byte of all zeros.
  * 5. reseed_counter = 0.
  *
  * Reseeding: Internal state updation with the sufficient entropy for all instances except first time.
  * 1. construct the seed material : seed_material = 0x01 || V || entropy_input
  * 2. seed = Hash_df (seed_material, seedlen).
  * 3. V = seed.
  * 4. C = Hash_df ((0x00 || V), seedlen). // Precede with a byte of all zeros.
  * 5. reseed_counter = 0.
  *
  * In both the cases only the step 1 differs. All other steps are common for both.
  * The internal state and other relevant members are protected by the mutex.
  * @param aEntropyInput, holds the entropy input values required to update the internal states
*/
void DSecureRNG::Reseed(const TDesC8& aEntropyInput)
	{
	const TUint8 KConstOne = 0x01;
	TBuf8<KMaxSeedMaterialLength> seedMaterial;

	// Construct the seed material.
	if(iRNGSecure)
		{
		// After the first seeding (which will set iRNGSecure), seed_material = 0x01 || V || entropy_input
		seedMaterial.Append(&KConstOne, sizeof(TUint8));
		seedMaterial.Append(iInternalStateV, KSeedLength);

		// Make the system idle on every reseed (will be reset next time the RNG is used)
		iSecureRNGIdle = ETrue;
		}

	// The enropy input is used whether it's the first seeding or not.
	seedMaterial.Append(aEntropyInput);

	// On the first seeding, include the personalizsation string for the instantiation.
	if(!iRNGSecure)
		{
		// System time in ticks is considered as a personalization string
		TInt64 personalizationString = Kern::SystemTimeSecure();
		seedMaterial.Append((TUint8*)&personalizationString, sizeof(TUint));
		}

	// Calculate the seed and update the internal state V
	HashDf(seedMaterial, iInternalStateV);

	// Calculate and update the internal state C
	seedMaterial.FillZ(1); // Put the 0x00 in the first byte of the buffer
	seedMaterial.Append(iInternalStateV, KSeedLength);
	HashDf(seedMaterial, iInternalStateC);

	// Reset the reseed counter and other related parameter values as well.
	iReseedCounter = 0;
	iEntropyPool.Reset();
	// Reset the current estimation.
	iEntropyEstimation = 0;
	}

/**
 * Controls the Reseed process with the following algorithm.
 * 1. Collect the received entropy in the entropy pool like hash object.
 * 2. Increment the entropy estimation counter by the received estimation value.
 * 3. If the so far collected estimation is higher than the threshold value,
 *  3.1. Call Reseed to generate the new seed and update the system internal state
 *  3.2. Decide the secure status of the new seed and hence the RNG system
 *  3.3. Decide whether the system is idle or not
 *  3.4. Reset the entropy pool hash object and the entropy estimation counter.
 *
*/
void DSecureRNG::AddEntropy(const TUint8* aEntropy, TInt aLength, TInt aEstimation)
	{
	// Get the mutex to update the entropy pool
	NKern::ThreadEnterCS();
	Kern::MutexWait(*iEntropyMutex);

	iEntropyPool.Update(aEntropy, aLength);
	iEntropyEstimation += aEstimation;

	if (iEntropyEstimation >= iEntropyThreshold)
		{
		// Get the mutex to update the internal state via Reseed.
		Kern::MutexWait(*iSecureRNGMutex);

		// Get the final hash value and pass on to the reseed, which inturn will update the internal state.
		Reseed(iEntropyPool.Final());

		// Set the threshold to 256 for all but the first reseed. For instantiation
		// the threshold should be 384 which was set already as part of initialization.
		iEntropyThreshold = KReseedThreshold;

		// As the entropy estimation has crossed the threshold, the system becomes secure.
		iRNGSecure = ETrue;

		// Send the reseed notification to the hook, if one is installed
		if (iReseedHookFn)
			iReseedHookFn(iReseedHookArg);

		// Updates are done. So, release the Mutex.
		Kern::MutexSignal(*iSecureRNGMutex);
		}

	// Updates are done. So, release the Mutex.
	Kern::MutexSignal(*iEntropyMutex);
	NKern::ThreadLeaveCS();
	}

// Allow a test driver to set a hook function which will be called on reseed.
void DSecureRNG::SetReseedHook(void (*aReseedHookFn)(TAny*), TAny* aReseedHookArg)
	{
	NKern::ThreadEnterCS();
	Kern::MutexWait(*iSecureRNGMutex);
	iReseedHookFn = aReseedHookFn;
	iReseedHookArg = aReseedHookArg;
	Kern::MutexSignal(*iSecureRNGMutex);
	NKern::ThreadLeaveCS();
	}
