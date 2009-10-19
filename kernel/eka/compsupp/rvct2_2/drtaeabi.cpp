// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

extern char * _ZTVN10__cxxabiv123__fundamental_type_infoE;
extern const char * const  $Sub$$_ZTSv = "v";


// This function is simple a way to get these EXPORT statements into
// the .in file. These symbols will therefore be referenced from
// the export table and so **forced** into the DLL 
__asm void __rt_exporter_dummy(void)
{
        EXTERN _ZTVN10__cxxabiv123__fundamental_type_infoE
	EXTERN _ZTSv
	EXPORT |$Sub$$_ZTIv|
|$Sub$$_ZTIv|
	DCD _ZTVN10__cxxabiv123__fundamental_type_infoE
	DCD _ZTSv
  
	AREA |.directive|, READONLY, NOALLOC

	PRESERVE8

	DCB "#<SYMEDIT>#\n"
	// Long long functions
	DCB "EXPORT __aeabi_lmul\n"
	DCB "EXPORT __aeabi_ldivmod\n"
	DCB "EXPORT __aeabi_uldivmod\n"
	DCB "EXPORT __aeabi_llsl\n"
	DCB "EXPORT __aeabi_llsr\n"
	DCB "EXPORT __aeabi_lasr\n"
	DCB "EXPORT __aeabi_lcmp\n"
	DCB "EXPORT __aeabi_ulcmp\n"

	// Integer division functions
	DCB "EXPORT __aeabi_idiv\n"
	DCB "EXPORT __aeabi_uidiv\n"

	// Integer (32/32 . 32) division functions
	DCB "EXPORT __aeabi_idivmod\n"
	DCB "EXPORT __aeabi_uidivmod\n"

	// Division by Zero
	// SIGFPE not supported on SymbianOS
	// Implemented in rtdiv0.cpp
	DCB "EXPORT __aeabi_idiv0\n"
	DCB "EXPORT __aeabi_ldiv0\n"

	// Unaligned memory access
	DCB "EXPORT __aeabi_uread4\n"
	DCB "EXPORT __aeabi_uwrite4\n"
	DCB "EXPORT __aeabi_uread8\n"
	DCB "EXPORT __aeabi_uwrite8\n"

	// Memory copying, clearing, and setting
	// we may want to override the toolchain supplied versions of these (or not)
	DCB "EXPORT __aeabi_memcpy8\n"
	DCB "EXPORT __aeabi_memcpy4\n"
	DCB "EXPORT __aeabi_memcpy\n"
	DCB "EXPORT __aeabi_memmove8\n"
	DCB "EXPORT __aeabi_memmove4\n"
	DCB "EXPORT __aeabi_memmove\n"

	DCB "EXPORT __aeabi_memset8\n"
	DCB "EXPORT __aeabi_memset4\n"
	DCB "EXPORT __aeabi_memset\n"
	DCB "EXPORT __aeabi_memclr8\n"
	DCB "EXPORT __aeabi_memclr4\n"
	DCB "EXPORT __aeabi_memclr\n"

	// C++ helper functions

	// Pure virtual call
	DCB "EXPORT __cxa_pure_virtual\n"

	// One-time construction API
	DCB "EXPORT __cxa_guard_acquire\n"
	DCB "EXPORT __cxa_guard_release\n"
	DCB "EXPORT __cxa_guard_abort\n"

	// Construction and destruction of arrays

	// Helper functions defined by the generic C++ ABI
	DCB "EXPORT __cxa_vec_new\n"
	DCB "EXPORT __cxa_vec_new2\n"
	DCB "EXPORT __cxa_vec_new3\n"
	DCB "EXPORT __cxa_vec_ctor\n"
	DCB "EXPORT __cxa_vec_dtor\n"
	DCB "EXPORT __cxa_vec_cleanup\n"
	DCB "EXPORT __cxa_vec_delete\n"
	DCB "EXPORT __cxa_vec_delete2\n"
	DCB "EXPORT __cxa_vec_delete3\n"
	DCB "EXPORT __cxa_vec_cctor\n"

	// Helper functions defined by the C++ ABI for the ARM Architecture
	DCB "EXPORT __aeabi_vec_ctor_nocookie_nodtor\n"
	DCB "EXPORT __aeabi_vec_ctor_cookie_nodtor\n"
	DCB "EXPORT __aeabi_vec_cctor_nocookie_nodtor\n"
	DCB "EXPORT __aeabi_vec_new_cookie_noctor\n"
	DCB "EXPORT __aeabi_vec_new_nocookie\n"
	DCB "EXPORT __aeabi_vec_new_cookie_nodtor\n"
	DCB "EXPORT __aeabi_vec_new_cookie\n"
	DCB "EXPORT __aeabi_vec_dtor\n"
	DCB "EXPORT __aeabi_vec_dtor_cookie\n"
	DCB "EXPORT __aeabi_vec_delete\n"
	DCB "EXPORT __aeabi_vec_delete3\n"
	DCB "EXPORT __aeabi_vec_delete3_nodtor\n"

	// Object finalization
	// these need SymbianOS specific implementations
	// implemented in ucppfini.cpp
	// and supplied by usrt.lib
#if 0
	DCB "EXPORT __cxa_atexit\n"
	DCB "EXPORT __aeabi_atexit\n"
	DCB "EXPORT __cxa_finalize\n"
#endif

/// Exception Support from here. Not needed by system side code.
#ifndef __KERNEL_MODE__
	// Standard (user-visible) C++  exception related functions
	DCB "EXPORT _ZSt9terminatev\n"
	DCB "EXPORT _ZSt10unexpectedv\n"
	DCB "EXPORT _ZSt18uncaught_exceptionv\n"
	DCB "EXPORT _ZSt13set_terminatePFvvE\n"
	DCB "EXPORT _ZSt14set_unexpectedPFvvE\n"
	
	DCB "EXPORT abort\n"

	// Exception-handling support
	DCB "EXPORT __cxa_allocate_exception\n"
	DCB "EXPORT __cxa_free_exception\n"
	DCB "EXPORT __cxa_throw\n"
	DCB "EXPORT __cxa_rethrow\n"
	DCB "EXPORT __cxa_begin_catch\n"
	DCB "EXPORT __cxa_end_catch\n"
	DCB "EXPORT __cxa_end_cleanup\n"

	// personality routines for ARM compact EH entries
	DCB "EXPORT __aeabi_unwind_cpp_pr0\n"
	DCB "EXPORT __aeabi_unwind_cpp_pr1\n"
	DCB "EXPORT __aeabi_unwind_cpp_pr2\n"

	// Unwinder helper routines
	DCB "EXPORT _Unwind_VRS_Get\n"
	DCB "EXPORT _Unwind_VRS_Set\n"
	DCB "EXPORT _Unwind_VRS_Pop\n"

	// personality routine helper functions
	DCB "EXPORT __cxa_begin_cleanup\n"

        // this needs SymbianOS specific implementation
	DCB "EXPORT __cxa_type_match\n"

        // this needs SymbianOS specific implementation
	DCB "EXPORT __cxa_call_terminate\n"

	DCB "EXPORT __cxa_call_unexpected\n"

	// Miscellaneous error handling related to exception processing
	DCB "EXPORT __cxa_bad_cast\n"
	DCB "EXPORT __cxa_bad_typeid\n"


        // this needs SymbianOS specific implementation
	DCB "EXPORT __cxa_get_globals\n"

        // this needs SymbianOS specific implementation
	DCB "EXPORT __cxa_current_exception_type\n"


	// rtti support from rtti.o
	DCB "EXPORT __dynamic_cast\n"		
	DCB "EXPORT __get_typeid\n"		
	DCB "EXPORT __ARM_get_typeid\n"

	// Standard typeinfo
	DCB "EXPORT _ZNSt9type_infoD1Ev\n"
	DCB "EXPORT _ZNSt9type_infoD2Ev\n"
	DCB "EXPORT _ZNSt9type_infoD0Ev\n"  
	DCB "EXPORT _ZNKSt9type_info4nameEv\n" 
	DCB "EXPORT _ZNKSt9type_infoeqERKS_\n"
	DCB "EXPORT _ZNKSt9type_infoneERKS_\n"
	DCB "EXPORT _ZNKSt9type_info6beforeERKS_\n"
	DCB "EXPORT _ZNSt8bad_castC1Ev\n"
	DCB "EXPORT _ZNSt8bad_castC2Ev\n"
	DCB "EXPORT _ZNSt8bad_castC1ERKS_\n"
	DCB "EXPORT _ZNSt8bad_castC2ERKS_\n"
	DCB "EXPORT _ZNSt8bad_castaSERKS_\n"
	DCB "EXPORT _ZNSt8bad_castD1Ev\n"
	DCB "EXPORT _ZNSt8bad_castD2Ev\n"
	DCB "EXPORT _ZNSt8bad_castD0Ev\n"
	DCB "EXPORT _ZNKSt8bad_cast4whatEv\n"
	DCB "EXPORT _ZNSt10bad_typeidC1Ev\n"
	DCB "EXPORT _ZNSt10bad_typeidC2Ev\n"
	DCB "EXPORT _ZNSt10bad_typeidC1ERKS_\n"
	DCB "EXPORT _ZNSt10bad_typeidC2ERKS_\n"
	DCB "EXPORT _ZNSt10bad_typeidaSERKS_\n"
	DCB "EXPORT _ZNSt10bad_typeidD1Ev\n"
	DCB "EXPORT _ZNSt10bad_typeidD2Ev\n"
	DCB "EXPORT _ZNSt10bad_typeidD0Ev\n"
	DCB "EXPORT _ZNKSt10bad_typeid4whatEv\n"
	DCB "EXPORT _ZN10__cxxabiv123__fundamental_type_infoD1Ev\n"
	DCB "EXPORT _ZN10__cxxabiv123__fundamental_type_infoD2Ev\n"
	DCB "EXPORT _ZN10__cxxabiv123__fundamental_type_infoD0Ev\n"
	DCB "EXPORT _ZN10__cxxabiv117__array_type_infoD1Ev\n"
	DCB "EXPORT _ZN10__cxxabiv117__array_type_infoD2Ev\n"
	DCB "EXPORT _ZN10__cxxabiv117__array_type_infoD0Ev\n"
	DCB "EXPORT _ZN10__cxxabiv120__function_type_infoD1Ev\n"
	DCB "EXPORT _ZN10__cxxabiv120__function_type_infoD2Ev\n"
	DCB "EXPORT _ZN10__cxxabiv120__function_type_infoD0Ev\n"
	DCB "EXPORT _ZN10__cxxabiv116__enum_type_infoD1Ev\n"
	DCB "EXPORT _ZN10__cxxabiv116__enum_type_infoD2Ev\n"
	DCB "EXPORT _ZN10__cxxabiv116__enum_type_infoD0Ev\n"
	DCB "EXPORT _ZN10__cxxabiv117__class_type_infoD1Ev\n"
	DCB "EXPORT _ZN10__cxxabiv117__class_type_infoD2Ev\n"
	DCB "EXPORT _ZN10__cxxabiv117__class_type_infoD0Ev\n"
	DCB "EXPORT _ZN10__cxxabiv120__si_class_type_infoD1Ev\n"
	DCB "EXPORT _ZN10__cxxabiv120__si_class_type_infoD2Ev\n"
	DCB "EXPORT _ZN10__cxxabiv120__si_class_type_infoD0Ev\n"
	DCB "EXPORT _ZN10__cxxabiv121__vmi_class_type_infoD1Ev\n"
	DCB "EXPORT _ZN10__cxxabiv121__vmi_class_type_infoD2Ev\n"
	DCB "EXPORT _ZN10__cxxabiv121__vmi_class_type_infoD0Ev\n"
	DCB "EXPORT _ZN10__cxxabiv117__pbase_type_infoD1Ev\n"
	DCB "EXPORT _ZN10__cxxabiv117__pbase_type_infoD2Ev\n"
	DCB "EXPORT _ZN10__cxxabiv117__pbase_type_infoD0Ev\n"
	DCB "EXPORT _ZN10__cxxabiv119__pointer_type_infoD1Ev\n"
	DCB "EXPORT _ZN10__cxxabiv119__pointer_type_infoD2Ev\n"
	DCB "EXPORT _ZN10__cxxabiv119__pointer_type_infoD0Ev\n"
	DCB "EXPORT _ZN10__cxxabiv129__pointer_to_member_type_infoD1Ev\n"
	DCB "EXPORT _ZN10__cxxabiv129__pointer_to_member_type_infoD2Ev\n"
	DCB "EXPORT _ZN10__cxxabiv129__pointer_to_member_type_infoD0Ev\n"
	DCB "EXPORT _ZSt21__gen_dummy_typeinfosv\n"
	DCB "EXPORT _ZTVSt9type_info\n"
	DCB "EXPORT _ZTVSt8bad_cast\n"
	DCB "EXPORT _ZTVSt10bad_typeid\n"
	DCB "EXPORT _ZTIv\n"
	DCB "EXPORT _ZTVN10__cxxabiv123__fundamental_type_infoE\n"
	DCB "EXPORT _ZTIPv\n"
	DCB "EXPORT _ZTVN10__cxxabiv119__pointer_type_infoE\n"
	DCB "EXPORT _ZTIPKv\n"
	DCB "EXPORT _ZTIb\n"
	DCB "EXPORT _ZTIPb\n"
	DCB "EXPORT _ZTIPKb\n"
	DCB "EXPORT _ZTIw\n"
	DCB "EXPORT _ZTIPw\n"
	DCB "EXPORT _ZTIPKw\n"
	DCB "EXPORT _ZTIc\n"
	DCB "EXPORT _ZTIPc\n"
	DCB "EXPORT _ZTIPKc\n"
	DCB "EXPORT _ZTIa\n"
	DCB "EXPORT _ZTIPa\n"
	DCB "EXPORT _ZTIPKa\n"
	DCB "EXPORT _ZTIh\n"
	DCB "EXPORT _ZTIPh\n"
	DCB "EXPORT _ZTIPKh\n"
	DCB "EXPORT _ZTIs\n"
	DCB "EXPORT _ZTIPs\n"
	DCB "EXPORT _ZTIPKs\n"
	DCB "EXPORT _ZTIt\n"
	DCB "EXPORT _ZTIPt\n"
	DCB "EXPORT _ZTIPKt\n"
	DCB "EXPORT _ZTIi\n"
	DCB "EXPORT _ZTIPi\n"
	DCB "EXPORT _ZTIPKi\n"
	DCB "EXPORT _ZTIj\n"
	DCB "EXPORT _ZTIPj\n"
	DCB "EXPORT _ZTIPKj\n"
	DCB "EXPORT _ZTIl\n"
	DCB "EXPORT _ZTIPl\n"
	DCB "EXPORT _ZTIPKl\n"
	DCB "EXPORT _ZTIm\n"
	DCB "EXPORT _ZTIPm\n"
	DCB "EXPORT _ZTIPKm\n"
	DCB "EXPORT _ZTIx\n"
	DCB "EXPORT _ZTIPx\n"
	DCB "EXPORT _ZTIPKx\n"
	DCB "EXPORT _ZTIy\n"
	DCB "EXPORT _ZTIPy\n"
	DCB "EXPORT _ZTIPKy\n"
	DCB "EXPORT _ZTIf\n"
	DCB "EXPORT _ZTIPf\n"
	DCB "EXPORT _ZTIPKf\n"
	DCB "EXPORT _ZTId\n"
	DCB "EXPORT _ZTIPd\n"
	DCB "EXPORT _ZTIPKd\n"
	DCB "EXPORT _ZTIe\n"
	DCB "EXPORT _ZTIPe\n"
	DCB "EXPORT _ZTIPKe\n"
	DCB "EXPORT _ZTVN10__cxxabiv117__array_type_infoE\n"
	DCB "EXPORT _ZTVN10__cxxabiv120__function_type_infoE\n"
	DCB "EXPORT _ZTVN10__cxxabiv116__enum_type_infoE\n"
	DCB "EXPORT _ZTVN10__cxxabiv117__class_type_infoE\n"
	DCB "EXPORT _ZTVN10__cxxabiv120__si_class_type_infoE\n"
	DCB "EXPORT _ZTVN10__cxxabiv121__vmi_class_type_infoE\n"
	DCB "EXPORT _ZTVN10__cxxabiv117__pbase_type_infoE\n"
	DCB "EXPORT _ZTVN10__cxxabiv129__pointer_to_member_type_infoE\n"
	DCB "EXPORT _ZTSv\n"
	DCB "EXPORT _ZTSPv\n"
	DCB "EXPORT _ZTSPKv\n"
	DCB "EXPORT _ZTSb\n"
	DCB "EXPORT _ZTSPb\n"
	DCB "EXPORT _ZTSPKb\n"
	DCB "EXPORT _ZTSw\n"
	DCB "EXPORT _ZTSPw\n"
	DCB "EXPORT _ZTSPKw\n"
	DCB "EXPORT _ZTSc\n"
	DCB "EXPORT _ZTSPc\n"
	DCB "EXPORT _ZTSPKc\n"
	DCB "EXPORT _ZTSa\n"
	DCB "EXPORT _ZTSPa\n"
	DCB "EXPORT _ZTSPKa\n"
	DCB "EXPORT _ZTSh\n"
	DCB "EXPORT _ZTSPh\n"
	DCB "EXPORT _ZTSPKh\n"
	DCB "EXPORT _ZTSs\n"
	DCB "EXPORT _ZTSPs\n"
	DCB "EXPORT _ZTSPKs\n"
	DCB "EXPORT _ZTSt\n"
	DCB "EXPORT _ZTSPt\n"
	DCB "EXPORT _ZTSPKt\n"
	DCB "EXPORT _ZTSi\n"
	DCB "EXPORT _ZTSPi\n"
	DCB "EXPORT _ZTSPKi\n"
	DCB "EXPORT _ZTSj\n"
	DCB "EXPORT _ZTSPj\n"
	DCB "EXPORT _ZTSPKj\n"
	DCB "EXPORT _ZTSl\n"
	DCB "EXPORT _ZTSPl\n"
	DCB "EXPORT _ZTSPKl\n"
	DCB "EXPORT _ZTSm\n"
	DCB "EXPORT _ZTSPm\n"
	DCB "EXPORT _ZTSPKm\n"
	DCB "EXPORT _ZTSx\n"
	DCB "EXPORT _ZTSPx\n"
	DCB "EXPORT _ZTSPKx\n"
	DCB "EXPORT _ZTSy\n"
	DCB "EXPORT _ZTSPy\n"
	DCB "EXPORT _ZTSPKy\n"
	DCB "EXPORT _ZTSf\n"
	DCB "EXPORT _ZTSPf\n"
	DCB "EXPORT _ZTSPKf\n"
	DCB "EXPORT _ZTSd\n"
	DCB "EXPORT _ZTSPd\n"
	DCB "EXPORT _ZTSPKd\n"
	DCB "EXPORT _ZTSe\n"
	DCB "EXPORT _ZTSPe\n"
	DCB "EXPORT _ZTSPKe\n"
	DCB "EXPORT _ZTISt9type_info\n"
	DCB "EXPORT _ZTISt8bad_cast\n"
	DCB "EXPORT _ZTISt10bad_typeid\n"
	DCB "EXPORT _ZTIN10__cxxabiv123__fundamental_type_infoE\n"
	DCB "EXPORT _ZTIN10__cxxabiv117__array_type_infoE\n"
	DCB "EXPORT _ZTIN10__cxxabiv120__function_type_infoE\n"
	DCB "EXPORT _ZTIN10__cxxabiv116__enum_type_infoE\n"
	DCB "EXPORT _ZTIN10__cxxabiv117__class_type_infoE\n"
	DCB "EXPORT _ZTIN10__cxxabiv120__si_class_type_infoE\n"
	DCB "EXPORT _ZTIN10__cxxabiv121__vmi_class_type_infoE\n"
	DCB "EXPORT _ZTIN10__cxxabiv117__pbase_type_infoE\n"
	DCB "EXPORT _ZTIN10__cxxabiv119__pointer_type_infoE\n"
	DCB "EXPORT _ZTIN10__cxxabiv129__pointer_to_member_type_infoE\n"
	DCB "EXPORT _ZTSSt9type_info\n"
	DCB "EXPORT _ZTSSt8bad_cast\n"
	DCB "EXPORT _ZTSSt10bad_typeid\n"
	DCB "EXPORT _ZTSN10__cxxabiv123__fundamental_type_infoE\n"
	DCB "EXPORT _ZTSN10__cxxabiv117__array_type_infoE\n"
	DCB "EXPORT _ZTSN10__cxxabiv120__function_type_infoE\n"
	DCB "EXPORT _ZTSN10__cxxabiv116__enum_type_infoE\n"
	DCB "EXPORT _ZTSN10__cxxabiv117__class_type_infoE\n"
	DCB "EXPORT _ZTSN10__cxxabiv120__si_class_type_infoE\n"
	DCB "EXPORT _ZTSN10__cxxabiv121__vmi_class_type_infoE\n"
	DCB "EXPORT _ZTSN10__cxxabiv117__pbase_type_infoE\n"
	DCB "EXPORT _ZTSN10__cxxabiv119__pointer_type_infoE\n"
	DCB "EXPORT _ZTSN10__cxxabiv129__pointer_to_member_type_infoE\n"

#endif // __KERNEL_MODE__

}

#ifndef __KERNEL_MODE__

extern "C" {
IMPORT_C int __rt_raise(int signal, int type);
int raise(int signal)
	{ 
	return __rt_raise(signal, 0); 
	}

IMPORT_C int __rt_exit(int aReturnCode);
EXPORT_C void abort(int signal)
	{ 
	__rt_raise(signal, 1); 
	__rt_exit(1);
	}

IMPORT_C void* __get_typeid(void*);
EXPORT_C void* __ARM_get_typeid(void* p)
	{
	return __get_typeid(p);
	}

}

#endif
