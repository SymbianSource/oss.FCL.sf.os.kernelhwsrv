// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\pbus\pccard\epoc\pccd_init.cpp
// 
//

#include <pccd_ifc.h>
#include "pbusmedia.h"

GLDEF_D TPcCardControllerInterface* ThePccdCntrlInterface=NULL;

EXPORT_C TInt TPcCardControllerInterface::Create()
//
// Allocate any resources. Only done once on kernel initialization so don't
// worry about cleanup if it fails.
//
	{
	__KTRACE_OPT(KPBUS1,Kern::Printf(">TPcCardControllerInterface::Create"));
	ThePccdCntrlInterface=this;
	Init();
	TInt r=KErrNone;
	SMediaDeviceInfo mdi;
	TInt i;
	for (i=0; i<KMaxPBusSockets && r==KErrNone; i++)
		{
		if (IsPcCardSocket(i,mdi))
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf("Socket %d is PC card",i));
			DPcCardSocket* pS=PccdIfc::NewSocket(i);
			if (!pS)
				{
				r=KErrNoMemory;
				break;
				}
			TheSockets[i]=pS;
			TInt mcid=MediaChangeFromSocket(i);
			__KTRACE_OPT(KPBUS1,Kern::Printf("Socket %d Media Change %d",i,mcid));
			DPcCardMediaChange* pM=(DPcCardMediaChange*)TheMediaChanges[mcid];
			if (!pM)
				{
				__KTRACE_OPT(KPBUS1,Kern::Printf("New Media Change"));
				pM=PccdIfc::NewMediaChange(mcid);
				if (!pM)
					{
					r=KErrNoMemory;
					break;
					}
				TheMediaChanges[mcid]=pM;
				__KTRACE_OPT(KPBUS1,Kern::Printf("Media Change %d at %08x",mcid,pM));
				r=pM->Create();
				if (r!=KErrNone)
					break;
				}
			else
				{
				__KTRACE_OPT(KPBUS1,Kern::Printf("Media Change %d already exists at %08x",mcid,pM));
				++pM->iReplyCount;
				}
			TInt vcc=VccFromSocket(i);
			__KTRACE_OPT(KPBUS1,Kern::Printf("Socket %d Vcc %d",i,vcc));
			DPcCardVcc* pV=(DPcCardVcc*)TheVccs[vcc];
			if (!pV)
				{
				__KTRACE_OPT(KPBUS1,Kern::Printf("New Vcc"));
				pV=PccdIfc::NewVcc(vcc,mcid);
				if (!pV)
					{
					r=KErrNoMemory;
					break;
					}
				TheVccs[vcc]=pV;
				__KTRACE_OPT(KPBUS1,Kern::Printf("Vcc %d at %08x",vcc,pV));
				r=pV->Create();
				if (r!=KErrNone)
					break;
				}
			else 
				{
				__KTRACE_OPT(KPBUS1,Kern::Printf("Vcc %d already exists at %08x, mcid=%d",vcc,pV,pV->iMediaChangeNum));
// DISALLOW SHARED PSUs UNTIL SOMEONE NEEDS THEM
//				if (pV->iMediaChangeNum!=mcid)
//					{
					r=KErrInUse;
//					break;
//					}
				}
			r=pS->Create(mdi.iDeviceName);
			if (r!=KErrNone)
				break;
			pS->iMediaChangeNumber=mcid;
			pS->iMediaChange=pM;
			pS->iVcc=pV;
			pV->iSocket=pS;
			
			r=pS->Init();
			if (r!=KErrNone)		
				break;

			__KTRACE_OPT(KPBUS1,Kern::Printf("Socket %d Created OK",i));
			}
		else
			__KTRACE_OPT(KPBUS1,Kern::Printf("Socket %d not PC card",i));
		}
	
	for (i=0; i<KMaxPBusSockets && r==KErrNone; i++)
		{
		if (IsPcCardSocket(i,mdi))
			{
			DPBusSocket* pS=TheSockets[i];
			DPBusPrimaryMedia* pM=new DPBusPrimaryMedia(pS);
			if (!pM)
				return KErrNoMemory;
			r=LocDrv::RegisterMediaDevice(mdi.iDevice,mdi.iDriveCount,mdi.iDriveList,pM,mdi.iNumMedia,*mdi.iDeviceName);
			__KTRACE_OPT(KPBUS1,Kern::Printf("Registering PcCard device %S (socket %d) for %d drives returns %d",mdi.iDeviceName,pM->iSocket->iSocketNumber,mdi.iDriveCount,r));
			if (r!=KErrNone)
				return r;
			}
		}
	__KTRACE_OPT(KPBUS1,Kern::Printf("<TPcCardControllerInterface::Create, ret %d",r));
	return r;
	}

