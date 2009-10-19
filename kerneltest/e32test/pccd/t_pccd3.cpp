// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\pccd\t_pccd3.cpp
// 
//

#include <e32test.h>
#include <e32svr.h>
#include <e32hal.h>
#include <e32uid.h>
#include <d_pccdif.h>
#include <hal.h>
#define TEST_NOTIFIER

LOCAL_D RTest test(_L("T_PCCD3"));
LOCAL_D TBool SimpleCDScheme;
LOCAL_D TInt TotalSockets;
LOCAL_D RPcCardCntrlIf PccdDrv[KMaxPBusSockets];

const TSocket KSocket=0;
const TInt KFunc=0;


#ifdef TEST_NOTIFIER
LOCAL_C void theMediaChangeNotifierTests()
//
// Test UserSvr::MediaChangeNotify() and UserSvr::ForceRemountMedia().
//
	{

	test.Next(_L("The media change notifier(s)"));

	// Test requesting on a non-removable device
	TRequestStatus rs[KMaxPBusSockets];
	test(UserSvr::MediaChangeNotify(EFixedMedia0,&rs[0])==KErrNotSupported);

#ifndef _DEBUG
	test(UserSvr::MediaChangeNotify(ERemovableMedia0,&rs[0])==KErrInUse);
#else
	test.Printf( _L("<<<THIS WILL BREAK F32 MEDIA CHANGE NOTIFICATION>>>\r\n"));
	TInt socket;
	TMediaDevice md;
	for (socket=0;socket<TotalSockets;socket++)
		{
		// Register the notifier on all sockets here - out of the main loop to 
		// test operation with multiple outstanding requests on different sockets
		md=(TMediaDevice)(ERemovableMedia0+socket);
		test(UserSvr::MediaChangeNotify(md,&rs[socket])==KErrNone);
		}

	for (socket=0;socket<TotalSockets;socket++)
		{
		md=(TMediaDevice)(ERemovableMedia0+socket);
		// Perform the entire test twice
		for (TInt cycleCount=1;cycleCount<=2;cycleCount++)
			{
			if (cycleCount>1)
				{
				// Second time around we need to make a request 
				test(UserSvr::MediaChangeNotify(md,&rs[socket])==KErrNone);
				}
			UserSvr::ForceRemountMedia(md); 	// Generate media change	
			User::WaitForRequest(rs[socket]);			// From door open
			test(rs[socket].Int()==KErrNone);
			test.Printf( _L("Media change notify on Socket %d\r\n"),socket);
			if (!SimpleCDScheme)
				{
				// For CD scheme, the open/close occur too close together to
				// detect separately
				test(UserSvr::MediaChangeNotify(md,&rs[socket])==KErrNone);
				User::WaitForRequest(rs[socket]);		// From door close again
				test(rs[socket].Int()==KErrNone);
				test.Printf( _L("Media change notify on Socket %d\r\n"),socket);
				}
			}
		}

	// Test requesting on an invalid socket for this platform
	if (TotalSockets<KMaxPBusSockets)
		test(UserSvr::MediaChangeNotify((TMediaDevice)(ERemovableMedia0+TotalSockets),&rs[0])==KErrGeneral);
	#endif
	}

#else
LOCAL_C void theMediaChangeNotifierTests()
//
// Test UserSvr::MediaChangeNotify() and UserSvr::ForceRemountMedia().
//
	{

	test.Next(_L("The media change notifier(s)"));

	// Test requesting on a non-removable device
	TRequestStatus rs;
	test(UserSvr::MediaChangeNotify(EFixedMedia0,&rs)==KErrNotSupported);

#ifndef _DEBUG
	test(UserSvr::MediaChangeNotify(ERemovableMedia0,&rs)==KErrInUse);
#else
	test.Printf( _L("<<<MEDIA CHANGE NOTIFICATION TESTS DISABLED>>>\r\n"),i);
#endif
	}
#endif

