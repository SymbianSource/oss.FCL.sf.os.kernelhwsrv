// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\euser\us_rwlock.cpp
// 
//


#include "us_std.h"
#include <e32atomics.h>

const TInt KReadersIndex				= 0;
const TInt KWriterIndex					= 1;
const TInt KReadersPendingIndex			= 2;
const TInt KWritersPendingIndex			= 3;
const TUint64 KReaderValue				= UI64LIT(0x0000000000000001);
const TUint64 KWriterValue				= UI64LIT(0x0000000000010000);
const TUint64 KReaderPendingValue		= UI64LIT(0x0000000100000000);
const TUint64 KWriterPendingValue		= UI64LIT(0x0001000000000000);
const TUint64 KReadersMask				= UI64LIT(0x000000000000ffff);
const TUint64 KWriterMask				= KWriterValue;
const TUint64 KReadersOrWritersMask		= KReadersMask | KWriterMask;
const TUint64 KReadersPendingClearMask	= UI64LIT(0xffff0000ffffffff);

/**
Initialise a read-write lock object.
@param		aPriority		Type of priority to use - see RReadWriteLockPriority::TReadWriteLockPriority
@return		KErrNone		Instance successfully created
			Otherwise an error returned by RSemaphore::CreateLocal
@panic		EReadWriteLockInvalidPriority if aPriority is not valid.
*/
EXPORT_C TInt RReadWriteLock::CreateLocal(TReadWriteLockPriority aPriority)
	{
	__ASSERT_ALWAYS(aPriority >= EWriterPriority && aPriority <= EReaderPriority, Panic(EReadWriteLockInvalidPriority));

	iPriority = aPriority;
	iValues = 0;
#ifdef _DEBUG
		iSpare[0] = 0; // Keep a rough track of writer starvation
#endif

	TInt ret = iReaderSem.CreateLocal(0, EOwnerProcess);
	if (ret == KErrNone)
		ret = iWriterSem.CreateLocal(0, EOwnerProcess);
	if (ret != KErrNone)
		iReaderSem.Close();

	return ret;
	}

/**
Close a read-write lock object, releasing the associated semaphores.
@panic		EReadWriteLockStillPending if there are any outstanding clients or pending clients
*/
EXPORT_C void RReadWriteLock::Close()
	{
	__ASSERT_ALWAYS(iValues == 0, Panic(EReadWriteLockStillPending));

	iReaderSem.Close();
	iWriterSem.Close();
	}

/**
Ask for a read lock. Will be granted if:
	1) No-one else currently holds the lock or
	2) Only readers hold the lock and:
		a) There are no pending writers or
		b) The priority is for readers.
Otherwise this function blocks until the lock becomes available to it.
Please note that ReadLock() is not re-entrant - calling it a second time without releasing the first lock
runs the risk of being blocked and risking a deadlock situation.
@panic		EReadWriteLockTooManyClients if the resulting number of readers or pending readers exceeds EReadWriteLockClientCategoryLimit
*/
EXPORT_C void RReadWriteLock::ReadLock()
	{
	TBool blocked;
	TUint64 initialValues;
	TUint16* indexedValues = (TUint16*)&initialValues;

	do	{
		initialValues = iValues;

		if (indexedValues[KWriterIndex] > 0 ||
			(iPriority != EReaderPriority && indexedValues[KWritersPendingIndex] > 0))
			{
			__ASSERT_ALWAYS(indexedValues[KReadersPendingIndex] < KMaxTUint16, Panic(EReadWriteLockTooManyClients));
			blocked = ETrue;
			}
		else
			{
			__ASSERT_ALWAYS(indexedValues[KReadersIndex] < KMaxTUint16, Panic(EReadWriteLockTooManyClients));
			blocked = EFalse;
			}
		}
	while (!__e32_atomic_cas_rel64(&iValues, &initialValues, initialValues + (blocked ? KReaderPendingValue : KReaderValue)));

	if (blocked)
		iReaderSem.Wait();
	}

