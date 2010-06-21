// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#if !defined(__BM_BM_LDD_H__)
#define __BM_BM_LDD_H__

#include <e32def.h>
#include <e32cmn.h>

/**
 * The filename of the benchmark-suite logical device driver DLL
 */
_LIT(KBMLddFileName, "bm_ldd");
/**
 * The name of the benchmark-suite logical device.
 */
_LIT(KBMLdName, "bm_dev");

/**
 * The filename of the benchmark-suite physical device driver DLL
 */
_LIT(KBMPddFileName, "bm_pdd");
/**
 * The name of the benchmark-suite physical device.
 */
_LIT(KBMPdName, "bm_dev.pdd");

typedef Uint64 TBMUInt64;
typedef Int64 TBMInt64;

/**
 * Integer type for high-resolution RBMTimer ticks.
 */
typedef TBMUInt64 TBMTicks;
/**
 * Integer type for nano-second
 */
typedef TBMUInt64 TBMNs;

/**
 * Translates seconds to nano-seconds
 */
 inline TBMNs BMSecondsToNs(TInt aSeconds)
	{
	return TBMNs(aSeconds) * 1000 * 1000 * 1000;
	}
/**
 * Translates milliseconds to nanoseconds
 */
 inline TBMNs BMMsToNs(TInt aMs)
	{
	return TBMNs(aMs) * 1000 * 1000;
	}
/**
 * Translates microseconds to nanoseconds
 */
 inline TBMNs BMUsToNs(TBMUInt64 aUs)
	{
	return TBMNs(aUs) * 1000;
	}
/**
 * Translates nanoseconds to seconds
 */
inline TInt BMNsToSeconds(TBMNs aNs)
	{
	return TInt(aNs/(1000 * 1000 * 1000));
	}
/**
 * Translates nanoseconds to milliseconds
 */
inline TInt BMNsToMs(TBMNs aNs)
	{
	return TInt(aNs/(1000 * 1000));
	}
/**
 * Translates nanoseconds to microseconds
 */
inline TBMUInt64 BMNsToUs(TBMNs aNs)
	{
	return aNs/(1000);
	}

/**
 * RBMChannel class defines the user-side API to the kernel-side half of the benchmark-suite.
 *
 * The kernel-side half is implmented as <code>KBMLdName</code> logical and <code>KBMPdName</code> physical 
 * devices by <code>KBMLddFileName</code> logical and <code>KBMPddFileName</code> physical device driver DLLs
 * respectively.
 *
 * The API enables to measure some kernel-side performace parameters such as interrupt and preemption latences. 
 */
class RBMChannel : public RBusLogicalChannel
	{
public:
	
	/**
	 * Measured performace parameters.
	 */
	enum TMode
		{
		/**
		 * Interrupt Latency is the elapsed time from the occurrence of an external event to the execution of 
		 * the first instruction of the corresponding interrupt service routine (ISR).
		 */
		EInterruptLatency,
		/**
		 * Kernel Preemption Latency is the elapsed time from the end of the ISR to the execution of the first
		 * instruction of a kernel thread activated by the ISR.
		 */
		EKernelPreemptionLatency,
		/**
		 * User Preemption Latency is the elapsed time from the end of the ISR to the execution of the first 
		 * instruction of a user thread activated by the ISR
		 */
		EUserPreemptionLatency,
		/**
		 * NTimer callback invocations' jitter. 
		 */
		ENTimerJitter,
		/**
		 * The kernel-side overhead of one high-precision timer read.
		 */
		ETimerStampOverhead
		};

	/**
	 * The benchmark-suite logical device controls.
	 * 
	 * There is three groups of controls: (1) measurement of a performance parameter which is accessible through
	 * RBMChannel, (2) high-resolution timer interface which is accessible through RBMTimer and (3) misc controls
	 * accessible through RBMDriver.
	 */
	enum TControl
		{
		/**
		 * Prepare to perform a sequence of measurements of a specific performance parameter.
		 */
		EStart,
		/**
		 * Perform one measurement.
		 */
		ERequestInterrupt,
		/**
		 * Get the result of the last measurement.
		 */
		EResult,

		/**
		 * Get the current high-resolution time.
		 */
		ETimerStamp,
		/**
		 * Get the high-resolution timer period.
		 */		
		ETimerPeriod,
		/**
		 * Translate a time value from high-resolution timer ticks to nanoseconds.
		 */	
		ETimerTicksToNs,
		/**
		 * Translate a time value from nanoseconds to high-resolution timer ticks.
		 */
		ETimerNsToTicks,

		/**
		 * Change the absolute priority of a thread.
		 */
		ESetAbsPriority
		};

#ifndef __KERNEL_MODE__
	/**
	 * Open the channel for measurements of one specific performance parameter.
	 * 
	 * @param aMode specifies the performance parameter.
	 *
	 * @return <code>KErrNone</code> on success; otherwise an error code.
	 */
	TInt Open(TMode aMode)
		{
		TInt r = DoCreate(KBMLdName, TVersion(1,0,1), KNullUnit, &KBMPdName, NULL);
		if (r == KErrNone)
			{
			r = DoControl(EStart, (TAny*) aMode);
			if (r != KErrNone)
				{
				Close();
				}
			}
		return r;
		}
	/**
	 * Perform one measurement.
	 */
	void RequestInterrupt()
		{ 
		DoControl(ERequestInterrupt); 
		}
	/**
	 * Get the result of the last measurement.
	 * 
	 * @retval aTicks the result of the last measurement in RBMTimer's ticks
	 */
	void Result(TBMTicks* aTicks)
		{
		User::WaitForAnyRequest();
		DoControl(EResult, aTicks); 
		}
#endif	
	};

