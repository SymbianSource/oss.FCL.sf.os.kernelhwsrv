// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Provides a class to manage the generation of lists
// 
//

#include "d_list_manager.h"
#include "d_process_tracker.h"
#include "debug_utils.h"
#include "plat_priv.h"
#include "debug_logging.h"
#include <arm.h>

// make accessing DThread's MState more intuitive
#define iMState iWaitLink.iSpare1
// make accessing NThread's NState more intuitive
#define iNState iSpare3

//constants to match against a rom entry's attributes,
//these are defined in the file server (can't be included kernel side)
//and in the ROM tools (also inaccessible) so redefined here
const TUint KEntryAttXIP=0x0080;
const TUint KEntryAttDir=0x0010;

using namespace Debug;

/**
  Get thread listing for the specified thread, if the thread data will not fit
  in the buffer then an error is returned.

  @param aBuffer buffer to put data in
  @param aDataSize on return will contain size of data
  @param aTargetThreadId thread ID to return listing for

  @return KErrNone on success,
  KErrTooBig if data won't fit in aBuffer
  or one of the other system wide error codes on failure
  */
TInt TListManager::GetThreadListForThread(TDes8& aBuffer, TUint32& aDataSize, const TUint64 aTargetThreadId) const
	{
	LOG_MSG("TListManager::GetThreadListForThread()");

	// open a handle to check whether the thread actually exists
	DThread* thread = DebugUtils::OpenThreadHandle(aTargetThreadId);
	if(!thread)
		{
		return KErrArgument;
		}
	DProcess* process = thread->iOwningProcess;
	if(!process)
		{
		return KErrArgument;
		}
	TUint64 processId = process->iId;
	thread->Close(NULL);

	//request a process specific list
	return GetThreadListForProcess(aBuffer, aDataSize, processId);
	}

TInt TListManager::GetThreadListForProcess(TDes8& aBuffer, TUint32& aDataSize, const TUint64 aTargetProcessId) const
	{
	LOG_MSG("TListManager::GetThreadListForProcess()");

	// open a handle to check whether the process actually exists
	DProcess* process = DebugUtils::OpenProcessHandle(aTargetProcessId);
	if(!process)
		{
		return KErrArgument;
		}
	process->Close(NULL);

	//request a process specific list
	return GetThreadList(aBuffer, aDataSize, EFalse, aTargetProcessId);
	}

/**
  Get global thread listing

  @param aBuffer buffer to put data in
  @param aDataSize on return will contain size of data

  @return KErrNone on success,
  KErrTooBig if data won't fit in aBuffer
  or one of the other system wide error codes on failure
  */
TInt TListManager::GetGlobalThreadList(TDes8& aBuffer, TUint32& aDataSize) const
	{
	LOG_MSG("TListManager::GetGlobalThreadList()");

	//request a global list
	return GetThreadList(aBuffer, aDataSize, ETrue, 0);
	}

/**
  Get thread listing, if the thread data will not fit
  in the buffer then an error is returned.

  @param aBuffer buffer to put data in
  @param aDataSize on return will contain size of data
  @param aGlobal whether or not the listing should be global or thread specific
  @param aTargetProcessId process ID to return listing for, relevant only if aGlobal == ETrue

  @return KErrNone on success,
  KErrTooBig if data won't fit in aBuffer
  or one of the other system wide error codes on failure
  */
TInt TListManager::GetThreadList(TDes8& aBuffer, TUint32& aDataSize, TBool aGlobal, const TUint64 aTargetProcessId) const
	{

	LOG_MSG("TListManager::GetThreadList\n");

	//have to read the threads in a critical section
	NKern::ThreadEnterCS();

	//get a pointer to the kernel's thread list
	DObjectCon *threads = Kern::Containers()[EThread];

	//if can't get container then exit
	if(threads == NULL)
		{
		NKern::ThreadLeaveCS();

		return KErrGeneral;
		}

	//stop the thread list from changing while we are processing them
	threads->Wait();

	aDataSize = 0;
	aBuffer.SetLength(0);
	//iterate through the threads adding them to the buffer
	for(TInt i=0; i<threads->Count(); i++)
		{
		DThread* thread = (DThread*)(*threads)[i];

		//skip this thread pointer is the thread is NULL
		if(thread)
			{
			NThread& nThread = thread->iNThread;

			// if the thread is marked as being dead then don't return information about it in the listing
#ifndef __SMP__
			if((NThread::EDead != nThread.iNState) && (DThread::EDead != thread->iMState))
#else
 			if((!nThread.IsDead()) && (DThread::EDead != thread->iMState))
#endif
				{
				if( aGlobal || (aTargetProcessId == (TUint64)thread->iOwningProcess->iId))
					{
					//store the data in the buffer
					AppendThreadData(aBuffer, aDataSize, thread);
					}
				}
			}
		}

	//leave critical section
	threads->Signal();
	NKern::ThreadLeaveCS();

	//return indication of whether the kernel's data was too big
	return (aDataSize > aBuffer.Length()) ? KErrTooBig : KErrNone;
	}

