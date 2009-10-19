/**
* Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* //File Name:	f32test\plugins\version_2\file64bit\inc\file64bit_plugin.h
* //Description: Header file for file64bit_plugin.cpp
* 
*
*/




#if !defined(__FILE64BIT_PLUGIN_H__)
#define __FILE64BIT_PLUGIN_H__

#include <f32plugin.h>

#define _LOG(a) {if(iLogging) RDebug::Print(a);}
#define _LOG2(a,b) {if(iLogging) RDebug::Print(a,b);}
#define _LOG3(a,b,c) {if(iLogging) RDebug::Print(a,b,c);}
#define _LOG4(a,b,c,d) {if(iLogging) RDebug::Print(a,b,c,d);}
#define _LOG5(a,b,c,d,e) {if(iLogging) RDebug::Print(a,b,c,d,e);}

const TInt KFile64BitPluginPos = 0x1FFFFFFF;

_LIT(KFile64BitPluginFileName,"file64bit_plugin");
_LIT(KFile64BitPluginName,"File64BitPlugin");

const TInt KFile64BitPluginIpcSlot1 = 1;


const TInt KEnableIntercept		= -101;
const TInt KDisableIntercept	= -102;
const TInt KClearData			= -103;

const TInt KGetError			= -201;
const TInt KGetPosition			= -202;
const TInt KGetLength			= -203;
const TInt KGetNewPosition		= -204;
const TInt KGetSize				= -205;
const TInt KGetFsRequestId		= -206;
const TInt64 KGB 					= 1 << 30;
const TInt64 K2GB 					= 2 * KGB;
const TInt64 K3GB 					= 3 * KGB;
const TInt64 K4GB 					= 4 * KGB;

const TInt64 K4GBMinusOne 					= K4GB - 1;


enum TFsRequestId
	{
	EFsrIdFileRead,
	EFsrIdFileWrite,
	EFsrIdFileLock,
	EFsrIdFileUnLock,
	EFsrIdFileSeek,
	EFsrIdFileSize,
	EFsrIdFileSetSize,
   	EFsrIdReadFileSection,
   	EFsrIdNotSupported
	};

class TInterceptedData 
	{
public:
	void ClearAll()
		{
		iFsRequestId 	= (TFsRequestId)0;
		iPosition		= 0;
		iLength			= 0;
		iNewPosition	= 0;
		iSize			= 0;
		iLastErrorCode	= 0;
		};
	
	TFsRequestId	iFsRequestId;
	TInt64 			iPosition;		// Read, Write, Lock, UnLock, Seek, ReadFileSection
	TInt64 			iLength;		// Lock  and UnLock
	TInt64 			iNewPosition;	// Seek
	TInt64 			iSize;			// Sise, SetSize
	TInt64 			iLastErrorCode;	// Latest error code
	};

class CFile64BitPlugin : public CFsPlugin
{

public:
	static CFile64BitPlugin* NewL();
	~CFile64BitPlugin();

	virtual void InitialiseL();
	virtual TInt DoRequestL(TFsPluginRequest& aRequest);

	TInt FsPluginDoControlL(CFsPluginConnRequest& aRequest);

protected:
	CFsPluginConn* NewPluginConnL();

private:
	CFile64BitPlugin();
	void ConstructL();

	void EnableInterceptsL();
	void DisableInterceptsL();

private:
	RFs iFs;
	TBool iInterceptsEnabled;
	TBool iLogging;
};

class CFile64BitPluginConn : public CFsPluginConn
	{
	virtual TInt DoControl(CFsPluginConnRequest& aRequest);
	virtual void DoRequest(CFsPluginConnRequest& aRequest);
	virtual void DoCancel(TInt aReqMask);
	};

#endif
