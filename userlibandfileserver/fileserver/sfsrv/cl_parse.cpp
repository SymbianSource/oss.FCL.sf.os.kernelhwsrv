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
// f32\sfsrv\cl_parse.cpp
// 
//

#include "cl_std.h"

const TInt KLexComponents=4;
const TInt KLexNames=3;




EXPORT_C TParseBase::TParseBase()
	: iWild(0)
/**
Default constructor.
*/
	{

	Mem::FillZ(&iField[0],sizeof(iField));
	}




TInt TParseBase::ParseDrive(TLex& aName,TBool& aDone)
//
// Parse the drive name.
//
	{

	TPtrC d=aName.RemainderFromMark();
	if (d.Length()<2 || d[1]!=KDriveDelimiter)
		return(KErrNone);	//must be Drive delimeter and longer that tow to be valid drive
	TCharF c=d[0];
	if (!c.IsAlpha())		//must be alphaber letter 
		return(KErrBadName);
	if (!aDone)
		{
		if(iMod)
			NameBuf()+=d.Left(2);
		aDone=ETrue;
		}
	aName.SkipAndMark(2);
	return(KErrNone);
	}

TInt TParseBase::ParsePath(TLex& aName,TBool& aDone)
//
// Parse the path.
//
	{

	TPtrC d=aName.RemainderFromMark();
	if (d.Length() && d[0]!=KPathDelimiter)
		return(KErrNone); // Require first char of path to be a '\'
	TInt n=d.LocateReverse(KPathDelimiter)+1;
	if (n && !aDone)
		{
		if(iMod)
			{
			if (NameBuf().Length()+n>KMaxFileName)
				return(KErrBadName);
			NameBuf()+=d.Left(n);
			}
		aDone=ETrue;
		}
	aName.SkipAndMark(n);
	return(KErrNone);
	}

LOCAL_C TBool IsSpace(const TDesC& aDes)
//
// Returns ETrue if aDes only contains spaces or is zero length
//
	{

	TInt len=aDes.Length();
	for (TInt i=0;i<len;i++)
		{
		TChar txt=aDes[i];
		if (!txt.IsSpace())
			return(EFalse);
		}
	return(ETrue);
	}

TInt TParseBase::ParseName(TLex& aName,TBool& aDone)
//
// Parse the name.
//
	{

	TPtrC d=aName.RemainderFromMark();
	if (d.Locate(KPathDelimiter)!=KErrNotFound)
		return(KErrBadName); // Illegal name - filenames cannot contain a '\'
	TInt n=d.LocateReverse(KExtDelimiter);
	if (n==KErrNotFound)
		{
		n=d.Length();
		if (IsSpace(d.Left(n)))
			return(KErrNone);
		}
	TPtrC v=d.Left(n);
	if (n && !aDone)
		{
		if (v.Locate(KMatchOne)!=KErrNotFound)	//	Found ? in the name
			iWild|=(EWildName|EWildEither|EWildIsKMatchOne);		
		if (v.Locate(KMatchAny)!=KErrNotFound)	//	Found * in the name
			iWild|=(EWildName|EWildEither|EWildIsKMatchAny);		
		if(iMod)
			{
			if (NameBuf().Length()+n>KMaxFileName)	
				return(KErrBadName);
			NameBuf()+=v;
			if (n==d.Length())
				NameBuf().TrimRight();
			}
		aDone=ETrue;
		}
	aName.SkipAndMark(n);
	return(KErrNone);
	}

TInt TParseBase::ParseExt(TLex& aName,TBool& aDone)
//
// Parse the extension.
//
	{

	TPtrC d=aName.RemainderFromMark();
	if (d.Length() && !IsSpace(d) && !aDone)
		{
		if (d.Locate(KMatchOne)!=KErrNotFound) //	Found ? in the name
			iWild|=(EWildExt|EWildEither|EWildIsKMatchOne);
		if (d.Locate(KMatchAny)!=KErrNotFound) //	Found * in the name
			iWild|=(EWildExt|EWildEither|EWildIsKMatchAny);
		
		if(iMod)
			{
			if (NameBuf().Length()+d.Length()>KMaxFileName)
				return(KErrBadName);
			NameBuf()+=d;
			NameBuf().TrimRight();
			}
		else
			aName.SkipAndMark(d.Length());
		aDone=ETrue;
		}
	return(KErrNone);
	}

