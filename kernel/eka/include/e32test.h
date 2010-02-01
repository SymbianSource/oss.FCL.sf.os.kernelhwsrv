// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32test.h
// 
//

/**
 @file e32test.h
 @publishedAll
 @released
*/

#ifndef __E32TEST_H__
#define __E32TEST_H__
#include <e32std.h>
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <e32std_private.h>
#include <e32base_private.h>
#endif
#include <e32base.h>
#include <e32cons.h>
#include <e32kpan.h>
#include <e32debug.h>
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <e32def_private.h>
#include <e32event_private.h>
#endif


/**
Test console.

The class creates a console window to which test results can be logged
through the various overloads of the operator().
*/
class RTest
	{
public:
	IMPORT_C RTest(const TDesC &aTitle,TInt aThrowaway,const TText* anOtherThrowaway);
	IMPORT_C RTest(const TDesC &aTitle,TInt aThrowaway);
	IMPORT_C RTest(const TDesC &aTitle);
	IMPORT_C void Close();
	IMPORT_C void Title();
	IMPORT_C void Start(const TDesC &aHeading);
	IMPORT_C void Next(const TDesC &aHeading);
	IMPORT_C void End();
	IMPORT_C void operator()(TInt aResult,TInt aLineNum,const TText* aFileName);
	IMPORT_C void operator()(TInt aResult,TInt aLineNum);
	IMPORT_C void operator()(TInt aResult);
	IMPORT_C void Panic(TInt anError,TRefByValue<const TDesC> aFmt,...);
	IMPORT_C void Panic(TRefByValue<const TDesC> aFmt,...);
	IMPORT_C void Printf(TRefByValue<const TDesC> aFmt,...);
	IMPORT_C TKeyCode Getch();
	inline static const TAny* String(TInt aSel,const TText8 *aBuf1,const TText16 *aBuf2);
	inline CConsoleBase* Console() const;
	inline void SetConsole(CConsoleBase* aConsole);
	inline TBool Logged() const;
	inline void SetLogged(TBool aToLog);
	inline void HandleError(TInt aError, TInt aLine, const TText* aFileName);
	inline void HandleNull(TInt aLine, const TText* aFileName);
	inline void HandleNotEqual(TInt aExpected, TInt aActual, TInt aLine, const TText* aFileName);
	inline void HandleFailedCompare(TInt aLeft, const TText* aComp, TInt aRight, TInt aLine, const TText* aFileName);
	inline void HandleValue(TInt aValue,  TInt aLine, const TText* aFileName);

	IMPORT_C static TInt CloseHandleAndWaitForDestruction(RHandleBase& aH);	/**< @internalTechnology */

protected:
	void CheckConsoleCreated();
	void DisplayLevel();
	inline void Push();
	inline void Pop();
private:
	enum {EMaxStack=0x100,EMaxBuffer=0x100};
private:
	TInt	iTest;
	TInt	iCheck;
	TInt	iLevel;
	TBool	iLogging;
	CConsoleBase *iConsole;
	TBuf<0x40> iTitle;
	TInt iStack[EMaxStack];
	TText iBuf[EMaxBuffer];
	};




/**
Gets the console.

@return A pointer to the console object.
*/
inline CConsoleBase* RTest::Console() const
	{ return(iConsole); }

	
	
	
/**
Utility function that returns a pointer to the specified TText8* argument
or the TText16* argument depending on the value of the aSel argument.

@param aSel  An integer containing the size of a TText8 type or TText16 type.
@param aBuf1 A pointer to 8-bit text.
@param aBuf2 A pointer to 16-bit text.

@return A pointer to aBuf1, if the value of aSel is the size of a TText8 type,
        otherwise a pointer to aBuf2.
*/	
inline const TAny *RTest::String(TInt aSel,const TText8 *aBuf1,const TText16 *aBuf2)
	{ return(aSel == sizeof(TText8) ? (TAny *)aBuf1 : (TAny *)aBuf2); }



/**
@internalComponent
*/
inline void RTest::Push()
	{ iStack[iLevel++] = iTest; iTest = 0; }



/**
@internalComponent
*/
inline void RTest::Pop()
	{ iTest = iStack[--iLevel]; }




/**
Sets the console.

@param aConsole A pointer to the console object to be used.
*/	
inline void RTest::SetConsole(CConsoleBase* aConsole)
    { iConsole = aConsole; }




/**
Tests whether the logging flag is set.

If the logging flag is set, console output is also written to
the debug output as represented by a RDebug object.

@return True, if the logging flag is set, false otherwise.
*/	
inline TBool RTest::Logged() const
	{ return(iLogging); }




/**
Sets the logging flag.

If the logging flag is set, console output is also written to
the debug output as represented by a RDebug object.

@param aToLog ETrue, if the logging flag is to be set, EFalse, otherwise.
*/	
inline void RTest::SetLogged(TBool aToLog)
	{ iLogging = aToLog; }




// test equivalent of _L
/**
@internalComponent
*/
#define _TL(a) (S*)RTest::String(sizeof(S),(TText8*)a,(TText16*)L ## a) 

// the next two, slightly confusing, macros are necessary in order
// to enable proper string merging with certain compilers.

/**
@internalComponent
*/
#define __test(x,l,f) test(x,l,_S(f))

/**
@internalComponent
*/
#define test(x) __test(x,__LINE__,__FILE__)


#ifdef __E32TEST_EXTENSION__

/**
@internalComponent
*/
#define __S(f) _S(f)

/**
@internalComponent

Panics and displays an appropriate error message if x is less then zero (Indicating an error code).
*/
#define test_NotNegative(x) { TInt _r = (x); if (_r < 0) test.HandleError(_r, __LINE__,__S(__FILE__)); }

