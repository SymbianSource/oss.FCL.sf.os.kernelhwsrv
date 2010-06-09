/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*/
//
// Contributors:
//
// Description:
// This file is part of the NE1_TB Variant Base Port
// Hardware Configuration Respoitory Platform Specific Layer (PSL) 
//


// -- INCLUDES ----------------------------------------------------------------


#include "hcr_debug.h"

#include "hcr_hai.h"

#include "hcr_uids.h"

#include <plat_priv.h>
#include <kernel/kernboot.h>


#ifdef __WINS__
// On WINS the EMapAttrSupRo and EMapAttrCachedMax doesn't exists
#define EMapAttrSupRo       0x01
#define EMapAttrCachedMax   0xF000
#endif


// -- GLOBALS -----------------------------------------------------------------


GLREF_C HCR::SRepositoryCompiled gRepository;
#define BUFFER_OFFSET_ZERO  0

// -- CLASSES- ----------------------------------------------------------------


class HCRVariant : public HCR::MVariant
    {    
    
public:

    HCRVariant();
    virtual ~HCRVariant();
    
public:    
    
    TInt Initialise();
    
    TBool IgnoreCoreImgRepository();
	TInt GetCompiledRepositoryAddress( TAny* & aAddr);
    TInt GetOverrideRepositoryAddress( TAny* & aAddr);
    
private:
    DChunk * iChunk;    
    };
    
    


// -- METHODS -----------------------------------------------------------------


HCRVariant::HCRVariant()
    : iChunk(0)
    {
    HCR_FUNC("HCRVariant");
    }


HCRVariant::~HCRVariant()
    {
    HCR_FUNC("~HCRVariant");
    if (iChunk != 0)
        {
        NKern::ThreadEnterCS();    
        TInt r = Kern::ChunkClose(iChunk);
        __NK_ASSERT_ALWAYS(r!=0);
        NKern::ThreadLeaveCS(); 
        }
    }


TInt HCRVariant::Initialise()
    {
    HCR_FUNC("HCRVariant::Initialise");
    
    HCR_TRACE_RETURN(KErrNone);
    }
    
    
TInt HCRVariant::GetCompiledRepositoryAddress( TAny* & aAddr)
    {
    HCR_FUNC("HCRVariant::GetCompiledRepositoryAddress");
        
    aAddr = static_cast<TAny*>(&gRepository);
    HCR_TRACE_RETURN(KErrNone);
    }

TBool HCRVariant::IgnoreCoreImgRepository()
    {
    HCR_FUNC("HCRVariant::IgnoreCoreImgRepository");
        
    HCR_TRACE_RETURN(EFalse);
    }

