// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// @file basetestcase.cpp
// @internalComponent
// 
//

#include "BaseTestCase.h"
#include <e32ver.h>
#include <d32usbdi.h>
#include "testdebug.h"
#include "testpolicy.h"
#include "OstTraceDefinitions.h"
#ifdef OST_TRACE_COMPILER_IN_USE
#include "BaseTestCaseTraces.h"
#endif

namespace NUnitTesting_USBDI
	{
	
	
const TUint8 KEpDirectionIn = 0x80;
const TUint8 KEpDirectionOut = 0x00;
const TUint8 KTransferTypeControl = 0x00;
const TUint8 KTransferTypeIsoch = 0x01;
const TUint8 KTransferTypeBulk = 0x02;
const TUint8 KTransferTypeInterrupt = 0x03;	

const TUint8 KChunkSize  		= 0x80 ; // 128 bytes
const TUint KTreeBufferSize 	= 32*1024 ; // 32k bytes

_LIT(KRefPath, "Z:\\scripts\\");
 _LIT(KGeneratedFilesPath,"C:\\");
_LIT(KExtensionFile,".txt"); 


CBaseTestCase::CBaseTestCase(const TDesC& aTestCaseId,TBool aHostFlag, TBool aHostOnly)
:	CActive(EPriorityStandard),
	iHost(aHostFlag),
	iHostOnly(aHostOnly)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_CBASETESTCASE_ENTRY, this );
	iTestCaseId.Copy(aTestCaseId);
	CActiveScheduler::Add(this);
	OstTraceFunctionExit1( CBASETESTCASE_CBASETESTCASE_EXIT, this );
	}
	
void CBaseTestCase::BaseConstructL()
	{
	OstTraceFunctionEntry1( CBASETESTCASE_BASECONSTRUCTL_ENTRY, this );
	OstTrace0(TRACE_NORMAL, CBASETESTCASE_BASECONSTRUCTL, "Creating test case timer");
	TInt err(iTimer.CreateLocal());
	if(err == KErrNone)
		{
		OstTrace0(TRACE_NORMAL, CBASETESTCASE_BASECONSTRUCTL_DUP01, "Test case timer created");
		}
	else
		{
		OstTrace1(TRACE_NORMAL, CBASETESTCASE_BASECONSTRUCTL_DUP02, "<Error %d> Test case timer could not be created",err);
		User::Leave(err);
		}	
	OstTraceFunctionExit1( CBASETESTCASE_BASECONSTRUCTL_EXIT, this );
	}

void CBaseTestCase::TimeoutIn(TInt aTimeoutPeriod)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_TIMEOUTIN_ENTRY, this );
	
	CancelTimeout();
	iTimer.After(iStatus,aTimeoutPeriod*1000000);
	SetActive();
	OstTraceFunctionExit1( CBASETESTCASE_TIMEOUTIN_EXIT, this );
	}


void CBaseTestCase::CancelTimeout()
	{
	OstTraceFunctionEntry1( CBASETESTCASE_CANCELTIMEOUT_ENTRY, this );
	iTimer.Cancel();
	OstTraceFunctionExit1( CBASETESTCASE_CANCELTIMEOUT_EXIT, this );
	}

TInt CBaseTestCase::GenerateRefFile(const TDesC& aFileName)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_GENERATEREFFILE_ENTRY, this );
	
	TBuf<256> refTreeFullFileName(KGeneratedFilesPath);
	refTreeFullFileName.Append(aFileName);	
	refTreeFullFileName.Append(KExtensionFile);
	
 	RFile refFile; 
	TInt ret = KErrNone; 
	ret = iFs.Connect();
	if(ret!=KErrNone && ret!=KErrAlreadyExists)
	// if already connected, ignore
		{ 
		OstTrace1(TRACE_NORMAL, CBASETESTCASE_GENERATEREFFILE, "iFs.Connect fails, ret = %d", ret);
		OstTraceFunctionExitExt( CBASETESTCASE_GENERATEREFFILE_EXIT, this, ret );
		return ret;
		}
			
	ret = iFs.Delete(refTreeFullFileName);
	if(ret == KErrNone || ret == KErrNotFound)
		{
		ret = refFile.Create(iFs,refTreeFullFileName,EFileShareAny|EFileWrite);
		}		
	
	if(ret!=KErrNone) 
		{ 
		OstTrace1(TRACE_NORMAL, CBASETESTCASE_GENERATEREFFILE_DUP01, "refFile.Create fails, ret = %d", ret);
		OstTraceFunctionExitExt( CBASETESTCASE_GENERATEREFFILE_EXIT_DUP01, this, ret );
		return ret;
		}
		 
	refFile.Write(iTreeBuffer);
	refFile.Flush(); 
	refFile.Close();
			 
	OstTraceFunctionExitExt( CBASETESTCASE_GENERATEREFFILE_EXIT_DUP02, this, KErrNone );
	return KErrNone;
	}	