/**
  Helper function for writing thread data into a buffer

  @pre call in a critical section
  @pre call only on threads which have NThread state not equal to NThread::EDead

  @param aBuffer buffer to put data in
  @param aDataSize on return will contain size of data
  @param aThread thread object to include information about

  @return KErrNone on success, or one of the other system wide error codes
*/
void TListManager::AppendThreadData(TDes8& aBuffer, TUint32& aDataSize, DThread* aThread) const
	{
	//get aThread's name
	TFileName fileName;
	aThread->FullName(fileName);
	TUint16 nameLength = fileName.Length();

	//increase aDataSize by the size of this entry
	aDataSize = Align4(aDataSize + (2*nameLength) + sizeof(TThreadListEntry) - sizeof(TUint16));
	//if the data would not cause overflow then add it to the buffer
	if(aDataSize <= aBuffer.MaxLength())
		{
		//Create a TThreadListEntry which references the buffer.
		TThreadListEntry& entry = *(TThreadListEntry*)(aBuffer.Ptr()+aBuffer.Length());
		//add data to entry
		entry.iProcessId = (TUint64)aThread->iOwningProcess->iId;
		entry.iThreadId = (TUint64)aThread->iId;
		entry.iSupervisorStackBase = (TUint32)aThread->iSupervisorStack;
		entry.iSupervisorStackBaseValid = ETrue;
		entry.iSupervisorStackSize = aThread->iSupervisorStackSize;
		entry.iSupervisorStackSizeValid = ETrue;
		entry.iNameLength = nameLength;

		//only ask for the supervisor stack pointer if aThread is suspended
		entry.iSupervisorStackPtrValid = EInValid;
		entry.iSupervisorStackPtr = 0;
		if(TheDProcessTracker.CheckSuspended(aThread))
			{
			NThread& nThread = aThread->iNThread;

			TArmRegSet regSet;
			TUint32 flags;
			NKern::ThreadGetSystemContext(&nThread, &regSet, flags);
			entry.iSupervisorStackPtr = (TUint32)regSet.iR13;
			//need to check that the stack pointer flag is valid
			if(flags & (1<<EArmSp))
				{
				entry.iSupervisorStackPtrValid = EValid;
				}
			}

		//copy name data into the buffer
		TUint16* ptr = &(entry.iName[0]);
		const TUint8* ptr8 = fileName.Ptr();
		const TUint8* ptr8End = ptr8 + nameLength;
		while(ptr8 < ptr8End)
			{
			*ptr++ = (TUint16)*ptr8++;
			}
 
		aBuffer.SetLength(aDataSize);
		}
	}

/**
  Get global process listing

  @param aBuffer buffer to put data in
  @param aDataSize on return will contain size of data

  @return KErrNone on success,
  KErrTooBig if data won't fit in aBuffer
  or one of the other system wide error codes on failure
  */