TInt HCRVariant::GetOverrideRepositoryAddress( TAny* & aAddr)
    {
    HCR_FUNC("HCRVariant::GetRAMRepositoryAddress");
    aAddr = 0;

	// Note: the SMR feature by which we obtain the address of the override
	// repository is only supported in the ARM bootstrap, not X86 or WINS so 
	// this test code needs conditional compilation.
#if !defined(__WINS__) && !defined(__X86__)
    
    // Note to future implementor:
	// #include <kernel/kernboot.h>
	// First check to see if SMRIB was created during boot time. 
    // If SSuperPageBase::iSmrData == KSuperPageAddressFieldUndefined (i.e. -1) 
	// it does not exist, return KErrNotSupported, SMR not support by base port
    // or it is not available due to boot scenario, i.e. boot from MMC
    //
    // If it does exist (i.e. boot from NAND) then read and process the 
	// SMR entries listed in the SMRIB looking for KHCRUID_SMRPayloadUID.
	// Next using the internal sizes from the HCR dat file within the SMR image
	// determine if the RAM holding the SMR image can be shrunk to return 
	// unused RAM pages at the end of the image.
	// 
	// Finally allocate the reserved RAM identified in the SMR entry to a 
	// DChunk and return the virtual address of the HCR data file payload
	// within the SMR image, i.e. iBase+(sizeof(SSmrRomHeader)>>2).
    // Locate SMRIB 
    const TSuperPage& superpage = Kern::SuperPage();
   	TUint32* smrIB;
   	smrIB = (TUint32 *) superpage.iSmrData;
   	
   	HCR_TRACE2("--- Superpage: 0x%08x, SMRIB: 0x%08x", &superpage, smrIB);
    
   	if( (smrIB == NULL) || (smrIB == (TUint32*)KSuperPageAddressFieldUndefined))
   	    {
        HCR_TRACE_RETURN(KErrNotSupported);   	        
   	    }
   	    
   	HCR_HEX_DUMP_ABS((TUint8 *)smrIB, 8*sizeof(SSmrBank) );
    SSmrBank * smrBank = (SSmrBank *) smrIB;
    
    // T_HCRUT designed to work ith the second HCR SMR image as the first 
	// is used by the t_hcr test suite.
    int smrInst = 2;
    
    while( smrBank->iBase != 0 ) 
        {
        HCR_TRACE2("--- smrBank: 0x%08x, smrBank->iPayloadUID: 0x%08x", smrBank, smrBank->iPayloadUID);
        
        if (smrBank->iPayloadUID == KHCRUID_SMRPayloadUID)
            {
            smrInst--;
			if (smrInst == 0) // PSL to use only the 2nd HCR SMR image
			    {
				
	            HCR_TRACE2("--- smrPhysAddr: 0x%08x, size:0x%08x", smrBank->iBase, smrBank->iSize);
	            NKern::ThreadEnterCS();
	    
	            TChunkCreateInfo info;
	            info.iType = TChunkCreateInfo::ESharedKernelSingle;
	            info.iMaxSize = smrBank->iSize;
	
	            // Enable to give supervisor read only access and maximum caching at both L1 and L2.
	            info.iMapAttr = EMapAttrSupRo|EMapAttrCachedMax;  
	
	            info.iOwnsMemory = EFalse; 
	            info.iDestroyedDfc = NULL;
	            TUint32 mapAttr;
	            TLinAddr chunkKernAddr;
	            TInt r = Kern::ChunkCreate(info, iChunk, chunkKernAddr, mapAttr);
	            if( r != KErrNone )
	                {
	                HCR_TRACE1("--- Kern::ChunkCreate failed: 0x%08x", r);
	                NKern::ThreadLeaveCS();
	                HCR_TRACE_RETURN(r);
	                }
	                
	            r = Kern::ChunkCommitPhysical(iChunk, BUFFER_OFFSET_ZERO, smrBank->iSize, smrBank->iBase);
	            if( r != KErrNone)
	                {
	                HCR_TRACE1("--- Kern::ChunkCommitPhysical failed: 0x%08x", r);
	                TInt r2 = Kern::ChunkClose(iChunk);
	                __NK_ASSERT_ALWAYS(r2!=0);
	                NKern::ThreadLeaveCS();
	                HCR_TRACE_RETURN(r);    
	                }   
	            NKern::ThreadLeaveCS();
	                
	            HCR_TRACE1("--- iChunkKernAddr: 0x%08x", chunkKernAddr);    
	            // It should contains SMR and HCR image headers and some settings
	            HCR_HEX_DUMP_ABS((TUint8 *)chunkKernAddr, 1024 );  
	            
	            // Skip the SMR header, so we return the address of the first byte in the Repository
	            aAddr = (TAny *) (chunkKernAddr + sizeof(SSmrRomHeader));  
	            
	            HCR_TRACE_RETURN(KErrNone);
	            }
	        }
         
        ++smrBank;    
        }
#endif // !__WINS__ && !__X86__
       	
    HCR_TRACE_RETURN(KErrNotSupported);
    }

 
// -- ENTRY POINTS ------------------------------------------------------------


GLDEF_C HCR::MVariant* CreateHCRVariant()
    {
    HCR_FUNC("CreateHCRVariant");

    return new HCRVariant;
    }
