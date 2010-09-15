// Copyright (c) 1994-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\kernel\random.cpp
// 
//

#include <kernel/kern_priv.h>
#include "securerng.h"
#include "execs.h"

// The global pointer to the RNG instance
DSecureRNG* SecureRNG;

/**
	Gets 32 random bits from the kernel's random number generator.

	The returned random data may or may not be cryptographically secure but should be of a high quality for
	non-cryptographic purposes.

	This function uses a cryptographically strong random number generator to generate the random data, which can
	be slower than insecure generators. If security is not important, you may wish to use a trivial RNG instead
	for performance.

	@return The 32 random bits.

	@pre Kernel Lock must not be held.
	@pre No fast mutex should be held
	@pre Interrupts should be enabled
	@pre Can be used in a device driver.
*/
EXPORT_C TUint32 Kern::Random()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD, "Kern::Random()");
	TBuf8<4> randomBuffer;
	randomBuffer.SetMax();
	SecureRNG->GenerateRandomNumber(randomBuffer);
	return *((const TUint32*)randomBuffer.Ptr());
	}

/**
	Fills the provided descriptor with random data up to its current length. The number of random bytes required
	should be specified by setting the length of the descriptor that is passed to the function.

	If the returned random data cannot be guaranteed to be cryptographically secure, the function will return
	KErrNotReady, but data will still be provided suitable for non-cryptographic purposes.

	The security strength of the cryptograpically strong random number generator is 256 bits.

	@param aRandomValue  on return, the descriptor is filled with the requested number of random bytes.

	@return KErrArgument	if more than 65536 bytes are requested in a single call.
			KErrNotReady	if the returned random data cannot be guaranteed to be cryptographically secure.
			KErrNone		if the returned random data is believed to be cryptographically secure.
		
	@pre Kernel Lock must not be held
	@pre No fast mutex should be held
	@pre Interrupts should be enabled
	@pre Can be used in a device driver.
*/
EXPORT_C TInt Kern::SecureRandom(TDes8& aRandomValue)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD, "Kern::SecureRandom(TDes8&)");
	return SecureRNG->GenerateRandomNumber(aRandomValue);
	}

/**
	Adds the given entropy input to the entropy pool used for random number generation.
	This is the only version of Kern::RandomSalt which allows entropy samples larger than 8 bytes to be added.

	Entropy estimates should be chosen carefully to reflect the minimum entropy that the sample may contain.

	@param aEntropyData			Pointer to the entropy data.
	@param aEntropyDataLength	Length of the entropy data in bytes.
	@param aBitsOfEntropy		The amount of entropy (in bits) present in the entropy data.

	@pre Kernel Lock must not be held
	@pre No fast mutex should be held
	@pre Interrupts should be enabled
	@pre Can be used in a device driver.
*/
EXPORT_C void Kern::RandomSalt(const TUint8* aEntropyData, TUint aEntropyDataLength, TUint aBitsOfEntropy)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD, "Kern::RandomSalt(const TUint8*, TUint, TUint)");

	// Check if the claimed entropy estimation (in bits) is not more than the maximum value.
	__ASSERT_ALWAYS(aBitsOfEntropy <= aEntropyDataLength*8, K::PanicKernExec(EEntropyEstimateOutOfRange));

	// If the Secure RNG system is not in idle mode, add the collected entropy to the entropy pool.
	if (!SecureRNG->SecureRNGIdle())
		{
		SecureRNG->AddEntropy(aEntropyData, aEntropyDataLength, aBitsOfEntropy);
		}
	}

/*
 * Exec handler function for obtaining secure random numbers
 */
TInt ExecHandler::MathSecureRandom(TDes8& aRandomValue)
	{
	TInt randomValueLength = 0;
	TInt randomValueMaxLength = 0;
	//Gets information about the user specified descriptor.
	TUint8* kernelPtr = (TUint8*)Kern::KUDesInfo(aRandomValue, randomValueLength, randomValueMaxLength);
	if(randomValueMaxLength == -1) //user passed descriptor is not writable
		{
		K::PanicKernExec(EKUDesSetLengthInvalidType);
		}

	// The random number generator requires a temporary buffer to write the data to, before we write it back to
	// userspace's buffer. The buffer is allocated here on the stack to avoid having to heap-allocate, and if
	// the requested amount of data is larger, a loop is used. 1024 bytes will always fit onto the stack in an
	// exec handler.
	const TInt KRandomBufferSize = 1024;
	TBuf8<KRandomBufferSize> randomBuffer;

	TInt err = KErrNone;
	TBool isKErrNotReadyTrue = EFalse;
	while(randomValueLength > 0)
		{
		TInt noOfBytesToGenerate = (randomValueLength > KRandomBufferSize) ? KRandomBufferSize : randomValueLength;
		randomBuffer.SetLength(noOfBytesToGenerate);
		// Generate random numbers 
		err = Kern::SecureRandom(randomBuffer);
		if(err == KErrNotReady)
			{
			isKErrNotReadyTrue = ETrue;
			}
		else if (err != KErrNone)
			{
			return err; // any other system wide error code needs to be returned immediately.
			}
		// Copy the generated random numbers to the user descriptor.
		umemput(kernelPtr, randomBuffer.Ptr(), noOfBytesToGenerate);
		kernelPtr += KRandomBufferSize;
		randomValueLength -= KRandomBufferSize;
		}

	// Atleast one KErrNotReady error was generated during processing the request, so return the state as not ready
	// indicating the internal states was not secure during random number generation. 
	if(isKErrNotReadyTrue)
		{
		return KErrNotReady;
		}
	return err;
	}
