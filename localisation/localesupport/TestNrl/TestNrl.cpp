/*
* Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
* TCUSTOMWRAP.CPP 
*
*/


#include <e32base.h>
#include <msvapi.h>
#include <msvids.h>
#include <mtclreg.h>
#include <eikstart.h>

#include <basched.h>
#include <banamedplugins.h>
#include <bautils.h>
#include <coecntrl.h>
#include <coeccntx.h>
#include <coemain.h>
#include <charconv.h>
#include <convnames.h>
#include <e32keys.h>
#include <techview\eikon.hrh>
#include <eikappui.h>
#include <eikapp.h>
#include <eikdoc.h>
#include <eikenv.h>
#include <techview\eikrted.h>
#include <techview\eikedwin.h>
#include <eikdef.h>
#include <techview\eikdialg.h>
#include <techview\eikdlgtb.h>
#include <techview\eikrted.h>
#include <techview\eiksbfrm.h>
#include <techview\eikconso.h>
#include <txtrich.h>
#include <hal.h>
#include <fbs.h>
#include "TestNrl.hrh"
#include <testnrl.rsg>
#include <techview\eikon.rsg>
#include <prnsetup.h>

#include <biodb.h>	
#include <biouids.h>
#include <gdi.h>

// forward declarations
class CNRLTestAppUi;
//class CEikScrollBarFrame;

const TInt EGranularity=4;

_LIT(KNewLine,"\n");
_LIT(KLitResourceFileNameAppendage, "_NAME.RSC");
#ifdef _DEBUG
_LIT(KPanicText, "NonRom_Test");
#endif

class TDummyObserver: public MMsvSessionObserver
	{
public:
		virtual void HandleSessionEventL(TMsvSessionEvent, TAny*, TAny*, TAny*) {};
	};


//
// class CNRLTestControl
//

class CNRLTestControl : public CCoeControl
    {
public:

    void ConstructL(const TRect& aRect);
	void ActivateL();
	~CNRLTestControl();
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	void PrintToScreen (TRefByValue<const TDesC> aFmt,...);
	void PrintLineToScreen (TRefByValue<const TDesC> aFmt,...);

private: // from CCoeControl
	void Draw(const TRect&) const;
	
private:
	CEikConsoleScreen* iConsole;
    };


CNRLTestControl::~CNRLTestControl ()
	{
	delete iConsole;
	}


void CNRLTestControl::ConstructL (const TRect& aRect)
	{
	CreateWindowL();
	Window().SetShadowDisabled(ETrue);
    Window().SetBackgroundColor(KRgbGray);
    EnableDragEvents();
	SetRect(aRect);
	SetBlank();
	TRect consoleSize = aRect;
	consoleSize.Shrink(1,1);
	iConsole=new(ELeave) CEikConsoleScreen;
	iConsole->ConstructL(_L("TEST"),TPoint(1,1),consoleSize.Size(),CEikConsoleScreen::ENoInitialCursor,EEikConsWinInPixels);
	}

void CNRLTestControl::ActivateL ()
	{
	CCoeControl::ActivateL();
	iConsole->SetKeepCursorInSight(ETrue);
	iConsole->SetHistorySizeL(500,5);
	iConsole->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EOn);
	iConsole->Write(_L("\nStarting tests for Non-Rom Localisation\nThis test requires some user interaction\n"));
	iConsole->FlushChars();
	iConsole->DrawCursor();
	iConsole->SetAtt(ATT_NORMAL);
	}

void CNRLTestControl::Draw(const TRect& /* aRect*/) const
	{
	CWindowGc& gc = SystemGc();
	TRect rect=Rect();
	//rect.Shrink(10,10);
	gc.DrawRect(rect);
	rect.Shrink(1,1);
	}


