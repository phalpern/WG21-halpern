// <xfunction.h> -*- C++ -*-

// Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
// 2011 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/*
 * Copyright (c) 1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

/** @file xfunction.h
 *  This is a Standard C++ Library header, modified by Pablo Halpern to
 *  demonstrate the implementation and use of type-erased allocator idioms
 *  described in N3525.
 */

#ifndef _XFUNCTION
#define _XFUNCTION 1

#include <utility>
#include <memory>
#include <typeinfo>
#include <new>
#include <type_traits>
#include <cassert>

#include "polymorphic_allocator.h"

BEGIN_NAMESPACE_XSTD

  /**
   *  @brief Exception class thrown when class template function's
   *  operator() is called with an empty target.
   *  @ingroup exceptions
   */
  class bad_function_call : public std::exception
  {
  public:
    virtual ~bad_function_call() throw();
  };

  /**
   *  Trait identifying "location-invariant" types, meaning that the
   *  address of the object (or any of its members) will not escape.
   *  Also implies a trivial copy constructor and assignment operator.
   */
  template<typename _Tp>
    struct __is_location_invariant
    : integral_constant<bool, (is_pointer<_Tp>::value
			       || is_member_pointer<_Tp>::value)>
    { };

  class _Undefined_class;

  union _Nocopy_types
  {
    void*       _M_object;
    const void* _M_const_object;
    void (*_M_function_pointer)();
    void (_Undefined_class::*_M_member_pointer)();
  };

  union _Any_data
  {
    void*       _M_access()       { return &_M_pod_data[0]; }
    const void* _M_access() const { return &_M_pod_data[0]; }

    template<typename _Tp>
      _Tp&
      _M_access()
      { return *static_cast<_Tp*>(_M_access()); }

    template<typename _Tp>
      const _Tp&
      _M_access() const
      { return *static_cast<const _Tp*>(_M_access()); }

    _Nocopy_types _M_unused;
    char _M_pod_data[sizeof(_Nocopy_types)];
  };

  enum _Manager_operation
  {
    __get_type_info,
    __get_functor_ptr,
    __clone_functor,
    __clone_functor_old = __clone_functor,
    __destroy_functor,

    // PGH 2012/12/27 new _Manager_operation codes
    __get_manager_without_alloc,
    __get_manager_with_alloc,
    __get_invoker,
    __clone_functor_new,
    __get_allocator_rsrc_ptr
  };

  // Simple type wrapper that helps avoid annoying const problems
  // when casting between void pointers and pointers-to-pointers.
  template<typename _Tp>
    struct _Simple_type_wrapper
    {
      _Simple_type_wrapper(_Tp __value) : __value(__value) { }

      _Tp __value;
    };

  template<typename _Tp>
    struct __is_location_invariant<_Simple_type_wrapper<_Tp> >
    : __is_location_invariant<_Tp>
    { };

  // Converts a reference to a function object into a callable
  // function object.
  template<typename _Functor>
    inline _Functor&
    __callable_functor(_Functor& __f)
    { return __f; }

  template<typename _Member, typename _Class>
    inline _Mem_fn<_Member _Class::*>
    __callable_functor(_Member _Class::* &__p)
    { return mem_fn(__p); }

  template<typename _Member, typename _Class>
    inline _Mem_fn<_Member _Class::*>
    __callable_functor(_Member _Class::* const &__p)
    { return mem_fn(__p); }

  // PGH Shared pointer to allocator resource
  typedef shared_ptr<polyalloc::allocator_resource> _Shared_alloc_rsrc_ptr;

  template <typename __A>
  static bool _M_is_same_alloc_rsrc(polyalloc::allocator_resource *__rsrca_p,
                                    const __A& __b)
  {
    typedef POLYALLOC_RESOURCE_ADAPTOR(__A) _Adaptor;
    _Adaptor* __adaptor_p = dynamic_cast<_Adaptor*>(__rsrca_p);
    if (! __adaptor_p)
      return false;

    return __adaptor_p->get_allocator() == __b;
  }

  template <typename _Rsrc>
  static bool _M_is_same_alloc_rsrc(polyalloc::allocator_resource *__rsrca_p,
                                    _Rsrc*                         __rsrcb_p)
    {
      return (__rsrca_p == __rsrcb_p ||
              (__rsrca_p && __rsrcb_p && __rsrca_p->is_equal(*__rsrcb_p)));
    }

  template <typename __T>
  static bool _M_is_same_alloc_rsrc(polyalloc::allocator_resource *__rsrca_p, 
                          const polyalloc::polymorphic_allocator<__T>& __b)
    {
      polyalloc::allocator_resource *__rsrcb_p = __b.resource();
      return _M_is_same_alloc_rsrc(__rsrca_p, __rsrcb_p);
    }

  template <typename __T>
  static bool _M_is_same_alloc_rsrc(polyalloc::allocator_resource *__rsrca_p, 
                                    const std::allocator<__T>& __b)
    {
      polyalloc::allocator_resource *__rsrcb_p =
        polyalloc::new_delete_resource_singleton();
      return _M_is_same_alloc_rsrc(__rsrca_p, __rsrcb_p);
    }

  static bool _M_is_same_alloc_rsrc(polyalloc::allocator_resource *__rsrca_p, 
                                    const _Shared_alloc_rsrc_ptr&  __rsrcb_p)
    {
      return _M_is_same_alloc_rsrc(__rsrca_p, __rsrcb_p.get());
    }

  template<typename _Signature>
    class function;

  struct _Noop_alloc_rsrc_deleter {
    void operator()(polyalloc::allocator_resource*) { }
  };

  /// Base class of all polymorphic function object wrappers.
  class _Function_base
  {
  public:
    static const std::size_t _M_max_size = sizeof(_Nocopy_types);
    static const std::size_t _M_max_align = __alignof__(_Nocopy_types);

    // Return a shared pointer to the new/delete singleton.  Each call to this
    // function returns the same shared pointer, so only one shared state
    // object is created.
    static const _Shared_alloc_rsrc_ptr& _M_new_delete_alloc_rsrc()
    {
      static _Shared_alloc_rsrc_ptr __new_del_rsrc_ptr(
        polyalloc::new_delete_resource_singleton(),
        _Noop_alloc_rsrc_deleter());
      return __new_del_rsrc_ptr;
    }

    static _Shared_alloc_rsrc_ptr _M_default_alloc_rsrc()
    {
      polyalloc::allocator_resource *__rsrc =
        polyalloc::allocator_resource::default_resource();

      if (*__rsrc == *polyalloc::new_delete_resource_singleton())
        return _M_new_delete_alloc_rsrc();
      else
        {
          polyalloc::polymorphic_allocator<char> __alloc(__rsrc);
          return _Shared_alloc_rsrc_ptr(__rsrc, _Noop_alloc_rsrc_deleter(),
                                        __alloc);
        }
    }

    // PGH Functor with allocator
    template<typename _Functor>
      class _Functor_wrapper
      {
        uses_allocator_construction_wrapper<_Functor> _M_functor;
        _Shared_alloc_rsrc_ptr                        _M_alloc_resource;

      public:
        _Functor_wrapper(const _Functor& __f,
                         const _Shared_alloc_rsrc_ptr& __a)
          : _M_functor(allocator_arg, &*__a, __f), _M_alloc_resource(__a) { }
        _Functor_wrapper(_Functor&& __f, const _Shared_alloc_rsrc_ptr& __a)
          : _M_functor(allocator_arg, &*__a, move(__f))
          , _M_alloc_resource(__a) { }

        void _M_delete_self()
        {
          // Move allocator pointer to stack before destroying original.
          _Shared_alloc_rsrc_ptr __alloc_rsrc = std::move(_M_alloc_resource);
          this->~_Functor_wrapper();
          // Dealloate self using allocator
          __alloc_rsrc->deallocate(this, sizeof(_Functor_wrapper));
        }

        _Functor      & _M_get_functor()       { return _M_functor.value(); }
        _Functor const& _M_get_functor() const { return _M_functor.value(); }

        // Return reference to avoid bumping ref-count unnecessarily
        const _Shared_alloc_rsrc_ptr& _M_get_alloc_resource() const
          { return _M_alloc_resource; }
      };

      // PGH: bundle of a source function object and a target allocator.  Used
      // as an argument to pass both arguments into a single call to the
      // manager function for cloning a function object using a new allocator.
      // This struct is not a permanent part of any larger data structure.
      struct _Functor_and_alloc
      {
        _Any_data              _M_f;  // source function object
        _Shared_alloc_rsrc_ptr _M_a;  // new allocator

        _Any_data _M_as_any_data()
          {
            _Any_data ret;
            ret._M_access<_Functor_and_alloc*>() = this;
            return ret;
          }

        template <typename __A>
        _Functor_and_alloc(const _Any_data& __f, const __A& __alloc)
          : _M_f(__f)
          , _M_a(_M_make_shared_alloc(__alloc))
          {
          }

        template <typename __A>
        _Functor_and_alloc(const _Any_data& __f, _Shared_alloc_rsrc_ptr&& __r)
          : _M_f(__f)
          , _M_a(std::move(__r))
          {
          }

        template <typename __A>
        _Functor_and_alloc(const _Any_data& __f,
                           const _Shared_alloc_rsrc_ptr& __r)
          : _M_f(__f)
          , _M_a(__r)
          {
          }
      };

    template<typename _Functor, bool __uses_custom_alloc>
      class _Base_manager
      {
        typedef _Functor_wrapper<_Functor> _M_functor_wrapper;

      protected:
        // True if _Functor could be stored locally (i.e., not on the heap)
        // when no allocator is used.
	static const bool __stored_locally_without_alloc =
	(__is_location_invariant<_Functor>::value
	 && sizeof(_Functor) <= _M_max_size
	 && __alignof__(_Functor) <= _M_max_align
	 && (_M_max_align % __alignof__(_Functor) == 0));

        // True if _Functor is actually stored locally.
	static const bool __stored_locally = (__stored_locally_without_alloc
                                              && !__uses_custom_alloc);

	typedef integral_constant<bool, __stored_locally> _Local_storage;

	// Retrieve a pointer to the function object
	static _Functor*
	_M_get_pointer(const _Any_data& __source)
	{
          const _Functor* __ptr =
            (__stored_locally ? 
             std::__addressof(__source._M_access<_Functor>()) :
             std::__addressof(
               __source._M_access<_M_functor_wrapper*>()->_M_get_functor()));
	  return const_cast<_Functor*>(__ptr);
	}

        static inline _M_functor_wrapper*
        _M_make_functor_wrapper(const _Functor&               __f,
                                const _Shared_alloc_rsrc_ptr& __a)
        {
          // Allocate clone from allocator resource and construct it
          _M_functor_wrapper *__ret = static_cast<_M_functor_wrapper *>(
            __a->allocate(sizeof(_M_functor_wrapper),
                          __alignof(_M_functor_wrapper)));
          return new (__ret) _M_functor_wrapper(__f, __a);
        }

	// Clone a location-invariant function object that fits within
	// an _Any_data structure.
	static void
	_M_clone_old(_Any_data& __dest, const _Any_data& __source, true_type)
	{
	  new (__dest._M_access()) _Functor(__source._M_access<_Functor>());
	}

	// Clone a function object that is not location-invariant or
	// that cannot fit into an _Any_data structure.
	static void
	_M_clone_old(_Any_data& __dest, const _Any_data& __source, false_type)
	{
          // Ignore allocator in source
          const _Functor& __f =
            __source._M_access<_M_functor_wrapper*>()->_M_get_functor();

          // Return result in __dest, using new/delete allocator
          __dest._M_access<_M_functor_wrapper*>() =
            _M_make_functor_wrapper(__f, 
                                    _Function_base::_M_new_delete_alloc_rsrc());
	}

	// Clone a location-invariant function object that fits within
	// an _Any_data structure.
	static void
	_M_clone_new(_Any_data& __dest, const _Any_data& __source,
                     const _Shared_alloc_rsrc_ptr& __a, true_type)
	{
          // Source functor is stored directly in __source
          const _Functor& __f = __source._M_access<_Functor>();

          // Return result in __dest
          if (*__a.get() == *polyalloc::new_delete_resource_singleton() &&
              __stored_locally_without_alloc)
            // Cloned functor fits locally and doesn't need an allocator
            new (__dest._M_access()) _Functor(__f);
          else
            // Cloned functor doesn't fit locally or needs an allocator
            __dest._M_access<_M_functor_wrapper*>() =
              _M_make_functor_wrapper(__f, __a);
	}

	// Clone a function object that is not location-invariant or
	// that cannot fit into an _Any_data structure.
	static void
	_M_clone_new(_Any_data& __dest, const _Any_data& __source,
                     const _Shared_alloc_rsrc_ptr& __a, false_type)
	{
          // Source functor is stored indirectly through a pointer to a wrapper
          const _Functor& __f =
            __source._M_access<_M_functor_wrapper*>()->_M_get_functor();

          // Return result in __dest
          if (*__a.get() == *polyalloc::new_delete_resource_singleton() &&
              __stored_locally_without_alloc)
            // Cloned functor fits locally and doesn't need an allocator
            new (__dest._M_access()) _Functor(__f);
          else
            // Cloned functor doesn't fit locally or needs an allocator
            __dest._M_access<_M_functor_wrapper*>() =
              _M_make_functor_wrapper(__f, __a);
	}

	// Destroying a location-invariant object may still require
	// destruction.
	static void
	_M_destroy(_Any_data& __victim, true_type)
	{
	  __victim._M_access<_Functor>().~_Functor();
	}

	// Destroying an object located on the heap.
	static void
	_M_destroy(_Any_data& __victim, false_type)
	{
          // Delete functor wrapper using its own allocator.
          __victim._M_access<_M_functor_wrapper*>()->_M_delete_self();
	}

        // Return the new/delete allocator if __uses_allocator is false
        static void
        _M_get_allocator_resource(_Any_data&       __resource,
                                  const _Any_data& __source,
                                  false_type)
        {
          __resource._M_access<const _Shared_alloc_rsrc_ptr*>() =
            &_M_new_delete_alloc_rsrc();
        }

        // Return custom allocator if __uses_allocator is true
        static void
        _M_get_allocator_resource(_Any_data&       __resource,
                                  const _Any_data& __source,
                                  true_type)
        {
          __resource._M_access<const _Shared_alloc_rsrc_ptr*>() =
            &__source._M_access<_M_functor_wrapper*>()->
            _M_get_alloc_resource();
        }

      public:
	static bool
	_M_manager(_Any_data& __dest, const _Any_data& __source,
		   _Manager_operation __op)
	{
          typedef bool (*_Manager_type)(_Any_data&, const _Any_data&,
                                        _Manager_operation);

	  switch (__op)
	    {
#ifdef __GXX_RTTI
	    case __get_type_info:
	      __dest._M_access<const type_info*>() = &typeid(_Functor);
	      break;
#endif
	    case __get_functor_ptr:
	      __dest._M_access<_Functor*>() = _M_get_pointer(__source);
	      break;

	    case __clone_functor_old:
	      _M_clone_old(__dest, __source, _Local_storage());
	      break;

	    case __destroy_functor:
	      _M_destroy(__dest, _Local_storage());
	      break;

            // PGH 2012/12/27
            // Invoking _M_manager with an unknown __op is a no-op.  The
            // following OP codes are new for allocator-enabled function
            // objects.  Since these cases were previously no-ops, the caller
            // should handle the situation where __dest is unchanged, for
            // binary compatibility with std::function objects that were
            // created by code that was compiled prior to these changes.
            case __get_manager_without_alloc:
              // Return a pointer to this manager function, for a function
              // object that does not use a custom allocator.
              __dest._M_access<_Manager_type>() =
                _Base_manager<_Functor, false>::_M_manager;
              break;

            case __get_manager_with_alloc:
              // Return a pointer to this manager function, for a function
              // object that uses a custom allocator.
              __dest._M_access<_Manager_type>() =
                _Base_manager<_Functor, true>::_M_manager;
              break;

            case __get_invoker:
              // Return a pointer to the invoker corresponding to this manager.
              __dest._M_access<void*>() = nullptr;
              break;

            case __clone_functor_new:
              {
                _Functor_and_alloc *__fa =
                  __source._M_access<_Functor_and_alloc *>();
                _M_clone_new(__dest, __fa->_M_f, __fa->_M_a, _Local_storage());
              }
	      break;

            case __get_allocator_rsrc_ptr:
              typedef integral_constant<bool,
                                        __uses_custom_alloc> _Custom_allocator;
              _M_get_allocator_resource(__dest, __source, _Custom_allocator());
              break;
            }
	  return false;
	}

	static void
	_M_init_functor(_Any_data& __functor, _Functor&& __f,
                        _Shared_alloc_rsrc_ptr&& __a)
        { _M_init_functor(__functor, std::move(__f),
                          std::move(__a), _Local_storage()); }

	template<typename _Signature>
	  static bool
	  _M_not_empty_function(const function<_Signature>& __f)
	  { return static_cast<bool>(__f); }

	template<typename _Tp>
	  static bool
	  _M_not_empty_function(_Tp* const& __fp)
	  { return __fp; }

	template<typename _Class, typename _Tp>
	  static bool
	  _M_not_empty_function(_Tp _Class::* const& __mp)
	  { return __mp; }

	template<typename _Tp>
	  static bool
	  _M_not_empty_function(const _Tp&)
	  { return true; }

      private:
	static void
	_M_init_functor(_Any_data& __functor, _Functor&& __f,
                        _Shared_alloc_rsrc_ptr&& __a, true_type /* local */)
	{ new (__functor._M_access()) _Functor(std::move(__f)); }

	static void
	_M_init_functor(_Any_data& __functor, _Functor&& __f,
                        _Shared_alloc_rsrc_ptr&& __a, false_type /* local */)
	{ 
          // Allocate a functor from the allocator resource and construct it.
          _M_functor_wrapper *__pfunctor = static_cast<_M_functor_wrapper *>(
            __a->allocate(sizeof(_M_functor_wrapper),
                          __alignof__(_M_functor_wrapper)));
          new (__pfunctor) _M_functor_wrapper(std::move(__f), std::move(__a));
          __functor._M_access<_M_functor_wrapper*>() = __pfunctor;
        }
      };

    template<typename _Functor, bool __uses_custom_alloc>
      class _Ref_manager : public _Base_manager<_Functor*,
                                                __uses_custom_alloc>
      {
	typedef _Function_base::_Base_manager<_Functor*,
                                              __uses_custom_alloc> _Base;

    public:
	static bool
	_M_manager(_Any_data& __dest, const _Any_data& __source,
		   _Manager_operation __op)
	{
          typedef bool (*_Manager_type)(_Any_data&, const _Any_data&,
                                        _Manager_operation);

	  switch (__op)
	    {
#ifdef __GXX_RTTI
	    case __get_type_info:
	      __dest._M_access<const type_info*>() = &typeid(_Functor);
	      break;
#endif
	    case __get_functor_ptr:
	      __dest._M_access<_Functor*>() = *_Base::_M_get_pointer(__source);
	      return is_const<_Functor>::value;
	      break;

            // PGH 2012/12/27
            // Invoking _M_manager with an unknown __op is a no-op.  The
            // following OP codes are new for allocator-enabled function
            // objects.  Since these cases were previously no-ops, the caller
            // should handle the situation where __dest is unchanged, for
            // binary compatibility with std::function objects that were
            // created by code that was compiled prior to these changes.
            case __get_manager_without_alloc:
              // Return a pointer to this manager function, for a function
              // object that does not use a custom allocator.
              __dest._M_access<_Manager_type>() =
                _Ref_manager<_Functor, false>::_M_manager;
              break;

            case __get_manager_with_alloc:
              // Return a pointer to this manager function, for a function
              // object that uses a custom allocator.
              __dest._M_access<_Manager_type>() =
                _Ref_manager<_Functor, true>::_M_manager;
              break;

            case __get_invoker:
              // Return a pointer to the invoker corresponding to this manager.
              __dest._M_access<void*>() = nullptr;
              break;

	    default:
	      _Base::_M_manager(__dest, __source, __op);
	    }
	  return false;
	}

	static void
	_M_init_functor(_Any_data& __functor, reference_wrapper<_Functor> __f,
                        _Shared_alloc_rsrc_ptr&& __a)
	{
	  // TBD: Use address_of function instead.
	  _Base::_M_init_functor(__functor, &__f.get(), std::move(__a));
	}
      };

    _Function_base() : _M_manager(0) { }

    ~_Function_base()
    {
      if (_M_manager)
	_M_manager(_M_functor, _M_functor, __destroy_functor);
    }


    bool _M_empty() const { return !_M_manager; }

    typedef bool (*_Manager_type)(_Any_data&, const _Any_data&,
				  _Manager_operation);

    _Any_data     _M_functor;
    _Manager_type _M_manager;
  };

  template <typename __A>
  static _Shared_alloc_rsrc_ptr _M_make_shared_alloc(const __A& __alloc)
  {
    return std::allocate_shared<POLYALLOC_RESOURCE_ADAPTOR(__A) >(__alloc,
                                                                  __alloc);
  }

  template <typename _Rsrc>
  static _Shared_alloc_rsrc_ptr _M_make_shared_alloc(_Rsrc* __p)
    {
      // Compiler failure if __Rsrc is not derived from allocator_resource
      if (*__p == *polyalloc::new_delete_resource_singleton())
        return _Function_base::_M_new_delete_alloc_rsrc();
      else
        {
          polyalloc::polymorphic_allocator<char> __alloc(__p);
          return _Shared_alloc_rsrc_ptr(__p, _Noop_alloc_rsrc_deleter(),
                                        __alloc);
        }
    }

  template <typename __T>
  static _Shared_alloc_rsrc_ptr _M_make_shared_alloc(
    const polyalloc::polymorphic_allocator<__T>& __alloc)
    {
      return _M_make_shared_alloc(__alloc.resource());
    }

  template <typename __T>
  static const _Shared_alloc_rsrc_ptr& _M_make_shared_alloc(
    const std::allocator<__T>& __alloc)
    {
      return _Function_base::_M_new_delete_alloc_rsrc();
    }

  static _Shared_alloc_rsrc_ptr _M_make_shared_alloc(_Shared_alloc_rsrc_ptr __p)
    {
      return __p;
    }

  template<typename _Signature, typename _Functor, bool __uses_custom_alloc>
    class _Function_handler;

  template<typename _Signature, typename _Functor, bool __uses_custom_alloc>
    struct _Function_handler_wrapper :
      _Function_handler<_Signature, _Functor, __uses_custom_alloc>
    {
      static bool
      _M_manager(_Any_data& __dest, const _Any_data& __source,
		 _Manager_operation __op)
      {
        typedef bool (*_Manager_type)(_Any_data&, const _Any_data&,
                                      _Manager_operation);

        typedef _Function_handler_wrapper<_Signature, _Functor,
                                          __uses_custom_alloc> _Self;
        typedef _Function_handler<_Signature, _Functor, __uses_custom_alloc>
          _Base;

	switch (__op)
	  {
#ifdef __GXX_RTTI
	  case __get_type_info:
	    __dest._M_access<const type_info*>() = &typeid(_Functor);
	    break;
#endif
          // PGH 2012/12/27
          // Invoking _M_manager with an unknown __op is a no-op.  The
          // following OP codes are new for allocator-enabled function
          // objects.  Since these cases were previously no-ops, the caller
          // should handle the situation where __dest is unchanged, for
          // binary compatibility with std::function objects that were
          // created by code that was compiled prior to these changes.
          case __get_manager_without_alloc:
            // Return a pointer to this manager function, for a function
            // object that does not use a custom allocator.
            __dest._M_access<_Manager_type>() =
              _Function_handler_wrapper<_Signature, _Functor,
                                        false>::_M_manager;
            break;

          case __get_manager_with_alloc:
            // Return a pointer to this manager function, for a function
            // object that uses a custom allocator.
            __dest._M_access<_Manager_type>() =
              _Function_handler_wrapper<_Signature, _Functor,
                                        true>::_M_manager;
            break;

          case __get_invoker:
            // Return a pointer to the invoker corresponding to this manager.
            typedef decltype(&_Self::_M_invoke) _Invoker_type;
            __dest._M_access<_Invoker_type>() = &_Self::_M_invoke;
            break;

	  default:
	    _Base::_M_manager(__dest, __source, __op);
	  }
	return false;
      }
      
    };

  template<typename _Res, typename _Functor, bool __uses_custom_alloc,
           typename... _ArgTypes>
    class _Function_handler<_Res(_ArgTypes...), _Functor, __uses_custom_alloc>
    : public _Function_base::_Base_manager<_Functor, __uses_custom_alloc>
    {
      typedef _Function_base::_Base_manager<_Functor,
                                            __uses_custom_alloc> _Base;

    public:
      static _Res
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	return (*_Base::_M_get_pointer(__functor))(
	    std::forward<_ArgTypes>(__args)...);
      }
    };

  template<typename _Functor, bool __uses_custom_alloc, typename... _ArgTypes>
    class _Function_handler<void(_ArgTypes...), _Functor, __uses_custom_alloc>
    : public _Function_base::_Base_manager<_Functor, __uses_custom_alloc>
    {
      typedef _Function_base::_Base_manager<_Functor,
                                            __uses_custom_alloc> _Base;

     public:
      static void
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	(*_Base::_M_get_pointer(__functor))(
	    std::forward<_ArgTypes>(__args)...);
      }
    };

  template<typename _Res, typename _Functor, bool __uses_custom_alloc,
           typename... _ArgTypes>
    class _Function_handler<_Res(_ArgTypes...), reference_wrapper<_Functor>,
                            __uses_custom_alloc>
    : public _Function_base::_Ref_manager<_Functor, __uses_custom_alloc>
    {
      typedef _Function_base::_Ref_manager<_Functor,__uses_custom_alloc> _Base;

     public:
      static _Res
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	return __callable_functor(**_Base::_M_get_pointer(__functor))(
	      std::forward<_ArgTypes>(__args)...);
      }
    };

  template<typename _Functor, bool __uses_custom_alloc, typename... _ArgTypes>
    class _Function_handler<void(_ArgTypes...), reference_wrapper<_Functor>,
                            __uses_custom_alloc>
    : public _Function_base::_Ref_manager<_Functor, __uses_custom_alloc>
    {
      typedef _Function_base::_Ref_manager<_Functor,__uses_custom_alloc> _Base;

     public:
      static void
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	__callable_functor(**_Base::_M_get_pointer(__functor))(
	    std::forward<_ArgTypes>(__args)...);
      }
    };

  template<typename _Class, typename _Member, bool __uses_custom_alloc,
           typename _Res, typename... _ArgTypes>
    class _Function_handler<_Res(_ArgTypes...), _Member _Class::*,
                            __uses_custom_alloc>
    : public _Function_handler<void(_ArgTypes...), _Member _Class::*,
                               __uses_custom_alloc>
    {
      typedef _Function_handler<void(_ArgTypes...), _Member _Class::*,
                                __uses_custom_alloc> _Base;

     public:
      static _Res
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	return mem_fn(_Base::_M_get_pointer(__functor)->__value)(
	    std::forward<_ArgTypes>(__args)...);
      }
    };

