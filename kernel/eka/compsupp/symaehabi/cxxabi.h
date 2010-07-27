/* Copyright (c) Edison Design Group, 2002-2004. */
/*
cxxabi.h -- Include file for IA-64 ABI entry points.
*/

#ifndef __CXXABI_H
#define __CXXABI_H

#ifndef __STDDEF_H
#include <stddef.h>
#endif  /* ifndef __STDDEF_H */

namespace std
{
	#ifdef __EDG__
	#pragma define_type_info
	#endif

	class type_info
		{
	public:
		IMPORT_C virtual ~type_info();
		IMPORT_C bool operator==(const type_info&) const;
		IMPORT_C bool operator!=(const type_info&) const;
		IMPORT_C bool before(const type_info&) const;
		IMPORT_C const char* name() const;
	private:
		type_info(const type_info&);
		type_info& operator=(const type_info&);
	private:
		const char *__type_name;
		};

	class exception
		{
	public:
		IMPORT_C exception() throw ();
		IMPORT_C exception(const exception&) throw ();
		IMPORT_C exception& operator=(const exception&) throw ();
		IMPORT_C virtual ~exception() throw ();
		IMPORT_C virtual const char* what() const throw ();
		};

	class bad_exception : public exception
		{
	public:
		bad_exception() throw ();
		bad_exception(const bad_exception&) throw ();
		bad_exception& operator=(const bad_exception&) throw ();

		virtual ~bad_exception() throw ();

		virtual const char* what() const throw ();
		};

	class bad_cast : public exception
		{
	public:
		IMPORT_C bad_cast() throw ();
		IMPORT_C bad_cast(const bad_cast&) throw ();
		IMPORT_C bad_cast& operator=(const bad_cast&) throw ();
		IMPORT_C virtual ~bad_cast() throw ();
		IMPORT_C virtual const char* what() const throw ();
		};

	class bad_typeid : public exception
		{
	public:
		IMPORT_C bad_typeid() throw ();
		IMPORT_C bad_typeid(const bad_typeid&) throw ();
		IMPORT_C bad_typeid& operator=(const bad_typeid&) throw ();
		IMPORT_C virtual ~bad_typeid() throw ();
		IMPORT_C virtual const char* what() const throw ();
		};
}



#ifdef __EDG_RUNTIME_USES_NAMESPACES
namespace __cxxabiv1 {
  using namespace std;
#endif /* ifdef __EDG_RUNTIME_USES_NAMESPACES */

  /* type_info implementation classes */

#pragma define_type_info
  class __fundamental_type_info : public type_info {
  public:
    virtual ~__fundamental_type_info();
  };

#pragma define_type_info
  class __array_type_info : public type_info {
  public:
    virtual ~__array_type_info();
  };

#pragma define_type_info
  class __function_type_info : public type_info {
  public:
    virtual ~__function_type_info();
  };

#pragma define_type_info
  class __enum_type_info : public type_info {
  public:
    virtual ~__enum_type_info();
  };

#pragma define_type_info
  class __class_type_info : public type_info {
  public:
    virtual ~__class_type_info();
  };

#pragma define_type_info
  class __si_class_type_info : public __class_type_info {
  public:
    virtual ~__si_class_type_info();
    const __class_type_info *__base_type;
  };

  struct __base_class_type_info {
    const __class_type_info *__base_type;
    long __offset_flags;

    enum __offset_flags_masks {
      __virtual_mask = 0x1,
      __public_mask = 0x2,
      __offset_shift = 8
    };
  };

#pragma define_type_info
  class __vmi_class_type_info : public __class_type_info {
  public:
    virtual ~__vmi_class_type_info();
    unsigned int __flags;
    unsigned int __base_count;
    __base_class_type_info __base_info[1];

    enum __flags_masks {
      __non_diamond_repeat_mask = 0x1,
      __diamond_shaped_mask = 0x2
    };
  };

#pragma define_type_info
  class __pbase_type_info : public type_info {
  public:
    virtual ~__pbase_type_info();
    unsigned int __flags;
    const type_info *__pointee;
    
    enum __masks {
      __const_mask = 0x1,
      __volatile_mask = 0x2,
      __restrict_mask = 0x4,
      __incomplete_mask = 0x8,
      __incomplete_class_mask = 0x10
    };
  }; 

#pragma define_type_info
  class __pointer_type_info : public __pbase_type_info {
    virtual ~__pointer_type_info();
  };

#pragma define_type_info
  class __pointer_to_member_type_info : public __pbase_type_info {
    virtual ~__pointer_to_member_type_info();
    const __class_type_info *__context;
  };

