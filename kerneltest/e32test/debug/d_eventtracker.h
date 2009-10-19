// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/debug/d_eventtracker.h
// 
//

#ifndef __D_EVENTTRACKER_H__
#define __D_EVENTTRACKER_H__

#include "platform.h"


/** Base class for representing objects being tracked.  */

class TTrackedItem
	{
public:
	TTrackedItem(const DBase* aObject);
	
public:
	SDblQueLink iLink;
	const DBase* const iObject; 		// key
	TBool iAccountedFor;
	};

/** Subclass for representing DObjects being tracked.  */

class TTrackedObject : public TTrackedItem
	{
public:
	TTrackedObject(DObject* aObject, TObjectType aType);
	TBool CheckIntegrity(const TDesC& aName, TObjectType aType) const;

public:
	TFullName iFullName;
	const TObjectType iType;
	};

/** Subclass for representing DCodeSegs being tracked.  */

class TTrackedCodeSeg : public TTrackedItem
	{
public:
	TTrackedCodeSeg(const DCodeSeg* aCodeSeg);
	TBool CheckIntegrity(TInt aAccessCount) const;

public:
	TInt iAccessCount;
	};


/** Event handler and container for all objects being tracked.  */

class DEventTracker : public DKernelEventHandler
	{
public:
	DEventTracker();
	TInt Create(DLogicalDevice* aDevice, TBool aUseHook);
	~DEventTracker();
	TInt Start();
	TInt Stop();

private:
	static TUint EventHandler(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aThis);
	TUint HandleEvent(TKernelEvent aType, TAny* a1, TAny* a2);
	TInt AddExistingObjects();
	TInt AddObjectsFromContainer(TObjectType aType);
	TInt AddCodeSegsFromList();
	TInt CheckIntegrity();
	void CheckContainerIntegrity(TObjectType aType);
	void CheckCodeSegListIntegrity();
	void CheckAllAccountedFor();

	TTrackedItem* LookupItem(DBase* aItem) const;

	void AddObject(TObjectType aType, DObject* aObject);
	void RemoveObject(TObjectType aType, DObject* aObject);
	void UpdateObject(TObjectType aType, DObject* aObject, TBool aMustBeRenamed);
	void AddCodeSeg(DCodeSeg* aCodeSeg, DProcess* aProcess);
	void RemoveCodeSeg(DCodeSeg* aCodeSeg, DProcess* aProcess);
	void ProcessLoaded(DProcess* aProcess);

	void StopTracking();
	void DumpCounters() const;

	static void BranchToEventHandler();
	static TUint BreakPointSize();

	// Dummy handler to be copied into RAM
	static TUint DummyHandler(TKernelEvent aEvent, TAny* a1, TAny* a2);

	// Size of dummy handler
	static TUint DummyHandlerSize();

	// Copy the default handler
	static void CopyDummyHandler(TLinAddr aLinAddr);

private:
	/** Lock serialising calls to event handler */
	DMutex* iLock;
	TBool iTracking;
	/** Tracking list (of TTrackedItem).
		Must be accessed only when tracking is disabled or with iLock held.
		Object addresses are used as keys and so must be unique.
	 */
	SDblQue iItems;
	TInt iOOM;
	TInt iErrorCount;
	TInt iCounters[EEventLimit];
	DLogicalDevice* iDevice;	// open reference to LDD for avoiding lifetime issues
	};

GLREF_D DEventTracker* TheEventTracker;

#endif // __D_EVENTTRACKER_H__
