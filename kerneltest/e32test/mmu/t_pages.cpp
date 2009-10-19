// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\mmu\t_pages.cpp
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include "d_shadow.h"

RShadow Shadow;
LOCAL_D TUint Read(TUint anAddr)
	{
	return Shadow.Read(anAddr);
	}
TUint PageTables = 0;
TCpu  Cpu = ECpuUnknown;
TInt  CpuArc = 0;
TInt  CpuSpecial = 0;
TUint ControlReg = 0;

const TUint KXPbitM = 0x800000;

LOCAL_C void ProcessCBTEX(TDes& aDes, TUint aCb, TUint aTex)
	{
	
	const TPtrC KTEXCB[9] = {_L("StOr"),_L("ShDv"),_L("WTRA"),_L("WBRA"),_L("NoCa"),_L("Resv"),_L("ImpD"),_L("WBWA"),_L("NSDv")};
	const TPtrC KCacheP[4] = {_L("NoCa"),_L("WBWA"),_L("WTRA"),_L("WBRA")};	
	aCb = aCb >> 2;
	
	TUint texCB = aCb | ((aTex&7) << 2);
	
	if ((texCB<9) && (CpuArc>5))
		aDes.Append(KTEXCB[texCB]);
	else if ((aTex&4) && (CpuArc>5))
		{
		aDes.Append(KCacheP[aTex&3]);
		aDes.Append(TChar('/'));
		aDes.Append(KCacheP[aCb]);
		}
	else
		{
		if (CpuArc>5)
			{
			if (aTex&4)
				aDes.Append(TChar('1'));
			else 
				aDes.Append(TChar('0'));
			
			if (aTex&2)
				aDes.Append(TChar('1'));
			else 
				aDes.Append(TChar('0'));	

			if (aTex&1)
				aDes.Append(TChar('1'));
			else 
				aDes.Append(TChar('0'));
			}
		else if (CpuSpecial == 2)
			{
			if (aTex==1)
				aDes.Append(TChar('X'));
			else if (aTex==0)
				aDes.Append(TChar('_'));
			else
				aDes.Append(TChar('?'));

			}
		
		if (aCb & 2)
			aDes.Append(TChar('C'));
		else
			aDes.Append(TChar('_'));
		if (aCb & 1)
			aDes.Append(TChar('B'));
		else
			aDes.Append(TChar('_'));
		}
	}


LOCAL_C void ProcessXNnGS(TDes& aDes, TUint anGS, TUint aXN)
	{
	if ((ControlReg & KXPbitM))
		{
		aDes.Append(TChar(' '));
		if (aXN )
			aDes.Append(_L("XN "));
		else
			aDes.Append(_L("__ "));
		
		if (anGS&2 )
			aDes.Append(_L("nG "));
		else
			aDes.Append(_L("__ "));
		
		if (anGS&1 )
			aDes.Append(TChar('S'));
		else
			aDes.Append(TChar('_'));
		
		}
	}


LOCAL_C void ProcessAP(TDes& aDes, TUint aAp)
	{
	const TPtrC KAccessP[7] = {_L(" RWNO"),_L(" RWRO"),_L(" RWRW"),_L(" Rsv0"),_L(" RONO"),_L(" RORO"),_L(" Rsv1")};
	const TPtrC KAccZrP[4] = {_L(" NONO"), _L(" RONO"), _L(" RORO"), _L(" Rsv2")};
	
	TUint access = aAp & 3;
	TUint apx = (aAp >> 3) & 4;
	
	if (((ControlReg & 0x300) !=0) && apx)
		{
			aDes.Append(_L(" Rsv3"));
		}
	else
		{	
		if ((ControlReg & KXPbitM))
			access |= apx;
		
		if (access==0)
			aDes.Append(KAccZrP[(ControlReg >> 8) & 3]);
		else 
			aDes.Append(KAccessP[access-1]);
		}
	}

