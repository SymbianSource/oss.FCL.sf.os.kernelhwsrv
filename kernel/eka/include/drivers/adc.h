// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\adc.h
// ADC controller header file
// Currently only used in Series 5mx port
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __M32ADC_H__
#define __M32ADC_H__
#include <platform.h>

const TInt KNumAdcChannelPriorities=4;

// ADC operation is specified by a list of commands
// Each command is specified by a 32 bit word:
// Bit 16=1 -> wait for n ms before proceeding to next command (n=bits 0-15)
// Bit 17=1 -> do preamble
// Bit 18=1 -> do postamble
// Bit 19=1 -> take a reading
// Bit 20=1 -> don't store reading

enum TAdcCommand
	{
	EAdcCmdWait=0x10000,
	EAdcCmdPreamble=0x20000,
	EAdcCmdPostamble=0x40000,
	EAdcCmdReading=0x80000,
	EAdcCmdDiscard=0x100000,
	EAdcCmdDummyReading=0x180000,
	};

class DAdc;
class TAdcChannel : public TPriListLink
	{
public:
	IMPORT_C TAdcChannel(TInt anAdc);
public:
	IMPORT_C void Read(TInt* aReadingBuffer);
public:
	IMPORT_C virtual void Preamble();
	IMPORT_C virtual void Postamble();
	virtual void Complete()=0;
public:
	DAdc* iAdc;
	TInt iChannelId;
	TInt iCommandCount;
	const TInt* iCommandList;
	TInt* iReadings;
	};

NONSHARABLE_CLASS(DAdc) : public DBase
	{
public:
	DAdc();
	~DAdc();
public:
	virtual void StartConversion(TInt aChannelId)=0;
public:
	void Add(TAdcChannel* aChannel);
	void Execute(TAdcChannel* aChannel);
	void NextCommand();
	void Start();
	void ConversionComplete(TInt aValue);
	void TimerExpired();
	TInt DoSetMinPriority(TInt aPriority);
	IMPORT_C static TInt SetMinPriority(TInt anAdc, TInt aPriority);
public:
	TPriList<TAdcChannel,KNumAdcChannelPriorities> iList;
	TAdcChannel* iCurrentChannel;
	TInt iCurrentCommand;
	const TInt* iCommandPtr;
	TInt iCommandCount;
	NTimer iTimer;
	TInt iMinPriority;
	static DAdc** TheAdcs;
	static TInt NumberOfAdcs;
	};

#endif