/**
Ask for a write lock. Will be granted if no-one else currently holds the lock.
Otherwise this function blocks until the lock becomes available to it.
Only one writer can hold the lock at one time. No readers can hold the lock while a writer has it.
Please note that WriteLock() is not re-entrant - calling it a second time without releasing the first lock
will block and cause a deadlock situation.
@panic		EReadWriteLockTooManyClients if the resulting number of pending writers exceeds EReadWriteLockClientCategoryLimit
*/
EXPORT_C void RReadWriteLock::WriteLock()
	{
	TBool blocked;
	TUint64 initialValues;
	TUint16* indexedValues = (TUint16*)&initialValues;

	do	{
		initialValues = iValues;

		if (initialValues & KReadersOrWritersMask)
			{
			__ASSERT_ALWAYS(indexedValues[KWritersPendingIndex] < KMaxTUint16, Panic(EReadWriteLockTooManyClients));
			blocked = ETrue;
			}
		else
			{
			blocked = EFalse;
			}
		}
	while (!__e32_atomic_cas_rel64(&iValues, &initialValues, initialValues + (blocked ? KWriterPendingValue : KWriterValue)));

	if (blocked)
		iWriterSem.Wait();
	}

/**
Ask for a read lock without blocking.
@return		ETrue - lock granted
			EFalse - failed to obtain the lock
@panic		EReadWriteLockTooManyClients if the resulting number of readers exceeds EReadWriteLockClientCategoryLimit
@see		ReadLock()
*/
EXPORT_C TBool RReadWriteLock::TryReadLock()
	{
	TUint64 initialValues;
	TUint16* indexedValues = (TUint16*)&initialValues;

	do	{
		initialValues = iValues;

		if (indexedValues[KWriterIndex] > 0 ||
			(iPriority != EReaderPriority && indexedValues[KWritersPendingIndex] > 0))
			return EFalse;

		__ASSERT_ALWAYS(indexedValues[KReadersIndex] < KMaxTUint16, Panic(EReadWriteLockTooManyClients));
		}
	while (!__e32_atomic_cas_rel64(&iValues, &initialValues, initialValues + KReaderValue));

	return ETrue;
	}

/**
Ask for a write lock without blocking.
@return		ETrue - lock granted
			EFalse - failed to obtain the lock
@see		WriteLock()
*/
EXPORT_C TBool RReadWriteLock::TryWriteLock()
	{
	TUint64 initialValues;

	do	{
		initialValues = iValues;

		if (initialValues & KReadersOrWritersMask)
			return EFalse;
		}
	while (!__e32_atomic_cas_rel64(&iValues, &initialValues, initialValues + KWriterValue));

	return ETrue;
	}

/**
Tries to atomically release a read lock and gain a write lock.
This function will succeed if:
	- This is the only reader and
		- There are no pending writers or
		- The priority is reader
@return		ETrue - write lock granted
			EFalse - failed to obtain a write lock, read lock retained
@panic		EReadWriteLockBadLockState if the read lock is not currently held
*/
EXPORT_C TBool RReadWriteLock::TryUpgradeReadLock()
	{
	__ASSERT_ALWAYS((iValues & KReadersMask) != 0, Panic(EReadWriteLockBadLockState)); // Check we actually hold a read lock
	__ASSERT_DEBUG((iValues & KWriterMask) == 0, Panic(EReadWriteLockBadLockState)); // Check we don't hold a write lock - shouldn't be possible

	TUint64 initialValues;
	TUint16* indexedValues = (TUint16*)&initialValues;

	do	{
		initialValues = iValues;

		if (indexedValues[KReadersIndex] > 1 ||
			(iPriority != EReaderPriority && indexedValues[KWritersPendingIndex] > 0))
              return EFalse;
		}
	while (!__e32_atomic_cas_acq64(&iValues, &initialValues, initialValues - KReaderValue + KWriterValue));

	return ETrue;
	}

