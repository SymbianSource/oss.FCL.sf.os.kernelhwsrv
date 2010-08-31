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
// e32test\heap\t_heapdl.cpp
// Overview:
// Tests RHybridHeap class.
// API Information:
// RHybridHeap
// Details:
// 
//

#include <e32test.h>
#include <e32math.h>
#include <e32def_private.h>
#include "dla.h"
#include "slab.h"
#include "page_alloc.h"
#include "heap_hybrid.h"

const TUint KTestIterations = 100;

const TInt KHeadSize = (TInt)RHeap::EAllocCellSize;

class TestHybridHeap
  {
public:
  static void TopSize(TInt& aTopSize, const RHybridHeap * aHybridHeap);
  static void DvSize(TInt& aDvSize, const RHybridHeap * aHybridHeap);
  static void SmallMap(TUint& aSmallMap, const RHybridHeap * aHybridHeap);
  static void TreeMap(TUint& aTreeMap, const RHybridHeap * aHybridHeap);
  static void TrimCheck(TInt& aTrimCheck, const RHybridHeap * aHybridHeap);
  static void GrowBy(TInt& aGrowBy, const RHybridHeap * aHybridHeap);
  static void PageSize(TInt& aPageSize, const RHybridHeap * aHybridHeap);
  };


void TestHybridHeap::TopSize(TInt& aTopSize, const RHybridHeap * aHybridHeap)
  {
  aTopSize = aHybridHeap->iGlobalMallocState.iTopSize;
  }


void TestHybridHeap::DvSize(TInt& aDvSize, const RHybridHeap * aHybridHeap)
  {
  aDvSize = aHybridHeap->iGlobalMallocState.iDvSize;
  }


void TestHybridHeap::SmallMap(TUint& aSmallMap, const RHybridHeap * aHybridHeap)
  {
  aSmallMap = aHybridHeap->iGlobalMallocState.iSmallMap;
  }


void TestHybridHeap::TreeMap(TUint& aTreeMap, const RHybridHeap * aHybridHeap)
  {
  aTreeMap = aHybridHeap->iGlobalMallocState.iTreeMap;
  }


void TestHybridHeap::TrimCheck(TInt& aTrimCheck, const RHybridHeap * aHybridHeap)
  {
  aTrimCheck = aHybridHeap->iGlobalMallocState.iTrimCheck;
  }


void TestHybridHeap::GrowBy(TInt& aGrowBy, const RHybridHeap * aHybridHeap)
  {
  aGrowBy = aHybridHeap->iGrowBy;
  }

void TestHybridHeap::PageSize(TInt& aPageSize, const RHybridHeap * aHybridHeap)
  {
  aPageSize = aHybridHeap->iPageSize;
  }


LOCAL_D RTest test(_L("T_HEAPDL"));


class TestRHeap
  {
public:
  void InitTests();
  void Test1(void);
  void Test2(void);
  void Test3(void);
  void Test4(void);
  void CloseTests();
private:
  RHybridHeap* iHybridHeap;
  RHeap *iHeap;
  };


void TestRHeap::InitTests()
  {
  // Allocate a chunk heap
  TPtrC testHeap=_L("TESTHEAP");
  iHeap=User::ChunkHeap(&testHeap,0x1800,0x16000);
        
  RHybridHeap::STestCommand cmd;
  cmd.iCommand = RHybridHeap::EHeapMetaData;
  iHeap->DebugFunction(RHeap::EHybridHeap, (TAny*)&cmd );
  iHybridHeap = (RHybridHeap*) cmd.iData;
  }


