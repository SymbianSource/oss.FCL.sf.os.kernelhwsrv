/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
*/


//------------------------------------ System Header Files ------------------------------------------------
#include <e32cons.h>        // ConsoleBase
//------------------------------------ Local Header Files -------------------------------------------------
#include "openc.h"

//-------------------------------- Constants, global variables and Macro Definitions ----------------------------------------
_LIT(KConsoleName, "NIST Test Console");
CConsoleBase* gConsole = NULL;


int PrintToScreen(const char* aString);

const TInt KIntStringLen = 10;
RFs gFileSession;

enum TDataType
    {
    EInteger,
    EFloat,
    EUnknownType = 0x10000
    };

const TInt KMaxReadSize = 50;
const TInt KMaxScreenBufferSize = 100;

TBuf<KMaxScreenBufferSize> gScreenBuffer;

class GlobalInitilizer
    {
public:
    GlobalInitilizer()
        {
        TInt err = gFileSession.Connect();

        if(err != KErrNone)
            {
            User::Exit(err);
            }

        TRAP(err, gConsole = Console::NewL(KConsoleName, TSize(KConsFullScreen,KConsFullScreen)));
        if(err != KErrNone)
            {
            User::Exit(err);
            }
        }
    ~GlobalInitilizer()
        {
        gFileSession.Close();
        delete gConsole;
        }
    };

GlobalInitilizer globalObj;

//------------------------------------ Function Definitions -----------------------------------------------

double log(double aSource)
    {
    double result = 0.0;
    Math::Ln(result, aSource);
    return result;
    }

double exp(double aSource)
    {
    double result = 0.0;
    Math::Exp(result, aSource);
    return result;
    }

double fabs(double aSource)
    {
    return (aSource >= 0.0)? aSource: -aSource;
    }


double floor(double aSource)
    {
    if(aSource >= 0.0 || aSource == (TInt64)aSource)
        {
        return (double)(TInt64)aSource;
        }
   
    return (double)((TInt64)aSource - 1);
    }

double sqrt(double aSource)
    {
    double result = 0.0;
    Math::Sqrt(result, aSource);
    return result;
    }

double pow(double aSource, double aPower)
    {
    double result = 0.0;
    Math::Pow(result, aSource, aPower);
    return result;
    }

// Math trigonometric functions
double sin(double aSource)
    {
    double result = 0.0;
    Math::Sin(result, aSource);
    return result;
    }

double cos(double aSource)
    {
    double result = 0.0;
    Math::Cos(result, aSource);
    return result;
    }

// Stdio functions
int printf(const char* aFormatString, ...)
    {
    TUint length = User::StringLength((TUint8*)aFormatString) + 100;
    HBufC8* buffer = HBufC8::New(length);
    if(NULL == buffer)
        {
        return KErrNoMemory;
        }
    
    TPtr8 targetPtr = buffer->Des();
    TPtrC8 formatPtr((TUint8*)aFormatString);

    VA_LIST list;
    VA_START(list, aFormatString);
    
    targetPtr.FormatList(formatPtr, list);
    
    PrintToScreen((const char*)targetPtr.PtrZ());

    delete buffer;

    return targetPtr.Length();
    }

int puts(const char* aString)
    {
    int ret = PrintToScreen(aString);
    gConsole->Printf(_L("\n"));
    
    return ret;
    }

int putchar(int aChar)
    {
    gConsole->Printf(_L("%c"), aChar);
    return aChar;
    }

char* strcpy(char* aDst, const char* aSrc)
    {
    char* cp = aDst;

    while((*cp++ = *aSrc++) != 0)
        ; // Copy src over dst

    return(aDst);
    }

int scanf(const char* aFormatString, ...)
    {
    TDataType type = EUnknownType;
    TBool byteRead = EFalse;
    
    if(Mem::Compare((const unsigned char*)aFormatString, 2, (const unsigned char*)"%d", 2) == 0)
        {
        type = EInteger;
        }
    else if(Mem::Compare((const unsigned char*)aFormatString, 2, (const unsigned char*)"%f", 2) == 0)
        {
        type = EFloat;
        }
    else if(Mem::Compare((const unsigned char*)aFormatString, 3, (const unsigned char*)"%1d", 3) == 0)
        {
        type = EInteger;
        byteRead = ETrue;
        }
    else
        {
        User::Panic(_L("NIST TestSuit Error"), KErrArgument);
        }
    
    if(!byteRead || (gScreenBuffer.Length() == 0))
        {
        ReadStringFromConsole(gScreenBuffer);
        }

    TLex parser(gScreenBuffer);
    parser.SkipSpace();
    
    VA_LIST list;
    VA_START(list, aFormatString);

    switch(type)
        {
        case EInteger:
            {
            TInt* ptr = VA_ARG(list, TInt*);
            if(byteRead)
                {
                TChar ch(gScreenBuffer[0]);
                gScreenBuffer.Delete(0, 1);
                *ptr = ch.GetNumericValue();
                }
            else
                {
                parser.Val(*ptr);
                gScreenBuffer.Zero();
                }
            break;
            }
        case EFloat:
            {
            float* ptr = VA_ARG(list, float*);
            parser.Val(*ptr);
            gScreenBuffer.Zero();
            break;
            }
        case EUnknownType:
            {
            User::Panic(_L("NIST TestSuit Error"), KErrArgument);
            }
        }

    return 1;
    }