TInt CBaseTestCase::CompareCurrentTreeToRef(const TDesC& aFileName, TBool& aIsIdentical)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_COMPARECURRENTTREETOREF_ENTRY, this );
	
	TBuf<256> refTreeFullFileName(KRefPath);
	refTreeFullFileName.Append(aFileName);
	refTreeFullFileName.Append(KExtensionFile);	 	

	TInt ret = KErrNone; 
	ret = iFs.Connect();
	if(ret!=KErrNone && ret!=KErrAlreadyExists)
	// if already connected, ignore
		{ 
		OstTrace1(TRACE_NORMAL, CBASETESTCASE_COMPARECURRENTTREETOREF, "iFs.Connect fails, ret = %d", ret);
		OstTraceFunctionExitExt( CBASETESTCASE_COMPARECURRENTTREETOREF_EXIT, this, ret );
		return ret;
		}

	RFile refFile;
	ret = refFile.Open(iFs,refTreeFullFileName,EFileShareAny|EFileRead);
		
	if(ret!=KErrNone)
		{
		OstTraceExt1(TRACE_NORMAL, CBASETESTCASE_COMPARECURRENTTREETOREF_DUP01, "Reference File path: %S", refTreeFullFileName);
		OstTrace1(TRACE_NORMAL, CBASETESTCASE_COMPARECURRENTTREETOREF_DUP02, "refFile.Open fails ret = %d", ret);
		OstTraceFunctionExitExt( CBASETESTCASE_COMPARECURRENTTREETOREF_EXIT_DUP01, this, ret );
		return ret;
		}
		
	TInt refFileSize;
	refFile.Size(refFileSize);   
	
	// check size is identical
	if(refFileSize != iTreeBuffer.Size())
		{		
		OstTraceExt2(TRACE_NORMAL, CBASETESTCASE_COMPARECURRENTTREETOREF_DUP03, "sizes are NOT identical, refFileSize = %d, iTreeBuffer.Size() = %d ", refFileSize, iTreeBuffer.Size());
		//return KErrGeneral; not an issue, \n encoded differently by perforce... x0D x0A. (x0A only in generated ref file)
		}
		
	// read the file, and put it in a local buffer
	RBuf8 refBuf;
	refBuf.CreateL(refFileSize);
	ret = refFile.Read(0, refBuf, refFileSize);

	if(ret!=KErrNone)
		{
		OstTrace1(TRACE_NORMAL, CBASETESTCASE_COMPARECURRENTTREETOREF_DUP04, "refFile.Read fails %d", ret);
		OstTraceFunctionExitExt( CBASETESTCASE_COMPARECURRENTTREETOREF_EXIT_DUP02, this, ret );
		return ret;
		}
		
	// find occurences of \n now 
	RBuf8 copyRefBuf;
	copyRefBuf.CreateL(refFileSize);
	
	for(TInt iRefBuffer=0; iRefBuffer < refFileSize; iRefBuffer++)
		{
		if(refBuf[iRefBuffer] == 0x0D && iRefBuffer != refFileSize-1) // not the last byte
			{			
			if(refBuf[iRefBuffer+1] == 0x0A)
				{				
				copyRefBuf.Append(_L8("\n"));
				continue;
				}			
			}
		// previous is 0x0D, skip... 
		if( refBuf[iRefBuffer] == 0x0A && refBuf[iRefBuffer-1] == 0x0D)
			{
			continue;
			}			
		copyRefBuf.AppendFormat(_L8("%c"), refBuf[iRefBuffer]);				  
		}
	refBuf.Close();
	
	OstTrace1(TRACE_NORMAL, CBASETESTCASE_COMPARECURRENTTREETOREF_DUP05, "copyRefBuf.Size %d", copyRefBuf.Size());
		

	// check size is identical, should be identical now
	if(copyRefBuf.Size() != iTreeBuffer.Size())
		{		
		OstTraceExt2(TRACE_NORMAL, CBASETESTCASE_COMPARECURRENTTREETOREF_DUP06, "sizes are NOT identical, copyRefBuf.Size() = %d, iTreeBuffer.Size() = %d ", refFileSize, iTreeBuffer.Size());
		OstTraceFunctionExitExt( CBASETESTCASE_COMPARECURRENTTREETOREF_EXIT_DUP03, this, KErrGeneral );
		return KErrGeneral;
		}
	
	// now compare the 2 buffers		
    // Can only go as far as the smallest buffer
    TInt bufferSizeToCheck = Min(copyRefBuf.Size(), iTreeBuffer.Size());
	OstTrace1(TRACE_NORMAL, CBASETESTCASE_COMPARECURRENTTREETOREF_DUP07, "bufferSizeToCheck = %d", bufferSizeToCheck);

	aIsIdentical = ETrue;	
	for(TInt iRefBuffer=0; iRefBuffer < bufferSizeToCheck; iRefBuffer++)
		{
		if(iTreeBuffer[iRefBuffer] != copyRefBuf[iRefBuffer])
			{
			OstTrace1(TRACE_NORMAL, CBASETESTCASE_COMPARECURRENTTREETOREF_DUP08, "Failed buffer comparison at position %d", iRefBuffer);
            OstTraceExt4(TRACE_NORMAL, CBASETESTCASE_COMPARECURRENTTREETOREF_DUP09, "Missmatching chars (%d %d) (%c %c)", iTreeBuffer[iRefBuffer], copyRefBuf[iRefBuffer], iTreeBuffer[iRefBuffer], copyRefBuf[iRefBuffer]); 
			aIsIdentical = EFalse;
			break;
			}			 	 	 
		}		

	OstTrace1(TRACE_NORMAL, CBASETESTCASE_COMPARECURRENTTREETOREF_DUP10, "Finished Buffer comparison aIsIdentical=%d", aIsIdentical); 

 	copyRefBuf.Close();
	
	OstTraceFunctionExitExt( CBASETESTCASE_COMPARECURRENTTREETOREF_EXIT_DUP04, this, KErrNone );
	return KErrNone;	
	}	