TKeyResponse CNRLTestControl::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
    {
	if (aType!=EEventKey)
		return(EKeyWasConsumed);
    TInt modifiers=aKeyEvent.iModifiers;
    TInt code=aKeyEvent.iCode;



	TRect range = iConsole->Selection(); // get current selected range
	switch (code)
		{
		case EKeyUpArrow:
			iConsole->Up();
			if (modifiers & EModifierShift)
				{
				range.iTl = iConsole->CursorPos();
				iConsole->SetSelection(range); 
				}
			else
				iConsole->SelectCursor(); 
			break;
		case EKeyDownArrow:
			iConsole->Down();
			if (modifiers & EModifierShift)
				{
				range.iTl = iConsole->CursorPos();
				iConsole->SetSelection(range); 
				}
			else
				iConsole->SelectCursor(); 
			break;
		case EKeyLeftArrow:
			iConsole->Left();
			if (modifiers & EModifierShift)
				{
				range.iTl = iConsole->CursorPos();
				iConsole->SetSelection(range); 
				}
			else
				iConsole->SelectCursor(); 
			break;
		case EKeyRightArrow:
			iConsole->Right();
			if (modifiers & EModifierShift)
				{
				range.iTl = iConsole->CursorPos();
				iConsole->SetSelection(range); 
				}
			else
				iConsole->SelectCursor(); 
			break;
		case EKeyEnter: 
				iConsole->Cr();
				iConsole->Lf();
			break;
		default:
			{
			iConsole->SelectCursor();	// forget previous selection
			TBuf<1> chr;
			chr.Format(_L("%c"),code);
			iConsole->Write(chr);
			iConsole->FlushChars();
			}
			break;
		}
    return(EKeyWasConsumed);
    }


void CNRLTestControl::PrintToScreen(TRefByValue<const TDesC> aFmt,...)
	{
	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<128> buf;
	buf.FormatList(aFmt,list);
	iConsole->Write(buf);
	}

void CNRLTestControl::PrintLineToScreen(TRefByValue<const TDesC> aFmt,...)
	{
	VA_LIST list;
	VA_START(list,aFmt);
	TBuf<128> buf;
	buf.FormatList(aFmt,list);
	iConsole->Write(buf);
	iConsole->Write(KNewLine);

	}



// 
//  ---------------------- CNRLTestAppView definition ---------------- 
//


class CNRLTestAppUi : public CEikAppUi ,private CBaNamedPlugins::MFallBackName
	{
public:
	void ConstructL();
	void CreateControlL();
	~CNRLTestAppUi();
	
	void StartTestsL();
	void TestLocaleL(const TDesC& aTestHeader);
	void TestCollation(const TDesC& aTestHeader);
	void TestResourcesL(const TDesC& aTestHeader);
	void TestCharSetNamesL(const TDesC& aTestHeader);
	void TestFEPNamesL(const TDesC& aTestHeader);
	void TestDisplayMessagingL(const TDesC& aTestHeader);
	void TestBIFL(const TDesC& aTestHeader);
	void TestPrinterNameL(const TDesC& aTestHeader);
	void TestLocalisableBitmapL(const TDesC& aTestHeader);


private: // from CEikAppUi -- framework
	void HandleCommandL(TInt aCommand);

	virtual HBufC* FallBackNameL(const TDesC& aFullResourceFileName) const;
	
	void WriteTestHeader(const TDesC& aTestHeader) const;
	void Pass(const TDesC& aTestHeader);
	void FailL(const TDesC& aTestHeader);
	void ReportEndResult() const;
private: 
	CNRLTestControl* iNRLTestControl;
	CDesCArray* iFailedTests;
	};

void CNRLTestAppUi::ConstructL()
	{
	BaseConstructL();
	CreateControlL();

	}

// The cleanup operation of the TCleanupItem

LOCAL_C void DestroyResourceFileArray(TAny* aArrayOfResourceFiles)
	{
	RArray<CBaNamedPlugins::TResourceFile>& arrayOfResourceFiles=*STATIC_CAST(RArray<CBaNamedPlugins::TResourceFile>*, aArrayOfResourceFiles);
	for (TInt i=arrayOfResourceFiles.Count()-1; i>=0; --i)
		{
		const CBaNamedPlugins::TResourceFile& resourceFile=arrayOfResourceFiles[i];
		delete resourceFile.iFullFileName;
		delete resourceFile.iIdentifier;
		}
	arrayOfResourceFiles.Close();
	}



void CNRLTestAppUi::CreateControlL()
	{
	iNRLTestControl=new(ELeave) CNRLTestControl;
    iNRLTestControl->ConstructL(ClientRect());
	AddToStackL(iNRLTestControl);
	iNRLTestControl->ActivateL();
	iFailedTests=new(ELeave) CDesCArrayFlat(3);
	}