template<typename _Class, typename _Member, bool __uses_custom_alloc,
         typename... _ArgTypes>
  class _Function_handler<void(_ArgTypes...), _Member _Class::*,
                          __uses_custom_alloc>
    : public _Function_base::_Base_manager<
                   _Simple_type_wrapper< _Member _Class::* >,
                   __uses_custom_alloc>
    {
      typedef _Member _Class::* _Functor;
      typedef _Simple_type_wrapper<_Functor> _Wrapper;
      typedef _Function_base::_Base_manager<_Wrapper,
                                            __uses_custom_alloc> _Base;

     public:
      static bool
      _M_manager(_Any_data& __dest, const _Any_data& __source,
		 _Manager_operation __op)
      {
        typedef bool (*_Manager_type)(_Any_data&, const _Any_data&,
                                      _Manager_operation);

	switch (__op)
	  {
#ifdef __GXX_RTTI
	  case __get_type_info:
	    __dest._M_access<const type_info*>() = &typeid(_Functor);
	    break;
#endif
	  case __get_functor_ptr:
	    __dest._M_access<_Functor*>() =
	      &_Base::_M_get_pointer(__source)->__value;
	    break;

	  default:
	    _Base::_M_manager(__dest, __source, __op);
	  }
	return false;
      }

      static void
      _M_invoke(const _Any_data& __functor, _ArgTypes... __args)
      {
	mem_fn(_Base::_M_get_pointer(__functor)->__value)(
	    std::forward<_ArgTypes>(__args)...);
      }
    };

  /**
   *  @brief Primary class template for std::function.
   *  @ingroup functors
   *
   *  Polymorphic function wrapper.
   */
  template<typename _Res, typename... _ArgTypes>
    class function<_Res(_ArgTypes...)>
    : public _Maybe_unary_or_binary_function<_Res, _ArgTypes...>,
      private _Function_base
    {
      typedef _Res _Signature_type(_ArgTypes...);

      // PGH Shared pointer to allocator resource
      typedef shared_ptr<polyalloc::allocator_resource> _Shared_alloc_rsrc_ptr;

      struct _Useless { };

    public:
      typedef _Res result_type;

      // PGH: New type-erased allocator typedef
      typedef erased_type allocator_type;

      // [3.7.2.1] construct/copy/destroy

      /**
       *  @brief Default construct creates an empty function call wrapper.
       *  @post @c !(bool)*this
       */
      function() : _Function_base() { _M_make_empty(); }

      /**
       *  @brief Creates an empty function call wrapper.
       *  @post @c !(bool)*this
       */
      function(nullptr_t) : _Function_base() { _M_make_empty(); }

      /**
       *  @brief %Function copy constructor.
       *  @param x A %function object with identical call signature.
       *  @post @c (bool)*this == (bool)x
       *
       *  The newly-created %function contains a copy of the target of @a
       *  x (if it has one).
       */
      function(const function& __x);

      /**
       *  @brief %Function move constructor.
       *  @param x A %function object rvalue with identical call signature.
       *
       *  The newly-created %function contains the target of @a x
       *  (if it has one).
       */
      function(function&& __x); // PGH: replace imp

      /**
       *  @brief Builds a %function that targets a copy of the incoming
       *  function object.
       *  @param f A %function object that is callable with parameters of
       *  type @c T1, @c T2, ..., @c TN and returns a value convertible
       *  to @c Res.
       *
       *  The newly-created %function object will target a copy of @a
       *  f. If @a f is @c reference_wrapper<F>, then this function
       *  object will contain a reference to the function object @c
       *  f.get(). If @a f is a NULL function pointer or NULL
       *  pointer-to-member, the newly-created object will be empty.
       *
       *  If @a f is a non-NULL function pointer or an object of type @c
       *  reference_wrapper<F>, this function will not throw.
       */
      template<typename _Functor>
	function(_Functor __f,
		 typename enable_if<
			   !is_integral<_Functor>::value, _Useless>::type
		   = _Useless());

      // PGH: allocator_arg_t versions
      template <typename __A>
      function(allocator_arg_t, const __A&);

      template <typename __A>
      function(allocator_arg_t, const __A&, nullptr_t);

      template <typename __A>
      function(allocator_arg_t, const __A&, const function& __x);

      template <typename __A>
      function(allocator_arg_t, const __A&, function&& __x);

      template<typename _Functor, typename __A>
	function(allocator_arg_t, const __A&, _Functor __f,
		 typename enable_if<
			   !is_integral<_Functor>::value &&
                           !is_convertible<_Functor, nullptr_t>::value,
                           _Useless>::type = _Useless());

      ~function();

      /**
       *  @brief %Function assignment operator.
       *  @param x A %function with identical call signature.
       *  @post @c (bool)*this == (bool)x
       *  @returns @c *this
       *
       *  The target of @a x is copied to @c *this. If @a x has no
       *  target, then @c *this will be empty.
       *
       *  If @a x targets a function pointer or a reference to a function
       *  object, then this operation will not throw an %exception.
       */
      function&
      operator=(const function& __x)
      {
	function(allocator_arg, _M_get_shared_alloc_rsrc_ptr(),
                 __x)._M_do_swap(*this);
	return *this;
      }

      /**
       *  @brief %Function move-assignment operator.
       *  @param x A %function rvalue with identical call signature.
       *  @returns @c *this
       *
       *  The target of @a x is moved to @c *this. If @a x has no
       *  target, then @c *this will be empty.
       *
       *  If @a x targets a function pointer or a reference to a function
       *  object, then this operation will not throw an %exception.
       */
      function&
      operator=(function&& __x)
      {
	function(allocator_arg, _M_get_shared_alloc_rsrc_ptr(),
                 std::move(__x))._M_do_swap(*this);
	return *this;
      }

      /**
       *  @brief %Function assignment to zero.
       *  @post @c !(bool)*this
       *  @returns @c *this
       *
       *  The target of @c *this is deallocated, leaving it empty.
       */
      function&
      operator=(nullptr_t)
      {
        function(allocator_arg,
                 _M_get_shared_alloc_rsrc_ptr())._M_do_swap(*this);
        return *this;
      }

      /**
       *  @brief %Function assignment to a new target.
       *  @param f A %function object that is callable with parameters of
       *  type @c T1, @c T2, ..., @c TN and returns a value convertible
       *  to @c Res.
       *  @return @c *this
       *
       *  This  %function object wrapper will target a copy of @a
       *  f. If @a f is @c reference_wrapper<F>, then this function
       *  object will contain a reference to the function object @c
       *  f.get(). If @a f is a NULL function pointer or NULL
       *  pointer-to-member, @c this object will be empty.
       *
       *  If @a f is a non-NULL function pointer or an object of type @c
       *  reference_wrapper<F>, this function will not throw.
       */
      template<typename _Functor>
	typename enable_if<!is_integral<_Functor>::value, function&>::type
	operator=(_Functor&& __f)
	{
          function(allocator_arg, _M_get_shared_alloc_rsrc_ptr(),
                   std::forward<_Functor>(__f))._M_do_swap(*this);
	  return *this;
	}

      /// @overload
      template<typename _Functor>
	typename enable_if<!is_integral<_Functor>::value, function&>::type
	operator=(reference_wrapper<_Functor> __f)
	{
          function(allocator_arg, _M_get_shared_alloc_rsrc_ptr(),
                   __f)._M_do_swap(*this);
	  return *this;
	}

      // [3.7.2.2] function modifiers

      /**
       *  @brief Swap the targets of two %function objects.
       *  @param f A %function with identical call signature.
       *
       *  Swap the targets of @c this function object and @a f. This
       *  function will not throw an %exception.
       */
      void swap(function& __x)
      {
        // Assert *this and __x have equal allocators
        assert(_M_is_same_alloc_rsrc(this->get_allocator_resource(),
                                     __x.get_allocator_resource()));
        this->_M_do_swap(__x);
      }

      // [3.7.2.3] function capacity

      /**
       *  @brief Determine if the %function wrapper has a target.
       *
       *  @return @c true when this %function object contains a target,
       *  or @c false when it is empty.
       *
       *  This function will not throw an %exception.
       */
      explicit operator bool() const
      { return !_M_empty(); }

      // [3.7.2.4] function invocation

      /**
       *  @brief Invokes the function targeted by @c *this.
       *  @returns the result of the target.
       *  @throws bad_function_call when @c !(bool)*this
       *
       *  The function call operator invokes the target function object
       *  stored by @c this.
       */
      _Res operator()(_ArgTypes... __args) const;

#ifdef __GXX_RTTI
      // [3.7.2.5] function target access
      /**
       *  @brief Determine the type of the target of this function object
       *  wrapper.
       *
       *  @returns the type identifier of the target function object, or
       *  @c typeid(void) if @c !(bool)*this.
       *
       *  This function will not throw an %exception.
       */
      const type_info& target_type() const;

      /**
       *  @brief Access the stored target function object.
       *
       *  @return Returns a pointer to the stored target function object,
       *  if @c typeid(Functor).equals(target_type()); otherwise, a NULL
       *  pointer.
       *
       * This function will not throw an %exception.
       */
      template<typename _Functor>       _Functor* target();

      /// @overload
      template<typename _Functor> const _Functor* target() const;
#endif

      polyalloc::allocator_resource* get_allocator_resource() const;

    private:
      typedef _Res (*_Invoker_type)(const _Any_data&, _ArgTypes...);
      _Invoker_type _M_invoker;

      // PGH: Swap without testing for validity
      void _M_do_swap(function& __x)
      {
	std::swap(_M_functor, __x._M_functor);
	std::swap(_M_manager, __x._M_manager);
	std::swap(_M_invoker, __x._M_invoker);
      }

      // Special invoker for empty function that holds an allocator.
      static _Res _M_null_invoker_with_alloc(const _Any_data&, _ArgTypes...);

      _Shared_alloc_rsrc_ptr
      _M_get_shared_alloc_rsrc_ptr() const
        {
          const _Shared_alloc_rsrc_ptr *__ret_p = nullptr;

          if (_M_manager)
          {
            // __get_allocator_rsrc_ptr returns a pointer to a shared_ptr
            _Any_data __alloc_data;
            __alloc_data._M_access<const _Shared_alloc_rsrc_ptr*>() = nullptr;
            _M_manager(__alloc_data, _M_functor, __get_allocator_rsrc_ptr);
            __ret_p = __alloc_data._M_access<const _Shared_alloc_rsrc_ptr*>();
          }
          else if (_M_invoker == _M_null_invoker_with_alloc)
            // shared_ptr is embedded in _M_functor.  Note that this is NOT an
            // indirect pointer to a shared_ptr, unlike the result of
            // __get_allocator_rsrc_ptr, above.
            return _M_functor._M_access<const _Shared_alloc_rsrc_ptr>();

          if (__ret_p)
            return *__ret_p;
          else
            // No stored allocator.  Return new/delete resource.
            return _Function_base::_M_new_delete_alloc_rsrc();
        }

      // Make an empty function object with the default allocator resource
      void _M_make_empty();

      // Make an empty function object with an allocator shared_ptr
      void _M_make_empty(_Shared_alloc_rsrc_ptr&& __alloc);
      
      // Make an empty function object an allocator
      template <typename __A>
        void _M_make_empty(const __A& __alloc);

      // Make an empty function object with the new/delete allocator resource
      template <typename __T>
        void _M_make_empty(const std::allocator<__T>& __alloc);

      // Make a copy with an allocator
      template<typename __A>
        void _M_make_copy(const __A& __alloc, const function& __x);
      
      template<typename _Handler, typename _Functor>
        void _M_make_from_functor(_Functor&& __f,
                                  _Shared_alloc_rsrc_ptr __rsrc);
  };

  // Out-of-line member definitions.
  template<typename _Res, typename... _ArgTypes>
      _Res function<_Res(_ArgTypes...)>::
         _M_null_invoker_with_alloc(const _Any_data&, _ArgTypes...)
  {
    // assert(0);  // Should never be executed
    __throw_bad_function_call();
  }

  template<typename _Res, typename... _ArgTypes>
  inline void
  function<_Res(_ArgTypes...)>::_M_make_empty()
  {
    // Store the default allocator resource only if it is different from the
    // new/delete allocator resource.
    _M_manager = nullptr;
    _M_invoker = nullptr;
    if (polyalloc::allocator_resource::default_resource() !=
        polyalloc::new_delete_resource_singleton())
      _M_make_empty(_Function_base::_M_default_alloc_rsrc());
  }

  template<typename _Res, typename... _ArgTypes>
  void
  function<_Res(_ArgTypes...)>::_M_make_empty(_Shared_alloc_rsrc_ptr&& __alloc)
    {
      // When constructing an empty function with an allocator, we squirrel
      // a shared_ptr to the allocator resource away in _M_functor.  It
      // would be nice if we could set _M_manager to something that will
      // manage that allocator, but _M_manager must be set to nullptr
      // because that is how the code has always detected empty function
      // objects.  Instead, we set _M_invoker to _M_null_invoker_with_alloc
      // as a signal to the destructor and other functions that this object
      // is not completely empty, but actually contains an allocator.
      static_assert(sizeof(_Shared_alloc_rsrc_ptr) <= sizeof(_Any_data),
                    "Cannot store a shared_ptr in _Any_data object");
      if (__alloc.get() == polyalloc::new_delete_resource_singleton())
        return;  // New/Delete allocator.  Nothing to store.
      new (_M_functor._M_access()) _Shared_alloc_rsrc_ptr(std::move(__alloc));
      _M_invoker = _M_null_invoker_with_alloc;
      _M_manager = nullptr;
    }

  template<typename _Res, typename... _ArgTypes>
    template <typename __A>
      void
      function<_Res(_ArgTypes...)>::_M_make_empty(const __A& __alloc)
      {
        _M_make_empty(_M_make_shared_alloc(__alloc));
      }

  template<typename _Res, typename... _ArgTypes>
    template <typename __T>
      void
      function<_Res(_ArgTypes...)>::_M_make_empty(
        const std::allocator<__T>& __alloc)
    {
      // No need to store std::allocator
      _M_manager = nullptr;
      _M_invoker = nullptr;
    }

  template<typename _Res, typename... _ArgTypes>
    template<typename __A>
      void
      function<_Res(_ArgTypes...)>::_M_make_copy(const __A&      __alloc,
                                                 const function& __x)
        {
          if (__x._M_manager)
          {
            _Any_data __out_param;
            // Do not need to store an allocator if __alloc is the new/delete
            // allocator.
            bool __no_alloc = _M_is_same_alloc_rsrc(
              polyalloc::new_delete_resource_singleton(), __alloc);

            // Get a variant of __x._M_manager that uses an allocator.
            // Note that __get_manager_with_alloc is a new op, so we must set a
            // default value in case it is a no-op for __x.
            __out_param._M_access<_Manager_type>() = nullptr;
            if (__no_alloc)
              // New/delete allocator
              __x._M_manager(__out_param, __out_param,
                             __get_manager_without_alloc);
            else
              // Non-new/delete allocator
              __x._M_manager(__out_param, __out_param,
                             __get_manager_with_alloc);
            _M_manager = __out_param._M_access<_Manager_type>();

            if (! _M_manager)
            {
              // Copying a legacy function for which __get_manager_with_alloc
              // is not implemented
              _M_invoker = __x._M_invoker;
              _M_manager = __x._M_manager;
              __x._M_manager(_M_functor, __x._M_functor, __clone_functor_old);
            }
              
            // Get a variant of __x._M_invoker that uses an allocator.
            __out_param._M_access<_Invoker_type>() = nullptr;
            this->_M_manager(__out_param, __out_param, __get_invoker);
            _M_invoker = __out_param._M_access<_Invoker_type>();
          
            // Construct clone of __x._M_functor with allocator
            _Shared_alloc_rsrc_ptr __x_rsrc = __x._M_get_shared_alloc_rsrc_ptr();
            if (_M_is_same_alloc_rsrc(__x_rsrc.get(), __alloc))
              __x._M_manager(_M_functor,
                             _Functor_and_alloc(__x._M_functor,
                                         std::move(__x_rsrc))._M_as_any_data(),
                             __clone_functor_new);
            else
              __x._M_manager(_M_functor,
                             _Functor_and_alloc(__x._M_functor,
                                                __alloc)._M_as_any_data(),
                             __clone_functor_new);
          }
          else if (_M_null_invoker_with_alloc == __x._M_invoker)
          {
            _Shared_alloc_rsrc_ptr __x_rsrc = __x._M_get_shared_alloc_rsrc_ptr();
            if (_M_is_same_alloc_rsrc(__x_rsrc.get(), __alloc))
              _M_make_empty(__x_rsrc);
            else
              _M_make_empty(__alloc);
          }
          else
            _M_make_empty(__alloc);
        }

  template<typename _Res, typename... _ArgTypes>
    template<typename _Handler, typename _Functor>
      void
      function<_Res(_ArgTypes...)>::_M_make_from_functor(_Functor&& __f,
                                                 _Shared_alloc_rsrc_ptr __rsrc)
        {
          if (_Handler::_M_not_empty_function(__f))
            {
              _M_invoker = &_Handler::_M_invoke;
              _M_manager = &_Handler::_M_manager;
              _Handler::_M_init_functor(_M_functor, std::move(__f),
                                        std::move(__rsrc));
            }
          else
            _M_make_empty(__rsrc);
        }

  template<typename _Res, typename... _ArgTypes>
    function<_Res(_ArgTypes...)>::
    function(const function& __x)
    : _Function_base()
    {
      _M_make_copy(polyalloc::allocator_resource::default_resource(), __x);
    }

  template<typename _Res, typename... _ArgTypes>
    function<_Res(_ArgTypes...)>::
    function(function&& __x)
    : _Function_base()
    {
      // Create empty function with same allocator as __x, then swap.
      _M_make_empty(__x._M_get_shared_alloc_rsrc_ptr());
      this->_M_do_swap(__x);
    }

  template<typename _Res, typename... _ArgTypes>
    template<typename _Functor>
      function<_Res(_ArgTypes...)>::
      function(_Functor __f,
	       typename enable_if<
			!is_integral<_Functor>::value, _Useless>::type)
      : _Function_base()
      {
        if (polyalloc::allocator_resource::default_resource() ==
            polyalloc::new_delete_resource_singleton())
          {
            // new/delete allocator
            typedef _Function_handler_wrapper<_Signature_type, _Functor, false>
              _My_handler;

            _M_make_from_functor<_My_handler>(std::move(__f),
                                              _M_new_delete_alloc_rsrc());
          }
        else
          {
            // Non-new/delete allocator
            typedef _Function_handler_wrapper<_Signature_type, _Functor, true>
              _My_handler;

            _M_make_from_functor<_My_handler>(std::move(__f),
                                              _M_default_alloc_rsrc());
          }
      }

  // PGH: allocator_arg_t versions
  template<typename _Res, typename... _ArgTypes>
    template<typename __A>
      function<_Res(_ArgTypes...)>::
      function(allocator_arg_t, const __A& __alloc)
      : _Function_base()
      {
        _M_make_empty(__alloc);
      }

  template<typename _Res, typename... _ArgTypes>
    template<typename __A>
      function<_Res(_ArgTypes...)>::
      function(allocator_arg_t, const __A& __alloc, nullptr_t)
      : _Function_base()
      {
        _M_make_empty(__alloc);
      }

  template<typename _Res, typename... _ArgTypes>
    template<typename __A>
      function<_Res(_ArgTypes...)>::
      function(allocator_arg_t, const __A& __alloc, const function& __x)
      : _Function_base()
        {
          _M_make_copy(__alloc, __x);
        }

  template<typename _Res, typename... _ArgTypes>
    template<typename __A>
      function<_Res(_ArgTypes...)>::
      function(allocator_arg_t, const __A& __alloc, function&& __x)
      : _Function_base()
        {
          _Shared_alloc_rsrc_ptr __x_rsrc = __x._M_get_shared_alloc_rsrc_ptr();

          if (_M_is_same_alloc_rsrc(__x_rsrc.get(), __alloc))
          {
            // Create an empty function with the correct allocator, then swap
            _M_make_empty(__x_rsrc);
            _M_do_swap(__x);
          }
          else
            _M_make_copy(__alloc, __x);
        }

  template<typename _Res, typename... _ArgTypes>
    template<typename _Functor, typename __A>
      function<_Res(_ArgTypes...)>::
      function(allocator_arg_t, const __A& __alloc, _Functor __f,
		 typename enable_if<
                   !is_integral<_Functor>::value &&
                   !is_convertible<_Functor, nullptr_t>::value, _Useless>::type)
      {
        if (_M_is_same_alloc_rsrc(polyalloc::new_delete_resource_singleton(),
                                  __alloc))
          {
            // new/delete allocator
            typedef _Function_handler_wrapper<_Signature_type, _Functor, false>
              _My_handler;

            _M_make_from_functor<_My_handler>(std::move(__f),
                                              _M_new_delete_alloc_rsrc());
          }
        else
          {
            // Non-new/delete allocator
            typedef _Function_handler_wrapper<_Signature_type, _Functor, true>
              _My_handler;

            _M_make_from_functor<_My_handler>(std::move(__f),
                                              _M_make_shared_alloc(__alloc));
          }
      }

  template<typename _Res, typename... _ArgTypes>
    function<_Res(_ArgTypes...)>::~function()
      {
        // If *this is empty (i.e., cannot be invoked) but is holding on to
        // an allocator, then the allocator is squirreled away in a shared_ptr
        // in _M_functor.  We must destroy it to avoid possibly leaking
        // memory.
        if (_M_empty() && &_M_null_invoker_with_alloc == _M_invoker)
        {
          _M_functor._M_access<_Shared_alloc_rsrc_ptr>().
            ~_Shared_alloc_rsrc_ptr();
          _M_invoker = nullptr;
        }
      }

  template<typename _Res, typename... _ArgTypes>
    _Res
    function<_Res(_ArgTypes...)>::
    operator()(_ArgTypes... __args) const
    {
      if (_M_empty())
	__throw_bad_function_call();
      return _M_invoker(_M_functor, std::forward<_ArgTypes>(__args)...);
    }

