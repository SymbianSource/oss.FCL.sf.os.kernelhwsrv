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
* Description:
* eka\kernel\srandombuff.cpp
* 
*/

#include <kernel/kern_priv.h>
#include <assp.h>
#include "kernel/securerng.h"

const TInt KWordIndexBitMask = 0x1FFF; // bottom 13 bits holds buffer word index
const TInt KEntropyCountBitShift = 14; // top 18 bits holds entropy count
const TUint KEntropyBufferThresholdWords = KEntropyBufferSizeWords * 3/4;

/**
	Adds a single bit to the random pool used to generate random numbers.

    @deprecated use Kern::RandomSalt(TUint32 aEntropyData, TUint aBitsOfEntropy) or Kern::RandomSalt(TUint64 aEntropyData, TUint aBitsOfEntropy) instead

	@param aBitOfSalt The least significant bit of this value is added to the random pool.

  	@pre Can be used in a device driver.
*/
EXPORT_C void Kern::RandomSalt(TUint32 aBitOfSalt)
	{
    // Check if RNG needs more entropy.
    if (SecureRNG->SecureRNGIdle())
        return;

    Kern::RandomSalt(aBitOfSalt, 1);
	}


/**
	Adds 32 bits of data, using the entropy estimate provided, to the random pool used to generate random numbers.

    @param aEntropyData The 32 bits of data to be added to the random pool.
    @param aBitsOfEntropy An estimate of the number of bits of entropy contained in the 32 bits of data.

  	@pre aBitsOfEntropy must be >=0 and <=32.
    @pre Can be used in a device driver.
*/
EXPORT_C void Kern::RandomSalt(TUint32 aEntropyData, TUint aBitsOfEntropy)
    {
    // Check if RNG needs more entropy.
    if (SecureRNG->SecureRNGIdle())
        return;

    __ASSERT_ALWAYS((TUint)aBitsOfEntropy <= 32, K::PanicKernExec(EEntropyEstimateOutOfRange));

    TUint32 addcounts = (aBitsOfEntropy << KEntropyCountBitShift) + 1; // top 18 bits holds entropy count, plus 1 to increment buffer word index

    TInt irq = NKern::DisableAllInterrupts();

    TInt currentcpu = NKern::CurrentCpu();

    TUint32 currentstatus = __e32_atomic_load_acq32(&K::EntropyBufferStatus[currentcpu]);
    TUint32 nextword = currentstatus & KWordIndexBitMask; // bottom 13 bits holds buffer word index

    if(nextword < KEntropyBufferSizeWords)
        {
        K::EntropyBuffer[currentcpu][nextword] = aEntropyData;
        __e32_atomic_add_ord32(&K::EntropyBufferStatus[currentcpu], addcounts);
        }

    NKern::RestoreInterrupts(irq);

    TUint32 entropycount = (currentstatus + addcounts) >> KEntropyCountBitShift; // top 18 bits holds entropy count
    if((nextword < KEntropyBufferSizeWords) && (entropycount >= KReseedThreshold || nextword > KEntropyBufferThresholdWords))
        {
        if (NKern::CurrentContext() == NKern::EInterrupt)
            K::EntropyBufferDfc.Add();
        else
            K::EntropyBufferDfc.Enque();
        }
    }