LOCAL_C void ProcessPteSE(TUint aPte, TUint anAddr)
	{
	TUint type=aPte&3;
	TBuf<36> buf;
	switch(type)
		{
		case 0:
			// not present
			break;
		case 1:
			{
			// large page
			TUint phys=aPte & 0xffff0000;
			TUint ap0=(aPte>>4)&3;
			TUint ap1=(aPte>>6)&3;
			TUint ap2=(aPte>>8)&3;
			TUint ap3=(aPte>>10)&3;
			TUint tex=(aPte>>12)&0xf;
			ProcessCBTEX(buf,aPte&0xc,tex);
			ProcessAP(buf,ap0);
			ProcessAP(buf,ap1);
			ProcessAP(buf,ap2);
			ProcessAP(buf,ap3);
			RDebug::Print(_L("\t%08x  Lpage: phys=%08x, %S"),anAddr,phys,&buf);
			break;
			}
		case 2:
			{
			// small page
			TUint phys=aPte & 0xfffff000;
			TUint ap0=(aPte>>4)&3;
			TUint ap1=(aPte>>6)&3;
			TUint ap2=(aPte>>8)&3;
			TUint ap3=(aPte>>10)&3;
			ProcessCBTEX(buf,aPte&0xc,0);
			ProcessAP(buf,ap0);
			ProcessAP(buf,ap1);
			ProcessAP(buf,ap2);
			ProcessAP(buf,ap3);
			RDebug::Print(_L("\t%08x  Spage: phys=%08x, %S"),anAddr,phys,&buf);
			break;
			}
		case 3:
			{
			// extended small page
			TUint phys=aPte & 0xfffff000;
			TUint ap=(aPte>>4)&3;
			TUint tex=(aPte>>6)&0xf;
			ProcessCBTEX(buf,aPte&0xc,tex);
			ProcessAP(buf,ap);
			RDebug::Print(_L("\t%08x XSpage: phys=%08x, %S"),anAddr,phys,&buf);
			break;
			}
		}
	}


LOCAL_C void ProcessPteSD(TUint aPte, TUint anAddr)
	{
	if ((aPte&3) != 0)
		{
		
		TBuf<36> buf;
		TUint ap=(aPte>>4)&23;	
		
		if (aPte&2) // XS-Page
			{
			// extended small page
			TUint phys=aPte & 0xfffff000;
			TUint tex=(aPte>>6)&0x7;
			ProcessCBTEX(buf,aPte&0xc,tex);
			ProcessAP(buf,ap);
			ProcessXNnGS(buf, (aPte>>10)&3, aPte &1);
			RDebug::Print(_L("\t%08x XSpage: phys=%08x, %S"),anAddr,phys,&buf);
			}
		else // L-Page
			{
					
			// large page
			TUint phys=aPte & 0xffff0000;
			TUint tex=(aPte>>12)&0x7;
			ProcessCBTEX(buf,aPte&0xc,tex);
			ProcessAP(buf,ap);
			ProcessXNnGS(buf, (aPte>>10)&3, (aPte >> 15) &1);
			RDebug::Print(_L("\t%08x  Lpage: phys=%08x, %S"),anAddr,phys,&buf);
			}		
		
		}// Else "Fault" - Not Present

	}