CBaseTestCase::~CBaseTestCase()
	{
	OstTraceFunctionEntry1( CBASETESTCASE_CBASETESTCASE_ENTRY_DUP01, this );
	Cancel();
	iTimer.Close();
	iTreeBuffer.Close();
	iFs.Close();
	OstTraceFunctionExit1( CBASETESTCASE_CBASETESTCASE_EXIT_DUP01, this );
	}

void CBaseTestCase::SelfComplete()
	{
	OstTraceFunctionEntry1( CBASETESTCASE_SELFCOMPLETE_ENTRY, this );
	SelfComplete(KErrNone);
	OstTraceFunctionExit1( CBASETESTCASE_SELFCOMPLETE_EXIT, this );
	}

void CBaseTestCase::SelfComplete(TInt aError)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_SELFCOMPLETE_ENTRY_DUP01, this );
	TRequestStatus* s = &iStatus;
	iStatus = KRequestPending;
	User::RequestComplete(s,aError);
	SetActive();
	OstTraceFunctionExit1( CBASETESTCASE_SELFCOMPLETE_EXIT_DUP01, this );
	}


void CBaseTestCase::DoCancel()
	{
	OstTraceFunctionEntry1( CBASETESTCASE_DOCANCEL_ENTRY, this );
	iTimer.Cancel();
	if(iHost)
		{
		HostDoCancel();
		}
	else
		{
		DeviceDoCancel();
		}
	OstTraceFunctionExit1( CBASETESTCASE_DOCANCEL_EXIT, this );
	}