CNRLTestAppUi::~CNRLTestAppUi()
	{
	RemoveFromStack(iNRLTestControl);
	delete iNRLTestControl;
	for (TInt i=0; i<iFailedTests->Count();i++)
		iFailedTests->Delete(i);
	delete iFailedTests;
	}

void CNRLTestAppUi::HandleCommandL(TInt aCommand)
	{
	switch(aCommand)
		{
		case EAppCmdExit:
			Exit();
			break;
		case EAppCmdTest:
			StartTestsL ();
			break;
		default:
			break;
		}
	} 


_LIT(KTestAnnouncer,"----------------");
_LIT(KLocaleTestTitle,"Locale ");
_LIT(KResourceTestTitle,"Application Resource ");
_LIT(KCharSetNamesTest,"Charset Names ");
_LIT(KFEPNameTestTitle,"FEP Names ");
_LIT(KCollationTestTitle, "Collation Table ");
_LIT(KTDisplayMessaging,"Messaging Resources ");
_LIT(KBIFTestTitle,"BIF Files ");
_LIT(KPrinterNameTestTitle,"Printer Names ");
_LIT(KLocalisableBimap,"Bitmap Files ");
_LIT(KOriginalBitmapPath,"z:\\resource\\apps\\testnrl.mbm");
_LIT(KNonROM,"Non-ROM");

void CNRLTestAppUi::WriteTestHeader(const TDesC& aTestHeader) const 
	{
	iNRLTestControl->PrintToScreen(KNewLine);
	iNRLTestControl->PrintToScreen(aTestHeader);
	iNRLTestControl->PrintLineToScreen (_L("Test"));
	iNRLTestControl->PrintToScreen(KNewLine);

	}


void CNRLTestAppUi::StartTestsL ()
	{
	TestLocaleL(KLocaleTestTitle);
	TestResourcesL (KResourceTestTitle);
	TestCharSetNamesL(KCharSetNamesTest);
	TestCollation(KCollationTestTitle);
	TestFEPNamesL(KFEPNameTestTitle);
	TestDisplayMessagingL(KTDisplayMessaging);
	TestBIFL(KBIFTestTitle);
	TestPrinterNameL (KPrinterNameTestTitle);
	TestLocalisableBitmapL (KLocalisableBimap);
	ReportEndResult();
	
	}


void CNRLTestAppUi::TestLocaleL (const TDesC& aTestHeader)
	{

	// print on console that we are starting the Locale Testing
	// just checking it is not enough, need to display as well that 
	// the result is as expected. 
	//Test a few other things besides the 
	// locale number, some thing like a currency symbol. == $$$ 
	
	WriteTestHeader(aTestHeader);
	iNRLTestControl->PrintLineToScreen(_L("Have to ensure that the correct locale is picked up"));
	iNRLTestControl->PrintLineToScreen(_L("Expected Locale with language extension"));

	TInt language; 
	(void)HAL::Get(HAL::ELanguageIndex,language);
	iNRLTestControl->PrintLineToScreen(_L("%d"),language);
	iNRLTestControl->PrintToScreen(_L("Current Locales language extension:	"));

	TInt currentLangId;
	currentLangId = TInt(User::Language());
	iNRLTestControl->PrintLineToScreen(_L("%d"),currentLangId);

	if (currentLangId == 95 )
		{
		TCurrencySymbol theCurrencySymbol;
		_LIT(KExpectedCurrencySymbol,"$$$");
		if(theCurrencySymbol==KExpectedCurrencySymbol)
			{
			iNRLTestControl->PrintLineToScreen(_L("The correct locale was loaded"));
			Pass(aTestHeader);
			}
		}
	else
		{
		iNRLTestControl->PrintToScreen(_L("The correct locale was not loaded"));
		FailL(aTestHeader);
		}
	}