LOCAL_C void ProcessPde(TUint aPde, TUint anAddr)
	{
	TUint type=aPde&3;
	TBuf<24> buf;
	switch(type)
		{
		case 0:
			// not present
			if (aPde) 
				RDebug::Print(_L(" Not Present\n"));
			break;
		case 1:
			{
			// page table
			TUint ptphys=aPde & 0xfffffc00;
			TUint domain=(aPde>>5)&15;
			TUint P=(aPde&0x200);
			TUint ptpgphys=ptphys & 0xfffff000;
			TUint ptlin=0;
			
			TInt i;
			for (i=0; i<256; i++)
				{
				if ((Read(PageTables+i*4)&0xfffff000)==ptpgphys)
					{
					if (ptlin==0)
						ptlin=PageTables+(i<<12)+(ptphys & 0xc00);
					else
						RDebug::Print(_L("WARNING Multiple page tables found! alt = %08x\n"), PageTables+(i<<12)+(ptphys & 0xc00));
					}
				}
			if (ptlin)
				{
				RDebug::Print(_L("%08x page table: phys=%08x, domain %2d, %s, page table lin=%08x"),anAddr,ptphys,domain,P?L"ECC":L"No ECC",ptlin);
				for (i=0; i<256; i++)
					{
					TUint addr=anAddr+(i<<12);
					TUint pte=Read(ptlin+i*4);
					
					if (ControlReg & KXPbitM)
						ProcessPteSD(pte,addr);
					else
						ProcessPteSE(pte,addr);
					}
				}
			else
				RDebug::Print(_L("%08x page table: phys=%08x, domain %2d, %s, page table not found"),anAddr,ptphys,domain,P?L"ECC":L"No ECC");
				
			break;
			}
		case 2:
			{	
			// section
			TUint phys=aPde & 0xfff00000;
			TUint perm=(aPde>>10)&0x23;
			TUint P=(aPde&0x200);
			TUint domain=(aPde>>5)&15;
			ProcessCBTEX(buf,aPde&0xc,(aPde>>12)&0xf);  // tex is bigger on xscale, but bit is masked off later.
			ProcessAP(buf,perm);
			ProcessXNnGS(buf, (aPde>>16)&3, (aPde>>4)&1);
			
			if ((ControlReg & KXPbitM) && (aPde & 0x40000))
				{
				domain = (domain << 4) | ((aPde >> 20) &15);
				RDebug::Print(_L("%08x Supersectn: phys=%08x, base %4d, %s, %S"),anAddr,phys,domain,P?L"ECC":L"No ECC",&buf);
				}
			else
				{			
				RDebug::Print(_L("%08x section   : phys=%08x, domain %2d, %s, %S"),anAddr,phys,domain,P?L"ECC":L"No ECC",&buf);
				}
			break;
			}
		default:
			// invalid
			RDebug::Print(_L("PDE for %08x invalid, value %08x"),anAddr,aPde);
			break;
		}
	}

const TInt KNotInInvalid = -2;
TInt StartInvalid=KNotInInvalid;
TInt StartInvalidBase=0;
TInt EndInvalidBase=0;

void DisplayNotPresent(TInt aCurrentPd)
	{
	aCurrentPd--;
	if (StartInvalid!=KNotInInvalid) // Display any Invalid ranges.
		{
		if (StartInvalid == aCurrentPd)
			RDebug::Print(_L("\nPage Directory 0x%x:  Base=0x%x - Not Present.\n"), StartInvalid, StartInvalidBase);
		else
			RDebug::Print(_L("\nPage Directorys 0x%x-0x%x:  Bases=0x%x-0x%x - Not Present.\n"), StartInvalid, aCurrentPd, StartInvalidBase, EndInvalidBase);
		
		StartInvalid=KNotInInvalid;
		}
	}

void ProcessPd(TUint aPd)
	{
	TUint i;
	TUint pdSize;
	TUint pdBase;
	TUint offset;
	
	TInt err = Shadow.GetPdInfo(aPd, pdSize, pdBase, offset);
	
	if (err==KErrNone)
		{
		DisplayNotPresent(aPd);
			
		if (aPd==KGlobalPageDirectory)
			RDebug::Print(_L("Global Page Directory: Base=0x%x, Entries=0x%x, Start index=0x%x\n"), pdBase, pdSize, offset);
		else
			RDebug::Print(_L("\nPage Directory 0x%x: Base=0x%x, Entries=0x%x %x\n"), aPd, pdBase, pdSize, offset);
		
		if (Cpu == ECpuArm)
			{		

			for (i=0; i<pdSize; i++)
				{
				TUint addr=(i+offset)<<20;
				TUint pde=Read(pdBase+i*4);
				ProcessPde(pde,addr);
				}
			}
			else
				RDebug::Print(_L("Cannot display pde for this CPU"));
		}
	else
		{ // Dont list all invalid PDs - there are too many.
		if (StartInvalid==KNotInInvalid)
			{
			StartInvalidBase = pdBase;
			StartInvalid = aPd;
			}
		else
			{
			EndInvalidBase=pdBase;
			}
				
		}
	}

