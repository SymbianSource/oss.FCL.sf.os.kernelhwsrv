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
// e32test\pccd\t_pccd2.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include <d_pccdif.h>

LOCAL_D RTest test(_L("T_PCCD2"));
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
//
	test.Start(_L("Load/open logical device"));
	TInt r;
	r=User::LoadLogicalDevice(_L("D_PCCDIF"));
	test(r==KErrNone||r==KErrAlreadyExists);
	RPcCardCntrlIf pccdDrv;
	r=pccdDrv.Open(KSocket,pccdDrv.VersionRequired());
	test(r==KErrNone);
//
	test.Next(_L("Power card and configure."));
	UserSvr::ForceRemountMedia(ERemovableMedia0); // Media change - ensures test always starts from same state
	User::After(300000);	// Allow 0.3s after power down for controller to detect door closed.
    pccdDrv.Reset();
	TRequestStatus mrs;
	test(pccdDrv.RegisterEvent(EPcCardEvMediaChange,&mrs)==KErrNone);
	TRequestStatus prs;
	pccdDrv.PwrUp(&prs);
	User::WaitForRequest(prs);
	test(prs.Int()==KErrNone);
	TPcCardStatus s;
	TSocketSignals ind;
	test(pccdDrv.SocketInfo(s,ind)==KErrNone);
	test(s==EPcCardReady);

	TPcCardTypePckg tyBuf;
	TPcCardType &ty=tyBuf();
	test(pccdDrv.CardInfo(&tyBuf)==KErrNone);
	test(ty.iFuncType[0]==EFFixedDiskCard);
	test(pccdDrv.ReqConfig(KFunc,1)==KErrNone);

	// Test we can get the same chunk as the controllers attrib. chunk (its shareable).
	test.Next(_L("Request 64K attribute chunk at 0H."));
	TPcCardChnk ch;
	ch.iMemType=EPcCardAttribMem;
	ch.iMemBaseAddr=0;
	ch.iMemLen=0x10000;
	TInt win0,win1,win2;
	test(pccdDrv.ReqMem(ch,EAccSpeed600nS,win0)==KErrNone);
	TBuf8<8> rdBuf;
	rdBuf.Fill(0,8);
	test(pccdDrv.ReadMem(win0,0,8,rdBuf)==KErrNone);
//	test(rdBuf.Compare(_L("\x01\xFF\x04\xFF\xDF\xFF\x72\xFF"))==0);
	test(rdBuf[0]==0x01&&rdBuf[2]==0x04&&rdBuf[4]==0xDF&&rdBuf[6]==0x72);

	// Test we can get small chunk within the controllers attrib. chunk (Win0 already allocated).
	test.Next(_L("Request 16byte attribute chunk at 10H."));
	ch.iMemType=EPcCardAttribMem;
	ch.iMemBaseAddr=0x10;
	ch.iMemLen=0x10;
	test(pccdDrv.ReqMem(ch,EAccSpeed300nS,win1)==KErrAccessDenied); // 1st chunk isn't shared
	pccdDrv.RelMem(win0);
	test(pccdDrv.ReqMem(ch,EAccSpeed300nS,win1)==KErrNone);
	rdBuf.Fill(0,8);
	test(pccdDrv.ReadMem(win1,0,8,rdBuf)==KErrNone);
//	test(rdBuf.Compare(_L("\x03\xFF\xD9\xFF\x01\xFF\xFF\xFF"))==0);
	test(rdBuf[0]==0x03&&rdBuf[2]==0xD9&&rdBuf[4]==0x01&&rdBuf[6]==0xFF);

	// Test we can get a second small chunk, also within the controllers attrib. chunk but
	// not clashing with previous chunk (Win1 already allocated).
	test.Next(_L("Request 16byte attribute chunk at 20H."));
	ch.iMemType=EPcCardAttribMem;
	ch.iMemBaseAddr=0x20;
	ch.iMemLen=0x10;
	test(pccdDrv.ReqMem(ch,EAccSpeed300nS,win0)==KErrNone);
	rdBuf.Fill(0,8);	
	test(pccdDrv.ReadMem(win0,0,8,rdBuf)==KErrNone);
