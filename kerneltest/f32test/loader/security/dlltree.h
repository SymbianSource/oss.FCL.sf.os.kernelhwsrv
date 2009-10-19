// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// \F32TEST\loader\gen\dlltree.h
// 
//

#ifndef __DLLTREE_H__
#define __DLLTREE_H__
#include <e32std.h>

//#define USESYSLIBS


#ifdef USESYSLIBS
_LIT(KDllfilename,"Z:\\System\\Libs\\DLLTS");
#if defined WIN32
_LIT(KNewDllName, "DLLTS");
#else
_LIT(KNewDllName, "C:\\System\\Libs\\DLLTS");
#endif
_LIT(KDllExt,".DLL");

_LIT(KExefilename,"Z:\\System\\Libs\\EXETS");
#if defined WIN32
_LIT(KNewExeName, "EXETS");
#else
_LIT(KNewExeName, "C:\\System\\Libs\\EXETS");
#endif
_LIT(KExeExt,".EXE");
#if defined WIN32
_LIT(KDll6,"DLLTS6.dll");
_LIT(KDll7,"DLLTS7.dll");
_LIT(KDll11,"DLLTS11.dll");
#else
_LIT(KDll6,"C:\\System\\Libs\\DLLTS6.dll");
_LIT(KDll7,"C:\\System\\Libs\\DLLTS7.dll");
_LIT(KDll11,"C:\\System\\Libs\\DLLTS11.dll");
#endif
_LIT(KSystemLibs,"C:\\System\\Libs\\");
_LIT(KAnyDirDll,"C:\\Anyoldname\\System\\Libs\\DLLTS");
_LIT(KAnyDirExe,"C:\\Anyoldname\\System\\Libs\\EXETS");
_LIT(KAnyDirShort,"C:\\Anyoldname\\");
_LIT(KJDllName,"N:\\System\\Libs\\DLLTS");
_LIT(KJExeName,"N:\\System\\Libs\\EXETS");

#else	//USESYSLIBS 

_LIT(KDllfilename,"Z:\\sys\\bin\\DLLTS");//this will need to change
#if defined WIN32
_LIT(KNewDllName, "DLLTS");
#else
_LIT(KNewDllName, "C:\\sys\\bin\\DLLTS");
#endif
_LIT(KDllExt,".DLL");
_LIT(KExefilename,"Z:\\sys\\bin\\EXETS");//this will need to change
#if defined WIN32
_LIT(KNewExeName, "EXETS");
#else
_LIT(KNewExeName, "C:\\sys\\bin\\EXETS");
#endif
_LIT(KExeExt,".EXE");
#if defined WIN32
_LIT(KDll6,"DLLTS6.dll");
_LIT(KDll7,"DLLTS7.dll");
_LIT(KDll11,"DLLTS11.dll");
#else
_LIT(KDll6,"C:\\sys\\bin\\DLLTS6.dll");
_LIT(KDll7,"C:\\sys\\bin\\DLLTS7.dll");
_LIT(KDll11,"C:\\sys\\bin\\DLLTS11.dll");
#endif
_LIT(KSystemLibs,"C:\\sys\\bin\\");
_LIT(KAnyDirDll,"C:\\Anyoldname\\sys\\bin\\DLLTS");
_LIT(KAnyDirExe,"C:\\Anyoldname\\sys\\bin\\EXETS");
_LIT(KAnyDirShort,"C:\\Anyoldname\\");
_LIT(KJDllName,"N:\\sys\\bin\\DLLTS");
_LIT(KJExeName,"N:\\sys\\bin\\EXETS");
_LIT(KJDllNameOnly,"DLLTS");
_LIT(KJExeNameOnly,"EXETS");
#endif

class MDllList;