void CBaseTestCase::RunL()
	{
	OstTraceFunctionEntry1( CBASETESTCASE_RUNL_ENTRY, this );
	if(iHost)
		{
		HostRunL();
		}
	else
		{
		DeviceRunL();
		}
	OstTraceFunctionExit1( CBASETESTCASE_RUNL_EXIT, this );
	}

TInt CBaseTestCase::RunError(TInt aError)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_RUNERROR_ENTRY, this );
	OstTraceExt2(TRACE_NORMAL, CBASETESTCASE_RUNERROR, "Test case C%lS::RunL left with %d",iTestCaseId,aError);
	iTestPolicy->SignalTestComplete(aError);
	OstTraceFunctionExitExt( CBASETESTCASE_RUNERROR_EXIT, this, KErrNone );
	return KErrNone;
	}
	
TDesC& CBaseTestCase::TestCaseId()
	{
	OstTraceFunctionEntry1( CBASETESTCASE_TESTCASEID_ENTRY, this );
	OstTraceFunctionExitExt( CBASETESTCASE_TESTCASEID_EXIT, this, ( TUint )&( iTestCaseId ) );
	return iTestCaseId;
	}
	
	
TInt CBaseTestCase::TestResult() const
	{
	OstTraceFunctionEntry1( CBASETESTCASE_TESTRESULT_ENTRY, this );
	OstTraceFunctionExitExt( CBASETESTCASE_TESTRESULT_EXIT, this, iTestResult );
	return iTestResult;
	}
	
TBool CBaseTestCase::IsHostOnly() const
	{
	OstTraceFunctionEntry1( CBASETESTCASE_ISHOSTONLY_ENTRY, this );
	OstTraceFunctionExitExt( CBASETESTCASE_ISHOSTONLY_EXIT, this, iHostOnly );
	return iHostOnly;
	}
		
TBool CBaseTestCase::IsHost() const
	{
	OstTraceFunctionEntry1( CBASETESTCASE_ISHOST_ENTRY, this );
	OstTraceFunctionExitExt( CBASETESTCASE_ISHOST_EXIT, this, iHost );
	return iHost;
	}
		
void CBaseTestCase::PerformTestL()
	{
	OstTraceFunctionEntry1( CBASETESTCASE_PERFORMTESTL_ENTRY, this );
	
	if(iHost)
		{
		iTreeBuffer.CreateL(KTreeBufferSize); //32k
		ExecuteHostTestCaseL();
		}
	else
		{
		ExecuteDeviceTestCaseL();
		}	
	OstTraceFunctionExit1( CBASETESTCASE_PERFORMTESTL_EXIT, this );
	}

void CBaseTestCase::SetTestPolicy(CBasicTestPolicy* aTestPolicy)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_SETTESTPOLICY_ENTRY, this );
	iTestPolicy = aTestPolicy;
	OstTraceFunctionExit1( CBASETESTCASE_SETTESTPOLICY_EXIT, this );
	}

void CBaseTestCase::TestFailed(TInt aFailResult)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_TESTFAILED_ENTRY, this );
	iTestResult = aFailResult;
	if(!iHostOnly)
		{
		OstTrace0(TRACE_NORMAL, CBASETESTCASE_TESTFAILED, "CActiveScheduler::Stop CBaseTestCase::TestFailed");
		CActiveScheduler::Stop();
		}		
	OstTraceFunctionExit1( CBASETESTCASE_TESTFAILED_EXIT, this );
	}
	
void CBaseTestCase::TestPassed()
	{
	OstTraceFunctionEntry1( CBASETESTCASE_TESTPASSED_ENTRY, this );
	iTestResult = KErrNone;	
	if(!iHostOnly)
		{
		OstTrace0(TRACE_NORMAL, CBASETESTCASE_TESTPASSED, "CActiveScheduler::Stop CBaseTestCase::TestPassed");
		CActiveScheduler::Stop();
		}
	OstTraceFunctionExit1( CBASETESTCASE_TESTPASSED_EXIT, this );
	}

CBasicTestPolicy& CBaseTestCase::TestPolicy()
	{
	OstTraceFunctionEntry1( CBASETESTCASE_TESTPOLICY_ENTRY, this );
	OstTraceFunctionExit1( CBASETESTCASE_TESTPOLICY_EXIT, this );
	return *iTestPolicy;
	}
	

