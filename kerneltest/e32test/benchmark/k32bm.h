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

#if !defined(__K32BM_H__)
#define __K32BM_H__

#include <e32cmn.h>
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>

#include "d32bm.h"

/**
 * <code>MBMIsr</code> interface is typically implemented by the LDD that binds
 * an ISR to the PDD interrupt source using <code>DBMPChannel::BindInterrupt(MBMIsr*)</code>. 
 */
class MBMIsr
	{
public:
	/**
	 * LDD-level Interrupt Service Routine interface. 
	 * This function is called by the PDD in response to a <code>DBMPChannel::RequestInterrupt()</code> request..
	 * 
	 * @param aNow the current time in ticks
	 */
	virtual void Isr(TBMTicks aNow) = 0;
	};

/**
 * <code>MBMInterruptLatencyIsr</code> interface is typically implemented by the LDD that binds
 * an ISR to the PDD interrupt source using <code>DBMPChannel::BindInterrupt(MBMInterruptLatencyIsr*)</code>. 
 */
class MBMInterruptLatencyIsr
	{
public:
	/**
	 * LDD-level Interrupt Service Routine interface. 
	 * This function is called by the PDD to deliver to LDD one interrupt latency measurement 
	 * that was performed by PDD in response to a <code>DBMPChannel::RequestInterrupt()</code> request.
	 *
	 * @param aLatency the result in tick of one interrupt latency measurement.
	 */
	virtual void InterruptLatencyIsr(TBMTicks aLatency) = 0;
	};

/**
 * The PDD interface.
 *
 * The caller must guarantee mutual excusion between the folloing calls:
 *		<code>BindInterrupt(MBMIsr* aIsr)</code>
 *		<code>BindInterrupt(MBMInterruptLatencyIsr* aIsr)</code>
 *		<code>RequestInterrupt()</code>
 *		<code>CancelInterrupt()</code>
 */
class DBMPChannel : public DBase
	{
public:
	/**
	 * Gets the high-resolution timer period
	 *
	 * @return timer period in ticks
	 */
	virtual TBMTicks TimerPeriod() = 0;
	/**
	 * Gets the current value of the high-resolution timer
	 *
	 * @return current time in ticks
	 */
	virtual TBMTicks TimerStamp() = 0;
	/**
	 * Translates high-resolution timer ticks to nanoseconds
	 *
	 * @param aTicks time in ticks. The implementation must truncate <code>aTicks</code> value to 
	 *			<code>aTicks % TimerPeriod()</code> prior to translation.
	 *
	 * @return time in nano-seconds
	 */
	virtual TBMNs TimerTicksToNs(TBMTicks aTicks) = 0;
	/**
	 * Translates nanoseconds to high-resolution timer ticks 
	 *
	 * @param aNs time in nanoseconds
	 *
	 * @return time in ticks
	 */
	virtual TBMTicks TimerNsToTicks(TBMNs aNs) = 0;
	/**
	 * Binds an LDD-level ISR to the PDD interrupt source.
	 *
	 * The <code>MBMIsr::Isr()</code> handler is called by the LDD's ISR 
	 * in response to a <code>DBMChannel::RequestInterrupt()</code> request.
	 * 
	 * @param aIsr points to the LDD ISR object.
	 * 
	 * @return KErrNone - on success;otherwise - an error code .
	 */
	virtual TInt BindInterrupt(MBMIsr* aIsr) = 0;
	/**
	 * Binds an LDD-level ISR to the PDD interrupt source.
	 *
	 * The <code>MBMInterruptLatencyIsr::Isr()</code> handler is called by the LDD's ISR 
	 * in response to a <code>DBMChannel::RequestInterrupt()</code> request.
	 * 
	 * @param aIsr points to the LDD ISR object.
	 * 
	 * @return KErrNone - on success;otherwise - an error code .
	 */	
	virtual TInt BindInterrupt(MBMInterruptLatencyIsr* aIsr) = 0;
	/**
	 * Asynchronously requests an interrupt.
	 * The implmentation must provide 
	 * When the interrupt will actually occur the PDD ISR will call the bound LDD-level
	 * <code>MBMIsr</code> or <code>MBMInterruptLatencyIsr</code> object. Note that only one
	 * LDD ISR object can be bound to the PDD interrupt source at any given moment of time.
	 */
	virtual void RequestInterrupt() = 0;
	/**
	 * Cancels a possibly outstanding interrupt request.
	 * When returns the PDD interrupt has been either occured or canceled.
	 */
	virtual void CancelInterrupt() = 0;
	};


#define BM_ASSERT(aCond) \
	__ASSERT_DEBUG( (aCond), (Kern::Printf("Assertion '" #aCond "' failed;\nFile: '" __FILE__ "' Line: %d\n", __LINE__), Kern::Fault("BM", 0)) )

enum
	{
	KBMLDDHighPriority = 64 - 1, // KNumPriorities - 1,
	KBMLDDMidPriority = KBMLDDHighPriority - 1,
	KBMLDDLowPriority  = KBMLDDMidPriority - 1,
	};

#endif
