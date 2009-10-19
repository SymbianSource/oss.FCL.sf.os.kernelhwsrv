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
// f32\etshell\ts_clicomp.cpp
// 
//

#include "ts_clicomp.h"
#include "ts_std.h"

CCliCompleter::CCliCompleter()
	{	
	}
	
CCliCompleter* CCliCompleter::NewL()
	{
	CCliCompleter* self = new(ELeave) CCliCompleter();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	
	return self;
	}
	
void CCliCompleter::ConstructL()
	{
	User::LeaveIfError(iFs.Connect());
	}

CCliCompleter::~CCliCompleter()
	{
	iFs.Close();
	}

void CCliCompleter::EstablishCompletionContext(TDesC& aBuf, TUint aPos, TDes& aKnownPart)
	{
	// Try and locate a double-quote to the left of the current cursor position
	TInt contextStart = aBuf.Mid(0, aPos).LocateReverse(TChar('"'));	
	TInt contextEnd;
	
	if(contextStart != KErrNotFound)
		{ // Double-quote found
		contextEnd = aPos - contextStart;
		// Grab a copy of the text we will be trying to complete
		aKnownPart = aBuf.Mid(contextStart, contextEnd);
		}
	else
		{ // Try from the last space character
		contextStart = aBuf.Mid(0, aPos).LocateReverse(TChar(' '));

		if(contextStart == KErrNotFound)
			{
			contextStart = 0;
			}
		else
			{
			contextStart += 1;
			}
			
		contextEnd = aPos - contextStart;

		aKnownPart = aBuf.Mid(contextStart, contextEnd);
		aKnownPart.Trim();
		}
	}

TBool CCliCompleter::AttemptCompletionL(TDes& aKnownPart, RPointerArray<HBufC>& aAlternatives)
	{
	// Turn the known part into something with a path
	TParse parsedKnownPart;

	// ShellFunction::GetFullPath() modifies the source path so create
	// a temporary buffer for a throwaway copy
	TBuf<KMaxPath> tmpKnownPart = aKnownPart;
	ShellFunction::StripQuotes(tmpKnownPart);
	tmpKnownPart.Append(KMatchAny);
	
	if(ShellFunction::GetFullPath(tmpKnownPart, parsedKnownPart) != KErrNone)
		{
		return EFalse;
		}
	
	CDir* entries;	
	if(iFs.GetDir(parsedKnownPart.FullName(), KEntryAttNormal | KEntryAttDir, ESortByName, entries) != KErrNone)
		{
		return EFalse;
		}

	CleanupStack::PushL(entries);

	TEntry entry;
	TBool result;

	if(entries->Count() == 1)
		{
		entry = (*entries)[0];
	
		TPtrC completedPart;	
		completedPart.Set(entry.iName.Mid(parsedKnownPart.NameAndExt().Length() - 1));
		aKnownPart.Append(completedPart);

		if(entry.IsDir()) aKnownPart.Append(KPathDelimiter);
		
		if(((TUint)aKnownPart[0] != TChar('"')) && 
			(aKnownPart.LocateF(TChar(' ')) != KErrNotFound))
			{
			aKnownPart.Insert(0, _L("\""));
			}
		
		result = ETrue;
		}
	else if(entries->Count() > 1)
		{
		TInt entryIdx;		
		
		// Find the smallest matching entry so as to not run off the end of
		// an index when trying to establish the greatest overlap between the
		// matches.  We're also caching the raw entries in a seperate array so
		// we can use them when displaying the ambiguous matches.
		TInt minLength = KMaxFileName;		
		
		for(entryIdx=0;entryIdx<entries->Count();entryIdx++)
			{
			entry = (*entries)[entryIdx];
			
			HBufC* buf = HBufC::NewLC(entry.iName.Length() + 100);
			*buf = entry.iName;
			if(entry.IsDir()) 
				{
				buf->Des().Append(KPathDelimiter);
				}
			aAlternatives.AppendL(buf);
			
			if(entry.iName.Length() < minLength)
				{
				minLength = entry.iName.Length();
				}
			}
		CleanupStack::Pop(entries->Count());
			
		// Find the greatest overlap between the matches.  Even though we can't
		// get a final match we can at least make a best-effort completion based 
		// on anything they've not told us but which matches anyway.
		
		// There's an asterisk on the end of the parsed filename that we want
		// to disregard when calculating the length of what we already know.
		TInt knownPartLength = parsedKnownPart.NameAndExt().Length() - 1;
		
		TInt matchLength = knownPartLength;
		TBool matching = ETrue;
		while(matching && matchLength < minLength)
			{			
			for(entryIdx=1;entryIdx<entries->Count();entryIdx++)
				{
				if(((*entries)[0].iName[matchLength]) != ((*entries)[entryIdx].iName[matchLength]))
					{
					matching = EFalse;
					}
				}
			if(matching) matchLength++;
			}

		if(matchLength > knownPartLength)
			{
			entry = (*entries)[0];
			
			TUint additionalKnownStart = knownPartLength;
			TUint additionalKnownLength = matchLength - knownPartLength;
			
			aKnownPart.Append(entry.iName.Mid(additionalKnownStart, additionalKnownLength));
			}

		result = EFalse;
		}
	else	
		{
		result = EFalse;
		}
	
	CleanupStack::PopAndDestroy(entries);
	
	return result;
	}


/*
	Note: method has the potential to modify the size of aAlternatives as 
	a side-effect of the call to ShellFunction::AlignTextIntoColumns().
*/
void CCliCompleter::DisplayAlternatives(RPointerArray<HBufC>& aAlternatives)
	{
	ShellFunction::AlignTextIntoColumns(aAlternatives);
	
	CShell::NewLine();
	for(TInt entryIdx=0;entryIdx<aAlternatives.Count();entryIdx++)
		{
		CShell::OutputStringToConsole(EFalse, *aAlternatives[entryIdx]);
		CShell::NewLine();
		}
	}
