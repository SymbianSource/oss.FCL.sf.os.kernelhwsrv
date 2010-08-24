#ifndef __TEST_DEBUG_H
#define __TEST_DEBUG_H

/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* @file testdebug.h
* @internalComponent
* 
*
*/



#include <e32debug.h>
#include <d32usbdescriptors.h>
#include <d32usbdi.h>
#include <f32file.h>

//#define LOG_INFO(x) RDebug::Print x;

#define LOG_INFO(x) 


/**
Debug function to print out (log) the data for the descriptor
@param aDescriptor the host-side view of the descriptor
*/
inline void PrintDescriptorBlob(TUsbGenericDescriptor& aDescriptor);

/**
*/
class TFunctionLog
	{
public:
	/**
	Constructor
	*/
	TFunctionLog(const char* aFunctionName,void* aThisPointer)
	:	iFunctionName(aFunctionName),
		iThisPointer(aThisPointer)
		{
		RDebug::Printf("\nIN  [%08x]    %s",iThisPointer,iFunctionName);
		}

	/**
	Destructor
	*/
	~TFunctionLog()
		{
		RDebug::Printf("OUT [%08x]    %s\n",iThisPointer,iFunctionName);
		}

private:
	const char* iFunctionName;
	void* iThisPointer;
	};

/**
This class describes a logger for test case actions
Pattern: Singleton
*/
class RLog
	{
public:
	/**
	*/
	static TInt Print(const TDesC8& aLogAction)
		{
		return Instance().iLogFile.Write(aLogAction);
		}
	
private:
	/**
	Get the singleton instance
	*/
	static RLog& Instance()
		{
		static RLog singleton;
		return singleton;
		}
	
	/**
	Constructor
	*/
	RLog()
		{
		TInt err(iFileServer.Connect());
		if(err != KErrNone)
			{
			User::Panic(_L("RTEST 84"),err);
			}
		err = iLogFile.Replace(iFileServer,_L("usbdi_testlog.txt"),EFileWrite);
		if(err != KErrNone)
			{
			
			}
		else
			{
			
			}
		}
	
	/**
	Destructor
	*/
	~RLog()
		{
		iLogFile.Close();
		iFileServer.Close();
		}
	
private:
	/**
	The session with the file server
	*/
	RFs iFileServer;

	/**
	*/
	RFile iLogFile;
	
	};
	
	
// Implementation
void PrintDescriptorBlob(TUsbGenericDescriptor& aDescriptor)
	{
	for(int i=0; i<aDescriptor.iBlob.Length(); i++)
		{
		RDebug::Printf("<Type %d><Blob 0x%02x>",aDescriptor.ibDescriptorType,aDescriptor.iBlob[i]);
		}
	}


#endif