TInt TListManager::GetProcessList(TDes8& aBuffer, TUint32& aDataSize) const
	{
	LOG_MSG("TListManager::GetProcessList()");

	//get a pointer to the kernel's process list
	DObjectCon* processes = Kern::Containers()[EProcess];

	if(processes == NULL)
		{
		//if can't get container then something is seriously wrong
		return KErrNotFound;
		}

	//have to read the processes in a critical section
	NKern::ThreadEnterCS();
	processes->Wait();

	aDataSize = 0;
	//iterate through the processes adding them to the buffer
	for(TInt i=0; i<processes->Count(); i++)
		{
		DProcess* process = (DProcess*)(*processes)[i];
		if(process)
			{
			//get process's file name length
			DCodeSeg* codeSeg = process->iCodeSeg;
			TUint16 fileNameLength = (codeSeg) ? (*codeSeg->iFileName).Length() : 0;

			//get process's dynamic name length and name
			TFullName fullName;
			process->FullName(fullName);
			TUint16 dynamicNameLength = fullName.Length();

			//increase aDataSize to reflect size of entry
			aDataSize = Align4(aDataSize + (2*fileNameLength) + (2*dynamicNameLength) + sizeof(TProcessListEntry) - sizeof(TUint16));
			//if the data would not cause overflow then add it to the buffer
			if(aDataSize <= aBuffer.MaxLength())
				{
				//Create a TProcessListEntry which references the buffer.
				TProcessListEntry& entry = *(TProcessListEntry*)(aBuffer.Ptr() + aBuffer.Length());

				//set values
				entry.iProcessId = (TUint64)process->iId;
				entry.iFileNameLength = fileNameLength;
				entry.iDynamicNameLength = dynamicNameLength;
				entry.iUid3 = process->iUids.iUid[2].iUid;

				if(codeSeg)
					{
					//create TPtr to where the file name should be written
					TPtr name = TPtr((TUint8*)&(entry.iNames[0]), fileNameLength*2, fileNameLength*2);
					//copy the file name
					TInt err = CopyAndExpandDes(*codeSeg->iFileName, name);
					if(err != KErrNone)
						{
						processes->Signal();
						NKern::ThreadLeaveCS();
						return KErrGeneral;
						}
					}

				//create TPtr to where the dynamic name should be written
				TPtr name = TPtr((TUint8*)(&(entry.iNames[0]) + fileNameLength), dynamicNameLength*2, dynamicNameLength*2);
				//copy the dynamic name
				TInt err = CopyAndExpandDes(fullName, name);
				if(err != KErrNone)
					{
					processes->Signal();
					NKern::ThreadLeaveCS();
					return KErrGeneral;
					}

				//set length same as aDataSize
				aBuffer.SetLength(aDataSize);
				}
			}
		}

	//leave critical section
	processes->Signal();
	NKern::ThreadLeaveCS();

	//return indication of whether the kernel's data was too big
	return (aDataSize > aBuffer.Length()) ? KErrTooBig : KErrNone;
	}

/**
  Copy the descriptor aSrc to aDest and converting each byte from aSrc
  into the two-byte equivalent. For example if aSrc contains 'XYZ' then
  aDest will be filled with 'X\0Y\0Z\0' where \0 is the null character.
  The length of aDest is set to twice the length of aSrc.

  @param aSrc source descriptor
  @param aDest destination descriptor to copy and expand aSrc into

  @return KErrNone on success,
  KErrArgument if the max length of aDest is less than twice the length of aSrc
  */
TInt TListManager::CopyAndExpandDes(const TDesC& aSrc, TDes& aDest) const
	{
	//check bounds
	if(aSrc.Length() * 2 > aDest.MaxLength())
		{
		return KErrArgument;
		}

	//get a pointer to the start of the destination descriptor
	TUint16* destPtr = (TUint16*)aDest.Ptr();

	//get pointers to the start and end of the aSrc descriptor
	const TUint8* srcPtr = aSrc.Ptr();
	const TUint8* srcEnd = srcPtr + aSrc.Length();

	//copy the characters from aSrc into aDest, expanding to make them 16-bit characters
	while(srcPtr < srcEnd)
		{
		*destPtr = (TUint16)*srcPtr;
		destPtr++;
		srcPtr++;
		}

	//set aDest's length to reflect the new contents
	aDest.SetLength(2*aSrc.Length());
	return KErrNone;
	}

/**
  Get global code segment listing

  @param aBuffer buffer to put data in
  @param aDataSize on return will contain size of data

  @return KErrNone on success,
  KErrTooBig if data won't fit in aBuffer,
  or one of the other system wide error codes
  */