void TestRHeap::Test1(void)
  {
  //
  //  Splitting a new cell off 'top' chunk
  //  Growing and shrinking 'top' chunk
  //  Coalesceing of adjacent free cells
  //
    
  TInt topSizeBefore, topSizeAfter, allocSize;
  TInt growBy, pageSize;
  TestHybridHeap::GrowBy(growBy,iHybridHeap);
  TestHybridHeap::PageSize(pageSize,iHybridHeap);
        
  //Splitting a new cell off 'top' chunk
  TestHybridHeap::TopSize(topSizeBefore,iHybridHeap);
  TAny* p1=iHeap->Alloc(0x256);
  TestHybridHeap::TopSize(topSizeAfter,iHybridHeap);
  test(topSizeBefore > topSizeAfter);
  iHeap->Check();
  iHeap->Free(p1);
  iHeap->Check();
    
  //Growing 'top' chunk
  test(iHeap!=NULL);
  TestHybridHeap::TopSize(topSizeBefore,iHybridHeap);
  p1=iHeap->Alloc(pageSize*2); 
  test(p1!=NULL);
  allocSize=iHeap->AllocLen(p1);
  TestHybridHeap::TopSize(topSizeAfter,iHybridHeap);
  test(topSizeBefore + growBy == topSizeAfter+allocSize+KHeadSize);
    
  //Splitting a new cell off 'top' chunk
  TAny *p2=iHeap->Alloc(pageSize/8); 
  test(p2!=NULL);
  //Splitting a new cell off 'top' chunk
  TAny *p3=iHeap->Alloc(pageSize/2); 
  test(p3!=NULL);
  //Growing 'top' chunk
  TAny *p4=iHeap->Alloc(pageSize*2); 
  test(p4!=NULL);
  //Take allocSize of p4
  allocSize=iHeap->AllocLen(p4);
    
  //Shrinking 'top' chunk
  TInt trimCheck;
  TestHybridHeap::TopSize(topSizeBefore,iHybridHeap);
  iHeap->Free(p4);
  TestHybridHeap::TopSize(topSizeAfter,iHybridHeap);
  TestHybridHeap::TrimCheck(trimCheck,iHybridHeap);
  test(topSizeAfter + trimCheck == topSizeBefore+allocSize+KHeadSize);
  iHeap->Check();
  
  //Insert chunk into treebin
  TUint treeMap,treeMap2;
  TestHybridHeap::TreeMap(treeMap,iHybridHeap);
  test(treeMap==0);
  iHeap->Free(p2);
  TestHybridHeap::TreeMap(treeMap,iHybridHeap);
  test(treeMap>0);
  iHeap->Check();
    
  //Coalesce adjacent free cells and insert chunk into treebin
  TestHybridHeap::TreeMap(treeMap,iHybridHeap);
  iHeap->Free(p1);
  TestHybridHeap::TreeMap(treeMap2,iHybridHeap);
  test(treeMap < treeMap2);
  iHeap->Check();
    
  //free last allocation
  iHeap->Free(p3);
  iHeap->Check();
  }


void TestRHeap::Test2(void)
  {
  //
  // Allocation of exact sized cells from 'small cell' lists (smallbin)
  // Freeing of exact sized cells back to 'small cell' lists (smallbin)
  //
  TInt ArraySize=32;
  TInt cellSize=0;
  TInt topSizeBefore, topSizeAfter;
     
  TAny** ArrayOfCells;
  ArrayOfCells= new TAny*[ArraySize];
  TInt ArrayIndex;
  // Allocate exact sized small cells 8,16,32,40--->
  // and put them to the array. They are allocated from TOP chunk
  for(ArrayIndex=0; ArrayIndex<ArraySize;ArrayIndex++)
    {
    TestHybridHeap::TopSize(topSizeBefore,iHybridHeap);
    cellSize=cellSize+8;
    ArrayOfCells[ArrayIndex]=iHeap->Alloc(cellSize);
    TestHybridHeap::TopSize(topSizeAfter,iHybridHeap);
    test(topSizeBefore > topSizeAfter);
    }
  iHeap->Check();
  
  TUint smallMap, smallMap2;
  TInt dvSize, dvSize2;
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  test(smallMap == 0);
  // Free some of small cells from the array. So they are inserted
  // to the smallbin
  for(ArrayIndex=2; ArrayIndex<ArraySize-1; ArrayIndex+=5)
    {
    TestHybridHeap::SmallMap(smallMap,iHybridHeap);
    iHeap->Free(ArrayOfCells[ArrayIndex]);
    TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
    test(smallMap<smallMap2);
    }
  iHeap->Check();
    
  // Allocate exact sized cells from smallbin (or Designated Victim)   
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  TAny* p1=iHeap->Alloc(32);
  TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
  test(smallMap>smallMap2);
    
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  TestHybridHeap::DvSize(dvSize,iHybridHeap);
  TAny* p2=iHeap->Alloc(32);
  TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
  TestHybridHeap::DvSize(dvSize2,iHybridHeap);
  if(dvSize <= dvSize2)
    test(smallMap>smallMap2);
    
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  TestHybridHeap::DvSize(dvSize,iHybridHeap);
  TAny* p3=iHeap->Alloc(32);
  TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
  TestHybridHeap::DvSize(dvSize2,iHybridHeap);
  if(dvSize <= dvSize2)
  	test(smallMap>smallMap2);
   
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  TestHybridHeap::DvSize(dvSize,iHybridHeap);
  TAny* p4=iHeap->Alloc(32);
  TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
  TestHybridHeap::DvSize(dvSize2,iHybridHeap);
  if(dvSize <= dvSize2)
    test(smallMap>smallMap2);
   
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  TestHybridHeap::DvSize(dvSize,iHybridHeap);
  TAny* p5=iHeap->Alloc(48);
  TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
  TestHybridHeap::DvSize(dvSize2,iHybridHeap);
  if(dvSize <= dvSize2)
    test(smallMap>smallMap2);
    
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  TestHybridHeap::DvSize(dvSize,iHybridHeap);
  TAny* p6=iHeap->Alloc(64);
  TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
  TestHybridHeap::DvSize(dvSize2,iHybridHeap);
  if(dvSize <= dvSize2)
    test(smallMap>smallMap2);
   
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  TestHybridHeap::DvSize(dvSize,iHybridHeap);
  TAny* p7=iHeap->Alloc(80);
  TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
  TestHybridHeap::DvSize(dvSize2,iHybridHeap);
  if(dvSize <= dvSize2)
    test(smallMap>smallMap2);
   
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  TestHybridHeap::DvSize(dvSize,iHybridHeap);
  TAny* p8=iHeap->Alloc(96);
  TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
  TestHybridHeap::DvSize(dvSize2,iHybridHeap);
  if(dvSize <= dvSize2)
    test(smallMap>smallMap2);
  iHeap->Check();
  
  // Freeing of exact sized cells back to smallbin
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  iHeap->Free(p1);
  iHeap->Free(p2);
  iHeap->Free(p3);
  iHeap->Free(p4);
  iHeap->Free(p5);
  iHeap->Free(p6);
  iHeap->Free(p7);
  iHeap->Free(p8);
  TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
  test(smallMap < smallMap2);
  iHeap->Check();
  
  // Now free rest of the array with Reset
  iHeap->Reset();
  iHeap->Check();
  
  delete [] ArrayOfCells;
  }


