// Copyright (c) 1998-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// hal\tsrc\t_newhal.cpp
// 
//

#define __E32TEST_EXTENSION__

#include <e32test.h>
#include <hal.h>

RTest test(_L("T_NEWHAL"));

const TText* AttributeNames[]=
	{
	_S("EManufacturer"),
	_S("EManufacturerHardwareRev"),
	_S("EManufacturerSoftwareRev"),
	_S("EManufacturerSoftwareBuild"),
	_S("EModel"),
	_S("EMachineUid"),
	_S("EDeviceFamily"),
	_S("EDeviceFamilyRev"),
	_S("ECPU"),
	_S("ECPUArch"),
	_S("ECPUABI"),
	_S("ECPUSpeed"),
	_S("ESystemStartupReason"),
	_S("ESystemException"),
	_S("ESystemTickPeriod"),		
	_S("EMemoryRAM"),
	_S("EMemoryRAMFree"),
	_S("EMemoryROM"),
	_S("EMemoryPageSize"),	
	_S("EPowerGood"),
	_S("EPowerBatteryStatus"),	
	_S("EPowerBackup"),
	_S("EPowerBackupStatus"),	
	_S("EPowerExternal"),
	_S("EKeyboard"),
	_S("EKeyboardDeviceKeys"),	
	_S("EKeyboardAppKeys"),
	_S("EKeyboardClick"),
	_S("EKeyboardClickState"),	
	_S("EKeyboardClickVolume"),
	_S("EKeyboardClickVolumeMax"),
	_S("EDisplayXPixels"),
	_S("EDisplayYPixels"),
	_S("EDisplayXTwips"),
	_S("EDisplayYTwips"),
	_S("EDisplayColors"),
	_S("EDisplayState"),
	_S("EDisplayContrast"),
	_S("EDisplayContrastMax"),
	_S("EBacklight"),
	_S("EBacklightState"),
	_S("EPen"),
	_S("EPenX"),
	_S("EPenY"),
	_S("EPenDisplayOn"),
	_S("EPenClick"),
	_S("EPenClickState"),
	_S("EPenClickVolume"),
	_S("EPenClickVolumeMax"),
	_S("EMouse"),
	_S("EMouseX"),
	_S("EMouseY"),			
	_S("EMouseState"),
	_S("EMouseSpeed"),
	_S("EMouseAcceleration"),
	_S("EMouseButtons"),
	_S("EMouseButtonState"),
	_S("ECaseState"),
	_S("ECaseSwitch"),
	_S("ECaseSwitchDisplayOn"),
	_S("ECaseSwitchDisplayOff"),
	_S("ELEDs"),
	_S("ELEDmask"),
	_S("EIntegratedPhone"),
	_S("EDisplayBrightness"),
	_S("EDisplayBrightnessMax"),
	_S("EKeyboardBacklightState"),
	_S("EAccessoryPower"),
	_S("ELanguageIndex"),
	_S("EKeyboardIndex"),
	_S("EMaxRAMDriveSize"),
	_S("EKeyboardState"),
	_S("ESystemDrive"),
	_S("EPenState"),
	_S("EDisplayIsMono"),
	_S("EDisplayIsPalettized"),
	_S("EDisplayBitsPerPixel"),
	_S("EDisplayNumModes"),
	_S("EDisplayMemoryAddress"),
	_S("EDisplayOffsetToFirstPixel"),
	_S("EDisplayOffsetBetweenLines"),
	_S("EDisplayPaletteEntry"),
	_S("EDisplayIsPixelOrderRGB"),
	_S("EDisplayIsPixelOrderLandscape"),
	_S("EDisplayMode"),
	_S("ESwitches"),
	_S("EDebugPort"),
	_S("ELocaleLoaded"),
	_S("EClipboardDrive"),
	_S("ECustomRestart"),
	_S("ECustomRestartReason"),
	_S("EDisplayNumberOfScreens"),
	_S("ENanoTickPeriod"),
	_S("EFastCounterFrequency"),
	_S("EFastCounterCountsUp"),
	_S("EPointer3D"),
	_S("EPointer3DMaxProximity"),
	_S("EPointer3DThetaSupported"),
	_S("EPointer3DPhiSupported"),
	_S("EPointer3DRotationSupported"),
	_S("EPointer3DPressureSupported"),
	_S("EHardwareFloatingPoint"),
	_S("ETimeNonSecureOffset"),
	_S("EPersistStartupModeKernel"),
	_S("EMaximumCustomRestartReasons"),
	_S("EMaximumRestartStartupModes"),
	_S("ECustomResourceDrive"),
	_S("EPointer3DProximityStep"),
	_S("EPointerMaxPointers"),
	_S("EPointerNumberOfPointers"),
	_S("EPointer3DMaxPressure"),
	_S("EPointer3DPressureStep"),
	_S("EPointer3DEnterHighPressureThreshold"),
	_S("EPointer3DExitHighPressureThreshold"),
	_S("EPointer3DEnterCloseProximityThreshold"),
	_S("EPointer3DExitCloseProximityThreshold"),
	_S("EDisplayMemoryHandle"),
	_S("ESerialNumber"),
	_S("ECpuProfilingDefaultInterruptBase"),
	_S("ENumCpus"),
	_S("EDigitiserOrientation")
	};

