// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This function is simple a way to for these EXPORT statements into
// the .in file. These symbols will therefore be referenced from
// the export table and so **forced** into the udrt.dll 
// 
//

__asm void __rt_exporter_dummy(void)
{
	AREA |.directive|, READONLY, NOALLOC

	PRESERVE8

	DCB "#<SYMEDIT>#\n"
	// From %ARMLIB%\armlib\c_a__un.l
	DCB "EXPORT _ll_cmpeq\n"	// _ll_cmpeq.o 
	DCB "EXPORT _ll_cmpne\n"	// _ll_cmpne.o 
	DCB "EXPORT _ll_scmpge\n"	// _ll_scmpge.o
	DCB "EXPORT _ll_scmpgt\n"       // _ll_scmpgt.o
	DCB "EXPORT _ll_scmple\n"       // _ll_scmple.o
	DCB "EXPORT _ll_scmplt\n"       // _ll_scmplt.o
	DCB "EXPORT _ll_ucmpge\n"       // _ll_ucmpge.o
	DCB "EXPORT _ll_ucmpgt\n"       // _ll_ucmpgt.o
	DCB "EXPORT _ll_ucmple\n"       // _ll_ucmple.o
	DCB "EXPORT _ll_ucmplt\n"       // _ll_ucmplt.o
	DCB "EXPORT _memcpy\n"		// _memcpy.o   
	DCB "EXPORT abs\n"              // abs.o       

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

	DCB "EXPORT llabs\n"		//  llabs.o   
	DCB "EXPORT _ll_add\n"		//  lladd.o   
	DCB "EXPORT _ll_addls\n"	//  lladdls.o 
	DCB "EXPORT _ll_addlu\n"	//  lladdlu.o 
	DCB "EXPORT _ll_addss\n"	//  lladdss.o 
	DCB "EXPORT _ll_adduu\n"	//  lladduu.o 
	DCB "EXPORT _ll_and\n"		//  lland.o   
	DCB "EXPORT _ll_cmpge\n"	//  llcmpge.o 
	DCB "EXPORT _ll_cmple\n"	//  llcmple.o 
	DCB "EXPORT _ll_cmpu\n"		//  llcmpu.o  
	DCB "EXPORT lldiv\n"		//  lldiv.o   
	DCB "EXPORT _ll_eor\n"		//  lleor.o   
	DCB "EXPORT _ll_from_l\n"	// llfroml.o  
	DCB "EXPORT _ll_from_u\n"	// llfromu.o  
	DCB "EXPORT _ll_mul\n"		//  llmul.o   
	DCB "EXPORT _ll_mulls\n"	//  llmulls.o 
	DCB "EXPORT _ll_mullu\n"	//  llmullu.o 
	DCB "EXPORT _ll_mulss\n"	//  llmulss.o 
	DCB "EXPORT _ll_muluu\n"	//  llmuluu.o 
	DCB "EXPORT _ll_neg\n"		//  llneg.o   
	DCB "EXPORT _ll_not\n"		//  llnot.o   
	DCB "EXPORT _ll_or\n"		//  llor.o    
	DCB "EXPORT _ll_rsb\n"		//  llrsb.o   
	DCB "EXPORT _ll_rsbls\n"	//  llrsbls.o 
	DCB "EXPORT _ll_rsblu\n"	//  llrsblu.o 
	DCB "EXPORT _ll_rsbss\n"	//  llrsbss.o 
	DCB "EXPORT _ll_rsbuu\n"	//  llrsbuu.o 
	DCB "EXPORT _ll_sdiv\n"		//  llsdiv.o  
	DCB "EXPORT _ll_sdiv10\n"	// llsdiv10.o 
	DCB "EXPORT _ll_shift_l\n"	// llshl.o    
	DCB "EXPORT _ll_srdv \n"	//  llsrdv.o  
	DCB "EXPORT _ll_sshift_r\n"	// llsshr.o   
	DCB "EXPORT _ll_sub\n"		//  llsub.o   
	DCB "EXPORT _ll_subls\n"	//  llsubls.o 
	DCB "EXPORT _ll_sublu\n"	//  llsublu.o 
	DCB "EXPORT _ll_subss\n"	//  llsubss.o 
	DCB "EXPORT _ll_subuu\n"	//  llsubuu.o 
	DCB "EXPORT _ll_to_l\n"		//  lltol.o   
	DCB "EXPORT _ll_udiv_small\n"	// lludiv.o   
	DCB "EXPORT _ll_udiv_big\n"	// lludiv.o   
	DCB "EXPORT _ll_udiv_ginormous\n"	// lludiv.o
	DCB "EXPORT _ll_div0\n"		//  lludiv.o   
	DCB "EXPORT _ll_udiv\n"		//  lludiv.o   
	DCB "EXPORT _ll_udiv10\n"	// lludiv10.o 
	DCB "EXPORT _ll_urdv\n"		//  llurdv.o  
	DCB "EXPORT _ll_ushift_r\n"	// llushr.o   

	DCB "EXPORT _memset\n"		// rt_memclr.o 
	DCB "EXPORT __rt_memclr\n"	// rt_memclr.o 
	DCB "EXPORT _memset_w\n"	// rt_memclr_w.o 
	DCB "EXPORT __rt_memclr_w\n"	// rt_memclr_w.o 
	DCB "EXPORT __rt_memcpy\n"	// rt_memcpy.o 
	DCB "EXPORT _memcpy_lastbytes\n"// rt_memcpy_w.o
	DCB "EXPORT __rt_memcpy_w\n"	// rt_memcpy_w.o
	DCB "EXPORT __rt_memmove\n"	// rt_memmove.o 
	DCB "EXPORT __memmove_aligned\n"// rt_memmove_w.o
	DCB "EXPORT __rt_memmove_w\n"	// rt_memmove_w.o
	DCB "EXPORT __rt_memset\n"	// rt_memset.o

	// New with RVCT 2.0 Release Candidte 2 
	DCB "EXPORT __rt_switch8\n"	// switch8.o	

	DCB "EXPORT __rt_sdiv\n"	// rt_sdiv.o 
	DCB "EXPORT __rt_udiv\n"	// rt_udiv.o 
	DCB "EXPORT __rt_sdiv10\n"	// rtsdiv10.o
	DCB "EXPORT __rt_udiv10\n"	// rtudiv10.o
	

        // export the array and vector constructors
	// From %ARMLIB%\cpplib\cpprt_a__un.l 
	DCB "EXPORT __cxa_vec_new\n"		// vec_newdel.o
	DCB "EXPORT __cxa_vec_new2\n"		// vec_newdel.o
	DCB "EXPORT __cxa_vec_new3\n"		// vec_newdel.o
	DCB "EXPORT __cxa_vec_ctor\n"		// vec_newdel.o
	DCB "EXPORT __cxa_vec_cctor\n"		// vec_newdel.o
	DCB "EXPORT __cxa_vec_dtor\n"		// vec_newdel.o
	DCB "EXPORT __cxa_vec_delete\n"		// vec_newdel.o
	DCB "EXPORT __cxa_vec_delete2\n"	// vec_newdel.o
	DCB "EXPORT __cxa_vec_delete3\n"	// vec_newdel.o
	DCB "EXPORT __cxa_pure_virtual\n"	// pure_virt.o
	DCB "EXPORT __cxa_guard_acquire\n"	// cxa_guard_acquire.o
	DCB "EXPORT __cxa_guard_release\n"	// cxa_guard_release.o
	DCB "EXPORT __cxa_guard_abort\n"	// cxa_guard_abort.o
	  // not present in beta b
	  //	DCB "EXPORT __memzero\n"		// memzero.o

	DCB "EXPORT _ZTVN10__cxxabiv117__class_type_infoE\n"		
	DCB "EXPORT __dynamic_cast\n"		
	DCB "EXPORT __get_typeid\n"		
	DCB "EXPORT _ZTVN10__cxxabiv120__si_class_type_infoE\n"		
	DCB "EXPORT _ZTVN10__cxxabiv121__vmi_class_type_infoE\n"	
	// From %ARMLIB%\cpplib\cpp_a__u.l
	// none from here yet
	// From %ARMLIB%\cpplib\cppfp_a__un.l
	// none from here yet.
}