#if defined(__DLLNUM0)
#define DLLNUM               0
#define INITFUNC             Init0
#define CHKCFUNC             ChkC0
#define BLKIFUNC             BlkI0
#define RBLKIFUNC            RBlkI0
#define CHKDEPS(r)           (\
	((r)=ChkC1())!=0 ||\
	((r)=ChkC2())!=0 ||\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=Init1(l))!=0 ||\
	((r)=Init2(l))!=0 ||\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	(i)=RBlkI1(i,g);\
	(i)=RBlkI2(i,g);\
	}
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C
extern "C" IMPORT_C TInt Init1(MDllList&);
extern "C" IMPORT_C TInt ChkC1();
extern "C" IMPORT_C TInt RBlkI1(TInt, TInt);
extern "C" IMPORT_C TInt Init2(MDllList&);
extern "C" IMPORT_C TInt ChkC2();
extern "C" IMPORT_C TInt RBlkI2(TInt, TInt);


#elif defined(__DLLNUM1)
#define DLLNUM               1
#define INITFUNC             Init1
#define CHKCFUNC             ChkC1
#define BLKIFUNC             BlkI1
#define RBLKIFUNC            RBlkI1
#define CHKDEPS(r)           (\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	}
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C


#elif defined(__DLLNUM2)
#define DLLNUM               2
#define INITFUNC             Init2
#define CHKCFUNC             ChkC2
#define BLKIFUNC             BlkI2
#define RBLKIFUNC            RBlkI2
#define CHKDEPS(r)           (\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	}
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C


#elif defined(__DLLNUM3)
#define DLLNUM               3
#define INITFUNC             Init3
#define CHKCFUNC             ChkC3
#define BLKIFUNC             BlkI3
#define RBLKIFUNC            RBlkI3
#define CHKDEPS(r)           (\
	((r)=ChkC4())!=0 ||\
	((r)=ChkC5())!=0 ||\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=Init4(l))!=0 ||\
	((r)=Init5(l))!=0 ||\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	(i)=RBlkI4(i,g);\
	(i)=RBlkI5(i,g);\
	}
#define __MODULE_HAS_DATA
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C
extern "C" IMPORT_C TInt Init4(MDllList&);
extern "C" IMPORT_C TInt ChkC4();
extern "C" IMPORT_C TInt RBlkI4(TInt, TInt);
extern "C" IMPORT_C TInt Init5(MDllList&);
extern "C" IMPORT_C TInt ChkC5();
extern "C" IMPORT_C TInt RBlkI5(TInt, TInt);


#elif defined(__DLLNUM4)
#define DLLNUM               4
#define INITFUNC             Init4
#define CHKCFUNC             ChkC4
#define BLKIFUNC             BlkI4
#define RBLKIFUNC            RBlkI4
#define CHKDEPS(r)           (\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	}
#define __MODULE_HAS_DATA
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C


#elif defined(__DLLNUM5)
#define DLLNUM               5
#define INITFUNC             Init5
#define CHKCFUNC             ChkC5
#define BLKIFUNC             BlkI5
#define RBLKIFUNC            RBlkI5
#define CHKDEPS(r)           (\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	}
#define __MODULE_HAS_DATA
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C


#elif defined(__DLLNUM6)
#define DLLNUM               6
#define INITFUNC             Init6
#define CHKCFUNC             ChkC6
#define BLKIFUNC             BlkI6
#define RBLKIFUNC            RBlkI6
#define CHKDEPS(r)           (\
	((r)=ChkC0())!=0 ||\
	((r)=ChkC3())!=0 ||\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=Init0(l))!=0 ||\
	((r)=Init3(l))!=0 ||\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	(i)=RBlkI0(i,g);\
	(i)=RBlkI3(i,g);\
	}
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C
extern "C" IMPORT_C TInt Init0(MDllList&);
extern "C" IMPORT_C TInt ChkC0();
extern "C" IMPORT_C TInt RBlkI0(TInt, TInt);
extern "C" IMPORT_C TInt Init3(MDllList&);
extern "C" IMPORT_C TInt ChkC3();
extern "C" IMPORT_C TInt RBlkI3(TInt, TInt);


