// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file implements the array helper functions defined in "C++ ABI for the ARM
// Architecture."
// 
//

#ifdef __KERNEL_MODE__
    #define RT_NO_EXCEPTIONS 1
#endif

// TODO:
//
// 1. If you prefer, you could use __KERNEL_MODE__ directly instead of
//    RT_NO_EXCEPTIONS.
// 2. If this code is only used kernel-side, you might delete all exception handling
//    code (thereby getting rid of any macro).

namespace std
{
    typedef unsigned size_t;

#if !RT_NO_EXCEPTIONS
    IMPORT_C void terminate();
#endif
}

using std::size_t;


namespace
{
    struct Cookie
    {
        size_t elem_size;
        size_t elem_count;
    };

    typedef void* (*Ctor)(void*);
    typedef void* (*Dtor)(void*);

    inline Cookie* _array_to_cookie(void* p)
    {
        return ( reinterpret_cast<Cookie*>(p) - 1 );
    }

    inline void* _cookie_to_array(Cookie* p)
    {
        return (p + 1);
    }

    inline void _init_cookie(void* p, size_t elem_size, size_t elem_count)
    {
        Cookie* cp = _array_to_cookie(p);

        cp->elem_size  = elem_size;
        cp->elem_count = elem_count;
    }

    inline void _maybe_init_cookie(void* p, size_t cookie_size, size_t elem_size, size_t elem_count)
    {
        if (cookie_size > 0)
        {
            _init_cookie(p, elem_size, elem_count);
        }
    }
}


namespace __cxxabiv1
{
    extern "C" EXPORT_C void __cxa_vec_cleanup(void *array, size_t elem_count, size_t elem_size, Dtor dtor)
    {
        if (dtor)
        {
            // The elemens of an array should be destroyed in reverse order.

            // Pointer to the last element.
            char* p = static_cast<char*>(array) + (elem_count - 1) * elem_size;

            #if RT_NO_EXCEPTIONS
            for (size_t i = elem_count; i > 0; i--, p -= elem_size)
            {
                (*dtor)(p);
            }
            #else
            try
            {
                for (size_t i = elem_count; i > 0; i--, p -= elem_size)
                {
                    (*dtor)(p);
                }
            }
            catch (...)
            {
                std::terminate();
            }
            #endif
        }
    }

    extern "C" EXPORT_C void __cxa_vec_dtor(void* array, size_t elem_count, size_t elem_size, Dtor dtor)
    {
        #if RT_NO_EXCEPTIONS
        __cxa_vec_cleanup(array, elem_count, elem_size, dtor);
        #else
        if (dtor)
        {
            char* p = static_cast<char*>(array) + (elem_count - 1) * elem_size;
            
            size_t i = elem_count;

            try
            {
                for ( ; i > 0; i--, p -= elem_size)
                {
                    (*dtor)(p);
                }
            }
            catch (...)
            {
                // An exeception was throw when destroying element i; ignore this
                // element and try to destroy the rest of the array.

                i--;
                p -= elem_size;

                try
                {
                    for ( ; i > 0; i--, p -= elem_size)
                    {
                        (*dtor)(p);
                    }
                }
                catch (...)
                {
                    std::terminate();
                }

                throw;
            }
        }
        #endif
    }

    extern "C" EXPORT_C void __cxa_vec_delete2(void* array, size_t elem_size, size_t, Dtor dtor, void (*dealloc)(void*))
    {
        if (!array)
        {
            return;
        }

        Cookie* cp = _array_to_cookie(array);

        if (dtor)
        {
            char* p = static_cast<char*>(array) + (cp->elem_count - 1) * elem_size;

            size_t i = cp->elem_count;

            #if RT_NO_EXCEPTIONS
            for ( ; i > 0; i--, p -= elem_size)
            {
                (*dtor)(p);
            }
            #else
            try
            {
                for ( ; i > 0; i--, p -= elem_size)
                {
                    (*dtor)(p);
                }
            }
            catch (...)
            {
                i--;
                p -= elem_size;

                try
                {
                    for ( ; i > 0; i--, p -= elem_size)
                    {
                        (*dtor)(p);
                    }
                }
                catch (...)
                {
                    std::terminate();
                }

                throw;
            }
            #endif
        }

        (*dealloc)(cp);
    }

    extern "C" EXPORT_C void __cxa_vec_delete(void* array, size_t elem_size, size_t cookie_size, Dtor dtor)
    {
        __cxa_vec_delete2(array, elem_size, cookie_size, dtor, &::operator delete[]);
    }

