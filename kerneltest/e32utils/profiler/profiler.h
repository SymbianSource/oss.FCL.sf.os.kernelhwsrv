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
// e32utils\profiler\profiler.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __PROFILER__
#define __PROFILER__

#include <e32base.h>


_LIT(KProfilerName,"Profiler");

/**
The Profiler class provides a functional interface to the sampling profiler. <p>
The engine must already be running for this interface to work, this can be
achieved by executing PROFILER.EXE. The control methods are all static, and 
require no other context.

@internalTechnology
*/
class Profiler : private RSessionBase
	{
public:
	enum TState {EStart, EStop, EClose, EUnload};
public:
	/** Start the sampler */
	static inline TInt Start();
	/** Stop the sampler */
	static inline TInt Stop();
	/** Release the sample trace file */
	static inline TInt Close();
	/** Unload the profile engine from memory */
	static inline TInt Unload();
//
	/** Issue a control request to the engine */
	static inline TInt Control(TState aRequest);
private:
	inline Profiler();
	};

inline Profiler::Profiler()
	{}

inline TInt Profiler::Control(TState aRequest)
//
// Connect to the profiler engine, and issue the control request if successful
//
	{
	Profiler p;
	TInt r = p.CreateSession(KProfilerName, TVersion(), 0);
	if (r == KErrNone)
		{
		p.SendReceive(aRequest);
		p.RSessionBase::Close();
		}
	return r;
	}

inline TInt Profiler::Start()
	{return Control(EStart);}

inline TInt Profiler::Stop()
	{return Control(EStop);}

inline TInt Profiler::Close()
	{return Control(EClose);}

inline TInt Profiler::Unload()
	{return Control(EUnload);}



/*
 * This is an internal interface to the profiling engine which allows
 * an additional control DLL to be loaded, replacing the profiler's
 * default console UI
 */

/**
Implementation class providing access to the profiler engine

@internalTechnology
*/
class MProfilerEngine
	{
public:
	virtual TInt Control(Profiler::TState aCommand) =0;
	virtual Profiler::TState State() const =0;
	};

/**
The interface that the extra controller must implement to access the profiler.

@internalTechnology
*/
class MProfilerController
	{
public:
	/** Release the controller from the profiler. This is invoked when the profiler is unloading. */
	virtual void Release() =0;
	/** Ask the profiler to change state */
	inline TInt Control(Profiler::TState aCommand) const;
	/* Query the profiler state */
	inline Profiler::TState GetState() const;
protected:
	inline MProfilerController(MProfilerEngine& aEngine);
private:
	MProfilerEngine& iEngine;
	};

/** The signature of ordinal 1 in the controller DLL */
typedef MProfilerController* (*TProfilerControllerFactoryL)(TInt aPriority, MProfilerEngine& aEngine);

/** The second UID required by the controller DLL */
const TUid KUidProfilerKeys={0x1000945c};


inline MProfilerController::MProfilerController(MProfilerEngine& aEngine)
	:iEngine(aEngine)
	{}
inline TInt MProfilerController::Control(Profiler::TState aCommand) const
	{return iEngine.Control(aCommand);}
inline Profiler::TState MProfilerController::GetState() const
	{return iEngine.State();}

#endif