#elif defined(__DLLNUM7)
#define DLLNUM               7
#define INITFUNC             Init7
#define CHKCFUNC             ChkC7
#define BLKIFUNC             BlkI7
#define RBLKIFUNC            RBlkI7
#define CHKDEPS(r)           (\
	((r)=ChkC8())!=0 ||\
	((r)=ChkC9())!=0 ||\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=Init8(l))!=0 ||\
	((r)=Init9(l))!=0 ||\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	(i)=RBlkI8(i,g);\
	(i)=RBlkI9(i,g);\
	}
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C
extern "C" IMPORT_C TInt Init8(MDllList&);
extern "C" IMPORT_C TInt ChkC8();
extern "C" IMPORT_C TInt RBlkI8(TInt, TInt);
extern "C" IMPORT_C TInt Init9(MDllList&);
extern "C" IMPORT_C TInt ChkC9();
extern "C" IMPORT_C TInt RBlkI9(TInt, TInt);


#elif defined(__DLLNUM8)
#define DLLNUM               8
#define INITFUNC             Init8
#define CHKCFUNC             ChkC8
#define BLKIFUNC             BlkI8
#define RBLKIFUNC            RBlkI8
#define CHKDEPS(r)           (\
	((r)=ChkC10())!=0 ||\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=Init10(l))!=0 ||\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	(i)=RBlkI10(i,g);\
	}
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C
extern "C" IMPORT_C TInt Init10(MDllList&);
extern "C" IMPORT_C TInt ChkC10();
extern "C" IMPORT_C TInt RBlkI10(TInt, TInt);


#elif defined(__DLLNUM9)
#define DLLNUM               9
#define INITFUNC             Init9
#define CHKCFUNC             ChkC9
#define BLKIFUNC             BlkI9
#define RBLKIFUNC            RBlkI9
#define CHKDEPS(r)           (\
	((r)=ChkC10())!=0 ||\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=Init10(l))!=0 ||\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	(i)=RBlkI10(i,g);\
	}
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C
extern "C" IMPORT_C TInt Init10(MDllList&);
extern "C" IMPORT_C TInt ChkC10();
extern "C" IMPORT_C TInt RBlkI10(TInt, TInt);


#elif defined(__DLLNUM10)
#define DLLNUM               10
#define INITFUNC             Init10
#define CHKCFUNC             ChkC10
#define BLKIFUNC             BlkI10
#define RBLKIFUNC            RBlkI10
#define CHKDEPS(r)           (\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	}
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C




#elif defined(__DLLNUM11)
#define DLLNUM               11
#define INITFUNC             Init11
#define CHKCFUNC             ChkC11
#define BLKIFUNC             BlkI11
#define RBLKIFUNC            RBlkI11
#define CHKDEPS(r)           (\
	((r)=ChkC12())!=0 ||\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=Init12(l))!=0 ||\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	(i)=RBlkI12(i,g);\
	}
#define __DLL_IN_CYCLE
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C
extern "C" IMPORT_C TInt Init12(MDllList&);
extern "C" IMPORT_C TInt ChkC12();
extern "C" IMPORT_C TInt RBlkI12(TInt, TInt);


#elif defined(__DLLNUM12)
#define DLLNUM               12
#define INITFUNC             Init12
#define CHKCFUNC             ChkC12
#define BLKIFUNC             BlkI12
#define RBLKIFUNC            RBlkI12
#define CHKDEPS(r)           (\
	((r)=ChkC13())!=0 ||\
	((r)=ChkC7())!=0 ||\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=Init13(l))!=0 ||\
	((r)=Init7(l))!=0 ||\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	(i)=RBlkI13(i,g);\
	(i)=RBlkI7(i,g);\
	}
#define __DLL_IN_CYCLE
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C
extern "C" IMPORT_C TInt Init13(MDllList&);
extern "C" IMPORT_C TInt ChkC13();
extern "C" IMPORT_C TInt RBlkI13(TInt, TInt);
extern "C" IMPORT_C TInt Init7(MDllList&);
extern "C" IMPORT_C TInt ChkC7();
extern "C" IMPORT_C TInt RBlkI7(TInt, TInt);


#elif defined(__DLLNUM13)
#define DLLNUM               13
#define INITFUNC             Init13
#define CHKCFUNC             ChkC13
#define BLKIFUNC             BlkI13
#define RBLKIFUNC            RBlkI13
#define CHKDEPS(r)           (\
	((r)=ChkC11())!=0 ||\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=Init11(l))!=0 ||\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	(i)=RBlkI11(i,g);\
	}
