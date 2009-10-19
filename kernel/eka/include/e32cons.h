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
// e32\include\e32cons.h
// 
//

#ifndef __E32CONS_H__
#define __E32CONS_H__
#include <e32base.h>
#include <e32keys.h>
//

/**
@publishedAll
@released

Defines a default console width that can be used when creating a console.

@see CConsoleBase::Create()
*/
const TInt KDefaultConsWidth=78;

/**
@publishedAll
@released

Defines a default console height that can be used when creating a console.

@see CConsoleBase::Create()
*/
const TInt KDefaultConsHeight=18;

/**
@publishedAll
@released

Implies a full size screen console when passed as the width and height
values when creating a console.

@see CConsoleBase::Create()
*/
const TInt KConsFullScreen=-1;


/**
@publishedAll
@released

Defines a set of text attributes used for consoles that support colour.

@see CColorConsoleBase::SetTextAttribute().
*/
enum TTextAttribute
	{
	ETextAttributeNormal,  /**< Defines the normal text attribute.   */
	ETextAttributeBold,    /**< Defines the bold text attribute.     */
	ETextAttributeInverse, /**< Defines the inverse text attribute.  */
	ETextAttributeHighlight/**< Defines the highlight text attribute.*/
	};


/**
@publishedAll
@released

A base class that defines a console interface.
*/
class CConsoleBase : public CBase
	{
public:
	IMPORT_C virtual ~CConsoleBase();
	IMPORT_C TKeyCode Getch();
	IMPORT_C void Printf(TRefByValue<const TDesC> aFmt,...);
	IMPORT_C void SetPos(TInt aX);
	IMPORT_C void SetPos(TInt aX,TInt aY);
	IMPORT_C TInt WhereX() const;
	IMPORT_C TInt WhereY() const;
// Pure virtual


    /**
    Creates a new console window.
    
    @param aTitle The title text for the console.
                  This should not be longer than 256 characters.
    @param aSize  The size of the console window.
    
    @return KErrNone, if successful; otherwise one of the other
                      system wide error codes.
    */
	virtual TInt Create(const TDesC &aTitle,TSize aSize) =0;

	
    /**
    Gets a keystroke from the console window, asynchronously.
    
    @param aStatus The request status object.
    */
	virtual void Read(TRequestStatus &aStatus) =0;
	
	
	/**
	Cancels any outstanding request to get a keystroke from the console window.
	*/
	virtual void ReadCancel() =0;
	
	
	/**
	Writes the content of the specified descriptor to the console window.
	
	@param aDes Descriptor containing the characters to be written to
	            the console window.
	*/
	virtual void Write(const TDesC &aDes) =0;
	
	
	/**
	Gets the current cursor position relative to the console window.
	
	@return  The current cursor position.
	*/
	virtual TPoint CursorPos() const =0;
	
	
	/**
    Puts the cursor at the absolute position in the window.
    
    @param aPoint The cursor position.
	*/
	virtual void SetCursorPosAbs(const TPoint &aPoint) =0;
	
	
	/**
	Puts the cursor at the specified position relative
	to the current cursor position.
	
	@param aPoint The cursor position.
	*/
	virtual void SetCursorPosRel(const TPoint &aPoint) =0;
	
	
	/**
	Sets the percentage height of the cursor.
	
    @param aPercentage The percentage height. This is a value from 0 to 100.
                       If 0 is specified, then no cursor is displayed.
	*/
	virtual void SetCursorHeight(TInt aPercentage) =0;

	
	/**
	Sets a new console title.
	
	@param aTitle The title text for the console.
                  This should not be longer than 256 characters.
	*/
	virtual void SetTitle(const TDesC &aTitle) =0;

	
	/**
	Clears the console.
	*/
	virtual void ClearScreen() =0;
	
	
	/**
	Clears the console from the current cursor position to
	the end of the line.
	*/
	virtual void ClearToEndOfLine() =0;
	
	
	/**
	Gets the size of the console.
	*/
	virtual TSize ScreenSize() const =0;
	
	
	/**
	Gets the current key code value.
	
	@return The key code value.
	*/
	virtual TKeyCode KeyCode() const =0;
	
	/**
	Gets the current key modifiers.
	
	@return The key modifiers.
	*/
	virtual TUint KeyModifiers() const =0;
protected:
	IMPORT_C CConsoleBase();
protected:
	IMPORT_C virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);
	};


class CProxyConsole;

/**
@publishedAll
@released

Adds colour support to the basic console interface.
*/
class CColorConsoleBase : public CConsoleBase
	{
public:

    /**
    Sets the text attribute as defined by TTextAttribute.
    
    @param anAttribute The text attribute to be set.
    */
	virtual void SetTextAttribute(TTextAttribute /*anAttribute*/); 
protected:
	IMPORT_C virtual TInt Extension_(TUint aExtensionId, TAny*& a0, TAny* a1);

	friend class CProxyConsole;
	};
//

/**
@publishedAll
@released
*/
extern "C" {
IMPORT_C TAny *NewConsole();
}
#endif