void TestRHeap::Test3(void)
  {
  //
  // Allocation of approximate sized cells from 'small cell' lists (smallbin)
  //
  const TInt ArraySize=32;
  TInt cellSize=0;
  TAny** ArrayOfCells;
  ArrayOfCells= new TAny*[ArraySize];
  TInt ArrayIndex;
  TInt topSizeBefore, topSizeAfter;
    
  // Allocate small approximate sized cells and put
  //them to the array. They are allocated from TOP chunk
  TUint8 randomSize[ArraySize];
  for(ArrayIndex=0; ArrayIndex<ArraySize;ArrayIndex++)
    {
    TestHybridHeap::TopSize(topSizeBefore,iHybridHeap);
    // Ensure that the size of the cell does not exceed 256 bytes on debug builds
    randomSize[ArrayIndex] = (TUint8) (Math::Random() % (MAX_SMALL_REQUEST + 1 - RHeap::EDebugHdrSize));
  	cellSize=randomSize[ArrayIndex];
  	ArrayOfCells[ArrayIndex]=iHeap->Alloc(cellSize);
  	TestHybridHeap::TopSize(topSizeAfter,iHybridHeap);
  	test(topSizeBefore > topSizeAfter);
  	}
  iHeap->Check();
  
  TUint smallMap, smallMap2;
  // Free some of allocated cells from the array. So they are inserted
  // to the smallbin
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  for(ArrayIndex=2; ArrayIndex<ArraySize-1; ArrayIndex+=5)
    {
    iHeap->Free(ArrayOfCells[ArrayIndex]);
    }
  TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
  test(smallMap<=smallMap2);
  iHeap->Check();
  
  // Allocate approximate sized cells from smallbin
  const TInt ArraySize2=6;
  TInt cellSize2=0;
  TAny** ArrayOfCells2;
  ArrayOfCells2= new TAny*[ArraySize2];
  TInt ArrayIndex2;
  TestHybridHeap::SmallMap(smallMap,iHybridHeap);
  TUint8 randomSize2[ArraySize2];
  for(ArrayIndex2=0; ArrayIndex2<ArraySize2;ArrayIndex2++)
    {
    randomSize2[ArrayIndex2]=randomSize[2+ArrayIndex2*5];
    cellSize2=randomSize2[ArrayIndex2];
    ArrayOfCells2[ArrayIndex2]=iHeap->Alloc(cellSize2);
    }
  TestHybridHeap::SmallMap(smallMap2,iHybridHeap);
  test(smallMap>=smallMap2);
  iHeap->Check();
  
  // Freeing of approximate sized cells back to smallbin
  for(ArrayIndex2=0; ArrayIndex2<ArraySize2-1; ArrayIndex2+=1)
    {
    iHeap->Free(ArrayOfCells2[ArrayIndex2]);
    }
  iHeap->Check();
  
  // Now free rest of the array with Reset
  iHeap->Reset();
  iHeap->Check();
  
  delete [] ArrayOfCells;
  delete [] ArrayOfCells2; 
  }