  extern "C" {
    /* Pure virtual function calls. */
    void __cxa_pure_virtual();

    /* Constructors return void in the IA-64 ABI.  But in the ARM EABI
       variant, they return void*. */
#ifdef __EDG_IA64_ABI_VARIANT_CTORS_AND_DTORS_RETURN_THIS
    typedef void* __ctor_dtor_return_type;
#else /* ifndef __EDG_IA64_ABI_VARIANT_CTORS_AND_DTORS_RETURN_THIS */
    typedef void __ctor_dtor_return_type;
#endif /* ifdef __EDG_IA64_ABI_VARIANT_CTORS_AND_DTORS_RETURN_THIS */

    /* Guard variables are 64 bits in the IA-64 ABI but only 32 bits in
       the ARM EABI. */
#ifdef __EDG_IA64_ABI_USE_INT_STATIC_INIT_GUARD
    typedef int __guard_variable_type;
#else /* ifndef __EDG_IA64_ABI_USE_INT_STATIC_INIT_GUARD */
    typedef unsigned long long __guard_variable_type;
#endif /* ifdef __EDG_IA64_ABI_USE_INT_STATIC_INIT_GUARD */
  
    /* Guard variables for the initialization of variables with static storage
       duration. */
    int __cxa_guard_acquire(__guard_variable_type *);
    void __cxa_guard_release(__guard_variable_type *);
    void __cxa_guard_abort(__guard_variable_type *);

    /* Construction and destruction of arrays. */
    void *__cxa_vec_new(size_t, size_t, size_t,
                        __ctor_dtor_return_type (*)(void *),
                        __ctor_dtor_return_type (*)(void *));
    void *__cxa_vec_new2(size_t, size_t, size_t,
                         __ctor_dtor_return_type (*)(void *),
                         __ctor_dtor_return_type (*)(void *),
                         void *(*)(size_t),
                         void (*)(void *));
    void *__cxa_vec_new3(size_t, size_t, size_t,
                         __ctor_dtor_return_type (*)(void *),
                         __ctor_dtor_return_type (*)(void *),
                         void *(*)(size_t),
                         void (*)(void *, size_t));
#ifndef CXXABI_VEC_CTOR_RETURNS_VOID
    /* The C++ ABI says this returns 'void' but we actually return
       'void *' to remain compatible with RVCT 2.0 objects.  But the
       compiler no longer assumes it. */
    void *
#else /* def CXXABI_VEC_CTOR_RETURNS_VOID */
    void
#endif /* def CXXABI_VEC_CTOR_RETURNS_VOID */
         __cxa_vec_ctor(void *, size_t, size_t,
                        __ctor_dtor_return_type (*)(void *),
                        __ctor_dtor_return_type (*)(void *));
    void __cxa_vec_dtor(void *, size_t, size_t,
                        __ctor_dtor_return_type (*)(void *));
    void __cxa_vec_cleanup(void *, size_t, size_t,
                           __ctor_dtor_return_type (*)(void *));
    void __cxa_vec_delete(void *, size_t, size_t,
                          __ctor_dtor_return_type (*)(void *));
    void __cxa_vec_delete2(void *, size_t, size_t,
                           __ctor_dtor_return_type (*)(void *),
                           void (*)(void *));
    void __cxa_vec_delete3(void *, size_t, size_t,
                           __ctor_dtor_return_type (*)(void *),
                           void (*)(void *, size_t));
#ifndef CXXABI_VEC_CTOR_RETURNS_VOID
    /* The C++ ABI says this returns 'void' but we actually return
       'void *' to remain compatible with RVCT 2.0 objects.  But the
       compiler no longer assumes it. */
    void *
#else /* def CXXABI_VEC_CTOR_RETURNS_VOID */
    void
#endif /* def CXXABI_VEC_CTOR_RETURNS_VOID */
         __cxa_vec_cctor(void *, void *, size_t, size_t, 
                         __ctor_dtor_return_type (*)(void *, void *),
                         __ctor_dtor_return_type (*)(void *));

    /* Finalization. */
    int __cxa_atexit(void (*)(void *), void *, void *);
    void __cxa_finalize(void *);

    /* Exception-handling support. */
    void __cxa_bad_cast();
    void __cxa_bad_typeid();

    /* Demangling interface. */
    char *__cxa_demangle(const char* __mangled_name,
                         char        *__buf,
                         size_t      *__n,
                         int         *__status);

  }  /* extern "C" */
#ifdef __EDG_RUNTIME_USES_NAMESPACES
}  /* namespace __cxxabiv1 */

/* Create the "abi" namespace alias. */
namespace abi = __cxxabiv1;
#endif /* ifdef __EDG_RUNTIME_USES_NAMESPACES */

#endif /* ifndef __CXXABI_H */