    extern "C" EXPORT_C void* __cxa_vec_ctor(void* array, size_t elem_count, size_t elem_size, Ctor ctor, Dtor dtor)
    {
        if (ctor)
        {
            char* p = static_cast<char*>(array);
            size_t i = 0;

            #if RT_NO_EXCEPTIONS
            for ( ; i < elem_count; i++, p += elem_size)
            {
                (*ctor)(p);
            }
            #else
            try
            {
                for ( ; i < elem_count; i++, p += elem_size)
                {
                    (*ctor)(p);
                }
            }
            catch (...)
            {
                // When the exception was thrown, elements [0,i) had already been
                // constructed. Those elements should be destroyed.
                __cxa_vec_cleanup(array, i, elem_size, dtor);
                throw;
            }
            #endif
        }

        return array;
    }

    extern "C" EXPORT_C void* __cxa_vec_new2(size_t elem_count, size_t elem_size, size_t cookie_size, Ctor ctor, Dtor dtor, void* (*alloc)(size_t), void (*dealloc)(void*))
    {
        char* p = static_cast<char*>( (*alloc)(elem_count*elem_size + cookie_size) );

        if (!p)
        {
            return 0;
        }

        p += cookie_size;
        _maybe_init_cookie(p, cookie_size, elem_size, elem_count);

        #if RT_NO_EXCEPTIONS
        (void) __cxa_vec_ctor(p, elem_count, elem_size, ctor, dtor);
        #else
        try
        {
            (void) __cxa_vec_ctor(p, elem_count, elem_size, ctor, dtor);
        }
        catch (...)
        {
            (*dealloc)(p - cookie_size);
            throw;
        }
        #endif

        return p;
    }

    extern "C" EXPORT_C void* __cxa_vec_new(size_t elem_count, size_t elem_size, size_t cookie_size, Ctor ctor, Dtor dtor)
    {
        return __cxa_vec_new2(elem_count, elem_size, cookie_size, ctor, dtor, &::operator new[], &::operator delete[]);
    }

    extern "C" EXPORT_C void* __cxa_vec_new3( size_t elem_count, size_t elem_size, size_t cookie_size, Ctor ctor, Dtor dtor, void* (*alloc)(size_t), void (*dealloc)(void*, size_t))
    {
        const size_t mem_size = elem_count*elem_size + cookie_size;

        char* p = static_cast<char*>( (*alloc)(mem_size) );

        if (!p)
        {
            return 0;
        }

        p += cookie_size;
        _maybe_init_cookie(p, cookie_size, elem_size, elem_count);

        #if RT_NO_EXCEPTIONS
        (void) __cxa_vec_ctor(p, elem_count, elem_size, ctor, dtor);
        #else
        try
        {
            (void) __cxa_vec_ctor(p, elem_count, elem_size, ctor, dtor);
        }
        catch (...)
        {
            (*dealloc)(p - cookie_size, mem_size);
            throw;
        }
        #endif

        return p;
    }

    extern "C" EXPORT_C void __cxa_vec_delete3(void* array, size_t elem_size, size_t, Dtor dtor, void (*dealloc)(void*, size_t))
    {
        if (!array)
        {
            return;
        }

        Cookie* cp = _array_to_cookie(array);

        if (dtor)
        {
            size_t i = cp->elem_count;

            char* p = static_cast<char*>(array) + (i - 1) * elem_size;

            #if RT_NO_EXCEPTIONS
            for ( ; i > 0; i--, p -= elem_size)
            {
                (*dtor)(p);
            }
            #else
            try
            {
                for ( ; i > 0; i--, p -= elem_size)
                {
                    (*dtor)(p);
                }
            }
            catch (...)
            {
                i--;
                p -= elem_size;

                try
                {
                    for ( ; i > 0; i--, p -= elem_size)
                    {
                        (*dtor)(p);
                    }
                }
                catch (...)
                {
                    std::terminate();
                }

                throw;
            }
            #endif
        }

        (*dealloc)(cp, elem_size);
    }

    extern "C" EXPORT_C void* __cxa_vec_cctor(void* dest, void* source, size_t elem_count, size_t elem_size, void* (*cctor)(void* to, void* from), Dtor dtor)
    {
        if (cctor) 
        {
            char* p1 = static_cast<char*>(dest);
            char* p2 = static_cast<char*>(source);

            size_t i = 0;

            #if RT_NO_EXCEPTIONS
            for ( ; i < elem_count; i++, p1 += elem_size, p2 += elem_size)
            {
                (*cctor)(p1, p2);
            }
            #else
            try
            {
                for ( ; i < elem_count; i++, p1 += elem_size, p2 += elem_size)
                {
                    (*cctor)(p1, p2);
                }
            }
            catch (...)
            {
                __cxa_vec_cleanup(dest, i, elem_size, dtor);
                throw;
            }
            #endif
        }

        return dest;
    }

} // namespace __cxxabiv1

namespace __aeabiv1
{
    extern "C" EXPORT_C void* __aeabi_vec_ctor_nocookie_nodtor(void* array, Ctor ctor, size_t elem_size, size_t elem_count)
    {
        return __cxxabiv1::__cxa_vec_ctor(array, elem_count, elem_size, ctor, 0);
    }

