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
// @file basetestcase.cpp
// @internalComponent
// 
//

#include "BaseTestCase.h"
#include <e32ver.h>
#include <d32usbdi.h>
#include "testdebug.h"
#include "testpolicy.h"

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
	iTestCaseId.Copy(aTestCaseId);
	CActiveScheduler::Add(this);
	}
	
void CBaseTestCase::BaseConstructL()
	{
	LOG_FUNC	
	RDebug::Printf("Creating test case timer");
	TInt err(iTimer.CreateLocal());
	if(err == KErrNone)
		{
		RDebug::Printf("Test case timer created");
		}
	else
		{
		RDebug::Printf("<Error %d> Test case timer could not be created",err);
		User::Leave(err);
		}	
	}

void CBaseTestCase::TimeoutIn(TInt aTimeoutPeriod)
	{
	LOG_FUNC
	
	CancelTimeout();
	iTimer.After(iStatus,aTimeoutPeriod*1000000);
	SetActive();
	}


void CBaseTestCase::CancelTimeout()
	{
	iTimer.Cancel();
	}

TInt CBaseTestCase::GenerateRefFile(const TDesC& aFileName)
	{
	
	LOG_FUNC
	TBuf<256> refTreeFullFileName(KGeneratedFilesPath);
	refTreeFullFileName.Append(aFileName);	
	refTreeFullFileName.Append(KExtensionFile);
	
 	RFile refFile; 
	TInt ret = KErrNone; 
	ret = iFs.Connect();
	if(ret!=KErrNone && ret!=KErrAlreadyExists)
	// if already connected, ignore
		{ 
		RDebug::Printf("iFs.Connect fails, ret = %d", ret);
		return ret;
		}
			
	ret = iFs.Delete(refTreeFullFileName);
	if(ret == KErrNone || ret == KErrNotFound)
		{
		ret = refFile.Create(iFs,refTreeFullFileName,EFileShareAny|EFileWrite);
		}		
	
	if(ret!=KErrNone) 
		{ 
		RDebug::Printf("refFile.Create fails, ret = %d", ret);
		return ret;
		}
		 
	refFile.Write(iTreeBuffer);
	refFile.Flush(); 
	refFile.Close();
			 
	return KErrNone;
	}	