/**
Gets the first endpoint address that satisfies the parameters
So caution when there are multiple endpoints on the interface setting
See method below for specifying the endpoint index if more than 
one endpoint of the given type exists on the interface setting
*/
TInt CBaseTestCase::GetEndpointAddress(RUsbInterface& aUsbInterface,TInt aInterfaceSetting,
		TUint8 aTransferType,TUint8 aDirection,TInt& aEndpointAddress)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_GETENDPOINTADDRESS_ENTRY, this );
	
	return GetEndpointAddress(aUsbInterface, aInterfaceSetting, aTransferType, aDirection, 0, aEndpointAddress);
	}
	
/**
Gets the (aIndex+1)th endpoint address that satisfies the parameters
Allows the specification of the endpoint index (starting from ZERO)if more than 
one endpoint of the given type exists on the interface setting
*/
TInt CBaseTestCase::GetEndpointAddress(RUsbInterface& aUsbInterface,TInt aInterfaceSetting,
		TUint8 aTransferType,TUint8 aDirection,TUint8 aIndex,TInt& aEndpointAddress)
	{
		OstTraceFunctionEntryExt( CBASETESTCASE_GETENDPOINTADDRESS_ENTRY_DUP01, this );
		
	// Get the interface descriptor
	OstTrace0(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS, "Getting the interface descriptor for this alternate setting");

	TUsbInterfaceDescriptor alternateInterfaceDescriptor;
	TInt err = aUsbInterface.GetAlternateInterfaceDescriptor(aInterfaceSetting, alternateInterfaceDescriptor);

	if(err)
		{
		OstTraceExt2(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP01, "<Error %d> Unable to get alternate interface (%d) descriptor",err,aInterfaceSetting);
		OstTraceFunctionExitExt( CBASETESTCASE_GETENDPOINTADDRESS_EXIT, this, err );
		return err;
		}

	// Parse the descriptor tree from the interface 	
	OstTrace0(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP02, "Search the child descriptors for matching endpoint attributes");
	
	TUsbGenericDescriptor* descriptor = alternateInterfaceDescriptor.iFirstChild;
	TUint8 indexCount = 0;
	while(descriptor)
		{
		OstTrace0(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP03, "Check descriptor type for endpoint");

		// Cast the descriptor to an endpoint descriptor
		TUsbEndpointDescriptor* endpoint = TUsbEndpointDescriptor::Cast(descriptor);
		
		if(endpoint)
			{
			OstTrace0(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP04, "Match attributes for transfer type");
			
			if( (endpoint->Attributes() & aTransferType) == aTransferType)
				{
				OstTrace0(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP05, "Match attributes for endpoint direction");
				
				if( (endpoint->EndpointAddress() & aDirection) == aDirection) 
					{
					if(indexCount==aIndex)
						{
						aEndpointAddress = endpoint->EndpointAddress();
						OstTrace0(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP06, "Endpoint address found");
						OstTraceFunctionExitExt( CBASETESTCASE_GETENDPOINTADDRESS_EXIT_DUP01, this, KErrNone );
						return KErrNone;
						}
					else
						{
						indexCount++;
						}
					}
				}
			}

		descriptor = descriptor->iNextPeer;
		}

	// Unable to find the endpoint address	
	OstTrace0(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP07, "Unable to find endpoint address matching the specified attributes");
	
	OstTraceFunctionExitExt( CBASETESTCASE_GETENDPOINTADDRESS_EXIT_DUP02, this, KErrNotFound );
	return KErrNotFound;
	}
	
/*static*/ void CBaseTestCase::LogWithCondAndInfo(const TDesC& aCondition, const TDesC& aFileName, TInt aLine)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_LOGWITHCONDANDINFO_ENTRY, 0 );
	TBuf<256> buf;
 	buf.Format(KFailText, &aCondition, &aFileName, aLine);
 	OstTrace0(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP08, buf);
 	OstTraceFunctionExit1( CBASETESTCASE_LOGWITHCONDANDINFO_EXIT, 0 );
 	} 	
 
  	