    extern "C" EXPORT_C void* __aeabi_vec_ctor_cookie_nodtor(Cookie* cp, Ctor ctor, size_t elem_size, size_t elem_count)
    {
        if (!cp)
        {
            return 0;
        }
        else
        {
            cp->elem_size  = elem_size;
            cp->elem_count = elem_count;

            return __aeabi_vec_ctor_nocookie_nodtor( _cookie_to_array(cp), ctor, elem_count, elem_size);
        }
    }

    extern "C" EXPORT_C void* __aeabi_vec_cctor_nocookie_nodtor(void* dest, void* source, size_t elem_size, size_t elem_count, void* (*cctor)(void*, void*))
    {
        return __cxxabiv1::__cxa_vec_cctor(dest, source, elem_count, elem_size, cctor, 0);
    }

    extern "C" EXPORT_C void* __aeabi_vec_new_cookie_noctor(size_t elem_size, size_t elem_count)
    {
        char* p = static_cast<char*>( ::operator new[]( elem_count*elem_size + sizeof(Cookie) ) );

        p += sizeof(Cookie);

        _init_cookie(p, elem_size, elem_count);

        return p;
    }

    extern "C" EXPORT_C void* __aeabi_vec_new_nocookie(size_t elem_size, size_t elem_count, Ctor ctor)
    {
        return __cxxabiv1::__cxa_vec_new(elem_count, elem_size, 0, ctor, 0);
    }

    extern "C" EXPORT_C void* __aeabi_vec_new_cookie_nodtor(size_t elem_size, size_t elem_count, Ctor ctor)
    {
        return __cxxabiv1::__cxa_vec_new(elem_count, elem_size, sizeof(Cookie), ctor, 0);
    }

    extern "C" EXPORT_C void* __aeabi_vec_new_cookie(size_t elem_size, size_t elem_count, Ctor ctor, Dtor dtor)
    {
        return __cxxabiv1::__cxa_vec_new(elem_count, elem_size, sizeof(Cookie), ctor, dtor);
    }

    extern "C" EXPORT_C void* __aeabi_vec_dtor(void* array, Dtor dtor, size_t elem_size, size_t elem_count)
    {
        char* p = static_cast<char*>(array) + (elem_count - 1) * elem_size;
        size_t i = elem_count;

        #if RT_NO_EXCEPTIONS
        for ( ; i > 0; i--, p -= elem_size)
        {
            (*dtor)(p);
        }
        #else
        try
        {
            for ( ; i > 0; i--, p -= elem_size)
            {
                (*dtor)(p);
            }
        }
        catch (...)
        {
            i--;
            p -= elem_size;

            try
            {
                for ( ; i > 0; i--, p -= elem_size)
                {
                    (*dtor)(p);
                }
            }
            catch (...)
            {
                std::terminate();
            }

            throw;
        }
        #endif

        return _array_to_cookie(array);
    }

    extern "C" EXPORT_C void* __aeabi_vec_dtor_cookie(void* array, Dtor dtor)
    {
        if (!array)
        {
            return 0;
        }

        Cookie* cp = _array_to_cookie(array);

        return __aeabi_vec_dtor(array, dtor, cp->elem_size, cp->elem_count);
    }

    extern "C" EXPORT_C void __aeabi_vec_delete(void* array, Dtor dtor)
    {
        #if RT_NO_EXCEPTIONS
        ::operator delete[]( __aeabi_vec_dtor_cookie(array, dtor) );
        #else
        try
        {
            ::operator delete[]( __aeabi_vec_dtor_cookie(array, dtor) );
        }
        catch (...)
        {
            if (array)
            {
                ::operator delete[]( _array_to_cookie(array) );
            }

            throw;
        }
        #endif
    }

    extern "C" EXPORT_C void __aeabi_vec_delete3(void* array, Dtor dtor, void (*dealloc)(void*, size_t))
    {
        if (array)
        {
            Cookie* cp = _array_to_cookie(array);

            size_t size = cp->elem_count * cp->elem_size + sizeof(Cookie);

            #if RT_NO_EXCEPTIONS
            (void) __aeabi_vec_dtor_cookie(array, dtor);
            #else
            try
            {
                (void) __aeabi_vec_dtor_cookie(array, dtor);
            }
            catch (...)
            {
                try
                {
                    (*dealloc)(cp, size);
                }
                catch (...)
                {
                    std::terminate();
                }

                throw;
            }
            #endif

            (*dealloc)(cp, size);
        }
    }

    extern "C" EXPORT_C void __aeabi_vec_delete3_nodtor(void* array, void (*dealloc)(void*, size_t))
    {
        if (array)
        {
            Cookie* cp = _array_to_cookie(array);

            (*dealloc)(cp, sizeof(Cookie) + cp->elem_count * cp->elem_size);
        }
    }

} // namespace __aeabiv1