int sprintf(char *aBuffer, const char* aFormatString, ...)
    {
    TUint length = User::StringLength((TUint8*)aFormatString) + 100;
    TPtr8 aTargetPtr((TUint8*)aBuffer, length);
    TPtrC8 formatPtr((TUint8*)aFormatString);

    VA_LIST list;
    VA_START(list, aFormatString);
    
    aTargetPtr.FormatList(formatPtr, list);
    aTargetPtr.ZeroTerminate();

    return User::StringLength((TUint8*)aBuffer);;
    }

int GetFileMode(const char* aModeStr, TFileMode& aFileMode, TBool& aIsAppend)
    {
    aIsAppend = EFalse;
    switch (*aModeStr)
        {
        case 'r':
            aFileMode = EFileRead;
            break;
            
        case 'w':
            aFileMode = EFileWrite;
            break;
            
        case 'a':
            aFileMode = EFileWrite;
            aIsAppend = ETrue;
            break;
            
        default:
            return KErrArgument;
        } 
      
    return KErrNone;
    }

FILE *fopen(const char *aFileName, const char *aMode)
    {
    TPtrC8 fileNamePtr(reinterpret_cast<const unsigned char*>(aFileName));
    TFileName fileName;
    fileName.Copy(fileNamePtr);
    RFile* file = new RFile;
    if(NULL == file)
        {
        return NULL;
        }

    TFileMode mode = EFileRead;
    TBool isAppend = EFalse;
    GetFileMode(aMode, mode, isAppend);
    int err = KErrArgument;
    switch(mode)
        {
        case EFileRead:
            {
            err = file->Open(gFileSession, fileName, mode);
            break;
            }
        case EFileWrite:
            {
            if(isAppend)
                {
                err = file->Open(gFileSession, fileName, mode);
                if(err == KErrNone)
                    {
                    TInt offset = 0;
                    err = file->Seek(ESeekEnd, offset);
                    break;
                    }
                }
            
            err = file->Replace(gFileSession, fileName, mode);
            break;
            }
        default:
            err = KErrArgument;
        }

    if(KErrNone != err)
        {
        file->Close();
        delete file;
        file = NULL;
        }
    return file;
    }



int fclose(FILE *aFp)
    {
    if(NULL != aFp)
        {
        aFp->Close();
        delete aFp;
        }
    return KErrNone;
    }

int fprintf(FILE* aFile, const char* aFormatString, ...)
    {
    TUint length = User::StringLength((TUint8*)aFormatString) + 100;
    HBufC8* buffer = HBufC8::New(length);
    if(NULL == buffer)
        {
        return KErrNoMemory;
        }
    
    TPtr8 targetPtr = buffer->Des();
    TPtrC8 formatPtr((TUint8*)aFormatString);

    VA_LIST list;
    VA_START(list, aFormatString);
    
    targetPtr.FormatList(formatPtr, list);
    targetPtr.ZeroTerminate();
    
    aFile->Write(targetPtr);
    
    delete buffer;

    return targetPtr.Length();
    }

int fscanf(FILE* aFp, const char * aFormatString, ...)
    {
    TDataType type = EUnknownType;
    TBool byteRead = EFalse;
    
    if(Mem::Compare((const unsigned char*)aFormatString, 2, (const unsigned char*)"%d", 2) == 0)
        {
        type = EInteger;
        }
    else if(Mem::Compare((const unsigned char*)aFormatString, 2, (const unsigned char*)"%f", 2) == 0)
        {
        type = EFloat;
        }
    else if(Mem::Compare((const unsigned char*)aFormatString, 3, (const unsigned char*)"%1d", 3) == 0)
        {
        type = EInteger;
        byteRead = ETrue;
        }
    else
        {
        User::Panic(_L("NIST TestSuit Error"), KErrArgument);
        }
    
    TInt initialOffset = 0;
    aFp->Seek(ESeekCurrent, initialOffset);
    TBuf8<KMaxReadSize + 1> readBuffer;
    aFp->Read(readBuffer, KMaxReadSize);
    readBuffer.ZeroTerminate();
    TLex8 parser(readBuffer);
    parser.SkipSpace();
    
    VA_LIST list;
    VA_START(list, aFormatString);

    switch(type)
        {
        case EInteger:
            {
            TInt* ptr = VA_ARG(list, TInt*);
            TChar ch = parser.Peek();
            
            if(!ch.IsDigit())
                {
                break;
                }
            
            if(byteRead)
                {
                ch = parser.Get();
                *ptr = ch.GetNumericValue();
                }
            else
                {
                parser.Val(*ptr);
                }

            break;
            }
        case EFloat:
            {
            float* ptr = VA_ARG(list, float*);
            parser.Val(*ptr);
            break;
            }
        case EUnknownType:
            {
            User::Panic(_L("NIST TestSuit Error"), KErrArgument);
            }
        }

    TInt len = initialOffset + parser.Offset();
    aFp->Seek(ESeekStart, len);
    
    return 1;
    }