TInt MatchAbbrev(const TDesC& anInput, const TText** aList, TInt aListLen)
	{
	TInt first_match=KErrNotFound;
	TInt nmatches=0;
	TInt i;
	for (i=0; i<aListLen; i++)
		{
		TPtrC list_entry(aList[i]);
		TInt r=list_entry.FindF(anInput);
		if (r>=0)
			{
			// substring matches
			if (r==0 && list_entry.Length()==anInput.Length()) 
				{
				// exact match
				return i;
				}
			if (first_match<0)
				first_match=i;
			++nmatches;
			}
		}
	if (nmatches>1)
		return KErrGeneral;	// ambiguous
	return first_match;		// either KErrNotFound or match index
	}

TInt MatchAttribute(const TDesC& anInput)
	{
	return MatchAbbrev(anInput, AttributeNames, sizeof(AttributeNames)/sizeof(TText*));
	}

void TestGet()
	{
	TInt i;
	TInt n=0;

	for (i=0; i<HAL::ENumHalAttributes; i++)
		{
		TPtrC att_name(AttributeNames[i]);
		TInt val=-1;
		TInt r=HAL::Get((HAL::TAttribute)i,val);
		test.Printf(_L("%S: return %d, value %d(0x%08x)\n"),&att_name,r,val,val);
		if (++n==16)
			{
			n=0;
			test.Printf(_L("\nPress a key to continue...\n"));
			test.Getch();
			}
		}
	}


void TestGetAll()
	{
	User::Allocator().Check();
	HAL::SEntry* pE=NULL;
	TInt nEntries=0;
	TInt r=HAL::GetAll(nEntries, pE);
	test(r==KErrNone);
	test.Printf(_L("%d attributes defined\n"),nEntries);
	test(nEntries==HAL::ENumHalAttributes);
	TInt i;
	for (i=0; i<nEntries; ++i)
		{
		TPtrC att_name(AttributeNames[i]);
		TInt f=pE[i].iProperties;
		TInt v=pE[i].iValue;
		TInt v2=-1;
		TInt r=HAL::Get((HAL::TAttribute)i,v2);
		if (f & HAL::EEntryValid)
			{
			test(r==KErrNone);
			if (f & HAL::EEntryDynamic)
				{
				test.Printf(_L("Attribute %S dynamic "),&att_name);
				if (v2!=v)
					{
					test.Printf(_L("Values %d(%08x), %d(%08x)"),v,v,v2,v2);
					}
				test.Printf(_L("\n"));
				}
			else
				{
				test(v==v2);	// constant attribute
				}
			}
		else
			{
			test.Printf(_L("Attribute %S not supported on this platform or requires parameter: r==%d\n"),&att_name,r);
			// For some reason the following attribtues come back "KErrNone" on 
			// emulator...so guard added
			if (i != HALData::EDisplayMemoryHandle)
				test(r==KErrNotSupported || r==KErrArgument);
			}
		}
	User::Free(pE);
	User::Allocator().Check();
	}