#define __DLL_IN_CYCLE
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C
extern "C" IMPORT_C TInt Init11(MDllList&);
extern "C" IMPORT_C TInt ChkC11();
extern "C" IMPORT_C TInt RBlkI11(TInt, TInt);


#elif defined(__DLLNUM14)
#define DLLNUM               14
#define EXENUM               14
_LIT(KServerName, "ExeA");
#define INITFUNC             Init14
#define CHKCFUNC             ChkC14
#define BLKIFUNC             BlkI14
#define RBLKIFUNC            RBlkI14			//need to sort out the rest of this dll
#define CHKDEPS(r)           (\
	((r)=ChkC6())!=0 ||\
//	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=Init6(l))!=0 ||\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	(i)=RBlkI6(i,g);\
	}
#define __MODULE_EXPORT		EXPORT_C
#define __MODULE_IMPORT		IMPORT_C
extern "C" IMPORT_C TInt Init6(MDllList&);
extern "C" IMPORT_C TInt ChkC6();
extern "C" IMPORT_C TInt RBlkI6(TInt, TInt);


#elif defined(__DLLNUM15)
#define DLLNUM               15
#define EXENUM               15
_LIT(KServerName, "ExeB");
#define INITFUNC             Init15
#define CHKCFUNC             ChkC15
#define BLKIFUNC             BlkI15
#define RBLKIFUNC            RBlkI15			//need to sort out the rest of this dll
#define CHKDEPS(r)           (\
	((r)=ChkC11())!=0 ||\
	((r)=0)!=0 )
#define INITDEPS(r,l)        (\
	((r)=Init11(l))!=0 ||\
	((r)=0)!=0 )
#define RBLKIFUNC_DEPS(i,g)  {\
	(i)=RBlkI11(i,g);\
	}
#define __MODULE_EXPORT
#define __MODULE_IMPORT
extern "C" IMPORT_C TInt Init11(MDllList&);
extern "C" IMPORT_C TInt ChkC11();
extern "C" IMPORT_C TInt RBlkI11(TInt, TInt);
#endif

const TInt KNumModules=16;

//sorted up to here



static const TText* const ModuleName[KNumModules] =
	{
	(const TText*)L"STree0",					/*0*/
	(const TText*)L"STree1",					/*1*/
	(const TText*)L"STree2",					/*2*/
	(const TText*)L"SDTree3",					/*3*/
	(const TText*)L"SDTree4",					/*4*/
	(const TText*)L"SDTree5",					/*5*/
	(const TText*)L"SPDTree6",					/*6*/
	(const TText*)L"SLat7",						/*7*/
	(const TText*)L"SLat8",						/*8*/
	(const TText*)L"SLat9",						/*9*/
	(const TText*)L"SLat10",					/*10*/
	(const TText*)L"SCycS11",					/*11*/
	(const TText*)L"SCycS12",					/*12*/
	(const TText*)L"SCycS13",					/*13*/
	(const TText*)L"SExe14",					/*14*/
	(const TText*)L"SExe15"						/*15*/
	};

#define MODULE_NAME(n)	TPtrC(ModuleName[n])

static const TText* const ModuleFileName[KNumModules] =
	{
	(const TText*)L"DLLTS0.DLL",
	(const TText*)L"DLLTS1.DLL",
	(const TText*)L"DLLTS2.DLL",
	(const TText*)L"DLLTS3.DLL",
	(const TText*)L"DLLTS4.DLL",
	(const TText*)L"DLLTS5.DLL",
	(const TText*)L"DLLTS6.DLL",
	(const TText*)L"DLLTS7.DLL",
	(const TText*)L"DLLTS8.DLL",
	(const TText*)L"DLLTS9.DLL",
	(const TText*)L"DLLTS10.DLL",
	(const TText*)L"DLLTS11.DLL",
	(const TText*)L"DLLTS12.DLL",
	(const TText*)L"DLLTS13.DLL",
	(const TText*)L"EXETS14.EXE",
	(const TText*)L"EXETS15.EXE"
	};

#define MODULE_FILENAME(n)	TPtrC(ModuleFileName[n])

static const TInt Module0Deps[] =
	{2,1,2};