TUint32 fread(void* aPtr, TUint32 aSize, TUint32 aCount, FILE* aFile)
    {
    TUint32 size = aSize * aCount;
    TPtr8 dataPtr((TUint8*)aPtr, size);
    TInt err = aFile->Read(dataPtr);
    if(KErrNone != err)
        {
        size = (TUint32)dataPtr.Length();
        }
    return size;
    }

int fseek(FILE* aFile, long aOffset, int aWhence)
    {
    int ret = KErrNone;
    int fileOffset = aOffset;
    switch(aWhence)
        {
        case SEEK_SET:
            {
            ret = aFile->Seek(ESeekStart, fileOffset);
            break;
            }
        case SEEK_CUR:
            {
            ret = aFile->Seek(ESeekCurrent, fileOffset);
            break;
            }
        case SEEK_END:
            {
            ret = aFile->Seek(ESeekEnd, fileOffset);
            break;
            }
        default:
            User::Panic(_L("NIST TestSuit Error"), KErrArgument);
        }
    
    return ret;
    }



int fflush(FILE *aFile)
    {
    TInt err = aFile->Flush();
    if(err != KErrNone)
        {
        err = EOF;
        }
    return err;
    }


// Conio functions
void* calloc(TUint32 aElementCount, TUint32 aSize)
    {
    aSize *= aElementCount;
    return User::AllocZ(aSize);
    }

void free(void *aMemory)
    {
    User::Free(aMemory);
    }

TBool IsCharacterKey(TKeyCode aKey)
    {
    if(aKey > EKeyEscape && aKey < EKeyDelete)
        {
        return ETrue;
        }
    
    return EFalse;
    }

_LIT(KLineBreaker, "\r\n");

void ReadStringFromConsole(TDes& aString)
    {
    // This infinte loop terminates when user hits an "enter" key
    FOREVER
        {
        // Get a key(character) from the console
        TKeyCode ch = gConsole->Getch();
        
        switch(ch)
            {
            case EKeyEnter:
                {
                if(aString.Length() == 0)
                    {
                    break;// At least one character should be read.
                    }
                gConsole->Printf(KLineBreaker);
                return;
                }
            case EKeyBackspace:
                {
                if(0 != aString.Length())
                    {
                    // Back-space only takes the cursor one position back
                    // So to delete a character blank-space is inserted at
                    // that position and later cursor is again adjusted.
                    gConsole->Printf(_L("%c%c%c"), EKeyBackspace, 
                                                   EKeySpace, 
                                                   EKeyBackspace);
                    // Delete the character from the target string also. 
                    aString.Delete(aString.Length() - 1, 1);
                    }
                break;
                }
            default:
                {
                TInt maxBufferLength = aString.MaxLength();
                // IsCharacterKey will return true if ch is a displayable
                // character else it will return false.
                if(IsCharacterKey(ch) && aString.Length() != maxBufferLength)
                    {
                    gConsole->Printf(_L("%c"), ch);
                    aString.Append(ch);
                    }
                }
            }
        }
    }
  
TInt ReadIntL(TInt& aValue)
    {
    TBuf<KIntStringLen> string;
    ReadStringFromConsole(string);
    TLex lexObj(string);
    return lexObj.Val(aValue);
    }

int PrintToScreen(const char* aString)
    {
    TUint length = User::StringLength((TUint8*)aString);
    HBufC* buffer = HBufC::New(length);
    if(NULL == buffer)
        {
        return EOF;
        }
    
    TPtr targetPtr = buffer->Des();

    TPtrC8 stringPtr((TUint8*)aString);

    targetPtr.Copy(stringPtr);
    gConsole->Printf(targetPtr);
    
    delete buffer;
    
    return KErrNone;
    }

void exit ( int status )
    {
    User::Exit(status);
    for(;;){} // So that GCC compiler don't complain about noreturn function returns.
    }

//------------------------------------------  E  O  F -----------------------------------------------------

