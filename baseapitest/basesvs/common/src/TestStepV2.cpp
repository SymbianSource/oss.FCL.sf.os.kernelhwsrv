/*
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/


#include "TestStepV2.h"

/*@{*/
///	Constant Literals used.
_LIT(KIncludeSection,		"include");
_LIT(KFile,					"file%d");
_LIT(KMatch,				"*{*,*}*");
_LIT(KStart,				"{");
_LIT(KSeparator,			",");
_LIT(KEnd,					"}");
_LIT(KDataRead,				"INI READ : %S %S %S");
/*@}*/

CTestStepV2::CTestStepV2()
:	CTestStep()
	{
	}

CTestStepV2::~CTestStepV2()
	{
	iInclude.ResetAndDestroy();
	iBuffer.ResetAndDestroy();
	}

enum TVerdict CTestStepV2::doTestStepPreambleL()
	{
	TBuf<KMaxTestExecuteCommandLength>	tempStore;

	TVerdict	ret=CTestStep::doTestStepPreambleL();
	TPtrC		fileName;
	TBool		moreData=ETrue;
	TBool		index=0;
	while ( moreData )
		{
		tempStore.Format(KFile(), ++index);
		moreData=GetStringFromConfig(KIncludeSection, tempStore, fileName);
		if ( moreData )
			{
			CIniData*	iniData=CIniData::NewL(fileName);
			CleanupStack::PushL(iniData);
			iInclude.Append(iniData);
			CleanupStack::Pop(iniData);
			}
		}

	return ret;
	}

TBool CTestStepV2::GetBoolFromConfig(const TDesC& aSectName,const TDesC& aKeyName,TBool& aResult)
	{
	TPtrC	result;
	TBool	ret=EFalse;
	TRAPD(err, ret=GetCommandStringParameterL(aSectName, aKeyName, result));
	if ( err != KErrNone )
		{
		ret=EFalse;
		}
	if ( ret )
		{
		_LIT(KTrue,"true");
		aResult=(result.FindF(KTrue) != KErrNotFound);
		}
	return ret;
	}

TBool CTestStepV2::GetIntFromConfig(const TDesC& aSectName, const TDesC& aKeyName, TInt& aResult)
	{
	TPtrC	result;
	TBool	ret=EFalse;
	TRAPD(err, ret=GetCommandStringParameterL(aSectName, aKeyName, result));
	if ( err != KErrNone )
		{
		ret=EFalse;
		}
	if ( ret )
		{
		TLex	lex(result);
		ret=(lex.Val(aResult)==KErrNone);
		}

	return ret;
	}

TBool CTestStepV2::GetStringFromConfig(const TDesC& aSectName, const TDesC& aKeyName, TPtrC& aResult)
	{
	TBool	ret=EFalse;
	TRAPD(err, ret=GetCommandStringParameterL(aSectName, aKeyName, aResult));
	if ( err != KErrNone )
		{
		ret=EFalse;
		}
	return ret;
	}

TBool CTestStepV2::GetHexFromConfig(const TDesC& aSectName, const TDesC& aKeyName, TInt& aResult)
	{
	TPtrC	result;
	TBool	ret=EFalse;
	TRAPD(err, ret=GetCommandStringParameterL(aSectName, aKeyName, result));
	if ( err != KErrNone )
		{
		ret=EFalse;
		}
	if ( ret )
		{
		TLex	lex(result);
		ret=(lex.Val((TUint &)aResult, EHex)==KErrNone);
		}

	return ret;
	}

TBool CTestStepV2::GetCommandStringParameterL(const TDesC& aSectName, const TDesC& aKeyName, TPtrC& aResult)
	{
	TBool	ret=CTestStep::GetStringFromConfig(aSectName, aKeyName, aResult);

	for ( TInt index=iInclude.Count(); (index>0) && (!ret);  )
		{
		ret=iInclude[--index]->FindVar(aSectName, aKeyName, aResult);
		}

	if ( ret )
		{
		if ( aResult.Match(KMatch)!=KErrNotFound )
			{
			//	We have an entry of the format
			//	entry =*{section,entry}*
			//	where * is one or more characters
			//	We need to construct this from other data in the ini file replacing {*,*}
			//	with the data from
			//	[section]
			//	entry =some_value
			HBufC*	buffer=HBufC::NewLC(aResult.Length());
			buffer->Des().Copy(aResult);

			TInt	startLength=KStart().Length();
			TInt	sparatorLength=KSeparator().Length();
			TInt	endLength=KEnd().Length();
			TInt	bufferLength;
			TInt	start;
			TInt	sparator;
			TInt	end;
			TPtrC	remaining;
			TLex	lex;
			do
				{
				bufferLength=buffer->Length();
				start=buffer->Find(KStart);

				remaining.Set(buffer->Des().Right(bufferLength-start-startLength));
				sparator=remaining.Find(KSeparator);
				remaining.Set(remaining.Right(remaining.Length()-sparator-sparatorLength));
				sparator += (start + startLength);

				end=remaining.Find(KEnd) + sparator + sparatorLength;

				TPtrC	sectionName(buffer->Ptr()+start+startLength, sparator-start-startLength);
				TPtrC	keyName(buffer->Ptr()+sparator+sparatorLength, end-sparator-sparatorLength);
				sectionName.Set(TLex(sectionName).NextToken());
				keyName.Set(TLex(keyName).NextToken());

				TInt	entrySize=0;
				TPtrC	entryData;
				TBool	found=CTestStep::GetStringFromConfig(sectionName, keyName, entryData);
				for ( TInt index=iInclude.Count(); (index>0) && (!found);  )
					{
					found=iInclude[--index]->FindVar(sectionName, keyName, entryData);
					}
				if ( found )
					{
					entrySize=entryData.Length();
					}

				TInt	newLength=start + bufferLength - end - endLength + entrySize;
				HBufC*	bufferNew=HBufC::NewLC(newLength);
				bufferNew->Des().Copy(buffer->Ptr(), start);
				if ( entrySize>0 )
					{
					bufferNew->Des().Append(entryData);
					}
				bufferNew->Des().Append(buffer->Ptr() + end + endLength, bufferLength - end - endLength);
				CleanupStack::Pop(bufferNew);
				CleanupStack::PopAndDestroy(buffer);
				buffer=bufferNew;
				CleanupStack::PushL(buffer);
				}
			while ( buffer->Match(KMatch)!=KErrNotFound );
			iBuffer.Append(buffer);
			CleanupStack::Pop(buffer);
			aResult.Set(*buffer);
			INFO_PRINTF4(KDataRead, &aSectName, &aKeyName , &aResult);
			}
		}

	return ret;
	}
