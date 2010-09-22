// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/*
    @file
    @internalTechnology

    "fs_auto_mounter" filesystem classes definition
*/


#ifndef AUTOMOUNTER_FILESYSTEM_H
#define AUTOMOUNTER_FILESYSTEM_H

#include "filesystem_utils.h"
#include <f32fsys.h>
#include <f32dbg.h>


IMPORT_C TUint32 DebugRegister();

//-- define this for having logs disregarding DebugRegister() settings
//#define FORCE_LOGS


#if defined(_DEBUG) || defined(_DEBUG_RELEASE)
//----------------- DEBUG mode -----------------

#ifndef FORCE_LOGS 
    #define __PRINT(t)          {if (DebugRegister()&KFSYS) RDebug::Print(t);}
    #define __PRINT1(t,a)       {if (DebugRegister()&KFSYS) RDebug::Print(t,a);}
    #define __PRINT2(t,a,b)     {if (DebugRegister()&KFSYS) RDebug::Print(t,a,b);}
    #define __PRINT3(t,a,b,c)   {if (DebugRegister()&KFSYS) RDebug::Print(t,a,b,c);}
    #define __PRINT4(t,a,b,c,d) {if (DebugRegister()&KFSYS) RDebug::Print(t,a,b,c,d);}
#else
    #define __PRINT(t)          {RDebug::Print(t);}
    #define __PRINT1(t,a)       {RDebug::Print(t,a);}
    #define __PRINT2(t,a,b)     {RDebug::Print(t,a,b);}
    #define __PRINT3(t,a,b,c)   {RDebug::Print(t,a,b,c);}
    #define __PRINT4(t,a,b,c,d) {RDebug::Print(t,a,b,c,d);}
#endif

#define DBG_STATEMENT(text) text

#else
//----------------- RELEASE mode -----------------
#define __PRINT(t)
#define __PRINT1(t,a)
#define __PRINT2(t,a,b)
#define __PRINT3(t,a,b,c)
#define __PRINT4(t,a,b,c,d)
#define __PRINT8BIT1(t,a)
#define __PRINT1TEMP(t,a)

#define DBG_STATEMENT(text)

#endif //#if defined(_DEBUG) || defined(_DEBUG_RELEASE)


//#######################################################################################################################################
//#     constants definitions here
//#######################################################################################################################################



//-----------------------------------------------------------------------------

/**
Internal fault codes for Automounter.fsy
*/
enum TFault
    {
    ENotImplemented,
    EWrongDriveAttributes,
    EPluginInitialise,
    EFileSysNotAdded,
    EWrongConfiguration,
    EIncompatibleFileSystems,

    EMustNotBeCalled

    };

//-----------------------------------------------------------------------------

void Fault(TFault aFault);


//-----------------------------------------------------------------------------
/**
    This class is a container for child file system names that are supported by automounter.
    Child FS names are stored in Upper Case to enable simple FSName hash calculations.
    The names must be unique.
*/
class XStringArray
    {
 public:
    XStringArray();
    ~XStringArray();

    void Reset();
    const TDesC& operator[](TUint aIndex) const;
    TInt Append(const TDesC& aString);
    TUint Count() const {return iStrings.Count();}
    TUint32 GetStringHash(TUint aIndex) const;


 private:
    XStringArray(const XStringArray&);
    XStringArray& operator=(const XStringArray&);

    /** panic codes */
    enum TPanicCode
        {
        EIndexOutOfRange, ///< index out of range
        ENotImplemented,  ///< functionality isn't implemented
        };

    void Panic(TPanicCode aPanicCode) const;

 private:
    RPointerArray<HBufC> iStrings;
    
    };


//-----------------------------------------------------------------------------


/**
    A min. number of the child file systems supported by the automounter. 
    using less than 2 child fs doesn't make much sense, though can be useful in some circumstances.
    The shild FS names must be set in config.
*/
const TUint KMinChildFsNum = 1;


/**
    File system class definition
*/
class CAutoMounterFileSystem : public CFileSystem, CFileSystem::MFileSystemExtInterface
    {
public:
    static CAutoMounterFileSystem* New();
    ~CAutoMounterFileSystem();
public:
    
    //-- pure virtual interface implementation, overrides from CFileSystem
    TInt Install();
    CMountCB* NewMountL() const;
    CFileCB* NewFileL() const;
    CDirCB* NewDirL() const;
    CFormatCB* NewFormatL() const;
    
    //-- non-pure virtual interface, overrides from CFileSystem
#ifdef _DEBUG
    TInt Remove();
    TBool QueryVersionSupported(const TVersion& aVer) const;
#endif
    
    TBool IsExtensionSupported() const;
    TInt DefaultPath(TDes& aPath) const;
    TInt GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput);
    
    
    
    
protected:
    CAutoMounterFileSystem();

    TInt TryMountFilesystem(TDrive* apDrive, CMountCB** apMount, CFileSystem** apFS);


    //-------------------------------
    virtual CMountCB* NewMountExL(TDrive* apDrive, CFileSystem** apFileSystem, TBool aForceMount, TUint32 aFsNameHash);
    virtual TInt GetSupportedFileSystemName(TInt aFsNumber, TDes& aFsName) const;   


private:
    
    /** possible states of this object */
    enum TState
        {
        EInvalid = 0,       ///< initial, invalid
        ENotInitialised,
        EInitialising,
        EInitialised, 
        };

    inline TState State() const {return iState;}
    inline void SetState(TState aState) {iState = aState;}

    
    /** "default child" file system name index in the child names container. "default child" is used in some weird cases, when
        it doesn't matter which particular child FS to use, e.g. getting access to the media driver for media unlocking. */
    enum {KDefaultFSNo = 0}; 


    CFileSystem* GetChildFileSystem(TUint aIndex) const;
    CFileSystem* GetChildFileSysteByNameHash(TUint32 aFsNameHash) const;

    TUint ChildFsNum() const {ASSERT(State() == EInitialised); return iFSNames.Count();} ///< @return Number of supported Child file systems

    void InitialiseFileSystem();
    TInt DoProcessProxyDriveSupport();
    void ParseConfig();
    void DoParseChildNames(const TDesC8& aList);

private:
    
    TState       iState;    ///< this object current state
    XStringArray iFSNames;  ///< child file system names container.
    TInt            iChildFsForDefFmt;  ///< child FS number for formatting "unrecognised" media by default. value <0, means "not specified" 

    };


//-----------------------------------------------------------------------------

#endif //AUTOMOUNTER_FILESYSTEM_H