/**
Atomically releases a held write lock and gains a read lock. Also unblocks any
pending readers if:
	- Priority is EPriorityReader or
	- There are no pending writers
This function can not fail, so it does not return anything.
@panic		EReadWriteLockBadLockState if the lock is not currently held
*/
EXPORT_C void RReadWriteLock::DowngradeWriteLock()
	{
	__ASSERT_ALWAYS((iValues & KWriterMask) == KWriterValue, Panic(EReadWriteLockBadLockState)); // Check we actually hold a write lock
	__ASSERT_DEBUG((iValues & KReadersMask) == 0, Panic(EReadWriteLockBadLockState)); // Check we don't hold a read lock - shouldn't be possible

	TUint unlockReaders;
	TUint64 initialValues;
	TUint16* indexedValues = (TUint16*)&initialValues;
	TUint64 newValues;

	do	{
		unlockReaders = 0;
		initialValues = iValues;
		newValues = initialValues - KWriterValue + KReaderValue; // Clear current write lock flag and add a read lock

		if (indexedValues[KReadersPendingIndex] > 0 &&
			(indexedValues[KWritersPendingIndex] == 0 || iPriority == EReaderPriority)) // Release any other pending readers
			{
			unlockReaders = indexedValues[KReadersPendingIndex];
			newValues &= KReadersPendingClearMask; // Clear pending readers

			if (unlockReaders == KMaxTUint16) // Put a pending reader back to avoid overflow in the readers field
				{
				unlockReaders--;
				newValues += KReaderPendingValue;
				}

			newValues += unlockReaders;
			}
		}
	while (!__e32_atomic_cas_acq64(&iValues, &initialValues, newValues));

	if (unlockReaders > 0)
		iReaderSem.Signal(unlockReaders);
	}

/**
Releases a held read or write lock. If no-one else holds this lock (ie other
readers) then this will unblock one or more pending clients based on the priority:
	EAlternatePriority	- If a read lock is being released then:
							- Give the lock to the first pending writer, if there is one
							- Else give the lock to all pending readers, if there are any
						- If a write lock is being released then:
							- If there are pending readers:
								- If there are pending writers then unblock one pending reader
								- Else if there are no pending writers then unblock all pending readers
							- Else unblock one pending writer, if there is one
	EReaderPriority		- Unblock all pending readers. If none then unblock one pending writer, if there is one
	EWriterPriority		- Unblock one pending writer, if there is one. If none then unblock any and all pending readers
@panic		EReadWriteLockBadLockState if the lock is not currently held
*/
EXPORT_C void RReadWriteLock::Unlock()
	{
	__ASSERT_ALWAYS((iValues & KReadersOrWritersMask) != 0, Panic(EReadWriteLockBadLockState)); // Check we actually hold a lock
	__ASSERT_DEBUG((iValues & KReadersOrWritersMask) <= KWriterValue, Panic(EReadWriteLockBadLockState)); // Check we don't hold a read lock and a write lock at the same time - shouldn't be possible

	TInt unlockClients = 0;

	switch (iPriority)
		{
	case EWriterPriority:
		unlockClients = UnlockWriter(); break;
	case EAlternatePriority:
		unlockClients = UnlockAlternate(); break;
	default: // EReaderPriority:
		unlockClients = UnlockReader(); break;
		};

	if (unlockClients == -1)
		{
#ifdef _DEBUG
		iSpare[0] = 0; // Keep a rough track of writer starvation
#endif
		iWriterSem.Signal();
		}
	else if (unlockClients > 0)
		{
#ifdef _DEBUG
		const TUint64 KWritersPendingMask = UI64LIT(0xffff000000000000);
		if (iValues & KWritersPendingMask)
			iSpare[0]++; // Keep a rough track of writer starvation
		if (iSpare[0] > 1000)
			Panic(EReadWriteLockWriterStarvation);
#endif
		iReaderSem.Signal(unlockClients);
		}
	}