static const TInt Module1Deps[] =
	{0};
static const TInt Module2Deps[] =
	{0};
static const TInt Module3Deps[] =
	{2,4,5};
static const TInt Module4Deps[] =
	{0};
static const TInt Module5Deps[] =
	{0};
static const TInt Module6Deps[] =
	{6,1,2,0,4,5,3};
static const TInt Module7Deps[] =
	{3,10,8,9};
static const TInt Module8Deps[] =
	{1,10};
static const TInt Module9Deps[] =
	{1,10};
static const TInt Module10Deps[] =
	{0};
static const TInt Module11Deps[] =
	{7,11,13,10,8,9,7,12};

static const TInt Module12Deps[] =
	{7,10,8,9,7,12,11,13};

static const TInt Module13Deps[] =
	{7,13,10,8,9,7,12,11};

static const TInt Module14Deps[] =
	{7,1,2,0,4,5,3,6};

static const TInt Module15Deps[] =
	{8,11,13,10,8,9,7,12,11};

static const TInt* const ModuleDependencies[KNumModules] =
	{
	Module0Deps,
	Module1Deps,
	Module2Deps,
	Module3Deps,
	Module4Deps,
	Module5Deps,
	Module6Deps,
	Module7Deps,
	Module8Deps,
	Module9Deps,
	Module10Deps,
	Module11Deps,
	Module12Deps,
	Module13Deps,
	Module14Deps,
	Module15Deps
	};

const TInt KModuleFlagExe=0x01;
const TInt KModuleFlagFixed=0x02;
const TInt KModuleFlagData=0x04;
const TInt KModuleFlagXIP=0x08;
const TInt KModuleFlagDllInCycle=0x10;
const TInt KModuleFlagDataInTree=0x20;
const TInt KModuleFlagXIPDataInTree=0x40;
const TInt KModuleFlagExports=0x80;

static const TInt ModuleFlags[KNumModules] =
	{
	0x80,		/*0*/
	0x80,		/*1*/
	0x80,		/*2*/
	0xa4,		/*3*/
	0xa4,		/*4*/
	0xa4,		/*5*/
	0xa0,		/*6*/
	0x80,		/*7*/
	0x80,		/*8*/
	0x80,		/*9*/
	0x80,		/*10*/
	0x90,		/*11*/
	0x90,		/*12*/
	0x90,		/*13*/
	0xeb,		/*14*/
	0x61		/*15*/
	};

static const TInt Module0RBlkIParams[2] = { 0, 0 };
static const TInt Module1RBlkIParams[2] = { 0, 0 };
static const TInt Module2RBlkIParams[2] = { 0, 0 };
static const TInt Module3RBlkIParams[2] = { 3, 34 };
static const TInt Module4RBlkIParams[2] = { 1, 12 };
static const TInt Module5RBlkIParams[2] = { 1, 13 };
static const TInt Module6RBlkIParams[2] = { 0, 0 };
static const TInt Module7RBlkIParams[2] = { 0, 0 };
static const TInt Module8RBlkIParams[2] = { 0, 0 };
static const TInt Module9RBlkIParams[2] = { 0, 0 };
static const TInt Module10RBlkIParams[2] = { 0, 0 };
static const TInt Module11RBlkIParams[2] = { 0, 0 };
static const TInt Module12RBlkIParams[2] = { 0, 0 };
static const TInt Module13RBlkIParams[2] = { 0, 0 };
static const TInt Module14RBlkIParams[2] = { 0, 0 };
static const TInt Module15RBlkIParams[2] = { 0, 0 };

static const TInt* const ModuleRBlkIParams[KNumModules] =
	{
	Module0RBlkIParams,
	Module1RBlkIParams,
	Module2RBlkIParams,
	Module3RBlkIParams,
	Module4RBlkIParams,
	Module5RBlkIParams,
	Module6RBlkIParams,
	Module7RBlkIParams,
	Module8RBlkIParams,
	Module9RBlkIParams,
	Module10RBlkIParams,
	Module11RBlkIParams,
	Module12RBlkIParams,
	Module13RBlkIParams,
	Module14RBlkIParams,
	Module15RBlkIParams
	};