TInt TListManager::GetGlobalCodeSegList(TDes8& aBuffer, TUint32& aDataSize) const
	{
	LOG_MSG("TListManager::GetGlobalCodeSegList()");

	// Acquire code seg lock mutex
	NKern::ThreadEnterCS();
	DMutex* codeMutex = Kern::CodeSegLock();
	Kern::MutexWait(*codeMutex);

	//get global code seg list
	SDblQue* codeSegList = Kern::CodeSegList();

	//create a memory info object for use in the loop
	TModuleMemoryInfo memoryInfo;

	//iterate through the list
	aDataSize = 0;
	for (SDblQueLink* codeSegPtr= codeSegList->First(); codeSegPtr!=(SDblQueLink*) (codeSegList); codeSegPtr=codeSegPtr->iNext)
		{
		DEpocCodeSeg* codeSeg = (DEpocCodeSeg*)_LOFF(codeSegPtr,DCodeSeg, iLink);
		//the code seg shouldn't be null as we're in critical section, ignore if it is null
		if(codeSeg)
			{
			//get the memory info
			TInt err = codeSeg->GetMemoryInfo(memoryInfo, NULL);
			if(err != KErrNone)
				{
				// Release the codeseglock mutex again
				Kern::MutexSignal(*codeMutex);
				NKern::ThreadLeaveCS();

				//there's been an error so return it
				return err;
				}
			//calculate data values
			TFileName fileName(codeSeg->iFileName->Ptr());
			TBool isXip = (TBool)(codeSeg->iXIP);

			//get the code seg type, can ignore error as have already checked codeSeg is not NULL
			TCodeSegType type = EUnknownCodeSegType;
			err = GetCodeSegType(codeSeg, type);
			if(err != KErrNone)
				{
				LOG_MSG("TListManager::GetGlobalCodeSegList() : code seg is NULL");
				}

			TUint32 uid3 = codeSeg->iUids.iUid[2].iUid;
			//append data to buffer
			err = AppendCodeSegData(aBuffer, aDataSize, memoryInfo, isXip, type, fileName, uid3);
			if(err != KErrNone)
				{
				// Release the codeseglock mutex again
				Kern::MutexSignal(*codeMutex);
				NKern::ThreadLeaveCS();

				return KErrGeneral;
				}
			}
		}

	// Release the codeseglock mutex again
	Kern::MutexSignal(*codeMutex);
	NKern::ThreadLeaveCS();

	return (aDataSize > aBuffer.MaxLength()) ? KErrTooBig : KErrNone;
	}

/**
  Get code segment list for a thread

  @param aBuffer buffer to store data in
  @param aDataSize size of kernel's data
  @param thread ID to get listing for

  @return KErrNone on success,
  KErrTooBig if data won't fit in aBuffer,
  or one of the other system wide error codes
  */
TInt TListManager::GetCodeSegListForThread(TDes8& aBuffer, TUint32& aDataSize, const TUint64 aTargetThreadId) const
	{
	LOG_MSG("TListManager::GetCodeSegListForThread()");

	// open a handle to check whether the thread actually exists
	DThread* thread = DebugUtils::OpenThreadHandle(aTargetThreadId);
	if(!thread)
		{
		return KErrArgument;
		}
	DProcess* process = thread->iOwningProcess;
	if(!process)
		{
		return KErrArgument;
		}
	TUint64 processId = process->iId;
	thread->Close(NULL);

	return GetCodeSegListForProcess(aBuffer, aDataSize, processId);
	}
/**
  Get code segment list for a process

  @param aBuffer buffer to store data in
  @param aDataSize size of kernel's data
  @param process ID to get listing for

  @return KErrNone on success,
  KErrTooBig if data won't fit in aBuffer,
  or one of the other system wide error codes
  */
TInt TListManager::GetCodeSegListForProcess(TDes8& aBuffer, TUint32& aDataSize, const TUint64 aTargetProcessId) const
	{
	LOG_MSG("TListManager::GetCodeSegListForProcess()");

	//get the process
	DProcess* process = DebugUtils::OpenProcessHandle(aTargetProcessId);

	if(!process)
		{
		return KErrArgument;
		}

	//enter thread critical section and acquire code segment mutex
	Kern::AccessCode();

	//memory info object to use in loop
	TModuleMemoryInfo memoryInfo;

	//get code seg list
	SDblQue queue;
	process->TraverseCodeSegs(&queue, NULL, DCodeSeg::EMarkDebug, DProcess::ETraverseFlagAdd);

	//iterate through the list
	aDataSize = 0;
	for(SDblQueLink* codeSegPtr= queue.First(); codeSegPtr!=(SDblQueLink*) (&queue); codeSegPtr=codeSegPtr->iNext)
		{
		//get the code seg
		DEpocCodeSeg* codeSeg = (DEpocCodeSeg*)_LOFF(codeSegPtr,DCodeSeg, iTempLink);

		//the code seg shouldn't be null as we're in critical section, ignore if it is null
		if(codeSeg)
			{
			TInt err = codeSeg->GetMemoryInfo(memoryInfo, NULL);
			if(err != KErrNone)
				{
				process->Close(NULL);
				return err;
				}

			TFileName fileName(codeSeg->iFileName->Ptr());
			TBool isXip = (TBool)(codeSeg->iXIP);

			//get the code seg type, can ignore error as have already checked codeSeg is not NULL
			TCodeSegType type = EUnknownCodeSegType;
			err = GetCodeSegType(codeSeg, type);
			if(err != KErrNone)
				{
				LOG_MSG("TListManager::GetCodeSegListForProcess() : code seg is NULL");
				}

			TUint32 uid3 = codeSeg->iUids.iUid[2].iUid;
			//append data to buffer
			err = AppendCodeSegData(aBuffer, aDataSize, memoryInfo, isXip, type, fileName, uid3);
			if(err != KErrNone)
				{
				process->Close(NULL);
				return KErrGeneral;
				}
			}
		}

	//un mark the code segs that we've iterated over
	DCodeSeg::EmptyQueue(queue, DCodeSeg::EMarkDebug);

	//release mutex and leave CS
	Kern::EndAccessCode();

	process->Close(NULL);
	return (aDataSize > aBuffer.MaxLength()) ? KErrTooBig : KErrNone;
	}