/*static*/ void CBaseTestCase::PrintAndStoreTree(TUsbGenericDescriptor& aDesc, TInt aDepth)
	{ 
	OstTraceFunctionEntryExt( CBASETESTCASE_PRINTANDSTORETREE_ENTRY, 0 );
	
	TBuf8<20> buf;	
	for(TInt depth=aDepth;depth>=0;--depth)
		{
		buf.Append(_L8("  "));
		}
		
	//##==TBuf16<40> unicodeBuf;
	TBuf8<40> unicodeBuf;
	unicodeBuf.Copy(buf);	// Ideally this needs conversion to UNICODE
	if(aDesc.iRecognisedAndParsed == TUsbGenericDescriptor::ERecognised)
		{ 
		OstTraceExt3(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP09, "%s+ length=%u, type=0x%02x", unicodeBuf, aDesc.ibLength, (TUint32)aDesc.ibDescriptorType);
   		iTreeBuffer.AppendFormat(_L8("%S+ length=%d, type=0x%02x\n"), &buf, aDesc.ibLength, aDesc.ibDescriptorType);		
		}
	else
		{
		OstTraceExt3(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP10, "%s- length=%u, type=0x%02x", unicodeBuf, aDesc.ibLength, (TUint32)aDesc.ibDescriptorType);
		iTreeBuffer.AppendFormat(_L8("%S- length=%d, type=0x%02x\n"), &buf, aDesc.ibLength, aDesc.ibDescriptorType);
		} 		

		PrintAndStoreBlob(buf ,aDesc.iBlob);		
		
		if(aDesc.iFirstChild)    
		{
		OstTraceExt1(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP11, "%s \n", unicodeBuf);
		iTreeBuffer.AppendFormat(_L8("%S \\ \n"), &buf);		
		
		PrintAndStoreTree(*(aDesc.iFirstChild), aDepth+1);		
	
		OstTraceExt1(TRACE_NORMAL, CBASETESTCASE_GETENDPOINTADDRESS_DUP12, "%s \n", unicodeBuf);
		iTreeBuffer.AppendFormat(_L8("%S / \n"), &buf);
		}
	if(aDesc.iNextPeer)
		{
		PrintAndStoreTree(*(aDesc.iNextPeer), aDepth);
		}		
	OstTraceFunctionExit1( CBASETESTCASE_PRINTANDSTORETREE_EXIT, 0 );
	} 
	   
void CBaseTestCase::PrintAndStoreBlob(TDes8& aBuf, TPtrC8& aBlob)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_PRINTANDSTOREBLOB_ENTRY, this );
	
	HBufC8* chunk = HBufC8::New(KChunkSize);
	
	TUint nbIter = aBlob.Length()/(KChunkSize/2);
	TUint remainderSize = aBlob.Length()%(KChunkSize/2);

	if(nbIter == 0)  
		{
		PrintAndStoreChunk(chunk, aBlob.Length() ,aBlob, 0, 0, aBuf );      
		}
	else
		{
		// print chunks
		TUint offset = 0;
		TInt i = 0;
		for(i=0;i<nbIter;++i)
			{
			PrintAndStoreChunk(chunk, (KChunkSize/2) ,aBlob, offset, i, aBuf); 
			offset+=(KChunkSize/2);
			} 
		// remainder
		PrintAndStoreChunk(chunk, remainderSize ,aBlob,offset, i ,aBuf);				
		}
	delete chunk;
	OstTraceFunctionExit1( CBASETESTCASE_PRINTANDSTOREBLOB_EXIT, this );
	} 
	
void CBaseTestCase::PrintAndStoreChunk(HBufC8* aChunk, TUint aSize, TPtrC8& aBlob, TUint aOffset, TUint aIter, TDes8& aBuf)
	{	
	OstTraceFunctionEntryExt( CBASETESTCASE_PRINTANDSTORECHUNK_ENTRY, this );
	for(TInt i=0;i<aSize;++i)
		{
		aChunk->Des().AppendFormat(_L8("%02x"), aBlob[i+aOffset]);
		}
		
	TBuf16<40> unicodeBuf;	
	unicodeBuf.Copy(aBuf);
	TBuf16<256> unicodeChunk;	
	unicodeChunk.Copy(aChunk->Des());
			
	if(aIter ==0)
		{		
		OstTraceExt2(TRACE_NORMAL, CBASETESTCASE_PRINTANDSTORECHUNK, "%S >%S", unicodeBuf, unicodeChunk);					
		iTreeBuffer.AppendFormat(_L8("%S >%S\n"), &aBuf, aChunk);	
		}
	else
		{	
		OstTraceExt2(TRACE_NORMAL, CBASETESTCASE_PRINTANDSTORECHUNK_DUP01, "%S  %S\n", unicodeBuf, unicodeChunk); 
		iTreeBuffer.AppendFormat(_L8("%S  %S\n"), &aBuf, aChunk);
		}
	aChunk->Des().Zero();		
	OstTraceFunctionExit1( CBASETESTCASE_PRINTANDSTORECHUNK_EXIT, this );
	}	
	