void InputLine(const TDesC& aPrompt, TDes& aLine)
	{
	test.Printf(_L("%S"),&aPrompt);
	aLine.SetLength(0);
	FOREVER
		{
		TKeyCode k=test.Getch();
		switch (k)
			{
			case EKeyEnter:
				test.Printf(_L("\n"));
				return;
			case EKeyEscape:
				aLine.SetLength(0);
				test.Printf(_L("\n%S"),&aPrompt);
				break;
			case EKeyBackspace:
			case EKeyDelete:
				if (aLine.Length()>0)
					{
					aLine.SetLength(aLine.Length()-1);
					test.Printf(_L("\x8"));
					}
				break;
			default:
				{
				TChar ch((TUint)k);
				TBuf<1> b;
				b.Append(ch);
				aLine.Append(ch);
				test.Printf(_L("%S"),&b);
				break;
				}
			}
		}
	}

void TestSet()
	{
	TBuf<256> line;
	FOREVER
		{
		TInt attrib=KErrNotFound;
		while (attrib<0)
			{
			InputLine(_L("Attribute: "),line);
			if (line.Length()==0)
				return;
			attrib=MatchAttribute(line);
			if (attrib==KErrNotFound)
				test.Printf(_L("Unrecognised attribute\n"));
			else if (attrib==KErrGeneral)
				test.Printf(_L("Ambiguous attribute\n"));
			}
		TPtrC attrib_name(AttributeNames[attrib]);
		test.Printf(_L("Attribute %d (%S) selected\n"),attrib,&attrib_name);
		TInt value=0;
		TInt r=KErrGeneral;
		TBool set=ETrue;
		while(r!=KErrNone)
			{
			InputLine(_L("Value: "),line);
			line.Trim();
			TLex lex(line);
			if (line.MatchF(_L("r"))>=0)
				{
				set=EFalse;
				r=KErrNone;
				}
			else if (line.MatchF(_L("0x"))>=0)
				{
				lex.Inc(2);
				r=lex.Val((TUint&)value,EHex);
				}
			else
				{
				r=lex.Val(value);
				}
			}
		if (set)
			{
			r=HAL::Set((HAL::TAttribute)attrib,value);
			test.Printf(_L("Set returns %d\n"),r);
			}
		TInt v2;
		r=HAL::Get((HAL::TAttribute)attrib,v2);
		test.Printf(_L("Get returns %d, value %d(%08x)\n"),r,v2,v2);
		}
	}

GLDEF_C TInt E32Main()
	{
	test.Title();
	__UHEAP_MARK;
	__UHEAP_SETFAIL(RHeap::EDeterministic,1);
	TInt machine_id;
	TInt r=HAL::Get(HAL::EMachineUid,machine_id);
	__UHEAP_RESET;
	test(r==KErrNone);
	test.Printf(_L("Machine ID %08x\n"),machine_id);

	test.Start(_L("Check AttributeNames[] is up-to-date"));
	test_Equal(HAL::ENumHalAttributes, sizeof(AttributeNames)/sizeof(TText*));

	test.Next(_L("Test Get"));
	TestGet();

	test.Next(_L("Test GetAll"));
	TestGetAll();

	test.Next(_L("Test Set"));
	TestSet();

	test.End();

	__UHEAP_MARKEND;	// problem if HAL uses TLS

	return KErrNone;
	}