/**
 * RBMDriver class defines the user-side API to kernel-side utility operations.
 *
 * The operations are implmented as <code>KBMLdName</code> logical device by <code>KBMLddFileName</code>
 * logical device driver DLL.
 *
 * The API enables to change the absolute priority of a thread. 
 */
class RBMDriver : public RBusLogicalChannel
	{
public:
#ifndef __KERNEL_MODE__
	/**
	 * Opens the channel
	 *
	 * @return <code>KErrNone</code> on success; otherwise an error code
	 */
	TInt Open()
		{
		return DoCreate(KBMLdName, TVersion(1,0,1), KNullUnit, &KBMPdName, NULL);
		}
	/**
	 * Change the absolute priority of a thread.
	 *
	 * @param aThread a handle to the target thread
	 * @param aNewPrio a new absolute priority for the target thread
	 *
	 * @retval aOldPrio the old absolute priority of the target thread
	 *
	 * @return <code>KErrNone</code> on success; otherwise an error code
	 */	
	TInt SetAbsPriority(RThread aThread, TInt aNewPrio, TInt* aOldPrio)
		{
		TInt aPrio = aNewPrio;
		TInt r = DoControl(RBMChannel::ESetAbsPriority, (TAny*) aThread.Handle(), (TAny*) &aPrio);
		if (r == KErrNone)
			{
			*aOldPrio = aPrio;
			}
		return r;
		}
#endif
	};

/**
 * RBMTimer class defines the user-side API to the high-precision timer.
 *
 * The timer is implmented as <code>KBMLdName</code> logical and <code>KBMPdName</code> physical 
 * devices by <code>KBMLddFileName</code> logical and <code>KBMPddFileName</code> physical device driver DLLs
 * respectively.
 */
class RBMTimer : public RBusLogicalChannel
	{
public:

#ifndef __KERNEL_MODE__
	/**
	 * Opens the channel to the high-precision timer.
	 *
	 * @return <code>KErrNone</code> on success; otherwise an error code
	 */
	TInt Open()
		{
		return DoCreate(KBMLdName, TVersion(1,0,1), KNullUnit, &KBMPdName, NULL); 
		}
	/**
	 * Gets the current time in ticks.
	 *
	 * @retval aTicks the current time in <code>TBMTicks</code>
	 */
	void Stamp(TBMTicks* aTicks)
		{ 
		DoControl(RBMChannel::ETimerStamp, aTicks); 
		}
	/**
	 * Gets the timer period in ticks.
	 *
	 * @retval aPriod the timer period in <code>TBMTicks</code>
	 */	
	void Period(TBMTicks* aPeriod)
		{ 
		DoControl(RBMChannel::ETimerPeriod, aPeriod); 
		}
	/**
	 * Translates ticks to nano-seconds.
	 *
	 * @param aTciks a pointer to the <code>TBMTicks</code> value to be translated.
	 *
	 * @retval aNs the resulting time value in nanoseconds.
	 */	
	void TicksToNs(TBMTicks* aTicks, TBMNs* aNs)
		{ 
		DoControl(RBMChannel::ETimerTicksToNs, aTicks, aNs); 
		}
	/**
	 * Translates nanoseconds to ticks.
	 *
	 * @param aNs a pointer to the time value in nanoseconds to be translated.
	 *
	 * @retval aTicks the resulting time in <code>TBMTicks</code>.
	 */		
	void NsToTicks(TBMNs* aNs, TBMTicks* aTicks)
		{ 
		DoControl(RBMChannel::ETimerTicksToNs, aNs, aTicks); 
		}
#endif
	};
	
#endif
