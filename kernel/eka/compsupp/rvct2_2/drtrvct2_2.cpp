// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// This function is simple a way to get these EXPORT statements into
// the .in file. These symbols will therefore be referenced from
// the export table and so **forced** into the DLL 
// 
//

__asm void __rt_exporter_dummy(void)
{
	AREA |.directive|, READONLY, NOALLOC

	PRESERVE8

	DCB "#<SYMEDIT>#\n"
	
	DCB "EXPORT abs\n"              // abs.o       
	DCB "EXPORT llabs\n"		//  llabs.o   

	DCB "EXPORT _btod_etento\n"	// bigflt0.o  

	DCB "EXPORT _btod_d2e\n"	// btod.o     
	DCB "EXPORT _btod_ediv\n"
	DCB "EXPORT _btod_emul\n"
	DCB "EXPORT _d2e_norm_op1\n"
	DCB "EXPORT _d2e_denorm_low\n"
	DCB "EXPORT _btod_emuld\n"
	DCB "EXPORT _btod_edivd\n"
	DCB "EXPORT _e2e\n"
	DCB "EXPORT _e2d\n"
	DCB "EXPORT __btod_mult_common\n"
	DCB "EXPORT __btod_div_common\n"
	
	DCB "EXPORT div\n"		// div.o

	DCB "EXPORT __rt_divtest\n"	// divtest.o  
	DCB "EXPORT __rt_sdiv32by16\n"	// dspdiv32.o 
	DCB "EXPORT __rt_udiv32by16\n"	// dspdiv32u.o
	DCB "EXPORT __rt_sdiv64by32\n"	// dspdiv64.o
	
	DCB "EXPORT ldiv\n"		// ldiv.o     

	DCB "EXPORT __ARM_ll_add\n"		//  lladd.o   
	DCB "EXPORT __ARM_ll_addls\n"	//  lladdls.o 
	DCB "EXPORT __ARM_ll_addlu\n"	//  lladdlu.o 
	DCB "EXPORT __ARM_ll_addss\n"	//  lladdss.o 
	DCB "EXPORT __ARM_ll_adduu\n"	//  lladduu.o 
	DCB "EXPORT __ARM_ll_and\n"		//  lland.o   
	DCB "EXPORT __ARM_ll_cmpge\n"	//  llcmpge.o 
	DCB "EXPORT __ARM_ll_cmple\n"	//  llcmple.o 
	DCB "EXPORT __ARM_ll_cmpu\n"		//  llcmpu.o  
	DCB "EXPORT lldiv\n"		//  lldiv.o   
	DCB "EXPORT __ARM_ll_eor\n"		//  lleor.o   
	DCB "EXPORT __ARM_ll_from_l\n"	// llfroml.o  
	DCB "EXPORT __ARM_ll_from_u\n"	// llfromu.o  
	DCB "EXPORT _ll_mul\n"		//  llmul.o   
	DCB "EXPORT __ARM_ll_mulls\n"	//  llmulls.o 
	DCB "EXPORT __ARM_ll_mullu\n"	//  llmullu.o 
	DCB "EXPORT __ARM_ll_mulss\n"	//  llmulss.o 

	DCB "EXPORT __ARM_ll_mlauu\n"	//  llmulss.o 
	DCB "EXPORT __ARM_ll_mlass\n"	//  llmulss.o 

	DCB "EXPORT __ARM_ll_muluu\n"	//  llmuluu.o 
	DCB "EXPORT __ARM_ll_neg\n"		//  llneg.o   
	DCB "EXPORT __ARM_ll_not\n"		//  llnot.o   
	DCB "EXPORT __ARM_ll_or\n"		//  llor.o    
	DCB "EXPORT __ARM_ll_rsb\n"		//  llrsb.o   
	DCB "EXPORT __ARM_ll_rsbls\n"	//  llrsbls.o 
	DCB "EXPORT __ARM_ll_rsblu\n"	//  llrsblu.o 
	DCB "EXPORT __ARM_ll_rsbss\n"	//  llrsbss.o 
	DCB "EXPORT __ARM_ll_rsbuu\n"	//  llrsbuu.o 

	DCB "EXPORT _ll_srdv \n"	//  llsrdv.o  

	DCB "EXPORT __ARM_ll_sub\n"		//  llsub.o   
	DCB "EXPORT __ARM_ll_subls\n"	//  llsubls.o 
	DCB "EXPORT __ARM_ll_sublu\n"	//  llsublu.o 
	DCB "EXPORT __ARM_ll_subss\n"	//  llsubss.o 
	DCB "EXPORT __ARM_ll_subuu\n"	//  llsubuu.o 
	DCB "EXPORT __ARM_ll_to_l\n"		//  lltol.o   
	DCB "EXPORT _ll_udiv_small\n"	// lludiv.o   
	DCB "EXPORT _ll_udiv_big\n"	// lludiv.o   
	DCB "EXPORT _ll_udiv_ginormous\n"	// lludiv.o

	DCB "EXPORT _ll_udiv10\n"	// lludiv10.o 
	DCB "EXPORT _ll_urdv\n"		//  llurdv.o  

	DCB "EXPORT _memset\n"		// rt_memclr.o 

	DCB "EXPORT _memset_w\n"	// rt_memclr_w.o 

	DCB "EXPORT __rt_memcpy\n"	// rt_memcpy.o 
	DCB "EXPORT _memcpy_lastbytes\n"// rt_memcpy_w.o

	DCB "EXPORT __memmove_aligned\n"// rt_memmove_w.o

	DCB "EXPORT __rt_memset\n"	// rt_memset.o

	DCB "EXPORT __rt_sdiv\n"	// rt_sdiv.o 
	DCB "EXPORT __rt_udiv\n"	// rt_udiv.o 
	DCB "EXPORT __rt_sdiv10\n"	// rtsdiv10.o
	DCB "EXPORT __rt_udiv10\n"	// rtudiv10.o
	
	// New with RVCT 2.0 Release Candidte 2 
	DCB "EXPORT __ARM_switch8\n"	// switch8.o	

	// rtti support from rtti.o
	// ** delete DCB "EXPORT __dynamic_cast\n"		
	// ** delete DCB "EXPORT __get_typeid\n"		

}