void CNRLTestAppUi::TestResourcesL(const TDesC& aTestHeader)
	{
	WriteTestHeader(aTestHeader);
	iNRLTestControl->PrintLineToScreen (_L("Reading information from resource file..."));

	TResourceReader resourceReader;
	CCoeEnv::Static()->CreateResourceReaderLC(resourceReader,R_NRL_COLLATE);
	CDesCArray* collation=new(ELeave) CDesCArrayFlat(3);
	CleanupStack::PushL(collation);
	TInt n=resourceReader.ReadUint16();
	
	for (TInt i=0;i<n;i++)
		collation->AppendL(resourceReader.ReadTPtrC());

	for (TInt j=0;j<n;j++)
		{
		iNRLTestControl->PrintLineToScreen((*collation)[j]);
		}
	
	_LIT(KResourceData,"David");
	TInt resourceTest; 
	collation->Find(KResourceData,resourceTest,ECmpCollated);
	if(resourceTest!=collation->MdcaCount())
		{
		Pass(aTestHeader);
		}
	else
		{
		FailL(aTestHeader);
		}
	CleanupStack::PopAndDestroy(2);//resourceReader,collation
	}

void CNRLTestAppUi::TestCharSetNamesL(const TDesC& aTestHeader)
	{
	WriteTestHeader(aTestHeader);
	RFs& aSession = iCoeEnv->FsSession();
	CArrayFix<CCnvCharacterSetConverter::SCharacterSet>* charsetArray=CCnvCharacterSetConverter::CreateArrayOfCharacterSetsAvailableLC(aSession);
	MDesCArray* nameArry=CCnvCharacterSetNames::NewL(aSession,charsetArray->Array());
	TInt index=((CCnvCharacterSetNames*)nameArry)->IndexOfIdentifier(KCharacterSetIdentifierAscii);
	TInt testResult;
	testResult=0;
	TPtrC bigFive=nameArry->MdcaPoint(index);
	TInt findNonROM;
	findNonROM=bigFive.Find(KNonROM);
	if(findNonROM!=KErrNotFound)
		{
		iNRLTestControl->PrintToScreen(KNewLine);
		iNRLTestControl->PrintLineToScreen(bigFive);
		testResult=1;
		}
	if(testResult)
		Pass(aTestHeader);
	else
		FailL(aTestHeader);
	delete nameArry;
	CleanupStack::PopAndDestroy();//charsetArray
	}



void CNRLTestAppUi::TestCollation(const TDesC& aTestHeader)
	{
	WriteTestHeader(aTestHeader);
	iNRLTestControl->PrintToScreen(_L("This test locale has it's own collation table\n"));
	iNRLTestControl->PrintToScreen(_L("Set Collation values in the order E-D-C-B-A\n"));
	iNRLTestControl->PrintToScreen(_L("Loading a few names which have been sorted using collation\n"));
	iNRLTestControl->PrintToScreen(_L("David should appear before BeiBei\n\n"));
	TResourceReader resourceReader;
	CCoeEnv::Static()->CreateResourceReaderLC(resourceReader,R_NRL_COLLATE);
	CDesCArray* collation=new(ELeave) CDesCArrayFlat(3);
	CleanupStack::PushL(collation);
	TInt n=resourceReader.ReadUint16();
	
	for (TInt i=0;i<n;i++)
		collation->AppendL(resourceReader.ReadTPtrC());
	//David is the first name stored in the array before the array is sorted.
	//When the array is sorted, David should still be the first name
	// because this locales collation table reverses the ordering between A & E 
	TPtrC david = (*collation)[0];
	collation->Sort(ECmpCollated);
	for (TInt j=0;j<n;j++)
		{
		iNRLTestControl->PrintLineToScreen((*collation)[j]);
		}
	
	TInt coltest;
	
	// searching for Davids name in the array,
	// according to the new collation rules David's name should be 
	// the first itm in the array. 

	collation->Find(david,coltest,ECmpCollated);
	// if coltest=0 then Davids is the first item, 
	// the new collation table was used. 
	if (!coltest)
		{
		Pass(aTestHeader);
		}
	else
		{
		FailL(aTestHeader);
		}
		
	CleanupStack::PopAndDestroy(2);//resourceReader,collation
	}