#ifdef __GXX_RTTI
  template<typename _Res, typename... _ArgTypes>
    const type_info&
    function<_Res(_ArgTypes...)>::
    target_type() const
    {
      if (_M_manager)
	{
	  _Any_data __typeinfo_result;
	  _M_manager(__typeinfo_result, _M_functor, __get_type_info);
	  return *__typeinfo_result._M_access<const type_info*>();
	}
      else
	return typeid(void);
    }

  template<typename _Res, typename... _ArgTypes>
    template<typename _Functor>
      _Functor*
      function<_Res(_ArgTypes...)>::
      target()
      {
	if (typeid(_Functor) == target_type() && _M_manager)
	  {
	    _Any_data __ptr;
	    if (_M_manager(__ptr, _M_functor, __get_functor_ptr)
		&& !is_const<_Functor>::value)
	      return 0;
	    else
	      return __ptr._M_access<_Functor*>();
	  }
	else
	  return 0;
      }

  template<typename _Res, typename... _ArgTypes>
    template<typename _Functor>
      const _Functor*
      function<_Res(_ArgTypes...)>::
      target() const
      {
	if (typeid(_Functor) == target_type() && _M_manager)
	  {
	    _Any_data __ptr;
	    _M_manager(__ptr, _M_functor, __get_functor_ptr);
	    return __ptr._M_access<const _Functor*>();
	  }
	else
	  return 0;
      }
