/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/


#include <drivers/iic.h>
#include <drivers/iic_channel.h>

#include "iic_psl.h"

#ifdef MASTER_MODE
#include "iic_master.h"
#endif
#ifdef SLAVE_MODE
#include "iic_slave.h"
#endif


// EXTENSION DLL ENTRY POINT
DECLARE_EXTENSION_WITH_PRIORITY(BUS_IMPLMENTATION_PRIORITY)
	{
	// Array of pointers to the Channels that the PSL creates, for registration with the Bus Controller
	// Use a local array, since the IIC Controller operates on a copy of the array entries.
	DIicBusChannel* channelPtrArray[KIicPslNumOfChannels];

	TInt r = KErrNone;

#ifdef MASTER_MODE
#ifndef SLAVE_MODE
	// If only MASTER_MODE is declared - Create only DIicBusChannelMasterPsl channels
	__KTRACE_OPT(KIIC, Kern::Printf("\n\nCreating DIicBusChannelMasterPsl only\n"));

	DIicBusChannel* chan = NULL;
	for (TInt i = 0; i < KIicPslNumOfChannels; ++i)
		{
		// The first argument repesents the PSL-assigned channel number
		// The second argument, DIicBusChannel::ESpi, should be replaced with the relevant bus type for the PSL
		chan = DIicBusChannelMasterPsl::New(i, DIicBusChannel::ESpi, DIicBusChannel::EFullDuplex);
		if (!chan)
			{
			return KErrNoMemory;
			}
		channelPtrArray[i] = chan;
		}

#else /*SLAVE_MODE*/
	// Master and Slave functionality is available, so create Master, Slave and MasterSlave Channels
	// Create channel 0 as Master, channel 1 as a Slave, and channel 2 as MasterSlave.
	__KTRACE_OPT(KIIC, Kern::Printf("\n\nCreating Master, Slave and MasterSlave channels\n"));

	DIicBusChannel* chan = NULL;

	// Master channel
	// The first argument repesents the PSL-assigned channel number
	// The second argument, DIicBusChannel::ESpi, should be replaced with the relevant bus type for the PSL
	chan = DIicBusChannelMasterPsl::New(0, DIicBusChannel::ESpi, DIicBusChannel::EFullDuplex);
	if (!chan)
		{
		return KErrNoMemory;
		}
	channelPtrArray[0] = chan;

	// Slave channel
	// The first argument repesents the PSL-assigned channel number
	// The second argument, DIicBusChannel::ESpi, should be replaced with the relevant bus type for the PSL
	chan = DIicBusChannelSlavePsl::New(1, DIicBusChannel::ESpi, DIicBusChannel::EFullDuplex);
	if (!chan)
		{
		return KErrNoMemory;
		}
	channelPtrArray[1] = chan;

	// MasterSlave channel
	// MasterSlave channels are not for derivation; instead, they have a pointer to a (derived) Master channel
	// and a pointer to a (derived) Slave channel
	DIicBusChannel* chanM = NULL;
	DIicBusChannel* chanS = NULL;
	// For MasterSlave channel, the channel number for both the Master and Slave channels must be the same
	TInt msChanNum = 2;
	// Master channel
	// The first argument repesents the PSL-assigned channel number
	// The second argument, DIicBusChannel::ESpi, should be replaced with the relevant bus type for the PSL
	chanM = DIicBusChannelMasterPsl::New(msChanNum, DIicBusChannel::ESpi, DIicBusChannel::EFullDuplex);
	if (!chanM)
		{
		return KErrNoMemory;
		}
	// Slave channel
	// The first argument repesents the PSL-assigned channel number
	// The second argument, DIicBusChannel::ESpi, should be replaced with the relevant bus type for the PSL
	chanS = DIicBusChannelSlavePsl::New(msChanNum, DIicBusChannel::ESpi, DIicBusChannel::EFullDuplex);
	if (!chanS)
		{
		return KErrNoMemory;
		}
	// MasterSlave channel
	// The first argument, DIicBusChannel::ESpi, should be replaced with the relevant bus type for the PSL
	chan = new DIicBusChannelMasterSlave(DIicBusChannel::ESpi, DIicBusChannel::EFullDuplex, (DIicBusChannelMaster*)chanM, (DIicBusChannelSlave*)chanS);
	if (!chan)
		{
		return KErrNoMemory;
		}
	r = ((DIicBusChannelMasterSlave*)chan)->DoCreate();
	channelPtrArray[2] = chan;


#endif /*SLAVE_MODE*/
#else /*MASTER_MODE*/

#ifdef SLAVE_MODE
	// If only SLAVE_MODE is declared - Create all as DIicBusChannelSlavePsl channels
	__KTRACE_OPT(KIIC, Kern::Printf("\n\nCreating DIicBusChannelSlavePsl only\n"));

	DIicBusChannel* chan = NULL;
	for (TInt i = 0; i < KIicPslNumOfChannels; ++i)
		{
		// The first argument repesents the PSL-assigned channel number
		// The second argument, DIicBusChannel::ESpi, should be replaced with the relevant bus type for the PSL
		chan = DIicBusChannelSlavePsl::New(i, DIicBusChannel::ESpi, DIicBusChannel::EFullDuplex);
		if (!chan)
			{
			return KErrNoMemory;
			}
		channelPtrArray[i] = chan;
		}

#endif
#endif /*MASTER_MODE*/

	// Register them with the Bus Controller
	r = DIicBusController::RegisterChannels(channelPtrArray, KIicPslNumOfChannels);

	return r;
	}