LOCAL_C void thePccdControllerMediaChangeEventTests()
//
// Test registering on media change events
//
	{

	test.Next(_L("PC Card Controller - Media change notification"));
	if (SimpleCDScheme)
		{
		TRequestStatus rs[KMaxPBusSockets];
		TInt i;
		for (i=0;i<TotalSockets;i++)
			test(PccdDrv[i].RegisterEvent(EPcCardEvMediaChange,&rs[i])==KErrNone);

		for (i=0;i<TotalSockets;i++)
			{
			test.Printf( _L("<<<Insert the card in socket %d >>>\r\n"),i);
			User::WaitForRequest(rs[i]);
			test(rs[i].Int()==KErrNone);
			}
		}

	else
		{
		TRequestStatus rs;
		test(PccdDrv[0].RegisterEvent(EPcCardEvMediaChange,&rs)==KErrNone);

#if defined (__WINS__)
		test.Printf( _L("<<<Hit F5>>>\r\n"));
#else
		test.Printf( _L("<<<Open and close CF card door>>>\r\n"));
		test.Printf( _L("<<<Machine will turn off as soon as door is opened>>>\r\n"));
#endif
		User::WaitForRequest(rs);
		test(rs.Int()==KErrNone);
#if defined (__WINS__)
		// Delay power off until after F5 key up event. Key up doesn't occur when in standby.
		// Without this the power-on test fails because the simulated door state is stuck open.
		User::After(500000);
#endif
		}
	}

LOCAL_C void thePccdControllerPowerEventTests()
//
// Test registering on power events
//
	{

	test.Next(_L("PC Card Controller - Power-off notification."));
	RTimer timer;
	TRequestStatus prs, trs;
	test(timer.CreateLocal()==KErrNone);
	test(PccdDrv[0].RegisterEvent(EPcCardEvPwrDown,&prs)==KErrNone);
	TTime tim;
	tim.HomeTime();
	tim+=TTimeIntervalSeconds(8);
	timer.At(trs,tim);
	UserHal::SwitchOff();
	User::WaitForRequest(prs);
	test(prs.Int()==KErrNone);
	User::WaitForRequest(trs);
	test(trs.Int()==KErrNone);

	test.Next(_L("PC Card Controller - Power-on notification."));
	test(PccdDrv[0].RegisterEvent(EPcCardEvPwrUp,&prs)==KErrNone);
	tim.HomeTime();
	tim+=TTimeIntervalSeconds(8);
	timer.At(trs,tim);
	UserHal::SwitchOff();
	User::WaitForRequest(prs);
	test(prs.Int()==KErrNone);
	User::WaitForRequest(trs);
	test(trs.Int()==KErrNone);
	}

LOCAL_C void thePccdControllerStatusChangeEventTests()
//
// Test registering on status change events
//
	{

	TRequestStatus rs;
	if (!SimpleCDScheme)
		{
		// We're going to ask tester to remove CF card in order to generate a card status change 
		// notification. However, on P2s this involves opening the CF card door first. Because of
		// the media notifier (F32), opening door gives us a spurious RegisterEvent() notification.
		test.Printf( _L("<<<Open (don't close) media door>>>\r\n"));
		test(PccdDrv[0].RegisterEvent(EPcCardEvMediaChange,&rs)==KErrNone);
		User::WaitForRequest(rs);
		test(rs.Int()==KErrNone); 
		}

	TInt i;
	for (i=(TotalSockets-1);i>=0;i--)
		{
		test.Next(_L("Card status change notification - card removal"));
		test(PccdDrv[i].RegisterEvent(EPcCardEvIndChange,&rs)==KErrNone);
		test.Printf( _L("<<<Remove the card from socket %d>>>\r\n"),i);
		User::WaitForRequest(rs);
		test(rs.Int()==KErrNone);

		test.Next(_L("Card status change notification - card insertion"));
		User::After(200000);	// 0.2s
		TInt j=(i>0)?(i-1):i;
		test(PccdDrv[j].RegisterEvent(EPcCardEvIndChange,&rs)==KErrNone);
		test.Printf( _L("<<<Insert the card back into socket %d>>>\r\n"),j);
		User::WaitForRequest(rs);
		test(rs.Int()==KErrNone);
		}

	if (!SimpleCDScheme)
		{
		test.Printf( _L("<<<Close) CF card door>>>\r\n"));
		test(PccdDrv[0].RegisterEvent(EPcCardEvPwrUp,&rs)==KErrNone);
		User::WaitForRequest(rs);
		test(rs.Int()==KErrNone); 
		}
	}