TInt RReadWriteLock::UnlockWriter()
	{
	TUint64 initialValues;
	TUint16* indexedValues = (TUint16*)&initialValues;
	TUint64 newValues;
	TInt unlockClients;

	do	{
		unlockClients = 0;
		initialValues = iValues;
		newValues = initialValues - (indexedValues[KReadersIndex] > 0 ? KReaderValue : KWriterValue); // Clear current lock flag

		if ((newValues & KReadersOrWritersMask) == 0) // No longer locked - release someone else
			{
			if (indexedValues[KWritersPendingIndex] > 0) // Release a writer
				{
				unlockClients = -1;
				newValues -= KWriterPendingValue;
				newValues += KWriterValue;
				}
			else if (indexedValues[KReadersPendingIndex] > 0) // Release all pending readers
				{
				unlockClients = indexedValues[KReadersPendingIndex];
				newValues &= KReadersPendingClearMask; // Clear pending readers
				newValues += unlockClients;
				}
			}
		}
	while (!__e32_atomic_cas_acq64(&iValues, &initialValues, newValues));

	return unlockClients;
	}

TInt RReadWriteLock::UnlockAlternate()
	{
	TUint64 initialValues;
	TUint16* indexedValues = (TUint16*)&initialValues;
	TUint64 newValues;
	TInt unlockClients;

	do	{
		unlockClients = 0;
		initialValues = iValues;
		newValues = initialValues - (indexedValues[KReadersIndex] > 0 ? KReaderValue : KWriterValue); // Clear current lock flag

		if ((newValues & KReadersOrWritersMask) == 0) // No longer locked - release someone else
			{
			if (indexedValues[KWritersPendingIndex] > 0 &&
				(indexedValues[KReadersIndex] > 0 || indexedValues[KReadersPendingIndex] == 0)) // Release a writer if there is one and either this is a read unlock or there are no readers pending
				{
				unlockClients = -1;
				newValues -= KWriterPendingValue;
				newValues += KWriterValue;
				}
			else if (indexedValues[KReadersPendingIndex] > 0) // Release one or more readers
				{
				if (indexedValues[KWritersPendingIndex] > 0) // Just one because there are pending writers
					{
					unlockClients = 1;
					newValues -= KReaderPendingValue;
					newValues += KReaderValue;
					}
				else // All of them
					{
					unlockClients = indexedValues[KReadersPendingIndex];
					newValues &= KReadersPendingClearMask; // Clear pending readers
					newValues += unlockClients;
					}
				}

			}
		}
	while (!__e32_atomic_cas_acq64(&iValues, &initialValues, newValues));

	return unlockClients;
	}

TInt RReadWriteLock::UnlockReader()
	{
	TUint64 initialValues;
	TUint16* indexedValues = (TUint16*)&initialValues;
	TUint64 newValues;
	TInt unlockClients;

	do	{
		unlockClients = 0;
		initialValues = iValues;
		newValues = initialValues - (indexedValues[KReadersIndex] > 0 ? KReaderValue : KWriterValue); // Clear current lock flag

		if ((newValues & KReadersOrWritersMask) == 0) // No longer locked - release someone else
			{
			if (indexedValues[KReadersPendingIndex] > 0) // Release all pending readers
				{
				unlockClients = indexedValues[KReadersPendingIndex];
				newValues &= KReadersPendingClearMask; // Clear pending readers
				newValues += unlockClients;
				}
			else if (indexedValues[KWritersPendingIndex] > 0) // Release a writer
				{
				unlockClients = -1;
				newValues -= KWriterPendingValue;
				newValues += KWriterValue;
				}
			}
		}
	while (!__e32_atomic_cas_acq64(&iValues, &initialValues, newValues));

	return unlockClients;
	}