TInt TParseBase::Set(const TDesC* aName,const TDesC* aRelated,const TDesC* aDefault,TBool allowWild)
//
// Parse a name. Optionally allow wild cards.
//
	{

	TInt (TParseBase::*parse[KLexComponents])(TLex& aName,TBool& aDone);
	parse[0]=&TParseBase::ParseDrive;
	parse[1]=&TParseBase::ParsePath;
	parse[2]=&TParseBase::ParseName;
	parse[3]=&TParseBase::ParseExt;
	
	iWild=0;

	Mem::FillZ(&iField[0],sizeof(iField));
	
	TLex name(*aName);
	TLex def;
	TLex rel;
	TInt lexnames;
	if(iMod)
		{
		if (aRelated)
			rel=(*aRelated);
		if (aDefault)
			def=(*aDefault);
		NameBuf().Zero();
		lexnames = KLexNames;
		}
	else
		{
		lexnames = 1;
		}
	
	TLex* lex[KLexNames];
	lex[0]=(&name);
	lex[1]=(&rel);
	lex[2]=(&def);
	
	name.Mark();
	rel.Mark();
	def.Mark();
	
	TInt r;
	TInt pos=0;
	
	for (TInt i=0;i<KLexComponents;i++)
		{
		TBool done=EFalse;
		for (TInt j=0;j<lexnames;j++)
			{
			if ((r=(this->*parse[i])(*lex[j],done))<KErrNone)
				return(r);
			if (j==0 && done)
				iField[i].present=ETrue;
			}
		TInt len;
		if(iMod)
			len=NameBuf().Length()-pos;
		else
			len=name.MarkedOffset()-pos;
		iField[i].len=(TUint8)len;
		iField[i].pos=(TUint8)pos;
		pos+=len;
		}
	if (!allowWild && iWild)
		return(KErrBadName);
	if (iField[EPath].len==1)
		iWild|=EIsRoot;
	return(KErrNone);
	}




EXPORT_C TInt TParseBase::PopDir()
/**
Removes the last directory from the path in the fully parsed specification.
 
This function may be used to navigate up one level in a directory hierarchy.
An error is returned if the specified directory is the root.

@return KErrNone if successful, otherwise one of the other system-wide error 
        codes.
*/
	{

	if (IsRoot())
		return(KErrGeneral);
	TInt len;
	if (iField[EName].pos==0 && NameBuf().Length()==256)
		len=256;
	else
		len=iField[EName].pos;
	TPtrC p(NameBuf().Ptr(),len-1);
	TInt pos=p.LocateReverse(KPathDelimiter)+1;
	len-=pos;
	NameBuf().Delete(pos,len);
	iField[EName].pos=(TUint8)(iField[EName].pos-len);
	iField[EExt].pos=(TUint8)(iField[EExt].pos-len);
	iField[EPath].len=(TUint8)(iField[EPath].len-len);
	if (iField[EPath].len==1)
		iWild|=EIsRoot;
	return(KErrNone);
	}




EXPORT_C TInt TParseBase::AddDir(const TDesC& aName)
/**
Adds a single directory onto the end of the path in
the fully parsed specification. 

The directory is inserted between the final directory, and the filename, if 
there is one.

@param aName The directory to be added. It must not start with a \\ otherwise 
             the function does not recognise it as a valid directory name
             and an error is returned.
             The directory name must not end with a \\ since the function 
             adds this automatically. It must not exceed the maximum
             filename length, KMaxFileName characters, otherwise  an error
             is returned.

@return KErrNone if successful, otherwise another of the system-wide error 
        codes.   
@see KMaxFileName
*/
	{

	if (aName.Length()==0)
		return(KErrNone);
	TInt len=aName.Length()+1;
	if ((len+NameBuf().Length())>NameBuf().MaxLength())
		return(KErrGeneral);
	TInt pos=aName.Locate(KPathDelimiter);
	if (pos!=KErrNotFound)
		return(KErrBadName);
	
	NameBuf().Insert(iField[EName].pos,aName);
	NameBuf().Insert(iField[EName].pos + aName.Length(),TPtrC16((const TUint16*)(&KPathDelimiter),1));
	iField[EPath].len=(TUint8)(iField[EPath].len+len);
	iField[EName].pos=(TUint8)(iField[EName].pos+len);
	iField[EExt].pos=(TUint8)(len+iField[EExt].pos);
	if (IsRoot())
		iWild&=~EIsRoot;
	return(KErrNone);
	}




EXPORT_C const TDesC& TParseBase::FullName() const
/**
Gets the complete file specification.

This is in the form:

drive-letter: \\path\\filename.extension

@return The fully parsed file specification.
*/
	{

	return(NameBufC());
	}




EXPORT_C TPtrC TParseBase::Drive() const
/**
Gets the drive letter.

The drive letter is in the form:

drive-letter:

Note that the drive letter is folded.

@return The drive letter and colon.
*/
	{

	const SField& f=iField[EDrive];
	return(NameBufC().Mid(f.pos,f.len));
	}




EXPORT_C TPtrC TParseBase::Path() const
/**
Gets the path.

The path is in the form:

\\path\\

@return The path. It always begins and ends in a backslash.
*/
	{

	const SField& f=iField[EPath];
	return(NameBufC().Mid(f.pos,f.len));
	}