LOCAL_C void thePccdControllerCardReadyEventTests()
//
// Test registering on ready events (tests 1 socket only!!!!).
//
	{

	test.Next(_L("Card ready notification"));
	TRequestStatus rs;
	TInt r;
	r=PccdDrv[0].RegisterEvent(EPcCardEvRdyChange,&rs);
	if (r==KErrNotSupported)
		{
		test.Printf( _L("<<<Not supported on this platform>>>\r\n"));
		return;
		}
	test(r==KErrNone);

	TPcCardStatus s;
	TSocketSignals ind;
	test(PccdDrv[0].SocketInfo(s,ind)==KErrNone);
	test(s==EPcCardNotReady);
	test(ind.iCardDetected);
	
	TRequestStatus prs;
	PccdDrv[0].PwrUp(&prs);
	User::WaitForRequest(rs);
	test(rs.Int()==KErrNone);
	User::WaitForRequest(prs);
	test(rs.Int()==KErrNone);
	}

GLDEF_C TInt E32Main()
	{

	TInt r;
#if defined (__WINS__)
	// Connect to all the local drives first as will be the case in ARM
	TBusLocalDrive Drive[KMaxLocalDrives];
	TBool ChangedFlag[KMaxLocalDrives];
	TInt j;
	for (j=0;j<KMaxLocalDrives;j++)
		Drive[j].Connect(j,ChangedFlag[j]);
#endif

	test.Title();
//
	test.Start(_L("Read machine info."));
	// Find out what sort of media change architecture we have. How many sockets
	// there are and whether we have a full blown media door scheme or just a system 
	// using the PC Card CD signals. The later information we can't read from the
	// machine info, the test program just has to be updated with the machine name of 
	// any machine which employs the CD scheme.
	SimpleCDScheme=EFalse;
	TInt muid=0;
	r=HAL::Get(HAL::EMachineUid, muid);
	test(r==KErrNone);
	if (muid==HAL::EMachineUid_Brutus)
		SimpleCDScheme=ETrue;
	TDriveInfoV1Buf dinfo;
	UserHal::DriveInfo(dinfo);
	TotalSockets=dinfo().iTotalSockets;

	if (SimpleCDScheme)
		test.Printf( _L("<<<Remove all PC/CF cards - hit a key>>>\r\n"));
	else
		{
		if (TotalSockets>0)
			test.Printf( _L("<<<Insert the card in socket %d - hit a key>>>\r\n"),(TotalSockets-1));
		}
	test.Getch();

	test.Next(_L("Load/open logical devices"));
	r=User::LoadLogicalDevice(_L("D_PCCDIF"));
	test(r==KErrNone||r==KErrAlreadyExists);
	TInt i;
	for (i=0;i<TotalSockets;i++)
		{
		r=PccdDrv[i].Open(i,PccdDrv[i].VersionRequired());
		test(r==KErrNone);
		}

	// Test UserSvr::MediaChangeNotify() and UserSvr::ForceRemountMedia().
	theMediaChangeNotifierTests();

	// Test registering on PC Card Controller events
	thePccdControllerMediaChangeEventTests();
	thePccdControllerPowerEventTests();
#if defined(__EPOC32__)
	thePccdControllerStatusChangeEventTests();
	thePccdControllerCardReadyEventTests();
#endif

	test.Next(_L("Close/free devices"));
	for (i=0;i<TotalSockets;i++)
		PccdDrv[i].Close();
	r=User::FreeLogicalDevice(_L("PccdIf"));
	test(r==KErrNone);

	test.End();

#if defined (__WINS__)
	for (i=0;i<KMaxLocalDrives;i++)
		Drive[i].Disconnect();
#endif
	return(0);
	}
  