TInt CBaseTestCase::CheckTree(TUsbGenericDescriptor& aDevDesc, TUsbGenericDescriptor& aConfigDesc, const TDesC& aFileName)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_CHECKTREE_ENTRY, this );
	TInt ret = KErrNone;
	
	// flush buffer
	iTreeBuffer.Zero();
	
	// print and store tree from aDevDesc & aConfigDesc
	PrintAndStoreTree(aDevDesc);
	PrintAndStoreTree(aConfigDesc);
	
	// generate file if needed	
	#ifdef GENERATE_TREES
	GenerateRefFile(aFileName);	
	#endif // GENERATE_TREES
	
	// compare tree to ref.
	TBool isIdentical;
	if(KErrNone != CompareCurrentTreeToRef(aFileName, isIdentical))
		{ 
		OstTrace0(TRACE_NORMAL, CBASETESTCASE_CHECKTREE, "CompareCurrentTreeToRef error");
		ret = KErrGeneral;
		}	
	if(!isIdentical)
		{ 
		OstTrace0(TRACE_NORMAL, CBASETESTCASE_CHECKTREE_DUP01, "!isIdentical");
		ret = KErrGeneral;
		}
	OstTraceFunctionExitExt( CBASETESTCASE_CHECKTREE_EXIT, this, ret );
	return ret;
	}
	
	
TInt CBaseTestCase::ParseConfigDescriptorAndCheckTree(TUsbDeviceDescriptor *devDesc, const TDesC8& configSet, TUint indexTest)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_PARSECONFIGDESCRIPTORANDCHECKTREE_ENTRY, this );
	// Parse config. descriptor
	TUsbGenericDescriptor* parsed = NULL;
	TInt err = UsbDescriptorParser::Parse(configSet, parsed);
	if(err != KErrNone)
		{
		OstTrace0(TRACE_NORMAL, CBASETESTCASE_PARSECONFIGDESCRIPTORANDCHECKTREE, "parsing error : UsbDescriptorParser::Parse");
		OstTraceFunctionExitExt( CBASETESTCASE_PARSECONFIGDESCRIPTORANDCHECKTREE_EXIT, this, err );
		return err;
		}
	TUsbConfigurationDescriptor* configDesc = TUsbConfigurationDescriptor::Cast(parsed);
	// checks 
	if(configDesc == 0)
		{
		OstTrace0(TRACE_NORMAL, CBASETESTCASE_PARSECONFIGDESCRIPTORANDCHECKTREE_DUP01, "configDesc == 0");
		OstTraceFunctionExitExt( CBASETESTCASE_PARSECONFIGDESCRIPTORANDCHECKTREE_EXIT_DUP01, this, KErrGeneral );
		return KErrGeneral; 
		}
		
	// checking tree 
	TBuf<KMaxName> fname(iTestCaseId);
	fname.AppendFormat(_L("_%d"), indexTest);
	return CheckTree(*devDesc, *configDesc, fname); 
	}	
	
TInt CBaseTestCase::CheckTreeAfterDeviceInsertion(CUsbTestDevice& aTestDevice, const TDesC& aFileName)
	{
	OstTraceFunctionEntryExt( CBASETESTCASE_CHECKTREEAFTERDEVICEINSERTION_ENTRY, this );
	TUsbGenericDescriptor deviceDesc = aTestDevice.DeviceDescriptor();
	TUsbGenericDescriptor configDesc = aTestDevice.ConfigurationDescriptor();	
	return CheckTree(deviceDesc, configDesc, aFileName); 	
	}	
	
	}