/**
  Appends data to a specified buffer and puts the resulting size in aDataSize.
  If the data won't fit then aDataSize is updated to reflect what the new length
  would be.

  @param aBuffer buffer to append data to
  @param aDataSize will contain buffer size (or the size the buffer would be) on return
  @param aMemoryInfo info to append to buffer
  @param aIsXip boolean indicating whether the code segment is XIP
  @param aFileName file name to append to buffer

  @return KErrNone on success, or one of the other system wide error codes
  */
TInt TListManager::AppendCodeSegData(TDes8& aBuffer, TUint32& aDataSize, const TModuleMemoryInfo& aMemoryInfo, const TBool aIsXip, const TCodeSegType aCodeSegType, const TDesC8& aFileName, const TUint32 aUid3) const
	{
	//get some data elements to put in buffer
	TUint16 fileNameLength = aFileName.Length();

	//calculate the resultant size
	aDataSize = Align4(aDataSize + sizeof(TCodeSegListEntry) + (2*fileNameLength) - sizeof(TUint16));
	if(aDataSize <= aBuffer.MaxLength())
		{
		//Create a TCodeSegListEntry which references the buffer.
		TCodeSegListEntry& entry = *(TCodeSegListEntry*)(aBuffer.Ptr() + aBuffer.Length());
		entry.iCodeBase = aMemoryInfo.iCodeBase;
		entry.iCodeSize = aMemoryInfo.iCodeSize;
		entry.iConstDataSize = aMemoryInfo.iConstDataSize;
		entry.iInitialisedDataBase = aMemoryInfo.iInitialisedDataBase;
		entry.iInitialisedDataSize = aMemoryInfo.iInitialisedDataSize;
		entry.iUninitialisedDataSize = aMemoryInfo.iUninitialisedDataSize;
		entry.iIsXip = aIsXip;
		entry.iCodeSegType = aCodeSegType;
		entry.iNameLength = fileNameLength;
		entry.iUid3 = aUid3;

		//have to convert the stored name to 16 bit unicode
		TPtr name = TPtr((TUint8*)&(entry.iName[0]), fileNameLength*2, fileNameLength*2);
		TInt err = CopyAndExpandDes(aFileName, name);
		if(err != KErrNone)
			{
			return KErrGeneral;
			}

		//increase length
		aBuffer.SetLength(aDataSize);
		}

	return KErrNone;
	}

/**
  Get global XIP libraries list. The ROM file system is searched for files in
  z:\sys\bin. The files are filtered to only include library files which
  correspond to the correct hardware variant.

  In the rom, a directory is represented as a list of TRomEntrys, corresponding to
  the files and directories in that directory. A TRomEntry corresponding to a file
  contains a pointer to that file's location in the rom. If the TRomEntry
  corresponds to a directory then it contains a pointer to that directory in the
  ROM header. As such, from a pointer to the root directory of the z: drive, it is
  possible to extract the directory contents for a particular directory (i.e. z:\sys\bin)
  by recursively finding the subdirectories (i.e. find 'sys' in 'z:', then 'bin' in 'sys')
  and then listing the contents of that directory.

  @param aBuffer buffer to store data in
  @param aDataSize size of kernel's data

  @return KErrNone on success,
  KErrTooBig if data won't fit in aBuffer,
  or one of the other system wide error codes
  */