void TestRHeap::Test4(void)
  {
  //
  // Allocation of approximate sized cells from digital trees (treebin) and splitting
  // Freeing of approximate sized cells back to digital trees (treebin)
  //
  const TInt ArraySize=32;
  TInt cellSize=0;
  TAny** ArrayOfCells;
  ArrayOfCells= new TAny*[ArraySize];
  TInt ArrayIndex;
        
  // Allocate approximate sized cells bigger than 256
  // and put them to the array. They are allocated from TOP chunk
  TUint8 randomSize[ArraySize];
  for(ArrayIndex=0; ArrayIndex<ArraySize;ArrayIndex++)
    {
    randomSize[ArrayIndex] = (TUint8)Math::Random();
    cellSize=(randomSize[ArrayIndex]+MAX_SMALL_REQUEST+1);
    ArrayOfCells[ArrayIndex]=iHeap->Alloc(cellSize);
    }
  iHeap->Check();
  
  TUint treeMap,treeMap2;
  // Free some of allocated cells from the array. So they are inserted
  // to the treebin
  for(ArrayIndex=2; ArrayIndex<ArraySize-1; ArrayIndex+=5)
    {
    TestHybridHeap::TreeMap(treeMap,iHybridHeap);
    iHeap->Free(ArrayOfCells[ArrayIndex]);
    TestHybridHeap::TreeMap(treeMap2,iHybridHeap);
    test(treeMap <= treeMap2);
    }
  iHeap->Check();
  
  // Allocate approximate sized cells from treebin
  const TInt ArraySize2=16;
  TInt cellSize2=0;    
  TAny** ArrayOfCells2;
  ArrayOfCells2= new TAny*[ArraySize2];
  TInt ArrayIndex2;
  TUint8 randomSize2[ArraySize2];
  for(ArrayIndex2=0; ArrayIndex2<ArraySize2;ArrayIndex2++)
    {
    TestHybridHeap::TreeMap(treeMap,iHybridHeap);
    randomSize2[ArrayIndex2] = (TUint8)Math::Random();
    cellSize2=(randomSize2[ArrayIndex2]+MAX_SMALL_REQUEST+1);
    ArrayOfCells2[ArrayIndex2]=iHeap->Alloc(cellSize2);
    TestHybridHeap::TreeMap(treeMap2,iHybridHeap);
    test(treeMap >= treeMap2);
    }
  iHeap->Check();
  
  // Freeing of approximate sized cells back to treebin
  TestHybridHeap::TreeMap(treeMap,iHybridHeap);
  for(ArrayIndex2=0; ArrayIndex2<ArraySize2-1; ArrayIndex2+=1)
    {
    iHeap->Free(ArrayOfCells2[ArrayIndex2]);
    }
  TestHybridHeap::TreeMap(treeMap2,iHybridHeap);
  test(treeMap <= treeMap2);
  iHeap->Check();
    
  // Now free rest of the array with Reset
  iHeap->Reset();
  iHeap->Check();
    
  delete [] ArrayOfCells;
  delete [] ArrayOfCells2; 
  }


void TestRHeap::CloseTests()
  {
  // close heap so we don't exceed chunk limit
  iHeap->Close();  
  }


GLDEF_C TInt E32Main(void)
  {
  test.Title();

  __KHEAP_MARK;

  TestRHeap T;
  TUint i;
  test.Start(_L("Init DL allocator tests"));
  T.InitTests();
  test.Next(_L("Test DL allocator 1"));
  for(i = 0; i < KTestIterations; i++)
    {
    T.Test1();
    }
  test.Next(_L("Test DL allocator 2"));
  for(i = 0; i < KTestIterations; i++)
    {
    T.Test2();
    }
  test.Next(_L("Test DL allocator 3"));
  for(i = 0; i < KTestIterations; i++)
    {
    T.Test3();
    }
  test.Next(_L("Test DL allocator 4"));
  for(i = 0; i < KTestIterations; i++)
    {
    T.Test4();
    }
  test.Next(_L("Close DL allocator tests"));
  T.CloseTests();
	
  __KHEAP_CHECK(0);
  __KHEAP_MARKEND;

  test.End();
  return(0);
  }
