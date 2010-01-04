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


#include "DataWrapperBase.h"

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

CDataWrapperBase::CDataWrapperBase()
:	CDataWrapper()
	{
	}

CDataWrapperBase::~CDataWrapperBase()
/**
 * Public destructor
 */
	{
	iInclude.ResetAndDestroy();
	iBuffer.ResetAndDestroy();
	}

void CDataWrapperBase::InitialiseL()
	{
	CDataWrapper::InitialiseL();

	TBuf<KMaxTestExecuteCommandLength>	tempStore;
	TPtrC		fileName;
	TBool		moreData=ETrue;
	TBool		index=0;
	while ( moreData )
		{
		tempStore.Format(KFile(), ++index);
		moreData=GetStringFromConfig(KIncludeSection, tempStore, fileName);
		
		if (moreData)
			{
			CIniData*	iniData=CIniData::NewL(fileName);
			CleanupStack::PushL(iniData);
			iInclude.Append(iniData);
			CleanupStack::Pop(iniData);
			}
		}
	}

TBool CDataWrapperBase::GetBoolFromConfig(const TDesC& aSectName,const TDesC& aKeyName,TBool& aResult)
	{
	TBool	ret=EFalse;
	TPtrC	result;
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

TBool CDataWrapperBase::GetIntFromConfig(const TDesC& aSectName, const TDesC& aKeyName, TInt& aResult)
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

TBool CDataWrapperBase::GetInt64FromConfig(const TDesC& aSectName, const TDesC& aKeyName, TInt64& aResult)
     {
     TPtrC     result;
     TBool     ret=EFalse;
     TRAPD(err, ret=GetCommandStringParameterL(aSectName, aKeyName, result));
     if ( err != KErrNone )
          {
          ret=EFalse;
          }
     if ( ret )
          {
          TLex     lex(result);
          ret=(lex.Val(aResult)==KErrNone);
          }

     return ret;
     }

TBool CDataWrapperBase::GetStringFromConfig(const TDesC& aSectName, const TDesC& aKeyName, TPtrC& aResult)
	{
	TBool	ret=EFalse;
	TRAPD(err, ret=GetCommandStringParameterL(aSectName, aKeyName, aResult));
	if ( err != KErrNone )
		{
		ret=EFalse;
		}
	return ret;
	}

TBool CDataWrapperBase::GetHexFromConfig(const TDesC& aSectName, const TDesC& aKeyName, TInt& aResult)
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

TBool CDataWrapperBase::GetCommandStringParameterL(const TDesC& aSectName, const TDesC& aKeyName, TPtrC& aResult)
	{
	TBool	ret=EFalse;

	if ( aSectName.Length()!=0 )
		{
		ret=CDataWrapper::GetStringFromConfig(aSectName, aKeyName, aResult);

		for ( TInt index=iInclude.Count(); (index>0) && (!ret); )
			{
			ret=iInclude[--index]->FindVar(aSectName, aKeyName, aResult);
			}
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
				TBool	found=CDataWrapper::GetStringFromConfig(sectionName, keyName, entryData);
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

TBool CDataWrapperBase::GetCommandStringParameter(const TDesC& aParameterName, const TDesC& aSection, TPtrC& aResult, TText8 *aFileName, TInt aLine, TBool aMandatory)
	{
	TBool	ret = GetStringFromConfig(aSection, aParameterName, aResult);
	if (aMandatory && !ret)
		{
		Logger().LogExtra(aFileName, aLine, ESevrErr, _L("No %S"), &aParameterName);
		SetBlockResult(EFail);
		}
	return ret;
	}

TBool CDataWrapperBase::GetCommandIntParameter(const TDesC& aParameterName, const TDesC& aSection, TInt& aResult, TText8* aFileName, TInt aLine, TBool aMandatory)
	{
	TBool	ret = GetIntFromConfig(aSection, aParameterName, aResult);
	if (aMandatory && !ret)
		{
		Logger().LogExtra(aFileName, aLine, ESevrErr, _L("No %S"), &aParameterName);
		SetBlockResult(EFail);
		}
	return ret;
	}
	
TBool CDataWrapperBase::GetCommandInt64Parameter(const TDesC& aParameterName, const TDesC& aSection, TInt64& aResult, TText8* aFileName, TInt aLine, TBool aMandatory)
	{
	TBool	ret = GetInt64FromConfig(aSection, aParameterName, aResult);
	if (aMandatory && !ret)
		{
		Logger().LogExtra(aFileName, aLine, ESevrErr, _L("No %S"), &aParameterName);
		SetBlockResult(EFail);
		}
	return ret;
	}

TBool CDataWrapperBase::GetCommandBoolParameter(const TDesC& aParameterName, const TDesC& aSection, TBool& aResult, TText8 *aFileName, TInt aLine, TBool aMandatory)
	{
	TBool	ret = GetBoolFromConfig(aSection, aParameterName, aResult);
	if (aMandatory && !ret)
		{
		Logger().LogExtra(aFileName, aLine, ESevrErr, _L("No %S"), &aParameterName);
		SetBlockResult(EFail);
		}
	return ret;
	}
