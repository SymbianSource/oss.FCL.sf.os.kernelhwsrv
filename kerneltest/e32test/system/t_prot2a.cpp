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
// e32test\system\t_prot2a.cpp
// 
//

#include <e32test.h>

LOCAL_D TInt TestData1=0xb504f334;
LOCAL_D TInt TestArray[128];

// put in loads of relocations to give loader something to do
LOCAL_C void DummyFunc0() {}
LOCAL_C void DummyFunc1() {}
LOCAL_C void DummyFunc2() {}
LOCAL_C void DummyFunc3() {}
LOCAL_C void DummyFunc4() {}
LOCAL_C void DummyFunc5() {}
LOCAL_C void DummyFunc6() {}
LOCAL_C void DummyFunc7() {}
LOCAL_C void DummyFunc8() {}
LOCAL_C void DummyFunc9() {}
LOCAL_C void DummyFunc10() {}
LOCAL_C void DummyFunc11() {}
LOCAL_C void DummyFunc12() {}
LOCAL_C void DummyFunc13() {}
LOCAL_C void DummyFunc14() {}
LOCAL_C void DummyFunc15() {}
LOCAL_C void DummyFunc16() {}
LOCAL_C void DummyFunc17() {}
LOCAL_C void DummyFunc18() {}
LOCAL_C void DummyFunc19() {}
LOCAL_C void DummyFunc20() {}
LOCAL_C void DummyFunc21() {}
LOCAL_C void DummyFunc22() {}
LOCAL_C void DummyFunc23() {}
LOCAL_C void DummyFunc24() {}
LOCAL_C void DummyFunc25() {}
LOCAL_C void DummyFunc26() {}
LOCAL_C void DummyFunc27() {}
LOCAL_C void DummyFunc28() {}
LOCAL_C void DummyFunc29() {}
LOCAL_C void DummyFunc30() {}
LOCAL_C void DummyFunc31() {}
LOCAL_C void DummyFunc32() {}
LOCAL_C void DummyFunc33() {}
LOCAL_C void DummyFunc34() {}
LOCAL_C void DummyFunc35() {}
LOCAL_C void DummyFunc36() {}
LOCAL_C void DummyFunc37() {}
LOCAL_C void DummyFunc38() {}
LOCAL_C void DummyFunc39() {}
LOCAL_C void DummyFunc40() {}
LOCAL_C void DummyFunc41() {}
LOCAL_C void DummyFunc42() {}
LOCAL_C void DummyFunc43() {}
LOCAL_C void DummyFunc44() {}
LOCAL_C void DummyFunc45() {}
LOCAL_C void DummyFunc46() {}
LOCAL_C void DummyFunc47() {}
LOCAL_C void DummyFunc48() {}
LOCAL_C void DummyFunc49() {}
LOCAL_C void DummyFunc50() {}
LOCAL_C void DummyFunc51() {}
LOCAL_C void DummyFunc52() {}
LOCAL_C void DummyFunc53() {}
LOCAL_C void DummyFunc54() {}
LOCAL_C void DummyFunc55() {}
LOCAL_C void DummyFunc56() {}
LOCAL_C void DummyFunc57() {}
LOCAL_C void DummyFunc58() {}
LOCAL_C void DummyFunc59() {}
LOCAL_C void DummyFunc60() {}
LOCAL_C void DummyFunc61() {}
LOCAL_C void DummyFunc62() {}
LOCAL_C void DummyFunc63() {}
LOCAL_C void DummyFunc64() {}
LOCAL_C void DummyFunc65() {}
LOCAL_C void DummyFunc66() {}
LOCAL_C void DummyFunc67() {}
LOCAL_C void DummyFunc68() {}
LOCAL_C void DummyFunc69() {}
LOCAL_C void DummyFunc70() {}
LOCAL_C void DummyFunc71() {}
LOCAL_C void DummyFunc72() {}
LOCAL_C void DummyFunc73() {}
LOCAL_C void DummyFunc74() {}
LOCAL_C void DummyFunc75() {}
LOCAL_C void DummyFunc76() {}
LOCAL_C void DummyFunc77() {}
LOCAL_C void DummyFunc78() {}
LOCAL_C void DummyFunc79() {}
LOCAL_C void DummyFunc80() {}
LOCAL_C void DummyFunc81() {}
LOCAL_C void DummyFunc82() {}
LOCAL_C void DummyFunc83() {}
LOCAL_C void DummyFunc84() {}
LOCAL_C void DummyFunc85() {}
LOCAL_C void DummyFunc86() {}
LOCAL_C void DummyFunc87() {}
LOCAL_C void DummyFunc88() {}
LOCAL_C void DummyFunc89() {}
LOCAL_C void DummyFunc90() {}
LOCAL_C void DummyFunc91() {}
LOCAL_C void DummyFunc92() {}
LOCAL_C void DummyFunc93() {}
LOCAL_C void DummyFunc94() {}
LOCAL_C void DummyFunc95() {}
LOCAL_C void DummyFunc96() {}
LOCAL_C void DummyFunc97() {}
LOCAL_C void DummyFunc98() {}
LOCAL_C void DummyFunc99() {}
LOCAL_C void DummyFunc100() {}
LOCAL_C void DummyFunc101() {}
LOCAL_C void DummyFunc102() {}
LOCAL_C void DummyFunc103() {}
LOCAL_C void DummyFunc104() {}
LOCAL_C void DummyFunc105() {}
LOCAL_C void DummyFunc106() {}
LOCAL_C void DummyFunc107() {}
LOCAL_C void DummyFunc108() {}
LOCAL_C void DummyFunc109() {}
LOCAL_C void DummyFunc110() {}
LOCAL_C void DummyFunc111() {}
LOCAL_C void DummyFunc112() {}
LOCAL_C void DummyFunc113() {}
LOCAL_C void DummyFunc114() {}
LOCAL_C void DummyFunc115() {}
LOCAL_C void DummyFunc116() {}
LOCAL_C void DummyFunc117() {}
LOCAL_C void DummyFunc118() {}
LOCAL_C void DummyFunc119() {}
LOCAL_C void DummyFunc120() {}
LOCAL_C void DummyFunc121() {}
LOCAL_C void DummyFunc122() {}
LOCAL_C void DummyFunc123() {}
LOCAL_C void DummyFunc124() {}
LOCAL_C void DummyFunc125() {}
LOCAL_C void DummyFunc126() {}
LOCAL_C void DummyFunc127() {}
LOCAL_C void DummyFunc128() {}
LOCAL_C void DummyFunc129() {}
LOCAL_C void DummyFunc130() {}
LOCAL_C void DummyFunc131() {}
LOCAL_C void DummyFunc132() {}
LOCAL_C void DummyFunc133() {}
LOCAL_C void DummyFunc134() {}
LOCAL_C void DummyFunc135() {}
LOCAL_C void DummyFunc136() {}
LOCAL_C void DummyFunc137() {}
LOCAL_C void DummyFunc138() {}
LOCAL_C void DummyFunc139() {}
LOCAL_C void DummyFunc140() {}
LOCAL_C void DummyFunc141() {}
LOCAL_C void DummyFunc142() {}
LOCAL_C void DummyFunc143() {}
LOCAL_C void DummyFunc144() {}
LOCAL_C void DummyFunc145() {}
LOCAL_C void DummyFunc146() {}
LOCAL_C void DummyFunc147() {}
LOCAL_C void DummyFunc148() {}
LOCAL_C void DummyFunc149() {}
LOCAL_C void DummyFunc150() {}
LOCAL_C void DummyFunc151() {}
LOCAL_C void DummyFunc152() {}
LOCAL_C void DummyFunc153() {}
LOCAL_C void DummyFunc154() {}
LOCAL_C void DummyFunc155() {}
LOCAL_C void DummyFunc156() {}
LOCAL_C void DummyFunc157() {}
LOCAL_C void DummyFunc158() {}
LOCAL_C void DummyFunc159() {}
LOCAL_C void DummyFunc160() {}
LOCAL_C void DummyFunc161() {}
LOCAL_C void DummyFunc162() {}
LOCAL_C void DummyFunc163() {}
LOCAL_C void DummyFunc164() {}
LOCAL_C void DummyFunc165() {}
LOCAL_C void DummyFunc166() {}
LOCAL_C void DummyFunc167() {}
LOCAL_C void DummyFunc168() {}
LOCAL_C void DummyFunc169() {}
LOCAL_C void DummyFunc170() {}
LOCAL_C void DummyFunc171() {}
LOCAL_C void DummyFunc172() {}
LOCAL_C void DummyFunc173() {}
LOCAL_C void DummyFunc174() {}
LOCAL_C void DummyFunc175() {}
LOCAL_C void DummyFunc176() {}
LOCAL_C void DummyFunc177() {}
LOCAL_C void DummyFunc178() {}
LOCAL_C void DummyFunc179() {}
LOCAL_C void DummyFunc180() {}
LOCAL_C void DummyFunc181() {}
LOCAL_C void DummyFunc182() {}
LOCAL_C void DummyFunc183() {}
LOCAL_C void DummyFunc184() {}
LOCAL_C void DummyFunc185() {}
LOCAL_C void DummyFunc186() {}
LOCAL_C void DummyFunc187() {}
LOCAL_C void DummyFunc188() {}
LOCAL_C void DummyFunc189() {}
LOCAL_C void DummyFunc190() {}
LOCAL_C void DummyFunc191() {}
LOCAL_C void DummyFunc192() {}
LOCAL_C void DummyFunc193() {}
LOCAL_C void DummyFunc194() {}
LOCAL_C void DummyFunc195() {}
LOCAL_C void DummyFunc196() {}
LOCAL_C void DummyFunc197() {}
LOCAL_C void DummyFunc198() {}
LOCAL_C void DummyFunc199() {}
LOCAL_C void DummyFunc200() {}
LOCAL_C void DummyFunc201() {}
LOCAL_C void DummyFunc202() {}
LOCAL_C void DummyFunc203() {}
LOCAL_C void DummyFunc204() {}
LOCAL_C void DummyFunc205() {}
LOCAL_C void DummyFunc206() {}
LOCAL_C void DummyFunc207() {}
LOCAL_C void DummyFunc208() {}
LOCAL_C void DummyFunc209() {}
LOCAL_C void DummyFunc210() {}
LOCAL_C void DummyFunc211() {}
LOCAL_C void DummyFunc212() {}
LOCAL_C void DummyFunc213() {}
LOCAL_C void DummyFunc214() {}
LOCAL_C void DummyFunc215() {}
LOCAL_C void DummyFunc216() {}
LOCAL_C void DummyFunc217() {}
LOCAL_C void DummyFunc218() {}
LOCAL_C void DummyFunc219() {}
LOCAL_C void DummyFunc220() {}
LOCAL_C void DummyFunc221() {}
LOCAL_C void DummyFunc222() {}
LOCAL_C void DummyFunc223() {}
LOCAL_C void DummyFunc224() {}
LOCAL_C void DummyFunc225() {}
LOCAL_C void DummyFunc226() {}
LOCAL_C void DummyFunc227() {}
LOCAL_C void DummyFunc228() {}
LOCAL_C void DummyFunc229() {}
LOCAL_C void DummyFunc230() {}
LOCAL_C void DummyFunc231() {}
LOCAL_C void DummyFunc232() {}
LOCAL_C void DummyFunc233() {}
LOCAL_C void DummyFunc234() {}
LOCAL_C void DummyFunc235() {}
LOCAL_C void DummyFunc236() {}
LOCAL_C void DummyFunc237() {}
LOCAL_C void DummyFunc238() {}
LOCAL_C void DummyFunc239() {}
LOCAL_C void DummyFunc240() {}
LOCAL_C void DummyFunc241() {}
LOCAL_C void DummyFunc242() {}
LOCAL_C void DummyFunc243() {}
LOCAL_C void DummyFunc244() {}
LOCAL_C void DummyFunc245() {}
LOCAL_C void DummyFunc246() {}
LOCAL_C void DummyFunc247() {}
LOCAL_C void DummyFunc248() {}
LOCAL_C void DummyFunc249() {}
LOCAL_C void DummyFunc250() {}
LOCAL_C void DummyFunc251() {}
LOCAL_C void DummyFunc252() {}
LOCAL_C void DummyFunc253() {}
LOCAL_C void DummyFunc254() {}
LOCAL_C void DummyFunc255() {}
LOCAL_C void DummyFunc256() {}
LOCAL_C void DummyFunc257() {}
LOCAL_C void DummyFunc258() {}
LOCAL_C void DummyFunc259() {}
LOCAL_C void DummyFunc260() {}
LOCAL_C void DummyFunc261() {}
LOCAL_C void DummyFunc262() {}
LOCAL_C void DummyFunc263() {}
LOCAL_C void DummyFunc264() {}
LOCAL_C void DummyFunc265() {}
LOCAL_C void DummyFunc266() {}
LOCAL_C void DummyFunc267() {}
LOCAL_C void DummyFunc268() {}
LOCAL_C void DummyFunc269() {}
LOCAL_C void DummyFunc270() {}
LOCAL_C void DummyFunc271() {}
LOCAL_C void DummyFunc272() {}
LOCAL_C void DummyFunc273() {}
LOCAL_C void DummyFunc274() {}
LOCAL_C void DummyFunc275() {}
LOCAL_C void DummyFunc276() {}
LOCAL_C void DummyFunc277() {}
LOCAL_C void DummyFunc278() {}
LOCAL_C void DummyFunc279() {}
LOCAL_C void DummyFunc280() {}
LOCAL_C void DummyFunc281() {}
LOCAL_C void DummyFunc282() {}
LOCAL_C void DummyFunc283() {}
LOCAL_C void DummyFunc284() {}
LOCAL_C void DummyFunc285() {}
LOCAL_C void DummyFunc286() {}
LOCAL_C void DummyFunc287() {}
LOCAL_C void DummyFunc288() {}
LOCAL_C void DummyFunc289() {}
LOCAL_C void DummyFunc290() {}
LOCAL_C void DummyFunc291() {}
LOCAL_C void DummyFunc292() {}
LOCAL_C void DummyFunc293() {}
LOCAL_C void DummyFunc294() {}
LOCAL_C void DummyFunc295() {}
LOCAL_C void DummyFunc296() {}
LOCAL_C void DummyFunc297() {}
LOCAL_C void DummyFunc298() {}
LOCAL_C void DummyFunc299() {}