GLDEF_C TInt E32Main()
	{
	TUint mmuId=0;
	TUint cacheType=0;

	
	
	TInt r=User::LoadLogicalDevice(_L("D_SHADOW"));
	if ((r!=KErrNone) && (r!=KErrAlreadyExists))
		User::Panic(_L("T_PAGES0"),r);
	r=Shadow.Open();
	if (r!=KErrNone)
		User::Panic(_L("T_PAGES1"),r);

	Shadow.GetMemoryArchitecture(Cpu, ControlReg);
	switch (Cpu)
		{
		case ECpuArm:
			{	
			mmuId=Shadow.MmuId();
			TUint implementor= (mmuId>>24);
			
			if (implementor==0x44)
				CpuSpecial = 1;
			
			switch ((mmuId>>12)&15)
				{
				case 0: // Pre-ARM7
					if ((mmuId>>4)==0x4156030)
						CpuArc = 2;
					else if ((mmuId>>8)==0x415606)
						CpuArc = 3;
					break;
				case 7: // Mid-ARM7
					CpuArc = 3;
				default:// Post-ARM7
					TUint arc = (mmuId >>16) &15;
					if (arc<3)
						{
						CpuArc = 4;
						if ((implementor==0x69) && (arc==4))
							CpuSpecial=1;
						}
					else if (arc<7)
						{
						CpuArc = 5;
						if ((implementor==0x69) && (arc==5))
							CpuSpecial=2;
						}
					else if (arc==7)
						CpuArc = 6;
					else 
						CpuArc = arc;
				}
			
			switch (CpuSpecial)
				{
				case 1:RDebug::Print(_L("\nCPU = ARMv%d (StrongArm), ControlRegister = 0x%x "),CpuArc, ControlReg);
					break;
				case 2:RDebug::Print(_L("\nCPU = ARMv%d (XScale), ControlRegister = 0x%x "),CpuArc, ControlReg);
					break;
				default: 
					if (CpuArc<7)
						RDebug::Print(_L("\nCPU = ARMv%d, ControlRegister = 0x%x "),CpuArc, ControlReg);
					else
						RDebug::Print(_L("\nCPU = ARM (#%d), ControlRegister = 0x%x "),CpuArc, ControlReg);
				}	
			
			RDebug::Print(_L("(MMU=%d, Alignment Checking=%d, Write Buffer=%d, System Protection=%d, "),
				ControlReg & 1,(ControlReg>>1)&1,(ControlReg>>3)&1, (ControlReg>>8)&1);
							
			if (ControlReg & KXPbitM)
				RDebug::Print(_L("ROM Protection=%d, Subpages Disabled, Exception Endian=%d)\n"), (ControlReg>>9)&1, (ControlReg>>25)&1);
			else
				RDebug::Print(_L("ROM Protection=%d, Subpages Enabled, Exception Endian=%d)\n"), (ControlReg>>9)&1, (ControlReg>>25)&1);
			
			RDebug::Print(_L("MMU ID=%08X"),mmuId);
			
			
					
			cacheType=Shadow.CacheType();
			RDebug::Print(_L("CACHE TYPE=%08X"),cacheType);
			break;
			
			}
					
		case ECpuX86:
			RDebug::Print(_L("\nCPU = x86\n"));
			break;
		case ECpuUnknown:
		default:
			RDebug::Print(_L("\nCPU = Unknown, Flags = 0x%x\n"), ControlReg);
		
		}
		
	TUint numPages = 0;
	TMemModel memModel = Shadow.GetMemModelInfo(PageTables, numPages);	
	
	switch (memModel)
		{
		case EMemModelMoving: RDebug::Print(_L("Moving Memory Model.\n"));
			break;
		case EMemModelMultiple : RDebug::Print(_L("Multiple Memory Model.\nMax number of PageDirectorys=0x%x.\n"), numPages);
			break;
		case EMemModelFlexible : RDebug::Print(_L("Flexible Memory Model.\nMax number of PageDirectorys=0x%x.\n"), numPages);
			break;
		default:
			RDebug::Print(_L("Unknown Memory Model.\n"));
			return KErrNone;
		}
	
	ProcessPd(KGlobalPageDirectory);

	if (memModel==2)
		{
		TUint i;
		for (i=0; i<numPages; i++)
			{
				ProcessPd(i);
			}
		DisplayNotPresent(numPages);
		}
	Shadow.Close();
	User::FreeLogicalDevice(_L("Shadow"));
	return KErrNone;
	}
