// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// @file
// 
// 
//


class TF32TestTimer
{
public:
    TF32TestTimer();

    void Start();
    void Stop();

    TInt64 TimeTaken() const;
    TTimeIntervalMicroSeconds Time() const;
    TTimeIntervalMicroSeconds32 Time32() const;
    int TimeInMicroSeconds() const;
    int TimeInMilliSeconds() const;

    static TInt TimeInMilliSeconds(TTimeIntervalMicroSeconds aTime);
    static TInt TimeInMilliSeconds(TTimeIntervalMicroSeconds32 aTime);

protected:
    TInt64 Diff(TUint32 aTicks) const;

protected:
    TInt iFastCounterFreq;
    TUint32 startTicks;
    TUint32 endTicks;
};


class TF32TestTimer2: public TF32TestTimer
{
public:
    void Stop2();

    TInt64 TimeTaken2() const;
    TTimeIntervalMicroSeconds32 Time2() const;
    int Time2InMicroSeconds() const;

private:
    TUint32 endTicks2;
};

inline TF32TestTimer::TF32TestTimer()
{   
    TInt r = HAL::Get(HAL::EFastCounterFrequency, iFastCounterFreq);
    test_KErrNone(r);
}

inline void TF32TestTimer::Start()
{
    startTicks = User::FastCounter();
}

inline void TF32TestTimer::Stop()
{
    endTicks = User::FastCounter();;
}

inline void TF32TestTimer2::Stop2()
{
    endTicks2 = User::FastCounter();;
}

inline TInt64 TF32TestTimer::Diff(TUint32 aTicks) const
{
    if (aTicks == startTicks)
        {
        test.Printf(_L("Warning: tick not advanced"));
        }

    TInt64 diff;
    if (aTicks > startTicks)
        {
        diff = static_cast<TInt64>(aTicks) - static_cast<TInt64>(startTicks);
        }
    else
        {
        // handle counter rollover
        diff = ((static_cast<TInt64>(KMaxTUint32) + 1 + aTicks) - static_cast<TInt64>(startTicks));
        }
    //RDebug::Printf("%x %x %ld", aTicks, startTicks, diff);
    diff *= TInt64(1000000);
    diff /= TInt64(iFastCounterFreq);
    
    return diff;
}

inline TInt64 TF32TestTimer::TimeTaken() const
{
    return Diff(endTicks);
}

inline TInt64 TF32TestTimer2::TimeTaken2() const
{
    return Diff(endTicks2);
}

inline int TF32TestTimer::TimeInMicroSeconds() const
{
    return static_cast <int>(TimeTaken());
}

inline int TF32TestTimer2::Time2InMicroSeconds() const
{
    return static_cast <int>(TimeTaken2());
}

inline TTimeIntervalMicroSeconds TF32TestTimer::Time() const
{
    return TimeTaken();
}

inline TTimeIntervalMicroSeconds32 TF32TestTimer2::Time2() const
{    
    return static_cast <int>(TimeTaken2());
}

inline TTimeIntervalMicroSeconds32 TF32TestTimer::Time32() const
{
    return static_cast <int>(TimeTaken());
}

inline int TF32TestTimer::TimeInMilliSeconds() const
{
    return static_cast <int>(TimeTaken() / 1000);
}

TInt TF32TestTimer::TimeInMilliSeconds(TTimeIntervalMicroSeconds aTime)
{
    return static_cast <int>(aTime.Int64()/static_cast<TInt64>(1000));
}

TInt TF32TestTimer::TimeInMilliSeconds(TTimeIntervalMicroSeconds32 aTime)
{
    return static_cast <int>(aTime.Int()/1000);
}