static const TInt Module0ExeInfo[2] = { -1, -1 };
static const TInt Module1ExeInfo[2] = { -1, -1 };
static const TInt Module2ExeInfo[2] = { -1, -1 };
static const TInt Module3ExeInfo[2] = { -1, -1 };
static const TInt Module4ExeInfo[2] = { -1, -1 };
static const TInt Module5ExeInfo[2] = { -1, -1 };
static const TInt Module6ExeInfo[2] = { -1, -1 };
static const TInt Module7ExeInfo[2] = { -1, -1 };
static const TInt Module8ExeInfo[2] = { -1, -1 };
static const TInt Module9ExeInfo[2] = { -1, -1 };
static const TInt Module10ExeInfo[2] = { -1, -1 };
static const TInt Module11ExeInfo[2] = { -1, -1 };
static const TInt Module12ExeInfo[2] = { -1, -1 };
static const TInt Module13ExeInfo[2] = { -1, -1 };
static const TInt Module14ExeInfo[2] = { 14, 14 };
static const TInt Module15ExeInfo[2] = { 15, 15 };


static const TInt* const ModuleExeInfo[KNumModules] =
	{
	Module0ExeInfo,
	Module1ExeInfo,
	Module2ExeInfo,
	Module3ExeInfo,
	Module4ExeInfo,
	Module5ExeInfo,
	Module6ExeInfo,
	Module7ExeInfo,
	Module8ExeInfo,
	Module9ExeInfo,
	Module10ExeInfo,
	Module11ExeInfo,
	Module12ExeInfo,
	Module13ExeInfo,
	Module14ExeInfo,
	Module15ExeInfo
	};


const TInt	KTestCases = 10;

//numbers 0 to 6 are for 14 and 7 to 13 are for 15

//													0	 1	  2	   3	 4 	  5     6    7     8    9
static const TUint32 Module0Caps[KTestCases] =	{0x0, 0xF, 0x7, 0x6,  0x7, 0x7,  0x7, 0x7,  0x7, 0x7};
static const TUint32 Module1Caps[KTestCases] =	{0x0, 0xF, 0xF, 0xF,  0xE, 0xF,  0xF, 0xF,  0xF, 0xF};

static const TUint32 Module2Caps[KTestCases] =	{0x0, 0xF, 0xF, 0xF,  0xF, 0xE,  0xF, 0xF,  0xF, 0xF};
static const TUint32 Module3Caps[KTestCases] =	{0x0, 0xF, 0x7, 0x7,  0x7, 0x7,  0x6, 0x7,  0x7, 0x7};

static const TUint32 Module4Caps[KTestCases] =	{0x0, 0xF, 0xF, 0xF,  0xF, 0xF,  0xF, 0xE,  0xF, 0xF};
static const TUint32 Module5Caps[KTestCases] =	{0x0, 0xF, 0xF, 0xF,  0xF, 0xF,  0xF, 0xF,  0xE, 0xF};

static const TUint32 Module6Caps[KTestCases] =	{0x0, 0xF, 0x3, 0x3,  0x3, 0x3,  0x3, 0x3,  0x3, 0x2};
static const TUint32 Module7Caps[KTestCases] =	{0x0, 0xF, 0x7, 0x6, 0x7,  0x7, 0x7,  0x7, 0x7,  0x7};

static const TUint32 Module8Caps[KTestCases] =	{0x0, 0xF, 0xF, 0xF, 0xE,  0xF, 0xF,  0xF, 0xF,  0xF};
static const TUint32 Module9Caps[KTestCases] =	{0x0, 0xF, 0xF, 0xF, 0xF,  0xE, 0xF,  0xF, 0xF,  0xF};

static const TUint32 Module10Caps[KTestCases] =	{0x0, 0xF, 0x1F,0x1F,0x1F, 0x1F,0x1E, 0x1F,0x1F, 0x1F};
static const TUint32 Module11Caps[KTestCases] =	{0x0, 0xF, 0x3, 0x3, 0x3,  0x3, 0x3,  0x2, 0x3,  0x3};