/**
@internalComponent

Panics and displays an appropriate error message if x is not equal to KErrNone.
*/
#define test_KErrNone(x) { TInt _r = (x); if (_r !=KErrNone) test.HandleError(_r, __LINE__,__S(__FILE__)); }

/**
@internalComponent

Panics and displays an appropriate error message if the trapped statement/block x leaves.
*/
#define test_TRAP(x) { TRAPD(_r, x); if (_r != KErrNone) test.HandleError(_r, __LINE__,__S(__FILE__)); }

/**
@internalComponent

Panics and displays an appropriate error message if x is not equal to NULL.
*/
#define test_NotNull(x) { TAny* _a = (TAny*)(x); if (_a == NULL) test.HandleNull(__LINE__,__S(__FILE__)); }
/**
@internalComponent

Panics and displays an appropriate error message if e (expected) is not equal to a (actual).
*/
#define test_Equal(e, a) { TInt _e = TInt(e); TInt _a = TInt(a); if (_e != _a) test.HandleNotEqual(_e, _a, __LINE__,__S(__FILE__)); }

/**
@internalComponent

Panics and displays an appropriate error message if the comparison specified with operator b, between a and c, is EFalse.
*/
#define test_Compare(a,b,c)  {TInt _a = TInt(a); TInt _c = TInt(c); if (!(_a b _c)) test.HandleFailedCompare(_a, __S(#b), _c, __LINE__,__S(__FILE__)); }


/**
@internalComponent

Panics and displays an appropriate error message displaying v, if the expression e is false.
*/
#define test_Value(v, e) if (!(e)) test.HandleValue(v,  __LINE__,__S(__FILE__));

/**
@internalComponent

If expression e is false, statement s is executed then a Panic is raised.
*/
#define test_Assert(e,s) if(!(e)) {s; test.operator()(EFalse, __LINE__,__S(__FILE__)); }
	
	

#endif


/**
Prints a failure message, including an error code at the console and raises a panic.


@param aError	 The error code to be printed in the failure massage.
@param aLineNum  A line number that is printed in the failure message.
@param aFileName A file name that is printed in the failure message.
                 
@panic USER 84 Always.
*/
inline void RTest::HandleError(TInt aError, TInt aLine, const TText* aFileName)
	{
	RDebug::Printf("RTEST: Error %d at line %d", aError,aLine);
	Printf(_L("RTEST: Error %d\n"), aError);
	operator()(EFalse, aLine, aFileName);
	}
/**
Prints a failure message indicating null was encountered, at the console and raises a panic.

@param aLineNum  A line number that is printed in the failure message.
@param aFileName A file name that is printed in the failure message.
                 
@panic USER 84 Always.
*/

inline void RTest::HandleNull(TInt aLine, const TText* aFileName)
	{
	RDebug::Printf("RTEST: Null value at line %d", aLine);
	Printf(_L("RTEST: Null value\n"));
	operator()(EFalse, aLine, aFileName);
	}


/**
Prints a failure message indicating that two value (also printed) where not equal, at the console and raises a panic.

@param aExpected The value that is to be printed as expected.
@param aActual	 The value that is to be printed as being actually received.
@param aLineNum  A line number that is printed in the failure message.
@param aFileName A file name that is printed in the failure message.
                 
@panic USER 84 Always.
*/

inline void RTest::HandleNotEqual(TInt aExpected, TInt aActual, TInt aLine, const TText* aFileName)
	{
	RDebug::Printf("RTEST: Expected 0x%x (%d) but got 0x%x (%d) at line %d", aExpected,aExpected,aActual,aActual,aLine);
	Printf(_L("RTEST: Expected 0x%x (%d) but got 0x%x (%d)\n"), aExpected,aExpected,aActual,aActual);
	operator()(EFalse, aLine, aFileName);
	}


/**
Prints a failure message indicating that a comparison between two values (also printed) resulted in EFalse,
at the console and raises a panic.

@param aLeft 	 The left value of the comparison.
@param aComp	 A string representing the comparison operator.
@param aRight	 The right value of the comparison.
@param aLineNum  A line number that is printed in the failure message.
@param aFileName A file name that is printed in the failure message.
                 
@panic USER 84 Always.
*/
inline void RTest::HandleFailedCompare(TInt aLeft, const TText* aComp, TInt aRight, TInt aLine, const TText* aFileName)
	{
	RDebug::Printf("RTEST: (0x%x (%d) %s 0x%x (%d)) == EFalse at line %d", aLeft,aLeft,aComp,aRight,aRight,aLine);
	Printf(_L("RTEST: (0x%x (%d) %s 0x%x (%d)) == EFalse\n"), aLeft,aLeft,aComp, aRight,aRight);
	operator()(EFalse, aLine, aFileName);
	}


/**
Prints a failure message indicating that aValue was not an expected value, at the console and raises a panic.

@param aValue The value that is to be printed as not being an expected value.
@param aLineNum  A line number that is printed in the failure message.
@param aFileName A file name that is printed in the failure message.
                 
@panic USER 84 Always.
*/
inline void RTest::HandleValue(TInt aValue,  TInt aLine, const TText* aFileName)
	{
	Printf(_L("RTEST: %d (0x%x) was not an expected value.\n"), aValue, aValue);
	operator()(EFalse, aLine, aFileName);
	}


/**
@internalTechnology
*/
_LIT(KLitCloseAndWait,"Close&Wait");

/**
@internalTechnology
*/
#define CLOSE_AND_WAIT(h)	((void)(RTest::CloseHandleAndWaitForDestruction(h) && (User::Panic(KLitCloseAndWait,__LINE__),1)))



#endif