typedef void (*PFV)(void);

LOCAL_D PFV Relocs[300]=
	{
	DummyFunc0, DummyFunc1, DummyFunc2, DummyFunc3, DummyFunc4, DummyFunc5, DummyFunc6, DummyFunc7, DummyFunc8, DummyFunc9,
	DummyFunc10, DummyFunc11, DummyFunc12, DummyFunc13, DummyFunc14, DummyFunc15, DummyFunc16, DummyFunc17, DummyFunc18, DummyFunc19,
	DummyFunc20, DummyFunc21, DummyFunc22, DummyFunc23, DummyFunc24, DummyFunc25, DummyFunc26, DummyFunc27, DummyFunc28, DummyFunc29,
	DummyFunc30, DummyFunc31, DummyFunc32, DummyFunc33, DummyFunc34, DummyFunc35, DummyFunc36, DummyFunc37, DummyFunc38, DummyFunc39,
	DummyFunc40, DummyFunc41, DummyFunc42, DummyFunc43, DummyFunc44, DummyFunc45, DummyFunc46, DummyFunc47, DummyFunc48, DummyFunc49,
	DummyFunc50, DummyFunc51, DummyFunc52, DummyFunc53, DummyFunc54, DummyFunc55, DummyFunc56, DummyFunc57, DummyFunc58, DummyFunc59,
	DummyFunc60, DummyFunc61, DummyFunc62, DummyFunc63, DummyFunc64, DummyFunc65, DummyFunc66, DummyFunc67, DummyFunc68, DummyFunc69,
	DummyFunc70, DummyFunc71, DummyFunc72, DummyFunc73, DummyFunc74, DummyFunc75, DummyFunc76, DummyFunc77, DummyFunc78, DummyFunc79,
	DummyFunc80, DummyFunc81, DummyFunc82, DummyFunc83, DummyFunc84, DummyFunc85, DummyFunc86, DummyFunc87, DummyFunc88, DummyFunc89,
	DummyFunc90, DummyFunc91, DummyFunc92, DummyFunc93, DummyFunc94, DummyFunc95, DummyFunc96, DummyFunc97, DummyFunc98, DummyFunc99,
	DummyFunc100, DummyFunc101, DummyFunc102, DummyFunc103, DummyFunc104, DummyFunc105, DummyFunc106, DummyFunc107, DummyFunc108, DummyFunc109,
	DummyFunc110, DummyFunc111, DummyFunc112, DummyFunc113, DummyFunc114, DummyFunc115, DummyFunc116, DummyFunc117, DummyFunc118, DummyFunc119,
	DummyFunc120, DummyFunc121, DummyFunc122, DummyFunc123, DummyFunc124, DummyFunc125, DummyFunc126, DummyFunc127, DummyFunc128, DummyFunc129,
	DummyFunc130, DummyFunc131, DummyFunc132, DummyFunc133, DummyFunc134, DummyFunc135, DummyFunc136, DummyFunc137, DummyFunc138, DummyFunc139,
	DummyFunc140, DummyFunc141, DummyFunc142, DummyFunc143, DummyFunc144, DummyFunc145, DummyFunc146, DummyFunc147, DummyFunc148, DummyFunc149,
	DummyFunc150, DummyFunc151, DummyFunc152, DummyFunc153, DummyFunc154, DummyFunc155, DummyFunc156, DummyFunc157, DummyFunc158, DummyFunc159,
	DummyFunc160, DummyFunc161, DummyFunc162, DummyFunc163, DummyFunc164, DummyFunc165, DummyFunc166, DummyFunc167, DummyFunc168, DummyFunc169,
	DummyFunc170, DummyFunc171, DummyFunc172, DummyFunc173, DummyFunc174, DummyFunc175, DummyFunc176, DummyFunc177, DummyFunc178, DummyFunc179,
	DummyFunc180, DummyFunc181, DummyFunc182, DummyFunc183, DummyFunc184, DummyFunc185, DummyFunc186, DummyFunc187, DummyFunc188, DummyFunc189,
	DummyFunc190, DummyFunc191, DummyFunc192, DummyFunc193, DummyFunc194, DummyFunc195, DummyFunc196, DummyFunc197, DummyFunc198, DummyFunc199,
	DummyFunc200, DummyFunc201, DummyFunc202, DummyFunc203, DummyFunc204, DummyFunc205, DummyFunc206, DummyFunc207, DummyFunc208, DummyFunc209,
	DummyFunc210, DummyFunc211, DummyFunc212, DummyFunc213, DummyFunc214, DummyFunc215, DummyFunc216, DummyFunc217, DummyFunc218, DummyFunc219,
	DummyFunc220, DummyFunc221, DummyFunc222, DummyFunc223, DummyFunc224, DummyFunc225, DummyFunc226, DummyFunc227, DummyFunc228, DummyFunc229,
	DummyFunc230, DummyFunc231, DummyFunc232, DummyFunc233, DummyFunc234, DummyFunc235, DummyFunc236, DummyFunc237, DummyFunc238, DummyFunc239,
	DummyFunc240, DummyFunc241, DummyFunc242, DummyFunc243, DummyFunc244, DummyFunc245, DummyFunc246, DummyFunc247, DummyFunc248, DummyFunc249,
	DummyFunc250, DummyFunc251, DummyFunc252, DummyFunc253, DummyFunc254, DummyFunc255, DummyFunc256, DummyFunc257, DummyFunc258, DummyFunc259,
	DummyFunc260, DummyFunc261, DummyFunc262, DummyFunc263, DummyFunc264, DummyFunc265, DummyFunc266, DummyFunc267, DummyFunc268, DummyFunc269,
	DummyFunc270, DummyFunc271, DummyFunc272, DummyFunc273, DummyFunc274, DummyFunc275, DummyFunc276, DummyFunc277, DummyFunc278, DummyFunc279,
	DummyFunc280, DummyFunc281, DummyFunc282, DummyFunc283, DummyFunc284, DummyFunc285, DummyFunc286, DummyFunc287, DummyFunc288, DummyFunc289,
	DummyFunc290, DummyFunc291, DummyFunc292, DummyFunc293, DummyFunc294, DummyFunc295, DummyFunc296, DummyFunc297, DummyFunc298, DummyFunc299
	};

GLDEF_C TInt E32Main()
	{
	if (TestData1!=(TInt)0xb504f334)
		User::Panic(_L("T_PROT2A 1"),TestData1);
	TInt i;
	TInt x=0;
	for (i=0; i<128; i++)
		{
		x|=TestArray[i];
		}
	if (x!=0)
		User::Panic(_L("T_PROT2A 2"),x);
	for (i=0; i<300; i++)
		{
		(*Relocs[i])();
		}
	return KErrNone;
	}