#endif

  template<typename _Res, typename... _Args>
    inline
    polyalloc::allocator_resource*
    function<_Res(_Args...)>::get_allocator_resource() const
      {
        return _M_get_shared_alloc_rsrc_ptr().get();
      }


  // [20.7.15.2.6] null pointer comparisons

  /**
   *  @brief Compares a polymorphic function object wrapper against 0
   *  (the NULL pointer).
   *  @returns @c true if the wrapper has no target, @c false otherwise
   *
   *  This function will not throw an %exception.
   */
  template<typename _Res, typename... _Args>
    inline bool
    operator==(const function<_Res(_Args...)>& __f, nullptr_t)
    { return !static_cast<bool>(__f); }

  /// @overload
  template<typename _Res, typename... _Args>
    inline bool
    operator==(nullptr_t, const function<_Res(_Args...)>& __f)
    { return !static_cast<bool>(__f); }

  /**
   *  @brief Compares a polymorphic function object wrapper against 0
   *  (the NULL pointer).
   *  @returns @c false if the wrapper has no target, @c true otherwise
   *
   *  This function will not throw an %exception.
   */
  template<typename _Res, typename... _Args>
    inline bool
    operator!=(const function<_Res(_Args...)>& __f, nullptr_t)
    { return static_cast<bool>(__f); }

  /// @overload
  template<typename _Res, typename... _Args>
    inline bool
    operator!=(nullptr_t, const function<_Res(_Args...)>& __f)
    { return static_cast<bool>(__f); }

  // [20.7.15.2.7] specialized algorithms

  /**
   *  @brief Swap the targets of two polymorphic function object wrappers.
   *
   *  This function will not throw an %exception.
   */
  template<typename _Res, typename... _Args>
    inline void
    swap(function<_Res(_Args...)>& __x, function<_Res(_Args...)>& __y)
    { __x.swap(__y); }

_GLIBCXX_END_NAMESPACE_VERSION
END_NAMESPACE_XSTD

#endif // _XFUNCTION

// Local Variables:
// c-basic-offset: 2
// End:
