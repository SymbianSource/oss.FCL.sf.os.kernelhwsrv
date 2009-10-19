// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\bench\t_userasmfnc.cpp
// 
//

#include "t_userbm.h"
#include <e32huffman.h>
#include <e32math.h>
#include <e32base.h>
#include <e32base_private.h>
#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>

TBool PointsEqual(const TPoint& a, const TPoint& b)
	{
	return a.iX == b.iX && a.iY == b.iY;
	}

TInt ComparePoints(const TPoint& a, const TPoint& b)
	{
	if (a.iX < b.iX)
		return -1;
	else if (a.iX > b.iX)
		return 1;
	else if (a.iY < b.iY)
		return -1;
	else
		return a.iY < b.iY;
	}

RArray<TInt>          ArrayTInt;   // RArrayPointerBase derived
RArray<TUint>         ArrayTUint;  // RArrayPointerBase derived
RPointerArray<TPoint> ArrayTPointPtr;
RArray<TPoint> 		  ArrayTPoint;

TPoint Point(6, 13);
TIdentityRelation<TPoint> PointIdentity(PointsEqual);
TLinearOrder<TPoint> PointOrder(ComparePoints);

CObjectConIx* ObjectConIx;
CObjectIx* ObjectIx;
CObjectCon* ObjectCon;
TInt ObjectHandle;

TUint32 HuffmanTable[] = { 0, 1, 3, 8, 10, 3, 1, 0 };
TBitInput BitInput;
TUint8 BitData[] = { 0, 0, 0, 0 };

RRegion Region, Region2, Region3;
TRect Rect1(20, 20, 40, 40);

SPoly* Poly;
TRealX PolyXCoeffs[] = { 1.0, 3.0, 5.0 };

RHeap* Heap;
const TInt HeapSize = 32768;
const TInt SmallAlloc = 4;
const TInt Allocs = HeapSize / 2 / (SmallAlloc + RHeap::EAllocCellSize) / 2 * 2;

void InitDataL()
	{
	TInt array1Data[] = { 1, 2, 3, 5, 7, 11, 13, 17, 23 };
	for (TUint i = 0 ; i < sizeof(array1Data) / sizeof(TInt) ; ++i)
		{
		User::LeaveIfError(ArrayTInt.Append(array1Data[i]));
		User::LeaveIfError(ArrayTUint.Append(array1Data[i]));
		TPoint* p = new (ELeave) TPoint(i, array1Data[i]);
		CleanupStack::PushL(p);
		User::LeaveIfError(ArrayTPointPtr.Append(p));
		CleanupStack::Pop(p);
		User::LeaveIfError(ArrayTPoint.Append(*p));
		}

	ObjectConIx = CObjectConIx::NewL();
	ObjectCon = ObjectConIx->CreateL();
	ObjectIx = CObjectIx::NewL();
	CObject* o = new (ELeave) CObject;
	ObjectCon->AddL(o);
	ObjectHandle = ObjectIx->AddL(o);

	Huffman::HuffmanL(HuffmanTable, sizeof(HuffmanTable) / sizeof(TUint32), HuffmanTable);
	Huffman::Decoding(HuffmanTable, sizeof(HuffmanTable) / sizeof(TUint32), HuffmanTable);	

	Region.AddRect(TRect(10, 10, 30, 30));
	Region.AddRect(TRect(30, 20, 50, 50));
	Region.AddRect(TRect(10, 50, 50, 60));

	Region2.AddRect(TRect(0, 40, 100, 60));
	Region2.AddRect(TRect(40, 0, 60, 100));
	Region2.AddRect(TRect(30, 30, 70, 70));

	Region3.AddRect(TRect(0, 30, 100, 40));
	Region3.AddRect(TRect(0, 0, 100, 10));
	Region3.AddRect(TRect(0, 20, 100, 30));
	Region3.AddRect(TRect(0, 10, 100, 20));
	Region3.AddRect(TRect(0, 40, 100, 50));

	Poly = (SPoly*) User::AllocL(sizeof(SPoly) + (3 - 1) * sizeof(TReal));
	Poly->num = 3;
	Poly->c[0] = 3;
	Poly->c[1] = 6;
	Poly->c[2] = 9;

	Heap = (RHeap*)User::LeaveIfNull(UserHeap::ChunkHeap(NULL, HeapSize, HeapSize));	
	TInt x;	
	TAny* cells[Allocs];
	for (x=0; x<Allocs; ++x)
		cells[x] = Heap->Alloc(4);
	for (x=0; x<Allocs; x += 2)
		Heap->Free(cells[x]);
	}

// Define benchmarks for assembler-coded euser functions.  Note that these are
// named after the function we're trying to test, which is not necessarily the
// one we call.

// RPointerArrayBase
DEFINE_USER_BENCHMARK(RPointerArrayBase_At,
					  ,
					  ArrayTInt[4]);

DEFINE_USER_BENCHMARK(RPointerArrayBase_Append,
					  RArray<TInt> a,
					  a.Reset(); a.Append(1));

DEFINE_USER_BENCHMARK(RPointerArrayBase_Find1,
					  ,
					  ArrayTInt.Find(5));

DEFINE_USER_BENCHMARK(RPointerArrayBase_Find2,
					  ,
					  ArrayTPointPtr.Find(&Point, PointIdentity));

DEFINE_USER_BENCHMARK(RPointerArrayBase_BinarySearchSigned,
					  TInt i,
					  ArrayTInt.SpecificFindInOrder(13, i, EArrayFindMode_Any));

DEFINE_USER_BENCHMARK(RPointerArrayBase_BinarySearchUnsigned,
					  TInt i,
					  ArrayTUint.SpecificFindInOrder(13, i, EArrayFindMode_Any));

DEFINE_USER_BENCHMARK(RPointerArrayBase_BinarySearch,
					  ,
					  ArrayTPointPtr.SpecificFindInOrder(&Point, PointOrder, EArrayFindMode_Any));

DEFINE_USER_BENCHMARK(RPointerArrayBase_FindIsqSigned,
					  ,
					  ArrayTInt.SpecificFindInOrder(5, EArrayFindMode_Any));

DEFINE_USER_BENCHMARK(RPointerArrayBase_FindIsqUnsigned,
					  ,
					  ArrayTUint.SpecificFindInOrder(5, EArrayFindMode_Any));

DEFINE_USER_BENCHMARK(RPointerArrayBase_FindIsq,
					  ,
					  ArrayTPointPtr.SpecificFindInOrder(&Point, PointOrder, EArrayFindMode_Any));

DEFINE_USER_BENCHMARK(RPointerArrayBase_HeapSortSigned,
					  ,
					  ArrayTInt.Sort());

DEFINE_USER_BENCHMARK(RPointerArrayBase_HeapSortUnsigned,
					  ,
					  ArrayTUint.Sort());

DEFINE_USER_BENCHMARK(RPointerArrayBase_HeapSort,
					  ,
					  ArrayTPointPtr.Sort(PointOrder));

// RArrayBase
DEFINE_USER_BENCHMARK(RArrayBase_At,
					  ,
					  ArrayTPoint[4]);

DEFINE_USER_BENCHMARK(RArrayBase_Append,
					  RArray<TPoint> a,
					  a.Reset(); a.Append(Point));

DEFINE_USER_BENCHMARK(RArrayBase_Find1,
					  ,
					  ArrayTPoint.Find(Point));

DEFINE_USER_BENCHMARK(RArrayBase_Find2,
					  ,
					  ArrayTPoint.Find(Point, PointIdentity));

DEFINE_USER_BENCHMARK(RArrayBase_BinarySearchSigned,
					  TInt i,
					  ArrayTPoint.SpecificFindInSignedKeyOrder(Point, i, EArrayFindMode_Any));

DEFINE_USER_BENCHMARK(RArrayBase_BinarySearchUnsigned,
					  TInt i,
					  ArrayTPoint.SpecificFindInUnsignedKeyOrder(Point, i, EArrayFindMode_Any));

DEFINE_USER_BENCHMARK(RArrayBase_BinarySearch,
					  TInt i,
					  ArrayTPoint.FindInOrder(Point, i, PointOrder));

DEFINE_USER_BENCHMARK(RArrayBase_FindIsqSigned,
					  ,
					  ArrayTPoint.SpecificFindInSignedKeyOrder(Point, EArrayFindMode_Any));

DEFINE_USER_BENCHMARK(RArrayBase_FindIsqUnsigned,
					  ,
					  ArrayTPoint.SpecificFindInUnsignedKeyOrder(Point, EArrayFindMode_Any));

DEFINE_USER_BENCHMARK(RArrayBase_HeapSortSigned,
					  ,
					  ArrayTPoint.SortSigned());

DEFINE_USER_BENCHMARK(RArrayBase_HeapSortUnsigned,
					  ,
					  ArrayTPoint.SortUnsigned());

DEFINE_USER_BENCHMARK(RArrayBase_HeapSort,
					  ,
					  ArrayTPointPtr.Sort(PointOrder));

// CObject related
DEFINE_USER_BENCHMARK(CObjectIx_At1,
					  ,
					  ObjectIx->At(ObjectHandle));

DEFINE_USER_BENCHMARK(CObjectIx_At2,
					  ,
					  ObjectIx->At(ObjectHandle, 0));

DEFINE_USER_BENCHMARK(CObjectIx_ArrayOperator,
					  ,
					  (*ObjectIx)[0]);

DEFINE_USER_BENCHMARK(CObjectCon_ArrayOperator,
					  ,
					  (*ObjectCon)[0]);

DEFINE_USER_BENCHMARK(CObjectCon_At,
					  ,
					  ObjectCon->At(ObjectHandle));

DEFINE_USER_BENCHMARK(CObjectCon_AtL,
					  ,
					  ObjectCon->AtL(ObjectHandle));

// Huffman coding
DEFINE_USER_BENCHMARK(TBitInput_ReadL1,
					  ,
					  BitInput.Set(BitData, 128); BitInput.ReadL());

DEFINE_USER_BENCHMARK(TBitInput_ReadL2,
					  ,
					  BitInput.Set(BitData, 128); BitInput.ReadL(7));

DEFINE_USER_BENCHMARK(TBitInput_HuffmanL,
					  ,
					  BitInput.Set(BitData, 128); BitInput.HuffmanL(HuffmanTable));

// Regions
DEFINE_USER_BENCHMARK(TRegion_BoundingRect,
					  ,
					  Region.BoundingRect());

DEFINE_USER_BENCHMARK(TRegion_IsContainedBy,
					  TRect r(0,0,90,90),
					  Region.IsContainedBy(r));

DEFINE_USER_BENCHMARK(TRegion_Copy,
					  RRegion r,
					  r.Copy(Region));

DEFINE_USER_BENCHMARK(TRegion_Offset1,
					  ,
					  Region.Offset(0, 0));

DEFINE_USER_BENCHMARK(TRegion_Contains,
					  TPoint p(0, 0),
					  Region.Contains(p));

DEFINE_USER_BENCHMARK(TRegion_Intersects,
					  TRect r(0, 0, 10, 10),
					  Region2.Intersects(r));

DEFINE_USER_BENCHMARK(TRegion_SubRect,
					  RRegion r,
					  r.Copy(Region); r.SubRect(Rect1));

DEFINE_USER_BENCHMARK(TRegion_Intersection,
					  RRegion r,
					  r.Intersection(Region, Region2));

DEFINE_USER_BENCHMARK(TRegion_ClipRect,
					  RRegion r,
					  r.Copy(Region); r.ClipRect(Rect1));

DEFINE_USER_BENCHMARK(TRegion_Tidy,
					  RRegion r,
					  r.Copy(Region3); r.Tidy());

// Maths
DEFINE_USER_BENCHMARK(Math_Frac,
					  TReal t,
					  Math::Frac(t, KPi));

DEFINE_USER_BENCHMARK(Math_Int1,
					  TReal t,
					  Math::Int(t, KPi));

DEFINE_USER_BENCHMARK(Math_Int2,
					  TInt16 t,
					  Math::Int(t, KPi));

DEFINE_USER_BENCHMARK(Math_Int3,
					  TInt32 t,
					  Math::Int(t, KPi));

DEFINE_USER_BENCHMARK(Math_IsZero,
					  ,
					  Math::IsZero(KPi));

DEFINE_USER_BENCHMARK(Math_IsNaN,
					  ,
					  Math::IsNaN(KPi));

DEFINE_USER_BENCHMARK(Math_IsInfinite,
					  ,
					  Math::IsInfinite(KPi));

DEFINE_USER_BENCHMARK(Math_IsFinite,
					  ,
					  Math::IsFinite(KPi));

DEFINE_USER_BENCHMARK(Math_Sqrt,
					  TReal t,
					  Math::Sqrt(t, KPi));

DEFINE_USER_BENCHMARK(Math_Poly,
					  TRealX t,
					  Math::PolyX(t, KPi, 3, PolyXCoeffs));

// Not tested due to not being exported:
//   Math::SetZero
//   Math::SetNan
//   Math::SetInfinite

// TRealX
DEFINE_USER_BENCHMARK(TRealX_Cons1,
					  TRealX t,
					  new (&t) TRealX);

DEFINE_USER_BENCHMARK(TRealX_Cons2,
					  TRealX t,
					  new (&t) TRealX(0, 0, 0));

// covers TRealX::Set(TInt), TRealX::TRealX(TInt) and TRealX::operator=(TInt)
DEFINE_USER_BENCHMARK(TRealX_SetInt,
					  TRealX t,
					  t = (TInt) 23);

// covers TRealX::Set(TUint), TRealX::TRealX(TUint) and TRealX::operator=(TUint)
DEFINE_USER_BENCHMARK(TRealX_SetUint,
					  TRealX t,
					  t = (TUint) 23);

// covers TRealX::Set(TInt64), TRealX::TRealX(TInt64) and TRealX::operator=(TInt64)
DEFINE_USER_BENCHMARK(TRealX_SetInt64,
					  TRealX t,
					  t = (TInt64) 23);

// covers TRealX::Set(TReal32), TRealX::TRealX(TReal32) and TRealX::operator=(TReal32)
DEFINE_USER_BENCHMARK(TRealX_SetReal32,
					  TRealX t; TReal32 v = 23,
					  t = v);

// covers TRealX::Set(TReal64), TRealX::TRealX(TReal64) and TRealX::operator=(TReal64)
DEFINE_USER_BENCHMARK(TRealX_SetReal64,
					  TRealX t; TReal64 v = 23,
					  t = v);

DEFINE_USER_BENCHMARK(TRealX_SetZero,
					  TRealX t,
					  t.SetZero(EFalse));

DEFINE_USER_BENCHMARK(TRealX_SetNaN,
					  TRealX t,
					  t.SetNaN());

DEFINE_USER_BENCHMARK(TRealX_SetInfinite,
					  TRealX t,
					  t.SetInfinite(EFalse));

DEFINE_USER_BENCHMARK(TRealX_IsZero,
					  TRealX t,
					  t.IsZero());

DEFINE_USER_BENCHMARK(TRealX_IsNaN,
					  TRealX t; t.SetNaN(),
					  t.IsNaN());

DEFINE_USER_BENCHMARK(TRealX_IsInfinite,
					  TRealX t; t.SetInfinite(EFalse),
					  t.IsInfinite());

DEFINE_USER_BENCHMARK(TRealX_IsFinite,
					  TRealX t,
					  t.IsFinite());

DEFINE_USER_BENCHMARK(TRealX_Int,
					  TRealX t = 2.3,
					  (TInt) t);

DEFINE_USER_BENCHMARK(TRealX_Uint,
					  TRealX t = 2.3,
					  (TUint) t);

DEFINE_USER_BENCHMARK(TRealX_Int64,
					  TRealX t = 2.3,
					  (TInt64) t);

DEFINE_USER_BENCHMARK(TRealX_Real32,
					  TRealX t = 2.3,
					  (TReal32) t);

DEFINE_USER_BENCHMARK(TRealX_Real64,
					  TRealX t = 2.3,
					  (TReal64) t);

DEFINE_USER_BENCHMARK(TRealX_GetReal32,
					  TRealX t = 2.3; TReal32 out,
					  t.GetTReal(out));

DEFINE_USER_BENCHMARK(TRealX_GetReal64,
					  TRealX t = 2.3; TReal64 out,
					  t.GetTReal(out));

DEFINE_USER_BENCHMARK(TRealX_UnaryPlus,
					  TRealX t,
					  +t);

DEFINE_USER_BENCHMARK(TRealX_UnaryMinus,
					  TRealX t,
					  -t);

DEFINE_USER_BENCHMARK(TRealX_Compare,
					  TRealX a = 0.2; TRealX b = 0.21,
					  a.Compare(b));

DEFINE_USER_BENCHMARK(TRealX_Sub,
					  TRealX a = 1.0; TRealX b = 0.2,
					  a - b);

DEFINE_USER_BENCHMARK(TRealX_Add,
					  TRealX a = 1.0; TRealX b = 0.2,
					  a + b);

DEFINE_USER_BENCHMARK(TRealX_Mult,
					  TRealX a = 1.0; TRealX b = 0.2,
					  a * b);

DEFINE_USER_BENCHMARK(TRealX_Div,
					  TRealX a = 1.0; TRealX b = 0.2,
					  a / b);

DEFINE_USER_BENCHMARK(TRealX_Mod,
					  TRealX a = 1.1; TRealX b = 0.2,
					  a % b);

DEFINE_USER_BENCHMARK(TRealX_PreInc,
					  TRealX t = 0.0,
					  ++t);

DEFINE_USER_BENCHMARK(TRealX_PostInc,
					  TRealX t = 0.0,
					  t++);

DEFINE_USER_BENCHMARK(TRealX_PreDec,
					  TRealX t = 0.0,
					  --t);

DEFINE_USER_BENCHMARK(TRealX_PostDec,
					  TRealX t = 0.0,
					  t--);

DEFINE_USER_BENCHMARK(RHeap_AllocFree,
					  TAny* mem,
					  mem = Heap->Alloc(8);
					  Heap->Free(mem));

// The following were added for timing functions affected by user-side thread data.

DEFINE_USER_BENCHMARK(User_Heap,
					  ,
					  User::Heap());

DEFINE_USER_BENCHMARK(CActiveScheduler_Current,
					  ,
					  CActiveScheduler::Current());

DEFINE_USER_BENCHMARK(User_TrapHandler,
					  ,
					  User::TrapHandler());

DEFINE_USER_BENCHMARK(UserSvr_DllSetTls,
					  ,
					  UserSvr::DllSetTls(0, 0, 0));

DEFINE_USER_BENCHMARK(UserSvr_DllTls,
					  ,
					  UserSvr::DllTls(0));