void CNRLTestAppUi::TestFEPNamesL(const TDesC& aTestHeader)
	{
	WriteTestHeader(aTestHeader);
	iNRLTestControl->PrintLineToScreen (_L("Fep Names are stored in resource files in the fep directory in system"));
	iNRLTestControl->PrintLineToScreen (_L("Compiled new resources with the word Non-ROM added into a few test fep names"));
	
	RArray<CBaNamedPlugins::TResourceFile> arrayOfResourceFiles;
	CleanupStack::PushL(TCleanupItem(DestroyResourceFileArray, &arrayOfResourceFiles));//arrayOfResourceFiles
	RFs& fileServerSession=iCoeEnv->FsSession();
	TInt numberofsession=fileServerSession.ResourceCount();
	TInt i;
	TParse* parser=new(ELeave) TParse;
	CleanupStack::PushL(parser);//parser
	TFileName* fileName=new(ELeave) TFileName;
	CleanupStack::PushL(fileName);//fileName
	 

	RArray<TUid> uidsOfAvailableFeps;
	CleanupClosePushL(uidsOfAvailableFeps);
	CDesCArray*  fileNamesOfAvailableFeps = new(ELeave) CDesCArrayFlat(EGranularity);
	CleanupDeletePushL(fileNamesOfAvailableFeps);//fileNamesOfAvailableFeps
	iCoeEnv->AvailableFepsL(uidsOfAvailableFeps,fileNamesOfAvailableFeps);//it is allocated some memory here!!!
	
	
	
	for (i=fileNamesOfAvailableFeps->MdcaCount()-1; i>=0; --i)
		{
		const TPtrC fullFileNameOfDll(fileNamesOfAvailableFeps->MdcaPoint(i));
		*fileName=TParsePtrC(fullFileNameOfDll).Name();
		fileName->Append(KLitResourceFileNameAppendage);
		User::LeaveIfError(parser->SetNoWild(*fileName, &fullFileNameOfDll, NULL));
		CBaNamedPlugins::TResourceFile resourceFile;
		resourceFile.iFullFileName=parser->FullName().AllocLC();
		resourceFile.iIdentifier=fullFileNameOfDll.AllocLC();
		resourceFile.iUid=uidsOfAvailableFeps[i];
		resourceFile.iFormat=CBaNamedPlugins::TResourceFile::EFormatTbuf;
		User::LeaveIfError(arrayOfResourceFiles.Append(resourceFile));
		CleanupStack::Pop(2, resourceFile.iFullFileName);//iFullFileName,iIdentifier
		}
	CleanupStack::PopAndDestroy(4, parser);//parser,fileName,uidsOfAvailableFeps,fileNamesOfAvailableFeps

	CBaNamedPlugins::CParameters* parameters=CBaNamedPlugins::CParameters::NewLC(fileServerSession, arrayOfResourceFiles.Array());
	parameters->SetFallBackName(*this);
	CBaNamedPlugins* namedPlugins=CBaNamedPlugins::NewL(*parameters);//numberofsession increased after this
	numberofsession=fileServerSession.ResourceCount(); 
	const TInt numberOfAvailableFeps=namedPlugins->MdcaCount();
	TInt testResult; 
	testResult=0;
	for (i=0; i<numberOfAvailableFeps; ++i)
		{
		TPtrC fepNames = namedPlugins->MdcaPoint(i);
		RDebug::Print(fepNames);
		TInt findNonROM;
		findNonROM=fepNames.Find(KNonROM);
		if (findNonROM!=KErrNotFound)
			{
			iNRLTestControl->PrintLineToScreen(fepNames);
			testResult =1;
			}
		}
	if (testResult)
		{
		Pass(aTestHeader);
		}
	else
		{
		FailL(aTestHeader);
		}
	delete namedPlugins;
	CleanupStack::PopAndDestroy(2, &arrayOfResourceFiles);//arrayOfResourceFiles,parameters
	}

HBufC* CNRLTestAppUi::FallBackNameL(const TDesC& aFullResourceFileName) const
	{
	const TPtrC nameAndExtension(TParsePtrC(aFullResourceFileName).NameAndExt());
	__ASSERT_DEBUG(nameAndExtension.Right(KLitResourceFileNameAppendage().Length())==KLitResourceFileNameAppendage, User::Panic(KPanicText,-1));
	return nameAndExtension.Left(nameAndExtension.Length()-KLitResourceFileNameAppendage().Length()).AllocL();
	}