EXPORT_C TPtrC TParseBase::DriveAndPath() const
/**
Gets the drive letter and path.

This is in the form

drive-letter:\\path\\

Note that the drive letter is folded

@return The drive and path.
*/
	{

	const SField& f=iField[EDrive];
	return(NameBufC().Mid(f.pos,f.len+iField[EPath].len));
	}




EXPORT_C TPtrC TParseBase::Name() const
/**
Gets the filename.

This is in the form

filename

@return The filename.
*/
	{

	const SField& f=iField[EName];
	return(NameBufC().Mid(f.pos,f.len));
	}




EXPORT_C TPtrC TParseBase::Ext() const
/**
Gets the extension.

This is in the form:

.extension

@return The extension and preceding dot.
*/
	{

	const SField& f=iField[EExt];
	return(NameBufC().Mid(f.pos,f.len));
	}




EXPORT_C TPtrC TParseBase::NameAndExt() const
/**
Gets the filename and extension.

This is in the form:

filename.ext

@return The filename and extension.
*/
	{

	const SField& f=iField[EName];
	return(NameBufC().Mid(f.pos,f.len+iField[EExt].len));
	}




EXPORT_C TBool TParseBase::DrivePresent() const
/**
Tests whether a drive is present.

Note that this function refers to a component 
in the aName argument specified in calls to TParse::Set(), TParse::SetNoWild() 
or RFs::Parse(), not to the resulting fully parsed file specification.

@return True if a drive present, false if not.

@see TParse
@see RFs
*/
	{

	return(iField[EDrive].present);
	}




EXPORT_C TBool TParseBase::PathPresent() const
/**
Tests whether a path is present.

Note that this function refers to a component 
in the aName argument specified in calls to TParse::Set(), TParse::SetNoWild() 
or RFs::Parse(), not to the resulting fully parsed file specification.

@return True if a path present, false if not.

@see TParse
@see RFs
*/
	{

	return(iField[EPath].present);
	}




EXPORT_C TBool TParseBase::NamePresent() const
/**
Tests whether a file name is present.

Note that this function refers to a component 
in the aName argument specified in calls to TParse::Set(), TParse::SetNoWild() 
or RFs::Parse(), not to the resulting fully parsed file specification.

This function returns true even if the filename specified in aName contains 
only wildcards. It only returns false if nothing is specified.

@return True if a name present, false if not.
*/
	{

	return(iField[EName].present);
	}




EXPORT_C TBool TParseBase::ExtPresent() const
/**
Tests whether an extension is present.

Note that this function refers to a component
in the aName argument specified in calls to TParse::Set(), TParse::SetNoWild() 
or RFs::Parse(), not to the resulting fully parsed file specification.

This function returns true even if the extension contains only wildcards. 
It only returns false if nothing is specified.

@return True if an extension present, false if not.
*/
	{

	return(iField[EExt].present);
	}




EXPORT_C TBool TParseBase::NameOrExtPresent() const
/**
Tests whether a filename or an extension are present.

Note that this function refers to a component in the aName argument
specified in calls to TParse::Set(), TParse::SetNoWild() or RFs::Parse(), not
to the resulting fully parsed file specification.

This function returns true even if the filename or extension specified in 
aName contain only wildcards. It only returns false if nothing is specified.

@return True if either a name or an extension or both are present,
        otherwise false.
*/
	{

	return(iField[EName].present || iField[EExt].present);
	}





EXPORT_C TBool TParseBase::IsRoot() const
/**
Tests whether the path in the fully parsed specification is the root directory.

@return True if path is root, false if not.
*/
	{

	return(iWild&EIsRoot);
	}




EXPORT_C TBool TParseBase::IsWild() const
/**
Tests whether the filename or the extension in the fully parsed specification 
contains one or more wildcard characters.

@return True if wildcards are present, false if not.
*/
	{

	return(iWild&EWildEither);	
	}




EXPORT_C TBool TParseBase::IsKMatchOne() const
/**
Tests whether the name or the extension contains a question mark wildcard.

@return True if either the name or extension has a ? wild card,
        false otherwise.
*/
	{

	return(iWild&EWildIsKMatchOne);	
	}




EXPORT_C TBool TParseBase::IsKMatchAny() const
/**
Tests whether the name or the extension contains asterisk wildcards.

@return True if either the name or extension has a * wild card,
        false otherwise.
*/
	{

	return(iWild&EWildIsKMatchAny);	
	}




EXPORT_C TBool TParseBase::IsNameWild() const
/**
Tests whether the filename in the fully parsed specification contains one or 
more wildcard characters.

@return True if the filename contains wildcard characters, false if not.
*/
	{

	return(iWild&EWildName);
	}




