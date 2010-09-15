// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// LDD for testing SDIO functions
// 
//

#if !defined(__SDIO_IO_H__)
#define __SDIO_IO_H__

#include "tdisplay.h"

/**
Macro to format a variable length number of parameters into the iText member variable.

@param c The format string

@internal
@test
*/
#define FORMAT_TEXT(c) VA_LIST list; \
	VA_START(list, c); \
	FormatText(c, list); \
	VA_END(list)

class CIOBase : public CBase
/**
Base class to provide input/output facilities. Uses the function signatures from THexDisplay, 
with dummy implementation.

@internal
@test
*/
	{
public:

#if defined(_UNICODE)
	class TIOOverflowHandler : public TDes16Overflow
	/**
	Base class to handle any logging overflows. 

	@internal
	@test
	*/
		{
		public:
			/**
			Handle the overflow. 

			@param aDes The descriptor at its maximum length.
			
			@internal
			@test
			*/
			virtual void Overflow(TDes16 &aDes);
		};
#else
	class TIOOverflowHandler : public TDes8Overflow
	/**
	Base class to handle any logging overflows. 

	@internal
	@test
	*/
		{
		public:
			/**
			Handle the overflow. 

			@param aDes The descriptor at its maximum length.
			
			@internal
			@test
			*/
			virtual void Overflow(TDes8 &aDes);
		};
#endif
	
	/**
	Constructor. 

	@internal
	@test
	*/
	CIOBase() {};

	/**
	Destructor. 

	@internal
	@test
	*/
	~CIOBase() {};

	/**
	Create the underlying resource. 

	@param aName The name for the tests.
	
	@internal
	@test
	*/
	virtual void CreateL(TPtrC aName) {};

	/**
	Provide a heading for the next test. 

	@param aFmt The heading format.
	
	@internal
	@test
	*/
	void Heading(TRefByValue<const TDesC> aFmt,...);

	/**
	Provide some instructions. 

	@param aTopLine Whether to start from the top.
	@param aFmt The formmatted text..
	
	@internal
	@test
	*/
	void Instructions(TBool aTopLine, TRefByValue<const TDesC> aFmt,...);

	/**
	Print formatted output. 

	@param aFmt The formmatted text.
	
	@internal
	@test
	*/
	void Printf(TRefByValue<const TDesC> aFmt,...);

	/**
	Report an error. 

	@param aErrText The heading format.
	@param aErr The error code.
	
	@internal
	@test
	*/
	virtual void ReportError(TPtrC aErrText, TInt aErr = KErrNone) {};

	/**
	Move to the curser to the start of the line. 
	
	@internal
	@test
	*/
	virtual void CurserToDataStart() {};

	/**
	Get a character from the input stream.
	
	@return The keycode of the input character. 
	
	@internal
	@test
	*/
	virtual TKeyCode Getch() { return EKeyNull; };
	
	/**
	Clear the screen.
		
	@internal
	@test
	*/
	virtual void ClearScreen() {};
	
protected:
	virtual void DoHeading() {};
	virtual void DoInstructions(TBool aTopLine) {};
	virtual void DoPrintf() {};
	void FormatText(TRefByValue<const TDesC> aFmt, VA_LIST aList);

protected:	
	/** A temporary buffer for formatting text */
	TBuf<512>			iText;
	/** An overflow handler */
	TIOOverflowHandler	iOverflowHandler;
	};

class CIOConsole : public CIOBase
/**
Class to provide input/output facilities using THexDisplay.

@internal
@test
*/
	{
public:
	/**
	Constructor. 

	@internal
	@test
	*/
	CIOConsole() {};

	/**
	Destructor. 

	@internal
	@test
	*/
	~CIOConsole();
	
	/**
	Create the underlying resource. 

	@param aName The name for the tests.
	
	@internal
	@test
	*/
	virtual void CreateL(TPtrC aName);

	/**
	Report an error. 

	@param aErrText The heading format.
	@param aErr The error code.
	
	@internal
	@test
	*/
	virtual void ReportError(TPtrC aErrText, TInt aErr=KErrNone);

	/**
	Move to the curser to the start of the line. 
	
	@internal
	@test
	*/
	virtual void CurserToDataStart();

	/**
	Get a character from the input stream.
	
	@return The keycode of the input character. 
	
	@internal
	@test
	*/
	virtual TKeyCode Getch();

	/**
	Clear the screen.
		
	@internal
	@test
	*/
	virtual void ClearScreen();	
	
protected:
	virtual void DoHeading();
	virtual void DoInstructions(TBool aTopLine);
	virtual void DoPrintf();

private:
	THexDisplay	iDisplay;
	};

class CIORDebug : public CIOBase
/**
Class to provide input/output facilities using RDebug.

@internal
@test
*/
	{
public:
	/**
	Constructor. 

	@internal
	@test
	*/
	CIORDebug() {};

	/**
	Destructor. 

	@internal
	@test
	*/
	~CIORDebug() {};
	
	/**
	Create the underlying resource. 

	@param aName The name for the tests.
	
	@internal
	@test
	*/
	virtual void CreateL(TPtrC aName);

	/**
	Report an error. 

	@param aErrText The heading format.
	@param aErr The error code.
	
	@internal
	@test
	*/
	virtual void ReportError(TPtrC aErrText, TInt aErr=KErrNone);
	
	/**
	Clear the screen.
	
	@internal
	@test
	*/
	virtual void ClearScreen();	

protected:
	virtual void DoHeading();
	virtual void DoPrintf();
	};

#endif // __SDIO_IO_H__