TInt TListManager::GetXipLibrariesList(TDes8& aBuffer, TUint32& aDataSize) const
	{
	LOG_MSG("TListManager::GetXipLibrariesList()");

	// z:\sys\bin expressed as 16 bit unicode..
	_LIT(KZSysBin, "z\0:\0\\\0s\0y\0s\0\\\0b\0i\0n\0\\\0");

	//array to store pointers to directory entries in
	RPointerArray<TRomEntry> entries;
	//get the entries in KZSysBin
	TInt err = GetDirectoryEntries(entries, KZSysBin());
	if(KErrNone != err)
		{
		entries.Close();
		return err;
		}

	aDataSize = 0;
	for(TInt i=0; i<entries.Count(); i++)
		{
		//if the entry is XIP and it's not a directory then it's a candidate to add
		if( (entries[i]->iAtt & KEntryAttXIP) && ! (entries[i]->iAtt & KEntryAttDir) )
			{
			//get a reference to the dll's header
			const TRomImageHeader& header = *(const TRomImageHeader*)(entries[i]->iAddressLin);

			//check that it's uid1 value corresponds to that for a library
			if(header.iUid1 == KDynamicLibraryUidValue)
				{
				//get the current hardware variant
				TSuperPage& superPage = Kern::SuperPage();
				TUint variant = superPage.iActiveVariant;
				TUint cpu = (variant >> 16) & 0xff;
				TUint asic = (variant >> 24);

				//check this dll is compatible with the current variant
				if(THardwareVariant(header.iHardwareVariant).IsCompatibleWith(cpu,asic,variant))
					{
					const TInt fileNameLength16 = entries[i]->iNameLength;
					const TInt fullNameLength16 = (KZSysBin().Length() / 2) + fileNameLength16;
					aDataSize += Align4((2 * fullNameLength16) + sizeof(TXipLibraryListEntry) - sizeof(TUint16));

					if(aDataSize <= aBuffer.MaxLength())
						{
						//Create a TXipLibraryListEntry which references the buffer.
						TXipLibraryListEntry& libraryInfo = *(TXipLibraryListEntry*)(aBuffer.Ptr() + aBuffer.Length());

						//add the data
						libraryInfo.iCodeBase = header.iCodeAddress;
						libraryInfo.iCodeSize = header.iTextSize;
						libraryInfo.iConstDataSize = header.iCodeSize - header.iTextSize;
						libraryInfo.iInitialisedDataBase = header.iDataBssLinearBase;
						libraryInfo.iInitialisedDataSize = header.iDataSize;
						libraryInfo.iUninitialisedDataSize = header.iBssSize;
						libraryInfo.iNameLength = fullNameLength16;

						//create a TPtr8 to contain the fully qualified name (i.e. z:\sys\bin\ prefixed)
						TPtr8 name((TUint8*)&(libraryInfo.iName[0]), 0, 2 * fullNameLength16);
						name.Append(KZSysBin());
						name.Append(TPtr8((TUint8*)&(entries[i]->iName), 2 * fileNameLength16, 2 * fileNameLength16));

						//increase the buffer's length to reflect the new data size
						aBuffer.SetLength(aDataSize);
						}
					}
				}
			}
		}
	entries.Close();
	return (aDataSize == aBuffer.Length()) ? KErrNone : KErrTooBig;
	}

