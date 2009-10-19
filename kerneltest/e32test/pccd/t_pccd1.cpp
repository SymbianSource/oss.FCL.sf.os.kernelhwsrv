// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pccd\t_pccd1.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include <d_pccdif.h>

LOCAL_D RTest test(_L("T_PCCD1"));
const TSocket KSocket=0;
const TInt KFunc=0;

GLDEF_C TInt E32Main()
	{
#if defined (__WINS__)
	// Connect to all the local drives first as will be the case in ARM
	TBusLocalDrive Drive[KMaxLocalDrives];
	TBool ChangedFlag[KMaxLocalDrives];
	TInt i;
	for (i=0;i<KMaxLocalDrives;i++)
		Drive[i].Connect(i,ChangedFlag[i]);
#endif

	test.Title();

	TDriveInfoV1Buf diBuf;
	UserHal::DriveInfo(diBuf);
	TDriveInfoV1 &di=diBuf();
	test.Printf(_L("SOCKETS PRESENT  :%d\r\n"),di.iTotalSockets);
	test.Printf(_L("1ST SOCKET NAME  :%- 16S\r\n"),&di.iSocketName[0]);
	test.Printf(_L("2ND SOCKET NAME  :%- 16S\r\n"),&di.iSocketName[1]);
	test.Printf(_L("3RD SOCKET NAME  :%- 16S\r\n"),&di.iSocketName[2]);
	test.Printf(_L("4TH SOCKET NAME  :%- 16S\r\n"),&di.iSocketName[3]);
	//test.Printf( _L("Hit a key\r\n"));
    //test.Getch();
//
	test.Start(_L("Load/open logical device"));
	TInt r;
	r=User::LoadLogicalDevice(_L("D_PCCDIF"));
	test(r==KErrNone||r==KErrAlreadyExists);
	RPcCardCntrlIf pccdDrv;
	r=pccdDrv.Open(KSocket,pccdDrv.VersionRequired());
	test(r==KErrNone);

	test.Next(_L("Power card, check card type."));
	UserSvr::ForceRemountMedia(ERemovableMedia0); // Media change - ensures test always starts from same state
	User::After(300000);	// Allow 0.3s after power down for controller to detect door closed.
    pccdDrv.Reset();
	TPcCardStatus s;
	TSocketSignals ind;
	test(pccdDrv.SocketInfo(s,ind)==KErrNone);
	test(s==EPcCardNotReady);
	test(ind.iCardDetected);
#if defined (__WINS__)
	test(ind.iVoltSense==0x01);
#else
	if(ind.iVoltSense!=0x03)
		test.Printf(_L("vs=%d"),ind.iVoltSense);
	test(ind.iVoltSense==0x03);
#endif
	TRequestStatus rs;
	pccdDrv.PwrUp(&rs);
	User::WaitForRequest(rs);
	test(rs.Int()==KErrNone);
	test(pccdDrv.SocketInfo(s,ind)==KErrNone);
	test(s==EPcCardReady);
	TPcCardTypePckg tyBuf;
	TPcCardType &ty=tyBuf();
	test(pccdDrv.CardInfo(&tyBuf)==KErrNone);
	test(ty.iFuncType[0]==EFFixedDiskCard);
	test(ty.iFuncCount==1);

	test.Next(_L("Tuple reading."));
	TBuf8<257> tbuf;
    test(pccdDrv.GetTuple(KFunc,KPccdNonSpecificTuple,&tbuf)==KErrNone);
	test(tbuf[0]==KCisTupleDevice);
    test(pccdDrv.GetTuple(KFunc,KCisTupleFuncId,&tbuf)==KErrNone);
	test(tbuf[0]==KCisTupleFuncId&&tbuf[1]==0x02&&tbuf[2]==0x04&&tbuf[3]==0x01); 
    test(pccdDrv.GetTuple(KFunc,KCisTupleFuncId,&tbuf)==KErrNotFound);
    test(pccdDrv.ResetCis(KFunc)==KErrNone);
    test(pccdDrv.GetTuple(KFunc,KCisTupleFuncId,&tbuf)==KErrNone);
    test(pccdDrv.GetTuple(KFunc,KCisTupleNoLink,&tbuf)==KErrNone);
	test(tbuf[0]==KCisTupleNoLink&&tbuf[1]==0x00); 
    test(pccdDrv.GetTuple(KFunc,KCisTupleBattery,&tbuf)==KErrNotFound);
    
	test.Next(_L("Read card configuration 1."));
	TPcCardConfigInfoPckg ciBuf;
	TPcCardConfigInfo &ci=ciBuf();
    test(pccdDrv.GetConfig(KFunc,&ciBuf)==KErrNone);
	test(ci.iAccessSpeed==EAccSpeed200nS);
	test(ci.iActiveSignals==(KSignalWaitRequired|KSignalReadyActive));
	test(ci.iVccMaxInMilliVolts==5500&&ci.iVccMinInMilliVolts==4500);			
	test(ci.iValidChunks==1);
	test(ci.iChnk[0].iMemType==EPcCardCommon16Mem&&ci.iChnk[0].iMemBaseAddr==0&&ci.iChnk[0].iMemLen==0x800); // 2K
	test(ci.iIsIoAndMem==FALSE&&ci.iIsDefault);
	test(ci.iVppMaxInMilliVolts==0&&ci.iVppMinInMilliVolts==0);
	test(ci.iPwrDown&&ci.iOperCurrentInMicroAmps==80000&&ci.iPwrDwnCurrentInMicroAmps==0);
	test(ci.iInterruptInfo==0);
	test(ci.iConfigOption==0&&ci.iConfigBaseAddr==0x200&&ci.iRegPresent==0x0F);

	test.Next(_L("Read card configuration 2."));
    test(pccdDrv.GetConfig(KFunc,&ciBuf)==KErrNone);
	test(ci.iAccessSpeed==EAccSpeed200nS);
	test(ci.iActiveSignals==(KSignalWaitRequired|KSignalReadyActive));
	test(ci.iVccMaxInMilliVolts==3465&&ci.iVccMinInMilliVolts==3135);			
	test(ci.iValidChunks==1);
	test(ci.iChnk[0].iMemType==EPcCardCommon16Mem&&ci.iChnk[0].iMemBaseAddr==0&&ci.iChnk[0].iMemLen==0x800); // 2K
	test(ci.iIsIoAndMem==FALSE&&ci.iIsDefault==FALSE);
	test(ci.iVppMaxInMilliVolts==0&&ci.iVppMinInMilliVolts==0);
	test(ci.iPwrDown&&ci.iOperCurrentInMicroAmps==45000&&ci.iPwrDwnCurrentInMicroAmps==0);
	test(ci.iInterruptInfo==0);
	test(ci.iConfigOption==0&&ci.iConfigBaseAddr==0x200&&ci.iRegPresent==0x0F);

	test.Next(_L("Read card configuration 3."));
    test(pccdDrv.GetConfig(KFunc,&ciBuf)==KErrNone);
	test(ci.iAccessSpeed==EAccSpeed200nS);
	test(ci.iActiveSignals==KSignalReadyActive);
	test(ci.iVccMaxInMilliVolts==5500&&ci.iVccMinInMilliVolts==4500);			
	test(ci.iValidChunks==1);
	test(ci.iChnk[0].iMemType==EPcCardIo16Mem&&ci.iChnk[0].iMemBaseAddr==0&&ci.iChnk[0].iMemLen==0x10); // 16bytes
	test(ci.iIsIoAndMem&&ci.iIsDefault);
	test(ci.iVppMaxInMilliVolts==0&&ci.iVppMinInMilliVolts==0);
	test(ci.iPwrDown&&ci.iOperCurrentInMicroAmps==80000&&ci.iPwrDwnCurrentInMicroAmps==0);
	test(ci.iInterruptInfo==(KPcCardIntShare|KPcCardIntPulse|KPcCardIntLevel));
	test(ci.iConfigOption==1&&ci.iConfigBaseAddr==0x200&&ci.iRegPresent==0x0F);

	test.Next(_L("Read card configuration 4."));
    test(pccdDrv.GetConfig(KFunc,&ciBuf)==KErrNone);
	test(ci.iAccessSpeed==EAccSpeed200nS);
	test(ci.iActiveSignals==KSignalReadyActive);
	test(ci.iVccMaxInMilliVolts==3465&&ci.iVccMinInMilliVolts==3135);			
	test(ci.iValidChunks==1);
	test(ci.iChnk[0].iMemType==EPcCardIo16Mem&&ci.iChnk[0].iMemBaseAddr==0&&ci.iChnk[0].iMemLen==0x10); // 16bytes
	test(ci.iIsIoAndMem&&ci.iIsDefault==FALSE);
	test(ci.iVppMaxInMilliVolts==0&&ci.iVppMinInMilliVolts==0);
	test(ci.iPwrDown&&ci.iOperCurrentInMicroAmps==45000&&ci.iPwrDwnCurrentInMicroAmps==0);
	test(ci.iInterruptInfo==(KPcCardIntShare|KPcCardIntPulse|KPcCardIntLevel));
	test(ci.iConfigOption==1&&ci.iConfigBaseAddr==0x200&&ci.iRegPresent==0x0F);

	test.Next(_L("Read card configuration 5."));
    test(pccdDrv.GetConfig(KFunc,&ciBuf)==KErrNone);
	test(ci.iAccessSpeed==EAccSpeed200nS);
	test(ci.iActiveSignals==KSignalReadyActive);
	test(ci.iVccMaxInMilliVolts==5500&&ci.iVccMinInMilliVolts==4500);			
	test(ci.iValidChunks==2);
	test(ci.iChnk[0].iMemType==EPcCardIo16Mem&&ci.iChnk[0].iMemBaseAddr==0x1F0&&ci.iChnk[0].iMemLen==8); // 8bytes
	test(ci.iChnk[1].iMemType==EPcCardIo16Mem&&ci.iChnk[1].iMemBaseAddr==0x3F6&&ci.iChnk[1].iMemLen==2); // 2bytes
	test(ci.iIsIoAndMem&&ci.iIsDefault);
	test(ci.iVppMaxInMilliVolts==0&&ci.iVppMinInMilliVolts==0);
	test(ci.iPwrDown&&ci.iOperCurrentInMicroAmps==80000&&ci.iPwrDwnCurrentInMicroAmps==0);
	test(ci.iInterruptInfo==(KPcCardIntShare|KPcCardIntPulse|KPcCardIntLevel));
	test(ci.iConfigOption==2&&ci.iConfigBaseAddr==0x200&&ci.iRegPresent==0x0F);

	test.Next(_L("Read card configuration 6."));
    test(pccdDrv.GetConfig(KFunc,&ciBuf)==KErrNone);
	test(ci.iAccessSpeed==EAccSpeed200nS);
	test(ci.iActiveSignals==KSignalReadyActive);
	test(ci.iVccMaxInMilliVolts==3465&&ci.iVccMinInMilliVolts==3135);			
	test(ci.iValidChunks==2);
	test(ci.iChnk[0].iMemType==EPcCardIo16Mem&&ci.iChnk[0].iMemBaseAddr==0x1F0&&ci.iChnk[0].iMemLen==8); // 8bytes
	test(ci.iChnk[1].iMemType==EPcCardIo16Mem&&ci.iChnk[1].iMemBaseAddr==0x3F6&&ci.iChnk[1].iMemLen==2); // 2bytes
	test(ci.iIsIoAndMem&&ci.iIsDefault==FALSE);
	test(ci.iVppMaxInMilliVolts==0&&ci.iVppMinInMilliVolts==0);
	test(ci.iPwrDown&&ci.iOperCurrentInMicroAmps==45000&&ci.iPwrDwnCurrentInMicroAmps==0);
	test(ci.iInterruptInfo==(KPcCardIntShare|KPcCardIntPulse|KPcCardIntLevel));
	test(ci.iConfigOption==2&&ci.iConfigBaseAddr==0x200&&ci.iRegPresent==0x0F);

	test.Next(_L("Read card configuration 7."));
    test(pccdDrv.GetConfig(KFunc,&ciBuf)==KErrNone);
	test(ci.iAccessSpeed==EAccSpeed200nS);
	test(ci.iActiveSignals==KSignalReadyActive);
	test(ci.iVccMaxInMilliVolts==5500&&ci.iVccMinInMilliVolts==4500);			
	test(ci.iValidChunks==2);
	test(ci.iChnk[0].iMemType==EPcCardIo16Mem&&ci.iChnk[0].iMemBaseAddr==0x170&&ci.iChnk[0].iMemLen==8); // 8bytes
	test(ci.iChnk[1].iMemType==EPcCardIo16Mem&&ci.iChnk[1].iMemBaseAddr==0x376&&ci.iChnk[1].iMemLen==2); // 2bytes
	test(ci.iIsIoAndMem&&ci.iIsDefault);
	test(ci.iVppMaxInMilliVolts==0&&ci.iVppMinInMilliVolts==0);
	test(ci.iPwrDown&&ci.iOperCurrentInMicroAmps==80000&&ci.iPwrDwnCurrentInMicroAmps==0);
	test(ci.iInterruptInfo==(KPcCardIntShare|KPcCardIntPulse|KPcCardIntLevel));
	test(ci.iConfigOption==3&&ci.iConfigBaseAddr==0x200&&ci.iRegPresent==0x0F);

	test.Next(_L("Read card configuration 8."));
    test(pccdDrv.GetConfig(KFunc,&ciBuf)==KErrNone);
	test(ci.iAccessSpeed==EAccSpeed200nS);
	test(ci.iActiveSignals==KSignalReadyActive);
	test(ci.iVccMaxInMilliVolts==3465&&ci.iVccMinInMilliVolts==3135);			
	test(ci.iValidChunks==2);
	test(ci.iChnk[0].iMemType==EPcCardIo16Mem&&ci.iChnk[0].iMemBaseAddr==0x170&&ci.iChnk[0].iMemLen==8); // 8bytes
	test(ci.iChnk[1].iMemType==EPcCardIo16Mem&&ci.iChnk[1].iMemBaseAddr==0x376&&ci.iChnk[1].iMemLen==2); // 2bytes
	test(ci.iIsIoAndMem&&ci.iIsDefault==FALSE);
	test(ci.iVppMaxInMilliVolts==0&&ci.iVppMinInMilliVolts==0);
	test(ci.iPwrDown&&ci.iOperCurrentInMicroAmps==45000&&ci.iPwrDwnCurrentInMicroAmps==0);
	test(ci.iInterruptInfo==(KPcCardIntShare|KPcCardIntPulse|KPcCardIntLevel));
	test(ci.iConfigOption==3&&ci.iConfigBaseAddr==0x200&&ci.iRegPresent==0x0F);

    test(pccdDrv.GetConfig(KFunc,&ciBuf)==KErrNotFound);

	test.Next(_L("Read card memory regions."));
	TPcCardRegionInfoPckg rgBuf;
	TPcCardRegionInfo &rg=rgBuf();
	test.Next(_L("Read card region."));
    test(pccdDrv.ResetCis(KFunc)==KErrNone);
	test(pccdDrv.GetRegion(KFunc,&rgBuf)==KErrNone);
#if defined (__WINS__)
	test(rg.iAccessSpeed==EAccSpeed750nS);
	test(rg.iExtendedAccSpeedInNanoSecs==700);
	test(rg.iActiveSignals==0);
	test(rg.iVcc==EPcCardSocket_5V0);			
#else
	test(rg.iAccessSpeed==EAccSpeed250nS);
	test(rg.iExtendedAccSpeedInNanoSecs==0);
	test(rg.iActiveSignals==KSignalWaitRequired);
	test(rg.iVcc==EPcCardSocket_3V3);			
#endif
	test(rg.iChnk.iMemType==EPcCardCommon16Mem&&rg.iChnk.iMemBaseAddr==0&&rg.iChnk.iMemLen==0x800); // 2K
	test(rg.iDeviceType==EMDeviceFunSpec);
	test(pccdDrv.GetRegion(KFunc,&rgBuf)==KErrNotFound);

	TInt reg=0;
	test.Next(_L("Request card configuration."));
	test(pccdDrv.ReadConfigReg(KFunc,0,reg)==KErrGeneral);
	test(pccdDrv.ReqConfig(KFunc,2)==KErrNone);
	test(pccdDrv.ReadConfigReg(KFunc,0,reg)==KErrNone);
	test(reg==2);
	test(pccdDrv.ReqConfig(KFunc,1)==KErrInUse);
	pccdDrv.Reset();							// Release config
	test(pccdDrv.ReqConfig(KFunc,1)==KErrNone);

	test.Next(_L("Configuration register read/write test.")); // Set IoIs8 bit in ConfigAndStatus register
	test(pccdDrv.ReadConfigReg(KFunc,1,reg)==KErrNone);
	test.Printf(_L("reg=0x%x"),reg);
	test((reg&0x20)==0);
	reg|=0x20;
	test(pccdDrv.WriteConfigReg(KFunc,1,reg)==KErrNone);
	TInt newReg=0;
	test(pccdDrv.ReadConfigReg(KFunc,1,newReg)==KErrNone);
	if(reg!=newReg)
		test.Printf(_L("reg=0x%x, new reg=0x%x"),reg,newReg);
	test(reg==newReg);
	reg&=0xDF;
	test(pccdDrv.WriteConfigReg(KFunc,1,reg)==KErrNone);

	test.Next(_L("Close/free device"));
	pccdDrv.Close();
	r=User::FreeLogicalDevice(_L("PccdIf"));
	test(r==KErrNone);

	test.End();

#if defined (__WINS__)
	for (i=0;i<KMaxLocalDrives;i++)
		Drive[i].Disconnect();
#endif
	return(0);
	}
  