//	test(rdBuf.Compare(_L("\x20\xFF\x06\xFF\x45\xFF"))==2&&rdBuf[6]==0x00&&rdBuf[7]==0xFF); // 'Null causes problems with compare
	test(rdBuf[0]==0x20&&rdBuf[2]==0x06&&rdBuf[4]==0x45&&rdBuf[6]==0x00);

	// Test that requesting a chunk which lies partly but not entirely within the controllers
	// chunk fails (Win0/Win1 already allocated).
	test.Next(_L("Request 64K attribute chunk at 800H."));
	test(pccdDrv.ReqMem(ch,EAccSpeed300nS,win2)==KErrInUse);
	pccdDrv.RelMem(win1);
	ch.iMemType=EPcCardAttribMem;
	ch.iMemBaseAddr=0x800;
	ch.iMemLen=0x10000;
	test(pccdDrv.ReqMem(ch,EAccSpeed300nS,win1)==KErrAccessDenied);

	// Test we can get a small chunk beyond the controllers chunk (Win0 already allocated).
	test.Next(_L("Request 16byte attribute chunk at 10800H."));
	ch.iMemType=EPcCardAttribMem;
	ch.iMemBaseAddr=0x10800;
	ch.iMemLen=0x10;
	test(pccdDrv.ReqMem(ch,EAccSpeed300nS,win1)==KErrNone);
	pccdDrv.RelMem(win1);

	// Test we can get a large chunk beyond the controllers chunk (Win0 already allocated).
	test.Next(_L("Request 4K attribute chunk at 10800H."));
	ch.iMemType=EPcCardAttribMem;
	ch.iMemBaseAddr=0x10800;
	ch.iMemLen=0x1000;
	test(pccdDrv.ReqMem(ch,EAccSpeed300nS,win1)==KErrNone);
	pccdDrv.RelMem(win1);

	// Test we can get a chunk in a different memory type (Win0 already allocated).
	test.Next(_L("Request 16byte IO chunk at 0H."));
	ch.iMemType=EPcCardIo8Mem;
	ch.iMemBaseAddr=0;
	ch.iMemLen=0x10;
//	test(pccdDrv.ReqMem(ch,EAccSpeed200nS,win1)==KErrNone); // ???
	test(pccdDrv.ReqMem(ch,EAccSpeed250nS,win1)==KErrNone);

	// Win0/Win1 allocated
	TBuf8<8> wrBuf;
	TInt reg=0;
	test.Next(_L("Write/Read from ATA registers."));
	test(pccdDrv.ReadConfigReg(KFunc,0,reg)==KErrNone); // Check its still configured
	test(reg==1);
	wrBuf.Copy(_L("\x04\x03\x02\x01"));
	wrBuf.SetLength(4);
	test(pccdDrv.WriteMem(win1,2,wrBuf)==KErrNone); // 4 bytes starting at Sector count
	rdBuf.Fill(0,4);	
	test(pccdDrv.ReadMem(win1,2,4,rdBuf)==KErrNone);
	test(rdBuf.Compare(_L8("\x04\x03\x02\x01"))==0);

	// Win0/Win1 allocated
	test.Next(_L("Attempt to access window after power down."));
	RTimer timer;
	TRequestStatus trs;
	test(timer.CreateLocal()==KErrNone);
	TTime tim;
	tim.HomeTime();
	tim+=TTimeIntervalSeconds(8);
	timer.At(trs,tim);
	UserHal::SwitchOff();
	User::WaitForRequest(trs);
	test(trs.Int()==KErrNone);
	pccdDrv.PwrUp(&prs);
	User::WaitForRequest(prs);
	test(prs.Int()==KErrNone);
	// Check its been re-configured
	reg=0;
	test(pccdDrv.ReadConfigReg(KFunc,0,reg)==KErrNone);
	test(reg==1);
	// Check that window still OK after power down
	test(pccdDrv.WriteMem(win1,2,wrBuf)==KErrNone); // 4 bytes starting at Sector count
	rdBuf.Fill(0,4);	
	test(pccdDrv.ReadMem(win1,2,4,rdBuf)==KErrNone);
	test(rdBuf.Compare(_L8("\x04\x03\x02\x01"))==0);

	// Win0/Win1 allocated
	test.Next(_L("Attempt to access window after media change."));
	UserSvr::ForceRemountMedia(ERemovableMedia0); // Generate media change
	User::WaitForRequest(mrs);
	if (mrs.Int()!=KErrNone)
		{
	    pccdDrv.Close();
		test(0);
		} 
	User::After(300000);	// Allow 0.3s after power down for controller to detect door closed.
	pccdDrv.PwrUp(&prs);
	User::WaitForRequest(prs);
	if (prs.Int()!=KErrNone)
		{
	    pccdDrv.Close();
		test(0);
		}
	// First access following media change must be supervisor call
	if (pccdDrv.CardInfo(&tyBuf)!=KErrNone)
		{
	    pccdDrv.Close();
		test(0);
		}
	// Check its not been re-configured
	if (pccdDrv.ReadConfigReg(KFunc,0,reg)!=KErrGeneral) 
		{
	    pccdDrv.Close();
		test(0);
		} 
	if (pccdDrv.ReadMem(win1,2,4,rdBuf)!=KErrNotReady)
		{
	    pccdDrv.Close();
		test(0);
		}

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
  