/**
Get the list of TRomEntry objects in the specified directory aDirectory

@param aRomEntryArray array to store pointers to the TRomEntry objects in
@param aDirectoryName directory to get contents of. The passed in string should be
16 bit unicode and should begin with z:. Single backslashes should be used as delimiters
rather than forward slashes and a terminating backslash is optional.
For example: z:\sys\bin

@return KErrNone on success, or one of the other system wide error codes
*/
TInt TListManager::GetDirectoryEntries(RPointerArray<TRomEntry>& aRomEntryArray, const TDesC& aDirectoryName) const
	{
	LOG_MSG("TListManager::GetDirectoryEntries()");

	//definition in 16 bit unicode
	_LIT(KForwardSlash, "/\0");

	//if directory has forward slashes then exit
	if(aDirectoryName.Find(KForwardSlash()) != KErrNotFound)
		{
		return KErrArgument;
		}

	//create an array to hold the folders in aDirectoryName
	RArray<TPtr8> folders;

	//split the directory up into its folders, i.e. z:\sys\bin is split into { 'z:', 'sys', 'bin' }
	TInt err = SplitDirectoryName(aDirectoryName, folders);
	if(KErrNone != err)
		{
		folders.Close();
		return err;
		}

	if(folders.Count() == 0)
		{
		folders.Close();
		//empty string passed in
		return KErrArgument;
		}

	// z: as 16 bit unicode
	_LIT(KZColon, "z\0:\0");
	if(folders[0].CompareF(KZColon()) != 0)
		{
		//first argument must be z: otherwise not in rom
		folders.Close();
		return KErrArgument;
		}
	//remove z: from array
	folders.Remove(0);
	for(TInt i=0; i<folders.Count(); i++)
		{
		if(folders[i].Length() == 0)
			{
			// there were two backslashes in a row
			folders.Close();
			return KErrArgument;
			}
		}

	//get a pointer to the start of the rom root directory list
	TLinAddr romRootDirectoryList = Epoc::RomHeader().iRomRootDirectoryList;

	//the first 4 bytes of the rom root directory list is a count of how many sections (rom roots) there are
	TUint32 rootDirectoryCount = (TUint32)*(TLinAddr*)romRootDirectoryList;

	//rootDirectoryPointer will be shifted through the rom root directory list and will contain pointers to the sections in the rom
	TLinAddr rootDirectoryPointer = romRootDirectoryList;
	for(TInt i=0; i<rootDirectoryCount; i++)
		{
		//the address of the section is stored in the second four bytes of the 8 byte pair reserved for each section
		rootDirectoryPointer += 8;

		//romRoot contains the address of the root of the section
		TLinAddr romRoot = *(TLinAddr*)rootDirectoryPointer;

		//append the directory entries from romRoot's z:\sys\bin subdirectory
		TInt err = GetDirectoryEntries(aRomEntryArray, folders, romRoot);
		if(KErrNone != err)
			{
			folders.Close();
			return err;
			}
		}
	folders.Close();
	return KErrNone;
	}

/**
  Recursively finds the subdirectories in aArray and stores references to the
  entries in the most derived subdirectory in aRomEntryArray

  @param aRomEntryArray on return will contain the entries in the directory corresponding to aArray
  @param aArray an array containing the directory to get the entries for, i.e. { 'sys', 'bin' }
  @param aAddress address in rom to being searching from

  @param KErrNone on success, or one of the other system wide error codes
*/
TInt TListManager::GetDirectoryEntries(RPointerArray<TRomEntry>& aRomEntryArray, RArray<TPtr8>& aArray, TLinAddr& aAddress) const
	{
	LOG_MSG2("TListManager::GetDirectoryEntries() aAddress: 0x%08x", aAddress);

	//find the next subdirectory and store its address in aAddress, return error if we can't find it
	TInt err = FindDirectory(aArray[0], aAddress);
	if(err != KErrNone)
		{
		return err;
		}

	//if this is the most derived sub-directory (i.e. the bin of z:\sys\bin) then get the dir contents
	if(aArray.Count() == 1)
		{
		return GetDirectoryContents(aRomEntryArray, aAddress);
		}
	else
		{
		//get the next subdirectory's contents
		aArray.Remove(0);
		return GetDirectoryEntries(aRomEntryArray, aArray, aAddress);
		}
	}

/**
Return the entries of a directory in the rom

@param aRomEntryArray array to store the entries in
@param aAddress address of a directory block in the rom
*/
TInt TListManager::GetDirectoryContents(RPointerArray<TRomEntry>& aRomEntryArray, const TLinAddr aAddress) const
	{
	LOG_MSG("TListManager::GetDirectoryContents()");

	TLinAddr address = aAddress;

	//get the size in bytes of the block of rom to iterate over
	const TUint32 sizeInBytes = *(TUint32*)aAddress;

	//get address of first TRomEntry
	const TLinAddr initialAddress = aAddress + sizeof(TUint32);

	//get pointer to subdir count
	address = initialAddress + sizeInBytes;

	//the upper two bytes of this entry contain the number of files in this directory, and the lower two bytes
	//contains the number of subdirectories in this directory
	TUint32 filesAndDirectories = *(TUint32*)address;

	//get number of subdirectories in this directory
	const TUint16 subDirCount = filesAndDirectories & 0xFFFF;

	//get the number of files in this dir
	const TUint16 filesCount = filesAndDirectories >> 16;

	//get total number of entries in dir
	const TUint numDirectoryEntries = subDirCount + filesCount;

	//set address to start of first entry
	address = initialAddress;

	for(TInt i=0; i<numDirectoryEntries; i++)
		{
		TRomEntry* romEntry = (TRomEntry*)address;

		//store the entry
		TInt err = aRomEntryArray.Append(romEntry);
		if(KErrNone != err)
			{
			return err;
			}

		//length of the name of the rom entry
		TInt nameLength = romEntry->iNameLength;

		//get the size of the entry including the name
		TUint32 romEntrySize = sizeof(TRomEntry) - sizeof(romEntry->iName) + (2 * nameLength);
		//adjust the address to the next entry
		address += Align4(romEntrySize);
		}
	return KErrNone;
	}