EXPORT_C TBool TParseBase::IsExtWild() const
/**
Tests whether the extension in the fully parsed specification contains one 
or more wildcard characters.

@return True if the extension contains wildcard characters, false if not.
*/
	{

	return(iWild&EWildExt);
	}




EXPORT_C TParse::TParse()
/**
Default constructor.
*/
	{
	iMod=1;
	}




EXPORT_C TInt TParse::Set(const TDesC& aName,const TDesC* aRelated,const TDesC* aDefault)
/**
Parses a file specification, allowing wildcards in the filename and extension.

This function sets up the TParse object so that it can be used to provide 
useful information.

@param aName    The file specification to be parsed.
@param aRelated The related file specification. This is optional,
                set to NULL to  omit.
@param aDefault The default file specification. This is optional,
                set to NULL to omit.
                
@return KErrNone, if successful, otherwise one of the other system-wide error
        codes.
*/
	{

	return(TParseBase::Set(&aName,aRelated,aDefault,ETrue));
	}




EXPORT_C TInt TParse::SetNoWild(const TDesC& aName,const TDesC* aRelated,const TDesC* aDefault)
/**
Parses a file specification; disallows wildcards in any part of the file name 
or extension.

If you need to specify wildcards use Set(). Otherwise, this 
function behaves in the same way as Set().

@param aName    The file specification to be parsed.
@param aRelated The related file specification. This is optional,
                set to NULL to omit.
@param aDefault The default file specification. This is optional,
                set to NULL to omit.
                
@return KErrNone, if successful, otherwise one of the other system-wide error 
        codes.

@see TParse::Set
*/
	{

	return(TParseBase::Set(&aName,aRelated,aDefault,EFalse));
	}




EXPORT_C TDes& TParse::NameBuf()
/**
Gets a reference to the descriptor containing the file specification passed to
the constructor of this object. 

@return A reference to the descriptor containing the filename.
*/
	{

	return(iNameBuf);
	}




EXPORT_C const TDesC& TParse::NameBufC() const
/**
Gets a const reference to the descriptor containing the file specification
passed to the constructor of this object. 

@return A const reference to the descriptor containing the file specification.
*/
	{

	return(iNameBuf);
	}




EXPORT_C TParsePtr::TParsePtr(TDes& aName)
	: iNameBuf((TText*)aName.Ptr(),aName.Length(),aName.MaxLength())
/**
Constructor taking a reference to a filename.

The specified filename is parsed and if this fails, a panic is raised.

@param aName Reference to the filename to be parsed. On return contains
             the fully parsed path specification. If a filename and extension
             are specified, they may both contain wildcards.
             The maximum length is KMaxFileName characters.
             
@panic FSCLIENT 24 if the the specified name fails to parse.
             
@see KMaxFileName
*/
	{
	iMod=1;
	TInt r=TParseBase::Set(&aName,NULL,NULL,ETrue);
	__ASSERT_ALWAYS(r==KErrNone,Panic(EParsePtrBadDescriptor0));
	}




EXPORT_C TDes& TParsePtr::NameBuf()
/**
Gets a reference to the descriptor containing the filename passed to
the constructor of this object. 

@return A reference to the descriptor containing the filename.
*/
	{

	return(iNameBuf);
	}




EXPORT_C const TDesC& TParsePtr::NameBufC() const
/**
Gets a const reference to the descriptor containing the filename passed to
the constructor of this object. 

@return A const reference to the descriptor containing the filename.
*/
	{

	return(iNameBuf);
	}




EXPORT_C TParsePtrC::TParsePtrC(const TDesC& aName)
/**
Constructor taking a constant reference to a filename.

The filename is parsed and if this fails, a panic is raised.
Note that the filename cannot be modified using this class.

@param aName Constant reference to the filename to be parsed.
             On return contains the fully parsed filename.
             If a file and extension are specified, they may both
             contain wildcards.
             The maximum length is KMaxFileName characters.
             
@panic FSCLIENT 24 if the the specified name fails to parse.

@see KMaxFileName
*/
	{
	iMod=0;
	iNameBuf.Set(aName);
	TInt r = TParseBase::Set(&aName,NULL,NULL,ETrue);
	__ASSERT_ALWAYS(r==KErrNone,Panic(EParsePtrBadDescriptor0));
	}




EXPORT_C TDes& TParsePtrC::NameBuf()
/**
Gets a reference to the descriptor containing the filename passed to
the constructor of this object. 

@return A reference to the descriptor containing the filename.
*/
	{

	Panic(EParsePtrCAccessError);
	return(*(TDes*)&iNameBuf);
	}




EXPORT_C const TDesC& TParsePtrC::NameBufC() const
/**
Gets a const reference to the descriptor containing the filename passed to
the constructor of this object. 

@return A const reference to the descriptor containing the filename.
*/
	{

	return(iNameBuf);
	}