static const TUint32 Module12Caps[KTestCases] =	{0x0, 0xF, 0x3, 0x3, 0x3,  0x3, 0x3,  0x3, 0x2,  0x3};
static const TUint32 Module13Caps[KTestCases] =	{0x0, 0xF, 0x3, 0x3, 0x3,  0x3, 0x3,  0x3, 0x3,  0x2};

static const TUint32 Module14Caps[KTestCases] =	{0x0, 0xF, 0x1, 0x1, 0x1,  0x1, 0x1,  0x1, 0x1,  0x1};	//exe A
static const TUint32 Module15Caps[KTestCases] =	{0x0, 0xF, 0x1, 0x1, 0x1,  0x1, 0x1,  0x1, 0x1,  0x1};	//exe B

/* These go back in when the capability check become mandatory
static const TInt ModuleResultsA[KTestCases] = 
	{
	KErrNone,				//all equal		0
	KErrNone,				//all equal		1
	KErrNone,				//asscending	2
	KErrPermissionDenied,	//asscending	3
	KErrPermissionDenied,	//asscending	4
	KErrPermissionDenied,	//asscending	5
	KErrPermissionDenied,	//asscending	6
	KErrPermissionDenied,	//asscending	7
	KErrPermissionDenied,	//asscending	8
	KErrPermissionDenied	//asscending	9
	};

static const TInt ModuleResultsB[KTestCases] = 
	{
	KErrPermissionDenied,	//all equal		0
	KErrNone,				//all equal		1
	KErrNone,				//asscending	2
	KErrPermissionDenied,	//asscending	3
	KErrPermissionDenied,	//asscending	4
	KErrPermissionDenied,	//asscending	5
	KErrPermissionDenied,	//asscending	6
	KErrPermissionDenied,	//asscending	7
	KErrPermissionDenied,	//asscending	8
	KErrPermissionDenied	//asscending	9
	};

static const TInt ModuleResultsC[KTestCases] = 
	{
	KErrPermissionDenied,	//all equal		0
	KErrNone,				//all equal		1
	KErrNone,				//asscending	2
	KErrPermissionDenied,	//asscending	3
	KErrPermissionDenied,	//asscending	4
	KErrPermissionDenied,	//asscending	5
	KErrPermissionDenied,	//asscending	6
	KErrNone,				//asscending	7
	KErrNone,				//asscending	8
	KErrNone				//asscending	9
	};
*/


static const TInt ModuleResultsA[KTestCases] = 
	{
	KErrNone,				//all equal		0
	KErrNone,				//all equal		1
	KErrNone,				//asscending	2
	KErrNone,	//asscending	3
	KErrNone,	//asscending	4
	KErrNone,	//asscending	5
	KErrNone,	//asscending	6
	KErrNone,	//asscending	7
	KErrNone,	//asscending	8
	KErrNone	//asscending	9
	};

static const TInt ModuleResultsB[KTestCases] = 
	{
	KErrNone,	//all equal		0
	KErrNone,	//all equal		1
	KErrNone,	//asscending	2
	KErrNone,	//asscending	3
	KErrNone,	//asscending	4
	KErrNone,	//asscending	5
	KErrNone,	//asscending	6
	KErrNone,	//asscending	7
	KErrNone,	//asscending	8
	KErrNone	//asscending	9
	};

static const TInt ModuleResultsC[KTestCases] = 
	{
	KErrNone,	//all equal		0
	KErrNone,	//all equal		1
	KErrNone,	//asscending	2
	KErrNone,	//asscending	3
	KErrNone,	//asscending	4
	KErrNone,	//asscending	5
	KErrNone,	//asscending	6
	KErrNone,	//asscending	7
	KErrNone,	//asscending	8
	KErrNone	//asscending	9
	};

static const TUint32* const ModuleCaps[KNumModules] =
	{
	Module0Caps,
	Module1Caps,
	Module2Caps,
	Module3Caps,
	Module4Caps,
	Module5Caps,
	Module6Caps,
	Module7Caps,
	Module8Caps,
	Module9Caps,
	Module10Caps,
	Module11Caps,
	Module12Caps,
	Module13Caps,
	Module14Caps,
	Module15Caps
	};

#endif