void CNRLTestAppUi::TestDisplayMessagingL(const TDesC& aTestHeader)
	{
	WriteTestHeader(aTestHeader);
	iNRLTestControl->PrintLineToScreen (_L("MTM are stored in resource files in the MTM directory in System"));
	iNRLTestControl->PrintLineToScreen (_L("Compiled new resources with the word Non-ROM added into the MTM resources"));

	TDummyObserver obs;
	CMsvSession *session=CMsvSession::OpenSyncL(obs);
	CleanupStack::PushL(session);
	CClientMtmRegistry *reg=CClientMtmRegistry::NewL(*session);
	CleanupStack::PushL(reg);
	TInt count=reg->NumRegisteredMtmDlls();
	
	TInt testResult; 
	testResult=0;
	while(count--)
		{
		const CMtmDllInfo& info=reg->RegisteredMtmDllInfo(reg->MtmTypeUid(count));
		TPtrC mtmNames = info.HumanReadableName();
		TInt findNonROM;
		findNonROM=mtmNames.Find(KNonROM);
		if (findNonROM!=KErrNotFound)
			{
			iNRLTestControl->PrintLineToScreen (mtmNames);
			testResult =1;
			}
		
		}

	CleanupStack::PopAndDestroy(reg);

	CMsvEntry *entry=session->GetEntryL(KMsvRootIndexEntryId);
	CleanupStack::PushL(entry);

	entry->SetEntryL(KMsvDraftEntryId);
	iNRLTestControl->PrintLineToScreen (entry->Entry().iDetails);
	

	entry->SetEntryL(KMsvGlobalInBoxIndexEntryId);
	iNRLTestControl->PrintLineToScreen (entry->Entry().iDetails);

	entry->SetEntryL(KMsvGlobalOutBoxIndexEntryId);
	iNRLTestControl->PrintLineToScreen (entry->Entry().iDetails);

	entry->SetEntryL(KMsvSentEntryId);
	iNRLTestControl->PrintLineToScreen (entry->Entry().iDetails);

	CleanupStack::PopAndDestroy(entry);
	CleanupStack::PopAndDestroy(session);
	
	if (testResult)
		{
		Pass(aTestHeader);
		}
	else
		{
		FailL(aTestHeader);
		}
	}



void CNRLTestAppUi::TestBIFL (const TDesC& aTestHeader)
	{
	WriteTestHeader(aTestHeader);
	iNRLTestControl->PrintLineToScreen(_L("BIF files are now stored as resources in System\\BIf directory"));
	iNRLTestControl->PrintLineToScreen(_L("Compiled new BIF resources with the phrase Non-ROM added ... "));

	RFs& gFs=iCoeEnv->FsSession();
	
	iNRLTestControl->PrintLineToScreen(_L("Opening & Searching DB"));

	CBIODatabase* bioDB = CBIODatabase::NewL(gFs);
	CleanupStack::PushL( bioDB );
	iNRLTestControl->PrintLineToScreen(_L("Opened DB Successfully!"));

	TInt testResult; 
	testResult=0;
	for (TInt i=0; i < bioDB->BIOCount(); i++)
		{
			const CBioInfoFileReader& bifReader = bioDB->BifReader(i);

			TPtrC desc;
			desc.Set(bifReader.Description()); 
			TInt findNonROM;
			findNonROM=desc.Find((KNonROM));
			if (findNonROM!=KErrNotFound)
				{
				iNRLTestControl->PrintLineToScreen(desc);
				testResult =1;
				}
		}

	if (testResult)
		{
		Pass(aTestHeader);
		}
	else
		{
		FailL(aTestHeader);
		}
	CleanupStack::PopAndDestroy();	// bioDB
	
	}

