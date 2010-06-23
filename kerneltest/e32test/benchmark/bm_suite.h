// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#if !defined(__BM_SUITE_H__)
#define __BM_SUITE_H__

#include <e32test.h>

#include "d32bm.h"

/**
 * Gets access to the benchmark suite global log window/file.
 */
extern RTest test;
/**
 * Tests for an error condition.
 *
 * If the error condition is detected the macro prints out the file name, the line number and 
 * the error code, and aborts the benchmark suite.  
 *
 * @param aError an error number; printed out in the case if the error condition is detected.
 * @param aCond non-error condition: if 1 - no errors; if 0 - error. 
 */
#define BM_ERROR(aError, aCond) \
	__ASSERT_ALWAYS(aCond, bm_error_detected((TInt) aError, "'"#aCond"'", __FILE__, __LINE__))
void bm_error_detected(TInt aError, char* aCond, char* aFile, TInt aLine);

/**
 * Verify a condition
 *
 * If the value of condition is 0 prints out the file name, the line number and 
 * aborts the benchmark suite.
 *
 * @param aCond the condition that shell be verified.
 */

#define BM_ASSERT(aCond) \
	__ASSERT_DEBUG(aCond, bm_assert_failed("'"#aCond"'", __FILE__, __LINE__))
void bm_assert_failed(char* aFile, char* aCond, TInt aLine);

/**
 * An object of <code>TBMResult</code> class collects results of a measurement of a single perfrormance characteristic.
 * 
 * Objects of <code>TBMResult</code> class are typically reside in benchmark programs' static data
 * and returned as programs' results.
 */
class TBMResult
	{
public:
	/**
	 * Constructs a non-initialized <code>TBMResult</code> object. 
	 * Such a non-intialised object must be reset using void <code>TBMResult::Reset(const TDesC&)</code> function 
	 * prior to be actually used.
	 */
	TBMResult()	{}
	/**
	 * Constructs a <code>TBMResult</code> object. 
	 *
	 * @param aName the measurement tite.
	 */
	TBMResult(const TDesC& aName); 
	/**
	 * Resets an existing <code>TBMResult</code> object. 
	 * Sets the object in exactly the same state as <code>TBMResult::TBMResult(const TDesC&)</code>.
	 *
	 * @param aName the measurement tite.
	 */ 
	void Reset(const TDesC& aName);
	/**
	 * Stores the result of a new iteration.
	 *
	 * @param aTicks the iteration's elapsed time in ticks.
	 */
	void Cumulate(TBMTicks aTicks);
	/**
	 * Stores the cumulated result of a number of iterations.
	 *
	 * @param aTicks the cumulated elapsed time
	 * @param aIter the number of iterations.
	 */
	void Cumulate(TBMTicks aTicks, TBMUInt64 aIter);
	/**
	 * Calculate <code>TBMResult::iMin, TBMResult::iMax, TBMResult::iAverage</code> in nano-seconds
	 */
	void Update();
	/**
	 * The title of the performance measurement
	 */
	TPtrC	iName;
	/**
	 * The number of iteration has been performed
	 */
	TBMUInt64	iIterations;
	/**
	 * The minimal elapsed time in nano-seconds.
	 */
	TBMNs	iMin;
	/**
	 * The maximal elapsed time in nano-seconds
	 */ 
	TBMNs	iMax;
	/**
	 * The average elapsed time in nano-seconds.
	 */
	TBMNs	iAverage;

	enum
		{ 	
		/**
		 * The size of the buffer for the results of leading iterations
		 */
		KHeadSize = 4
		};
	enum
		{
		/**
		 * The size of the buffer for the results of tailing iterations
		 */
		KTailSize = 4 
		};
	/**
	 * The buffer with the results of <code>KHeadSize</code> leading iterations.
	 */
	TBMNs	iHead[KHeadSize];
	/**
	 * The buffer with the results of <code>KTailSize</code> trailing iterations.
	 */
	TBMNs	iTail[KTailSize];

// private:

	void Reset();
	
	TBMTicks	iMinTicks;				// the minimal elapsed time in ticks
	TBMTicks	iMaxTicks;				// the maximal elapsed time in ticks
	TBMTicks	iCumulatedTicks;		// the elapsed time in ticks cumulated over iCumulatedIterations
	TBMUInt64	iCumulatedIterations;	// the number of iterations for iCumulatedTicks

	TBMTicks	iHeadTicks[KHeadSize];	// the first KHeadSize results in ticks
	TBMTicks	iTailTicks[KTailSize];	// the last KTailSize results in ticks
	};

/**
 * An object of <code>TBMTimeInterval</code> can be used to measure potentially very long time interval.
 * 
 * If the measured time interval is shorter than the period of <code>RBMTimer</code> 
 * this high-precision timer will be used; otherwise, <code>TTime</code> timer will be used.
 */
class TBMTimeInterval
	{
public:
	/**
	 * A static function to initialize the class static state. 
	 * Called once by the benchmark suite launcher.
	 */
	static void Init();
	/**
	 * Begins the time interval measurement.
	 */
	void Begin();
	/**
	 * Ends the time interval measurement.
	 *
	 * @return the elapsed time in nano-seconds
	 */
	TBMNs EndNs();
	/**
	 * Ends the time interval measurement.
	 *
	 * @return the elapsed time in <code>RBMTimer</code> ticks. 
	 *		Note that the time is always returned in <code>RBMTimer</code> ticks regardless which of two timers,
	 *		<code>TTime</code> or <code>RBMTimer</code>, was actually used.
	 */
	TBMTicks End();

	/**
	 * Period of RBMTimer in nano-seconds
	 */
	static TBMNs	iStampPeriodNs;
	/**
	 * Period of RBMTimer in ticks
	 */
	static TBMTicks	iStampPeriod;

private:

	TBMTicks	iStamp;				// the starting time in RBMTimer ticks
	TTime		iTime;				// the starting TTime
	};


/**
 * Calculates elapsed time in ticks taking care about possible timer overflow.
 *
 * @param aT0 the beginning of the measured interval
 * @param aT1 the end of the measured interval.
 * 
 */
inline TBMTicks TBMTicksDelta(TBMTicks aT0, TBMTicks aT1)
	{
	return (aT0 <= aT1) ? (aT1 - aT0) : TBMTimeInterval::iStampPeriod - (aT0 - aT1);
	}


/**
 * Absolute thread prioiries to be used by benchmark programs
 */
enum 
	{
	/**
	 * Absolute priority to be used for high-priority threads.
	 * 
	 */
	KBMPriorityHigh = 60, // 60 is above all DFC 26 is below timer DFC
	/**
	 * Absolute priority to be used for middle-priority threads.
	 * This is also the default priority - performance benchmarks are started at this prioirty.
	 */	
	KBMPriorityMid = KBMPriorityHigh - 1,
	/**
	 * Absolute priority to be used for low-priority threads.
	 */
	KBMPriorityLow = KBMPriorityMid - 1
	};


/**
 * An object of <code>TBMSpawnArgs</code> type is used to pass arguments through 
 * <code>BMProgram::SpawnChild()</code> function from a parent to a child thread.
 *
 * <code>TBMSpawnArgs</code> is typically used as the base class for the actual argument passing object 
 * which may define some additional benchmark program specific arguments. 
 * The command line descriptor is used to pass a copy of the whole <code>TBMSpawnArgs</code> object 
 * from the parent process to the child one.
 */
struct TBMSpawnArgs
	{
	/**
	 * A magic value to allow the child recognize a command line as a <code>TBMSpawnArgs</code> object.
	 * Intialized by constructor.
	 */
	TInt			iMagic;
	/**
	 * A handle to the parent thread. 
	 * Intialized by constructor.
	 */
	RThread			iParent;
	/**
	 * The id of the parent thread.
	 * Intialized by constructor.
	 */
	TThreadId		iParentId;
	/**
	 * If <code>ETrue</code> the child thread runs in a separate process; 
	 * otherwise, the child thread runs within the parent's process.
	 * Intialized by constructor.
	 */
	TBool			iRemote;
	/**
	 * The child entry point.
	 * Intialized by constructor.
	 */
	TThreadFunction iChildFunc;
	/**
	 * The child thread absolute priority.
	 * Intialized by constructor.
	 */
	TInt			iChildPrio;

	TInt			iChildOrigPriority;
	/**
	 * The actual size of the <code>TBMSpawnArgs</code> object.
	 * Intialized by constructor.
	 */
	TInt			iSize;
	/**
	 * The TBMSpawnArgs magic value.
	 */
	static const TInt KMagic;
	/**
	 * Construct a new <code>TBMSpawnArgs</code> object.
	 *
	 * @param aChildFunc the child entry point
	 * @param aChildPrio the child thread absolute priority
	 * @param aRemote if <code>ETrue</code> the child thread must be created in a separate process; 
	 *		otherwise, the child thread must be created within the parent's process.
	 */
	TBMSpawnArgs(TThreadFunction aChildFunc, TInt aChildPrio, TBool aRemote, TInt aSize);
	/**
	 * Releases all allocated resources e.g. (<code>iParent</code> handle)
	 */
	~TBMSpawnArgs();
	};

/**
 * Child thread interface. Returned by <code>BMProgram::SpawnChild()</code>
 */
class MBMChild
	{
public:
	/**
	 * Wait for the actual child thread termination.
	 */
	virtual void WaitChildExit() = 0;
	/**
	 * Requests the child thread termination.
	 */
	virtual void Kill() = 0;
	};

/**
 * A benchmark program is implemented as a sub-class of the abstract <code>BMProgram</code> class.
 * 
 * A typical benchmark program implements <code>BMProgram::Run()</code> pure virtual function and 
 * instantiate an object of the implemented sub-class in its static data.
 */
class BMProgram 
	{
private:

	BMProgram*	iNext;
	TPtrC		iName;

	MBMChild* SpawnLocalChild(TBMSpawnArgs*);
	MBMChild* SpawnRemoteChild(TBMSpawnArgs*);
	
public:

	/**
	 * Utility function to change a thread's absolute priority.
	 * 
	 * @param aThread a handle ro the target thread.
	 * @param aNewPrio a new absolute priority for the target thread
	 * @return the old absolute priority of the target thread 
	 */
	static TInt SetAbsPriority(RThread aThread, TInt aNewPrio);	
	
	/**
	 * Constructs a new <code>BMProgram</code> object.
	 *
	 * @param aName the bechmark program's title
	 */
	BMProgram(const TDesC& aName) : iName(aName)
		{}

	/**
	 * Gets the nest program in the banchmark suite
	 *
	 * @return a pointer to the <code>BMProgram</code> object corresponding to the next benchmark program
	 *		in the benchmark suite
	 */
	BMProgram*& Next()
		{
		return iNext;
		}
	/**
	 * Gets the benchmark program title
	 * 
	 * @return a refernce to the descriptor containing the benchmark program title.
	 */
	const TDesC& Name()
		{
		return iName;
		}
	/**
	 * Runs the benchmark program.
	 *
	 * @param aIter the required number of iterations
	 * @retval aCount the number of performance characteristics measured by the benchmark program.
	 * @return a pointer to an array of <code>TBMResult</code> objects with the results of measurements of
	 *		individual performance characteristics. The number of array's elements is returned as 
	 *		aCount argument.
	 */
	virtual TBMResult* Run(TBMUInt64 aIter, TInt* aCount) = 0;

	/**
	 * Spawn a child thread
	 * 
	 * @param args a pointer to a <code>TBMSpawnArgs</code> object that contains genric spawn operation parameters
	 *		as well as program specific arguments to be passed to the chile thread.
	 */
	MBMChild* SpawnChild(TBMSpawnArgs* args);

	/**
	 * The main benchmark thread's absolute priority as was assigned by the loader. 
	 */
	TInt	iOrigAbsPriority;
	};

/**
 * The benchmark suite wide handle to the high-precision <code>RBMTimer</code>.
 */
extern RBMTimer bmTimer;
extern BMProgram* bmSuite;

void AddrtLatency();
void AddOverhead();
void AddSync();
void AddIpc();
void AddThread();
void AddProperty();


#endif