/**
	Adds 64 bits of data, using the entropy estimate provided, to the random pool used to generate random numbers.

    @param aEntropyData The 64 bits of data to be added to the random pool.
    @param aBitsOfEntropy An estimate of the number of bits of entropy contained in the 32 bits of data.

  	@pre aBitsOfEntropy must be >=0 and <=64.
    @pre Can be used in a device driver.
*/
EXPORT_C void Kern::RandomSalt(TUint64 aEntropyData, TUint aBitsOfEntropy)
    {
    // Check if RNG needs more entropy.
    if (SecureRNG->SecureRNGIdle())
        return;

    __ASSERT_ALWAYS((TUint)aBitsOfEntropy <= 64, K::PanicKernExec(EEntropyEstimateOutOfRange));

    TUint32 addcounts = (aBitsOfEntropy << KEntropyCountBitShift) + 2; // top 18 bits holds entropy count, plus 2 to increment buffer word index

    TInt irq = NKern::DisableAllInterrupts();

    TInt currentcpu = NKern::CurrentCpu();

    TUint32 currentstatus = __e32_atomic_load_acq32(&K::EntropyBufferStatus[currentcpu]);
    TUint32 nextword = currentstatus & KWordIndexBitMask; // bottom 13 bits holds buffer word index

    if((nextword + 1) < KEntropyBufferSizeWords)
        {
        K::EntropyBuffer[currentcpu][nextword] = I64LOW(aEntropyData);
        K::EntropyBuffer[currentcpu][nextword+1] = I64HIGH(aEntropyData);
        __e32_atomic_add_ord32(&K::EntropyBufferStatus[currentcpu], addcounts);
        }

    NKern::RestoreInterrupts(irq);

    TUint32 entropycount = (currentstatus + addcounts) >> KEntropyCountBitShift; // top 18 bits holds entropy count
    if((nextword < KEntropyBufferSizeWords) && (entropycount >= KReseedThreshold || nextword > KEntropyBufferThresholdWords))
        {
        if (NKern::CurrentContext() == NKern::EInterrupt)
            K::EntropyBufferDfc.Add();
        else
            K::EntropyBufferDfc.Enque();
        }
    }


#if defined(__EPOC32__)

EXPORT_C void Interrupt::AddTimingEntropy()
    {
    // This function should only be called from Interrupt context otherwise timing may be predictable.
    CHECK_PRECONDITIONS(MASK_NOT_THREAD|MASK_NOT_IDFC, "Interrupt::AddTimingEntropy()");

    // Check if RNG needs more entropy.
    if (SecureRNG->SecureRNGIdle())
        return;

    TUint32 timestamp = NKern::FastCounter();

    Kern::RandomSalt(timestamp, 1);
    }

#endif

extern void DrainEntropyBuffers(TAny*)
    {
    TUint32 wordsavailable = 0;
    TUint32 wordscopied;
    TUint32 currentstatus;
    TBool result = 0;

    for(TInt cpu = (NKern::NumberOfCpus() - 1); cpu >= 0; --cpu)
        {
        wordscopied = 0;
        currentstatus = __e32_atomic_load_acq32(&K::EntropyBufferStatus[cpu]);

        // if this CPU's buffer is empty then skip processing
        if(currentstatus == 0) continue;

        do
            {
            wordsavailable = currentstatus & KWordIndexBitMask;

            memcpy(&K::TempEntropyBuffer[wordscopied], &K::EntropyBuffer[cpu][wordscopied], (wordsavailable - wordscopied)*4);

            wordscopied = wordsavailable;

            result = __e32_atomic_cas_ord32(&K::EntropyBufferStatus[cpu], &currentstatus, 0);
            }
        while(!result);

        SecureRNG->AddEntropy((TUint8*)&K::TempEntropyBuffer[0], wordscopied*4, currentstatus >> KEntropyCountBitShift);
        }
    }


extern TInt InitialiseEntropyBuffers()
    {
    TInt numcpus = NKern::NumberOfCpus();

    // Allocate space for all cpu buffers and store pointer to beginning as CPU 0 buffer.
    K::EntropyBuffer[0] = (TUint32*)Kern::Alloc(KEntropyBufferSizeWords * numcpus * 4);

    // Initialise pointers for remaining cpus
    for(TInt cpu = 1; cpu < numcpus; ++cpu)
        {
        K::EntropyBuffer[cpu] = K::EntropyBuffer[0] + (cpu * KEntropyBufferSizeWords);
        }

    // Initialise DFC
    K::EntropyBufferDfc.SetDfcQ(K::SvMsgQ);

    return KErrNone;
    }