/**
  Finds the subdirectory with name aDirectory in the directory at aAddress

  @param aDirectory name of subdirectory to search for (i.e. 'bin')
  @param aAddress address in rom of containing directory (i.e. address of 'sys' directory)

  @param KErrNone if aDirectory could be found in aAddress, KErrNotFound if it could not be found
  */
TInt TListManager::FindDirectory(const TDesC& aDirectory, TLinAddr& aAddress) const
	{
	LOG_MSG3("TListManager::FindDirectory() aDirectory: %S, aAddress: 0x%08x", &aDirectory, aAddress);

	//get the directory's contents
	RPointerArray<TRomEntry> dirContents;
	TInt err = GetDirectoryContents(dirContents, aAddress);
	if(KErrNone != err)
		{
		dirContents.Close();
		return err;
		}
	for(TInt i=0; i<dirContents.Count(); i++)
		{
		//create a reference to the TRomEntry in the rom to access its attributes
		TRomEntry& romEntry = *(dirContents[i]);
		if(romEntry.iAtt & KEntryAttDir)
			{
			// this entry's a directory so check if it matches aDirectory
			const TInt nameLength = romEntry.iNameLength;
			TPtr8 name((TUint8*)&(romEntry.iName), nameLength * 2, nameLength * 2);
			if(0 == aDirectory.CompareF(name))
				{
				// names matched so get the address of this directory's contents
				aAddress = romEntry.iAddressLin;
				dirContents.Close();
				return KErrNone;
				}
			}
		}
	dirContents.Close();
	//couldn't find it so return error
	return KErrNotFound;
	}

/**
  Helper function to get code seg type.

  @param aCodeSeg code seg to get type of
  @param aType will contain type on return

  @return KErrNone on success, KErrNotFound if aCodeSeg is NULL
  */
TInt TListManager::GetCodeSegType(const DCodeSeg* aCodeSeg, TCodeSegType& aType) const
	{
	if(!aCodeSeg)
		{
		return KErrNotFound;
		}

	if(aCodeSeg->IsExe())
		{
		aType = EExeCodeSegType;
		return KErrNone;
		}

	if(aCodeSeg->IsDll())
		{
		aType = EDllCodeSegType;
		return KErrNone;
		}

	aType = EUnknownCodeSegType;
	return KErrNone;
	}


/**
  Split a directory name into its subdirectories, using a 16-bit backslash ('\\\0') as a delimiter.
  For example z:\sys\bin would be split into { 'z:', 'sys', 'bin' }

  @param aDirectoryName directory name to split into subdirectories
  @param aSubDirectories array to store the subdirectories in
  */
TInt TListManager::SplitDirectoryName(const TDesC& aDirectoryName, RArray<TPtr8>& aSubDirectories) const
	{
	//definition in 16 bit unicode
	_LIT(KBackSlash, "\\\0");

	//split the directory up into its folders, i.e. z:\sys\bin is split into 
	TPtr8 string((TUint8*)aDirectoryName.Ptr(), aDirectoryName.Length(), aDirectoryName.Length());
	while(string.Ptr() < aDirectoryName.Ptr() + aDirectoryName.Length())
		{
		TInt offset = string.Find(KBackSlash());
		if(offset == KErrNotFound)
			{
			//reached the end of the string
			offset = string.Length();
			}
		//adjustedOffset takes account of the end of the string case
		TInt adjustedOffset = (offset == string.Length()) ? offset : offset + KBackSlash().Length();
		//add sub-folder name
		TInt err = aSubDirectories.Append(TPtr8((TUint8*)string.Ptr(), offset, offset));
		if(KErrNone != err)
			{
			return err;
			}
		//remove the sub-folder name and continue
		string.Set((TUint8*)string.Ptr() + adjustedOffset, string.Length() - adjustedOffset, string.Length() - adjustedOffset);
		}
	return KErrNone;
	}