TInt CBaseTestCase::CompareCurrentTreeToRef(const TDesC& aFileName, TBool& aIsIdentical)
	{
	
	LOG_FUNC								
	TBuf<256> refTreeFullFileName(KRefPath);
	refTreeFullFileName.Append(aFileName);
	refTreeFullFileName.Append(KExtensionFile);	 	

	TInt ret = KErrNone; 
	ret = iFs.Connect();
	if(ret!=KErrNone && ret!=KErrAlreadyExists)
	// if already connected, ignore
		{ 
		RDebug::Printf("iFs.Connect fails, ret = %d", ret);
		return ret;
		}

	RFile refFile;
	ret = refFile.Open(iFs,refTreeFullFileName,EFileShareAny|EFileRead);
		
	if(ret!=KErrNone)
		{
		RDebug::Printf("Reference File path: %S", &refTreeFullFileName);
		RDebug::Printf("refFile.Open fails ret = %d", ret);
		return ret;
		}
		
	TInt refFileSize;
	refFile.Size(refFileSize);   
	
	// check size is identical
	if(refFileSize != iTreeBuffer.Size())
		{		
		RDebug::Printf("sizes are NOT identical, refFileSize = %d, iTreeBuffer.Size() = %d ", refFileSize, iTreeBuffer.Size());
		//return KErrGeneral; not an issue, \n encoded differently by perforce... x0D x0A. (x0A only in generated ref file)
		}
		
	// read the file, and put it in a local buffer
	RBuf8 refBuf;
	refBuf.CreateL(refFileSize);
	ret = refFile.Read(0, refBuf, refFileSize);

	if(ret!=KErrNone)
		{
		RDebug::Printf("refFile.Read fails %d", ret);
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
	
	RDebug::Printf("copyRefBuf.Size %d", copyRefBuf.Size());
		

	// check size is identical, should be identical now
	if(copyRefBuf.Size() != iTreeBuffer.Size())
		{		
		RDebug::Printf("sizes are NOT identical, copyRefBuf.Size() = %d, iTreeBuffer.Size() = %d ", refFileSize, iTreeBuffer.Size());
		return KErrGeneral;
		}
	
	// now compare the 2 buffers		
    // Can only go as far as the smallest buffer
    TInt bufferSizeToCheck = Min(copyRefBuf.Size(), iTreeBuffer.Size());
	RDebug::Print(_L("bufferSizeToCheck = %d"), bufferSizeToCheck);

	aIsIdentical = ETrue;	
	for(TInt iRefBuffer=0; iRefBuffer < bufferSizeToCheck; iRefBuffer++)
		{
		if(iTreeBuffer[iRefBuffer] != copyRefBuf[iRefBuffer])
			{
			RDebug::Print(_L("Failed buffer comparison at position %d"), iRefBuffer);
            RDebug::Print(_L("Missmatching chars (%d %d) (%c %c)"), iTreeBuffer[iRefBuffer], copyRefBuf[iRefBuffer], iTreeBuffer[iRefBuffer], copyRefBuf[iRefBuffer]);
			aIsIdentical = EFalse;
			break;
			}			 	 	 
		}		

	RDebug::Print(_L("Finished Buffer comparison aIsIdentical=%d"), aIsIdentical); 

 	copyRefBuf.Close();
	
	return KErrNone;	
	}	

CBaseTestCase::~CBaseTestCase()
	{
	LOG_FUNC
	Cancel();
	iTimer.Close();
	iTreeBuffer.Close();
	iFs.Close();
	}

void CBaseTestCase::SelfComplete()
	{
	SelfComplete(KErrNone);
	}

void CBaseTestCase::SelfComplete(TInt aError)
	{
	TRequestStatus* s = &iStatus;
	iStatus = KRequestPending;
	User::RequestComplete(s,aError);
	SetActive();
	}


void CBaseTestCase::DoCancel()
	{
	LOG_FUNC
	iTimer.Cancel();
	if(iHost)
		{
		HostDoCancel();
		}
	else
		{
		DeviceDoCancel();
		}
	}

void CBaseTestCase::RunL()
	{
	if(iHost)
		{
		HostRunL();
		}
	else
		{
		DeviceRunL();
		}
	}

TInt CBaseTestCase::RunError(TInt aError)
	{
	LOG_FUNC
	RDebug::Printf("Test case C%lS::RunL left with %d",&iTestCaseId,aError);
	iTestPolicy->SignalTestComplete(aError);
	return KErrNone;
	}
	
TDesC& CBaseTestCase::TestCaseId()
	{
	return iTestCaseId;
	}
	
	
TInt CBaseTestCase::TestResult() const
	{
	return iTestResult;
	}
	
TBool CBaseTestCase::IsHostOnly() const
	{
	return iHostOnly;
	}
		
TBool CBaseTestCase::IsHost() const
	{
	return iHost;
	}
		
void CBaseTestCase::PerformTestL()
	{
	
	if(iHost)
		{
		iTreeBuffer.CreateL(KTreeBufferSize); //32k
		ExecuteHostTestCaseL();
		}
	else
		{
		ExecuteDeviceTestCaseL();
		}	
	}

void CBaseTestCase::SetTestPolicy(CBasicTestPolicy* aTestPolicy)
	{
	iTestPolicy = aTestPolicy;
	}

void CBaseTestCase::TestFailed(TInt aFailResult)
	{
	LOG_FUNC
	iTestResult = aFailResult;
	if(!iHostOnly)
		{
		RDebug::Printf("CActiveScheduler::Stop CBaseTestCase::TestFailed");
		CActiveScheduler::Stop();
		}		
	}
	
void CBaseTestCase::TestPassed()
	{
	LOG_FUNC
	iTestResult = KErrNone;	
	if(!iHostOnly)
		{
		RDebug::Printf("CActiveScheduler::Stop CBaseTestCase::TestPassed");
		CActiveScheduler::Stop();
		}
	}

CBasicTestPolicy& CBaseTestCase::TestPolicy()
	{
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
	LOG_FUNC
	
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
	LOG_FUNC
		
	// Get the interface descriptor
	RDebug::Printf("Getting the interface descriptor for this alternate setting");

	TUsbInterfaceDescriptor alternateInterfaceDescriptor;
	TInt err = aUsbInterface.GetAlternateInterfaceDescriptor(aInterfaceSetting, alternateInterfaceDescriptor);

	if(err)
		{
		RDebug::Printf("<Error %d> Unable to get alternate interface (%d) descriptor",err,aInterfaceSetting);
		return err;
		}

	// Parse the descriptor tree from the interface 	
	RDebug::Printf("Search the child descriptors for matching endpoint attributes");
	
	TUsbGenericDescriptor* descriptor = alternateInterfaceDescriptor.iFirstChild;
	TUint8 indexCount = 0;
	while(descriptor)
		{
		RDebug::Printf("Check descriptor type for endpoint");

		// Cast the descriptor to an endpoint descriptor
		TUsbEndpointDescriptor* endpoint = TUsbEndpointDescriptor::Cast(descriptor);
		
		if(endpoint)
			{
			RDebug::Printf("Match attributes for transfer type");
			
			if( (endpoint->Attributes() & aTransferType) == aTransferType)
				{
				RDebug::Printf("Match attributes for endpoint direction");
				
				if( (endpoint->EndpointAddress() & aDirection) == aDirection) 
					{
					if(indexCount==aIndex)
						{
						aEndpointAddress = endpoint->EndpointAddress();
						RDebug::Printf("Endpoint address found");
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
	RDebug::Printf("Unable to find endpoint address matching the specified attributes");
	
	return KErrNotFound;
	}
	
/*static*/ void CBaseTestCase::LogWithCondAndInfo(const TDesC& aCondition, const TDesC& aFileName, TInt aLine)
	{
	TBuf<256> buf;
 	buf.Format(KFailText, &aCondition, &aFileName, aLine);
 	RDebug::Print(buf); 
 	} 	
 
  	
/*static*/ void CBaseTestCase::PrintAndStoreTree(TUsbGenericDescriptor& aDesc, TInt aDepth)
	{ 
	
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
		RDebug::Printf("%S+ length=%d, type=0x%02x", &unicodeBuf, aDesc.ibLength, aDesc.ibDescriptorType);
   		iTreeBuffer.AppendFormat(_L8("%S+ length=%d, type=0x%02x\n"), &buf, aDesc.ibLength, aDesc.ibDescriptorType);		
		}
	else
		{
		RDebug::Printf("%S- length=%d, type=0x%02x", &unicodeBuf, aDesc.ibLength, aDesc.ibDescriptorType);
		iTreeBuffer.AppendFormat(_L8("%S- length=%d, type=0x%02x\n"), &buf, aDesc.ibLength, aDesc.ibDescriptorType);
		} 		

		PrintAndStoreBlob(buf ,aDesc.iBlob);		
		
		if(aDesc.iFirstChild)    
		{
		RDebug::Printf("%S \\ ", &unicodeBuf);
		iTreeBuffer.AppendFormat(_L8("%S \\ \n"), &buf);		
		
		PrintAndStoreTree(*(aDesc.iFirstChild), aDepth+1);		
	
		RDebug::Printf("%S / ", &unicodeBuf);
		iTreeBuffer.AppendFormat(_L8("%S / \n"), &buf);
		}
	if(aDesc.iNextPeer)
		{
		PrintAndStoreTree(*(aDesc.iNextPeer), aDepth);
		}		
	} 
	   
void CBaseTestCase::PrintAndStoreBlob(TDes8& aBuf, TPtrC8& aBlob)
	{
	
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
	} 
	
void CBaseTestCase::PrintAndStoreChunk(HBufC8* aChunk, TUint aSize, TPtrC8& aBlob, TUint aOffset, TUint aIter, TDes8& aBuf)
	{	
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
		RDebug::Printf("%S >%S", &unicodeBuf, &unicodeChunk);					
		iTreeBuffer.AppendFormat(_L8("%S >%S\n"), &aBuf, aChunk);	
		}
	else
		{	
		RDebug::Printf("%S  %S\n", &unicodeBuf, &unicodeChunk); 
		iTreeBuffer.AppendFormat(_L8("%S  %S\n"), &aBuf, aChunk);
		}
	aChunk->Des().Zero();		
	}	
	
TInt CBaseTestCase::CheckTree(TUsbGenericDescriptor& aDevDesc, TUsbGenericDescriptor& aConfigDesc, const TDesC& aFileName)
	{
	LOG_FUNC
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
		RDebug::Printf("CompareCurrentTreeToRef error"); 
		ret = KErrGeneral;
		}	
	if(!isIdentical)
		{ 
		RDebug::Printf("!isIdentical"); 
		ret = KErrGeneral;
		}
	return ret;
	}
	
	
TInt CBaseTestCase::ParseConfigDescriptorAndCheckTree(TUsbDeviceDescriptor *devDesc, const TDesC8& configSet, TUint indexTest)
	{
	LOG_FUNC
	// Parse config. descriptor
	TUsbGenericDescriptor* parsed = NULL;
	TInt err = UsbDescriptorParser::Parse(configSet, parsed);
	if(err != KErrNone)
		{
		RDebug::Printf("parsing error : UsbDescriptorParser::Parse"); 
		return err;
		}
	TUsbConfigurationDescriptor* configDesc = TUsbConfigurationDescriptor::Cast(parsed);
	// checks 
	if(configDesc == 0)
		{
		RDebug::Printf("configDesc == 0");
		return KErrGeneral; 
		}
		
	// checking tree 
	TBuf<KMaxName> fname(iTestCaseId);
	fname.AppendFormat(_L("_%d"), indexTest);
	return CheckTree(*devDesc, *configDesc, fname); 
	}	
	
TInt CBaseTestCase::CheckTreeAfterDeviceInsertion(CUsbTestDevice& aTestDevice, const TDesC& aFileName)
	{
	LOG_FUNC
	TUsbGenericDescriptor deviceDesc = aTestDevice.DeviceDescriptor();
	TUsbGenericDescriptor configDesc = aTestDevice.ConfigurationDescriptor();	
	return CheckTree(deviceDesc, configDesc, aFileName); 	
	}	
	
	}