void CNRLTestAppUi::TestPrinterNameL (const TDesC& aTestHeader)
	{
	
	WriteTestHeader(aTestHeader);

	CDesCArray* list=new(ELeave) CDesCArrayFlat(EGranularity);
	CleanupStack::PushL(list);

	CPrintSetup* aPrintSetup = CPrintSetup::NewL();
	CleanupStack::PushL(aPrintSetup);
	aPrintSetup->AddPrinterDriverDirL( KDefaultPrinterDriverPath );
	RFs& filesession = iCoeEnv->FsSession();
	CPrinterModelList* aModelList;
	aModelList = aPrintSetup->ModelNameListL(filesession);
	CEikonEnv::GetPrinterNamesL(aModelList,*list);

	aPrintSetup->FreeModelList();

	TInt foundNonRomResource=0; 

	for (TInt i=0; i < list->Count(); ++i)
		{
		TPtrC desc;
		desc.Set((*list)[i]);
		TInt findNonROM; 
		findNonROM = desc.Find((KNonROM));
		if (findNonROM!=KErrNotFound)
			{
			iNRLTestControl->PrintLineToScreen (desc);
			foundNonRomResource=1;
			}
		}
	if (foundNonRomResource)
		Pass(aTestHeader);
	else
		FailL(aTestHeader);


	CleanupStack::PopAndDestroy(2); // list, aPrintSetup, aModelList
	
	}

void CNRLTestAppUi::TestLocalisableBitmapL(const TDesC& aTestHeader)
	{
	WriteTestHeader(aTestHeader);

	TFileName filename(KOriginalBitmapPath);
	CWsBitmap* aBitmap;
	aBitmap=iEikonEnv->CreateBitmapL(filename,1);
	TBool nonROM;
	nonROM=aBitmap->IsRomBitmap();
	if(nonROM)
		{
		FailL(aTestHeader);
		}
	else
		{
		iNRLTestControl->PrintLineToScreen(_L("Right, the bitmap loaded should not be in ROM"));
		Pass(aTestHeader);
		}

	delete aBitmap;
	}


void CNRLTestAppUi::Pass(const TDesC& aTestHeader)
	{
	iNRLTestControl->PrintToScreen(KNewLine);
	iNRLTestControl->PrintLineToScreen(KTestAnnouncer);
	iNRLTestControl->PrintLineToScreen(_L("Expected new %S loaded"),&aTestHeader);
	iNRLTestControl->PrintLineToScreen(KTestAnnouncer);
	}

void CNRLTestAppUi::FailL(const TDesC& aTestHeader)
	{
	iNRLTestControl->PrintToScreen(KNewLine);
	iNRLTestControl->PrintLineToScreen(KTestAnnouncer);
	iNRLTestControl->PrintLineToScreen(_L("Unexpected old %S loaded"),&aTestHeader);
	iNRLTestControl->PrintLineToScreen(KTestAnnouncer);
	iFailedTests->AppendL(aTestHeader);

	}

void CNRLTestAppUi::ReportEndResult() const
	{
	TInt failedTestsCount = iFailedTests->Count();
	if (failedTestsCount)
		{
		// Print a list of the Tests Failed 
		iNRLTestControl->PrintLineToScreen(_L("The following tests failed\n"));
		for (TInt i=0; i<failedTestsCount; ++i)
			{
			iNRLTestControl->PrintLineToScreen ((*iFailedTests)[i]);
			}
		}
	else
		{
		// No Tests Failed, say that new resources were loaded where expected to
		}
	}


//
//  --------------------- CNRLTestAppDoc class Definition ------------ 
//

class CNRLTestAppDoc : public CEikDocument
	{
public:
	CNRLTestAppDoc(CEikApplication& aApp);
private:
	CEikAppUi* CreateAppUiL();
	};


CNRLTestAppDoc::CNRLTestAppDoc(CEikApplication& aApp):CEikDocument(aApp)
	{
	// Nothing else to do, just call the base class constructor
	//
	}

CEikAppUi* CNRLTestAppDoc::CreateAppUiL()
	{
	return new (ELeave) CNRLTestAppUi;
	}
//
//  ------------------------------ CNRLTestApp ----------------------- 
//


const TUid KTestNRLid = {0x1000AC5D};

class CNRLTestApp : public CEikApplication
	{
private:
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;

	};

TUid CNRLTestApp::AppDllUid() const 
	{
	return KTestNRLid;
	}

CApaDocument* CNRLTestApp::CreateDocumentL()
	{
	return new (ELeave) CNRLTestAppDoc(*this);
	}

////////////////////////////////////////////////////////////////////////////////////////////
//

	static CApaApplication* NewApplication()
		{
		return new CNRLTestApp;
		}

	TInt E32Main()
		{
		return EikStart::RunApplication(&NewApplication);
		}


