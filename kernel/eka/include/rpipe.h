// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//





#ifndef __RPIPE_H__
#define __RPIPE_H__

#include <e32cmn.h>
#include <e32ver.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

#if defined(DATAPAGING_TEST_ON)
#define DATAPAGING_TEST(s) s
#else
#define DATAPAGING_TEST(s)
#endif


class RPipe: public RBusLogicalChannel
/**
RPipe class object represent the user side handle.  It is derived from RBusLogicalChannel 
which is the base class for handle to the kernel side device driver instance. It encapsulates
both named and unnamed pipe methods. 
@internalTechnology
*/
    {
public:

    /**
    Structure for holding driver capabilities information
    (Just a version number.)
    */
    class TCaps
        {
    public:
        TVersion iVersion;
        };


	/**
	Enumeration for the modes of opening a pipe.
	*/
	enum TMode
	{
		EOpenToRead,
		EOpenToWrite,
		EOpenToWriteNamedPipeButFailOnNoReaders	
	};
	
	enum TChannelType
	{
		EChannelUnset = 0x0,
		EReadChannel = 0x1,
		EWriteChannel = 0x2
	};
	
	class TPipeInfo
        {
    public:
        TInt isize;
        TBuf8<KMaxKernelName> iName;
     
        };
    typedef TPckgBuf<TPipeInfo> TPipeInfoBuf;


public:

	/**
	 Returns the version information of the  DPipe Factory object
	 */
    inline static TVersion VersionRequired();

	/**
	 Returns the name of the DPipe factory object
	 */
	inline static const TDesC& Name();

  
	/**
	 Support for un-named pipe
	 */
	 IMPORT_C static TInt Create( TInt aSize, RPipe& aReader, RPipe& aWriter, TOwnerType aTypeR = EOwnerProcess,  TOwnerType aTypeW = EOwnerProcess);
	
	 TInt Create(const TInt sSize,  TOwnerType aType = EOwnerProcess);

	 TInt Open(const RPipe& aWriter, TOwnerType  aType = EOwnerProcess);


	/**
	Support for named pipe
	*/
	
	IMPORT_C static TInt Define( const TDesC& aName, TInt aSize);
	
	IMPORT_C static TInt Define( const  TDesC& aName, TInt aSize, const TSecurityPolicy& aPolicy);
								

	IMPORT_C static TInt Destroy(const TDesC& aName);

	IMPORT_C TInt Open(const TDesC& aName, TMode aMode);

	IMPORT_C TInt Open(TInt aArgumentIndex, TOwnerType aType=EOwnerProcess);
	
	IMPORT_C TInt Open(RMessagePtr2 aMessage, TInt aParam, TOwnerType aType=EOwnerProcess);
	
	IMPORT_C static TInt Init();
  	
	/**
	 Non-blocking read/write operations
	 */
	IMPORT_C TInt Read ( TDes8& aData, TInt aSize);

	IMPORT_C TInt Write( const TDesC8& aBuf, TInt aSize);

	/** 
	Blocking read/write operations
	*/
	IMPORT_C TInt ReadBlocking(TDes8& aData, TInt aSize);

	IMPORT_C TInt WriteBlocking (const TDesC8& aBuf, TInt aSize);

	IMPORT_C TInt Size();

	IMPORT_C void NotifySpaceAvailable( TInt aSize, TRequestStatus&);

    IMPORT_C void NotifyDataAvailable( TRequestStatus&); 
	
	IMPORT_C void Wait(const TDesC& aName, TRequestStatus& aStatus);
	
	IMPORT_C TInt MaxSize();


	IMPORT_C TInt CancelSpaceAvailable();

	IMPORT_C TInt CancelDataAvailable();

	IMPORT_C void CancelWait();

	IMPORT_C void Flush();

	IMPORT_C void Close();    
	
	IMPORT_C TInt HandleType()const;

	TInt PipeHandle()const;
	
	IMPORT_C void WaitForReader(const TDesC& aName, TRequestStatus& aStatus);
	
	IMPORT_C void WaitForWriter(const TDesC& aName, TRequestStatus& aStatus);
	    
    /*
     Enumeration of Request messages.
    */
    enum TRequest
        {
		EDefineNamedPipe,
		EOpenToReadNamedPipe,
		EOpenToWriteNamedPipe,
		EOpenToWriteButFailOnNoReaderNamedPipe,
		EDestroyNamedPipe,
		ECreateUnNamedPipe,
		EOpenUnNamedPipe,
		ERead,
        	EWrite,
	    	ESize,
	        EReadBlocking,
		EWriteBlocking,  	
		EDataAvailable,
		EDataAvailableCount,
		ESpaceAvailable,
		EWaitNotification,
		EWaitNotificationCheck,
		ECancelSpaceAvailable,
		ECancelDataAvailable,
		ECancelWaitNotification,
		EFlushPipe,
		EClosePipe,
		EGetPipeInfo
	   };
	/*
     Enumeration of Wait Request.
    */   
	enum TWaitRequest
		{
		EWaitForReader,
		EWaitForWriter
		};
		
	private:
	
	void Wait(const TDesC& aName, TRequestStatus& aStatus, TInt aChoice);
	
	void ReqComplete(TRequestStatus& aState, TInt aval);
	TInt iHandleType;
	TInt iSize;
	TInt iPipeHandle;
};



inline TVersion RPipe::VersionRequired()
{
	// Just a number 
    const TInt KMajorVersionNumber=1;
    const TInt KMinorVersionNumber=0;
    const TInt KBuildVersionNumber=1;
    return         
    TVersion (KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber);
}

inline const TDesC& RPipe::Name()
{
	_LIT(KRpipeDevice, "SymbianPipe");
	return KRpipeDevice;
}


#endif // __RPIPE_H__
