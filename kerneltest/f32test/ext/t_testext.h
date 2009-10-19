// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32test\ext\t_testext.h
// 
//

#ifndef __TEST_EXT_H__
#define __TEST_EXT_H__

#include <f32fsys.h>

//#define TESTEXT_DEBUG

#if (defined(_DEBUG) || defined(_DEBUG_RELEASE)) && defined(TESTEXT_DEBUG)
#define __PRINT(t) {RDebug::Print(t);}
#define __PRINT1(t,a) {RDebug::Print(t,a);}
#define __PRINT2(t,a,b) {RDebug::Print(t,a,b);}
#else
#define __PRINT(t)
#define __PRINT1(t,a)
#define __PRINT2(t,a,b)
#endif

#define EExtCustom (KMaxTInt/2)


/**
This is the test proxy drive extension base class.
Test proxy drives of specific file system should derive from this base class.
@internalTechnology
*/
class CTestProxyDrive : public CBaseExtProxyDrive
	{
public:
    enum TTestCmd 
        {
        ESetEvent  = EExtCustom,
        EMark,
        EUnmark,
        EUnmarkAll,
        EIsMarked,
        EDiskSize,
        ETestCmdEnd = EExtCustom + 100
        };
    enum TBadSectorEvent
        {
        ENone, ENext, EDeterministic, ERandom
        };

protected:
	CTestProxyDrive(CProxyDrive* aProxyDrive, CMountCB* aMount);
    virtual ~CTestProxyDrive();

public:
    // Derived functions from CBaseExtProxyDrive
    // Commented out because they are not changed. Sub-classes reimplement them
    //  if interested.
//	virtual TInt Initialise();
//	virtual TInt Dismounted();
//	virtual TInt Enlarge(TInt aLength);
//	virtual TInt ReduceSize(TInt aPos, TInt aLength);
//	virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt aOffset,TInt aFlags);
//	virtual TInt Read(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt aThreadHandle,TInt anOffset);
//	virtual TInt Read(TInt64 aPos,TInt aLength,TDes8& aTrg);
//	virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt aOffset,TInt aFlags);
//	virtual TInt Write(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt aThreadHandle,TInt anOffset);
//	virtual TInt Write(TInt64 aPos,const TDesC8& aSrc);
//	virtual TInt Caps(TDes8& anInfo);
//	virtual TInt Format(TFormatInfo& anInfo);
//	virtual TInt Format(TInt64 aPos,TInt aLength);
//	virtual TInt SetMountInfo(const TDesC8* aMountInfo,TInt aMountInfoThreadHandle=KCurrentThreadHandle);
//	virtual TInt ForceRemount(TUint aFlags=0);
//	virtual TInt Unlock(TMediaPassword &aPassword, TBool aStorePassword);
//	virtual TInt Lock(TMediaPassword &aOldPassword, TMediaPassword &aNewPassword, TBool aStorePassword);
//	virtual TInt Clear(TMediaPassword &aPassword);
//	virtual TInt ErasePassword();
	virtual TInt ControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2);
	virtual TInt GetLastErrorInfo(TDes8& aErrorInfo);

protected:
	/**
	Return a pointer to a specified interface extension - to allow future extension of this class without breaking
	binary compatibility.
	@param aInterfaceId Interface identifier of the interface to be retrieved.
	@param aInterface A reference to a pointer that retrieves the specified interface.
	@param aInput An arbitrary input argument.
	@return KErrNone If the interface is supported, KErrNotSupported otherwise.
	*/	
//	virtual TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);

protected:
    void InitL();
	TInt SetEvent(TInt aType, TInt aValue);
	TInt Mark(TInt aSector);
	TInt Unmark(TInt aSector);
	TInt UnmarkAll();
    TBool IsMarked(TInt aSector) const;
    TBool CheckEvent(TInt64 aPos, TInt aLength);
    virtual void DoInitL() = 0;
    virtual TBool DoCheckEvent(TInt64 aPos, TInt aLength) = 0;
    virtual TInt DoControlIO(const RMessagePtr2& aMessage,TInt aCommand,TAny* aParam1,TAny* aParam2) = 0;

protected:
    CMountCB* iMount;  // in CProxyDrive
    TUint8* iMap;
    TInt iTotalSectors;
    TBool iMountInitialised;
    TInt iEventType;
    TInt iCount;
    TInt iSuccessBytes;
    TErrorInfo::TReasonCode iLastErrorReason;
	};

#endif
