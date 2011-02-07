/* xstd_list.h                  -*-C++-*-
 *
 *            Copyright 2009 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at 
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef INCLUDED_LIST_DOT_H
#define INCLUDED_LIST_DOT_H

#include <allocator_traits.h>
#include <allocator_util.h>
#include <type_traits>
#include <iterator>
#include <utility>

BEGIN_NAMESPACE_XSTD

template <typename _Tp, typename _Alloc> class list;

namespace __details
{
    template <typename _NodePtr>
    inline void __link_nodes(_NodePtr __prev, _NodePtr __next)
    {
        __prev->_M_next = __next;
        __next->_M_prev = __prev;
    }

    template <typename _NodePtr>
    inline void __insert_node(_NodePtr __node,
                              _NodePtr __prev,
                              _NodePtr __next)
    {
        __link_nodes(__prev, __node);
        __link_nodes(__node, __next);
    }

    template <typename _Tp, typename _VoidPtr>
      struct _List_node
    {
#ifdef TEMPLATE_ALIASES  // template aliases not supported in g++-4.4.1
        typedef rebind<_VoidPtr,_List_node> _NodePtr;
#else
        typedef typename rebinder<_VoidPtr,_List_node>::type _NodePtr;
#endif

        _NodePtr _M_prev;
        _NodePtr _M_next;
        _Tp      _M_value;
        _List_node();                              // Declared but not defined
        _List_node(const _List_node&);             // Declared but not defined
        _List_node& operator=(const _List_node&);  // Declared but not defined
        ~_List_node();                             // Declared but not defined
    };

    
    template <typename _Tp, typename _NodePtr>
    class __list_iterator_base {
    protected:
	template <typename _T, typename _A> friend class list;

        _NodePtr _M_nodeptr;

	explicit __list_iterator_base(_NodePtr __p)
	    : _M_nodeptr(__p) { }
        __list_iterator_base() = default;
        __list_iterator_base(const __list_iterator_base&) = default;
        __list_iterator_base operator=(const __list_iterator_base&) = default;
        ~__list_iterator_base() = default;
    };

    template <typename _Tp, typename _NodePtr, typename _VoidPtr,
              typename _DiffType>
    class __list_iterator :
        public std::iterator<std::bidirectional_iterator_tag,
                             _Tp, _DiffType, _NodePtr,
                             typename std::remove_const<_Tp>::type&>,
        public __list_iterator_base<typename std::remove_const<_Tp>::type,
                                    _NodePtr>
    {
        typedef __list_iterator_base<typename std::remove_const<_Tp>::type,
                                     _NodePtr> _Base;
        typedef __details::_List_node<typename std::remove_const<_Tp>::type, 
                                      _VoidPtr> _Node;

    public:
        __list_iterator() = default;
	explicit __list_iterator(_NodePtr __p)
            : _Base(__p) { }
        __list_iterator(const _Base& other) : _Base(other) { } 
            // Handles conversion from non-const to const iterator.

        // Compiler-generated copy constructor,
        // destructor, and assignment operators:
        // __list_iterator(const __list_iterator&);
        // ~__list_iterator();
        // __list_iterator& operator=(const __list_iterator&);

        _Tp& operator*() const { return this->_M_nodeptr->_M_value; }
        _Tp* operator->() const
	    { return addressof(this->_M_nodeptr->_M_value); }
        __list_iterator& operator++() {
            this->_M_nodeptr = this->_M_nodeptr->_M_next;
            return *this;
        }
        __list_iterator& operator--() {
            this->_M_nodeptr = this->_M_nodeptr->_M_prev;
            return *this;
        }
        __list_iterator operator++(int) {
            __list_iterator __temp = *this;
            this->operator++();
            return __temp;
        }
        __list_iterator operator--(int) {
            __list_iterator __temp = *this;
            this->operator--();
            return __temp;
        }

        friend bool operator==(__list_iterator a, __list_iterator b)
	    { return a._M_nodeptr == b._M_nodeptr; }
    };

    template <typename _Tp, typename _NodePtr, typename _VoidPtr,
              typename _DiffType>
    inline bool operator!=(__list_iterator<_Tp,_NodePtr,_VoidPtr,_DiffType> a,
                           __list_iterator<_Tp,_NodePtr,_VoidPtr,_DiffType> b)
        { return ! (a == b); }
}

template <typename _Tp, typename _Alloc>
class list
{
    typedef typename allocator_traits<_Alloc>::void_pointer _VoidPtr;
    typedef __details::_List_node<_Tp, _VoidPtr>            _Node;

    typedef typename allocator_traits<_Alloc>::template rebind_traits<_Node>
        _AllocTraits;

    typedef typename _AllocTraits::allocator_type  _NodeAlloc;
    typedef typename _AllocTraits::pointer         _NodePtr;
    typedef typename _AllocTraits::difference_type _DiffType;

    _NodePtr _M_tail;
    __details::__allocator_wrapper<_NodeAlloc,
				   typename _AllocTraits::size_type>
        _M_alloc_and_size;

    _NodeAlloc& __allocator() { return _M_alloc_and_size.allocator(); }
    const _NodeAlloc& __allocator() const
	{ return _M_alloc_and_size.allocator(); }

    _NodePtr __head() const { return _M_tail->_M_next; }

    typename _AllocTraits::size_type& __size()
	{ return _M_alloc_and_size.data(); }
    const typename _AllocTraits::size_type& __size() const
	{ return _M_alloc_and_size.data(); }

    // Creates _M_tail node if empty.
    void __create_tail() const;

public:
    // types:
    typedef _Tp& reference;
    typedef const _Tp& const_reference;
    typedef __details::__list_iterator<_Tp,_NodePtr,_VoidPtr,_DiffType>
        iterator; // See 23.2
    typedef __details::__list_iterator<const _Tp,_NodePtr,_VoidPtr,_DiffType>
        const_iterator;
    typedef typename allocator_traits<_Alloc>::pointer       pointer;
    typedef typename allocator_traits<_Alloc>::const_pointer const_pointer;

    typedef typename _AllocTraits::size_type       size_type; // See 23.2
    typedef typename _AllocTraits::difference_type difference_type;// See 23.2
    typedef _Tp                                    value_type;
    typedef _Alloc                                 allocator_type;
    typedef std::reverse_iterator<iterator>        reverse_iterator;
    typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;

    // 23.3.4.1 construct/copy/destroy:
    explicit list(const _Alloc& = _Alloc());
    explicit list(size_type n);
    list(size_type n, const _Tp& value, const _Alloc& = _Alloc());
    template <typename InputIter>
      list(InputIter first, InputIter last, const _Alloc& = _Alloc());
    list(const list& x);
    list(list&& x);
    list(const list&, const _Alloc&);
    list(list&&, const _Alloc&);
//    list(initializer_list<_Tp>, const _Alloc& = _Alloc());
    ~list();

    list& operator=(const list& x);
    list& operator=(list&& x);
//    list& operator=(initializer_list<_Tp>);

    template <typename InputIter>
      void assign(InputIter first, InputIter last);
    void assign(size_type n, const _Tp& t);
//  void assign(initializer_list<_Tp>);

    allocator_type get_allocator() const;

    // iterators:
    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;
    reverse_iterator rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator rend();
    const_reverse_iterator rend() const;
    const_iterator cbegin() const;
    const_iterator cend() const;
    const_reverse_iterator crbegin() const;
    const_reverse_iterator crend() const;

    // 23.3.4.2 capacity:
    bool empty() const;
    size_type size() const;
    size_type max_size() const;
    void resize(size_type sz);
    void resize(size_type sz, const _Tp& c);

    // element access:
    reference front();
    const_reference front() const;
    reference back();
    const_reference back() const;

    // 23.3.4.3 modifiers:
    template <class... Args>
    void emplace_front(Args&&... args);
    void pop_front();
    template <class... Args>
      void emplace_back(Args&&... args);
    void pop_back();
    void push_front(const _Tp& x);
    void push_front(_Tp&& x);
    void push_back(const _Tp& x);
    void push_back(_Tp&& x);

    template <class... Args>
      iterator emplace(const_iterator position, Args&&... args);
    iterator insert(const_iterator position, const _Tp& x);
    iterator insert(const_iterator position, _Tp&& x);
    void insert(const_iterator position, size_type n, const _Tp& x);
    template <typename InputIter>
      void insert(const_iterator position, InputIter first, InputIter last);
//    void insert(const_iterator position, initializer_list<_Tp> il);

    iterator erase(const_iterator position);
    iterator erase(const_iterator position, const_iterator last);

    void swap(list&&);
    void clear();

    // 23.3.4.4 list operations:
    void splice(const_iterator position, list&& x);
    void splice(const_iterator position, list&& x, const_iterator i);
    void splice(const_iterator position, list&& x,
                const_iterator first, const_iterator last);

    void remove(const _Tp& value);
    template <typename Pred> void remove_if(Pred pred);
    void unique();
    template <typename EqPredicate>
    void unique(EqPredicate binary_pred);
    void merge(list&& x);
    template <typename Compare>
      void merge(list&& x, Compare comp);
    void sort();
    template <typename Compare>
      void sort(Compare comp);
    void reverse();
};

template <typename _Tp, class _Alloc> inline
  bool operator==(const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y);
template <typename _Tp, class _Alloc> inline
  bool operator< (const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y);
template <typename _Tp, class _Alloc> inline
  bool operator!=(const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y);
template <typename _Tp, class _Alloc> inline
  bool operator> (const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y);
template <typename _Tp, class _Alloc> inline
  bool operator>=(const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y);
template <typename _Tp, class _Alloc> inline
  bool operator<=(const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y);

// specialized algorithms:
template <typename _Tp, class _Alloc>
  void swap(list<_Tp,_Alloc>& x, list<_Tp,_Alloc>& y);
template <typename _Tp, class _Alloc>
  void swap(list<_Tp,_Alloc>&& x, list<_Tp,_Alloc>& y);
template <typename _Tp, class _Alloc>
  void swap(list<_Tp,_Alloc>& x, list<_Tp,_Alloc>&& y);

///////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
///////////////////////////////////////////////////////////////////////////////

template <typename _Tp, typename _Alloc>
inline void list<_Tp,_Alloc>::__create_tail() const
{
    list *const self = const_cast<list*>(this);
    self->_M_tail = _AllocTraits::allocate(self->__allocator(), 1);
    __link_nodes(_M_tail, _M_tail);  // circular
}

// 23.3.4.1 construct/copy/destroy:
template <typename _Tp, typename _Alloc>
list<_Tp,_Alloc>::list(const _Alloc& a)
    : _M_alloc_and_size(a, 0) { __create_tail(); }

template <typename _Tp, typename _Alloc>
list<_Tp,_Alloc>::list(size_type n)
    : _M_alloc_and_size(_Alloc(), 0)
{
    __create_tail();
    assign(n, _Tp());
}

template <typename _Tp, typename _Alloc>
list<_Tp,_Alloc>::list(size_type n, const _Tp& value, const _Alloc& a)
    : _M_alloc_and_size(a, 0)
{
    __create_tail();
    assign(n, value);
}

template <typename _Tp, typename _Alloc>
  template <typename InputIter>
    list<_Tp,_Alloc>::list(InputIter first, InputIter last, const _Alloc& a)
	: _M_alloc_and_size(a, 0)
{
    __create_tail();
    assign(first, last);
}

template <typename _Tp, typename _Alloc>
list<_Tp,_Alloc>::list(const list& x)
    : _M_alloc_and_size(
       _AllocTraits::select_on_container_copy_construction(x.__allocator()), 0)
{
    __create_tail();
    assign(x.begin(), x.end());
}

template <typename _Tp, typename _Alloc>
list<_Tp,_Alloc>::list(list&& x)
    : _M_alloc_and_size(std::move(x.__allocator()), x.__size())
{
    _M_tail = x._M_tail;
    x.__create_tail();
    x.__size() = 0;
}

template <typename _Tp, typename _Alloc>
list<_Tp,_Alloc>::list(const list& x, const _Alloc& a)
    : _M_alloc_and_size(a, 0)
{
    __create_tail();
    assign(x.begin(), x.end());
}

template <typename _Tp, typename _Alloc>
list<_Tp,_Alloc>::list(list&& x, const _Alloc& a)
    : _M_alloc_and_size(a, 0)
{
    if (a == x.__allocator()) {
        _M_tail = x._M_tail;
        __size() = x.__size();
        x.__create_tail();
        x.__size() = 0;
    }
    else {
        __create_tail();
        assign(x.begin(), x.end());
    }
}

//    list(initializer_list<_Tp>, const _Alloc& = _Alloc());

template <typename _Tp, typename _Alloc>
list<_Tp,_Alloc>::~list()
{
    clear();
    _AllocTraits::deallocate(__allocator(), _M_tail, 1);
}

template <typename _Tp, typename _Alloc>
list<_Tp,_Alloc>& list<_Tp,_Alloc>::operator=(const list& x)
{
    if (this == &x)
        return *this;

    clear();
    typename _AllocTraits::allocator_type oldAlloc = __allocator();
    if (_AllocTraits::on_container_copy_assignment(__allocator(),
                                                   x.__allocator())) {
        // Allocator changed
        _AllocTraits::deallocate(oldAlloc, _M_tail, 1);
        __create_tail();
    }
    assign(x.begin(), x.end());
    return *this;
}

template <typename _Tp, typename _Alloc>
list<_Tp,_Alloc>& list<_Tp,_Alloc>::operator=(list&& x)
{
    if (this == &x)
	return *this;

    clear();
    typename _AllocTraits::allocator_type oldAlloc = __allocator();
    if (__allocator() == x.__allocator())
    {
        // Equal allocators, just swap contents
	using std::swap;
	swap(this->_M_tail, x._M_tail);
	swap(this->__size(), x.__size());
    }
    else if (_AllocTraits::on_container_move_assignment(__allocator(),
                                                   std::move(x.__allocator())))
    {
	// Allocator was moved.  Move x members to this.
        _AllocTraits::deallocate(oldAlloc, _M_tail, 1);
        _M_tail = x._M_tail;
        __size() = x.__size();

        x.__size() == 0;
        x.__create_tail();
    }
    else
    {
	// Unequal allocators and no moving of allocators, do linear copy
	assign(x.begin(), x.end());
    }

    return *this;
}

//    list& operator=(initializer_list<_Tp>);

template <typename _Tp, typename _Alloc>
  template <typename InputIter>
    void list<_Tp,_Alloc>::assign(InputIter first, InputIter last)
{
    clear();
    for (; first != last; ++first)
	push_back(*first);
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::assign(size_type n, const _Tp& t)
{
    for (int i = 0; i < n; ++i)
	push_back(t);
}

//  void assign(initializer_list<_Tp>);

template <typename _Tp, typename _Alloc>
_Alloc list<_Tp,_Alloc>::get_allocator() const
{
    return __allocator();
}

// iterators:
template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::iterator list<_Tp,_Alloc>::begin()
{
    return iterator(__head());
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::const_iterator list<_Tp,_Alloc>::begin() const
{
    return const_iterator(__head());
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::iterator list<_Tp,_Alloc>::end()
{
    return iterator(_M_tail);
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::const_iterator list<_Tp,_Alloc>::end() const
{
    return const_iterator(_M_tail);
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::reverse_iterator list<_Tp,_Alloc>::rbegin()
{
    return reverse_iterator(end());
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::const_reverse_iterator
list<_Tp,_Alloc>::rbegin() const
{
    return const_reverse_iterator(end());
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::reverse_iterator list<_Tp,_Alloc>::rend()
{
    return reverse_iterator(begin());
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::const_reverse_iterator list<_Tp,_Alloc>::rend() const
{
    return const_reverse_iterator(begin());
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::const_iterator list<_Tp,_Alloc>::cbegin() const
{
    return begin();
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::const_iterator list<_Tp,_Alloc>::cend() const
{
    return end();
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::const_reverse_iterator
list<_Tp,_Alloc>::crbegin() const
{
    return rbegin();
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::const_reverse_iterator
list<_Tp,_Alloc>::crend() const
{
    return rend();
}

// 23.3.4.2 capacity:
template <typename _Tp, typename _Alloc>
bool list<_Tp,_Alloc>::empty() const
{
    return 0 == __size();
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::size_type list<_Tp,_Alloc>::size() const
{
    return __size();
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::size_type list<_Tp,_Alloc>::max_size() const
{
    return _AllocTraits::max_size(__allocator());
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::resize(size_type sz)
{
    if (sz > size()) {
        emplace_back();
        _Tp& c = back();
        while (sz > size())
            push_back(c);
    }
    else {
        while (sz < size())
            pop_back();
    }
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::resize(size_type sz, const _Tp& c)
{
    while (sz > size())
        push_back(c);
    while (sz < size())
        pop_back();
}

    // element access:
template <typename _Tp, typename _Alloc>
_Tp& list<_Tp,_Alloc>::front()
{
    return __head()->_M_value;
}

template <typename _Tp, typename _Alloc>
const _Tp& list<_Tp,_Alloc>::front() const
{
    return __head()->_M_value;
}

template <typename _Tp, typename _Alloc>
_Tp& list<_Tp,_Alloc>::back()
{
    _NodePtr __last = _M_tail->_M_prev;
    return __last->_M_value;
}

template <typename _Tp, typename _Alloc>
const _Tp& list<_Tp,_Alloc>::back() const
{
    _NodePtr __last = _M_tail->_M_prev;
    return __last->_M_value;
}

// 23.3.4.3 modifiers:
template <typename _Tp, typename _Alloc>
  template <class... Args>
    void list<_Tp,_Alloc>::emplace_front(Args&&... args)
{
    emplace(begin(), std::forward<Args>(args)...);
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::pop_front()
{
    erase(begin());
}

template <typename _Tp, typename _Alloc>
  template <class... Args>
    void list<_Tp,_Alloc>::emplace_back(Args&&... args)
{
    emplace(end(), std::forward<Args>(args)...);
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::pop_back()
{
    erase(--end());
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::push_front(const _Tp& x)
{
    emplace(begin(), x);
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::push_front(_Tp&& x)
{
    emplace(begin(), std::move(x));
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::push_back(const _Tp& x)
{
    emplace(end(), x);
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::push_back(_Tp&& x)
{
    emplace(end(), std::move(x));
}

template <typename _Tp, typename _Alloc>
  template <class... Args>
    typename list<_Tp,_Alloc>::iterator
    list<_Tp,_Alloc>::emplace(const_iterator position, Args&&... args)
{
    _NodePtr p = _AllocTraits::allocate(__allocator(), 1, _M_tail);
    try {
        _AllocTraits::construct(__allocator(), addressof(p->_M_value),
                                std::forward<Args>(args)...);
    }
    catch (...) {
        _AllocTraits::deallocate(__allocator(), p, 1);
        throw;
    }

    typename _AllocTraits::pointer __prev = position._M_nodeptr->_M_prev;
    __insert_node(p, __prev, position._M_nodeptr);
		  
    ++__size();
    return iterator(p);
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::iterator
list<_Tp,_Alloc>::insert(const_iterator position, const _Tp& x)
{
    return emplace(position, x);
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::iterator
list<_Tp,_Alloc>::insert(const_iterator position, _Tp&& x)
{
    return emplace(position, std::move(x));
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::insert(const_iterator position,
                              size_type n, const _Tp& x)
{
    for (; n > 0; --n)
        emplace(position, x);
}

template <typename _Tp, typename _Alloc>
    template <typename InputIter>
      void list<_Tp,_Alloc>::insert(const_iterator position,
                                    InputIter first, InputIter last)
{
    for (; first != last; ++first)
        insert(position, *first);
}

// template <typename _Tp, typename _Alloc>
//     void list<_Tp,_Alloc>::insert(const_iterator position, initializer_list<_Tp> il);

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::iterator
list<_Tp,_Alloc>::erase(const_iterator position)
{
    typename _AllocTraits::pointer p = position._M_nodeptr;

    __link_nodes(p->_M_prev, p->_M_next);
    _AllocTraits::destroy(__allocator(), addressof(p->_M_value));
    _AllocTraits::deallocate(__allocator(), p, 1);
    --__size();
}

template <typename _Tp, typename _Alloc>
typename list<_Tp,_Alloc>::iterator
list<_Tp,_Alloc>::erase(const_iterator position, const_iterator last)
{
    while (position != last) {
        const_iterator curr = position;
        ++position;
        erase(curr);
    }
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::swap(list&& x)
{
    if (! _AllocTraits::on_container_swap(__allocator(), x.__allocator())) {
        // assert(__allocator() == x.__allocator());
    }

    using std::swap;
    swap(_M_tail, x._M_tail);
    swap(__size(), x.__size());
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::clear()
{
    erase(begin(), end());
}

// 23.3.4.4 list operations:
template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::splice(const_iterator position, list&& x)
{
    // assert(__allocator() == x.__allocator());
    if (x.empty())
        return;

    typename _AllocTraits::pointer __pos = position._M_nodeptr;
    typename _AllocTraits::pointer __first = x.__head();
    typename _AllocTraits::pointer __last  = x._M_tail->_M_prev;
    size_type n = x.__size();
    
    // Splice contents out of x.
    __link_nodes(x._M_tail, x._M_tail);
    x.__size() = 0;

    // Splice contents into *this.
    __link_nodes(__pos->_M_prev, __first);
    __link_nodes(__last, __pos);
    __size() += n;
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::splice(const_iterator position, list&& x,
                              const_iterator i)
{
    // assert(__allocator() == x.__allocator());
    typename _AllocTraits::pointer __pos = position._M_nodeptr;
    typename _AllocTraits::pointer __i   = i._M_nodeptr;
    typename _AllocTraits::pointer __inext = __i->_M_next;

    if (__pos == __i || __pos == __i->_M_next)
        return;  // Do nothing

    // Splice contents out of x.
    __link_nodes(__i->_M_prev, __inext);
    --x.__size();

    // Splice contents into *this.
    __link_nodes(__pos->_M_prev, __i);
    __link_nodes(__i, __pos);
    ++__size();
}

template <typename _Tp, typename _Alloc>
void list<_Tp,_Alloc>::splice(const_iterator position, list&& x,
                              const_iterator first, const_iterator last)
{
    // assert(__allocator() == x.__allocator());
    size_type n = std::distance(first, last);

    if (0 == n)
        return;

    typename _AllocTraits::pointer __pos   = position._M_nodeptr;
    typename _AllocTraits::pointer __first = first._M_nodeptr;
    typename _AllocTraits::pointer __next  = last._M_nodeptr;
    typename _AllocTraits::pointer __last  = last._M_nodeptr->_M_prev;

    // Splice contents out of x.
    __link_nodes(__first->_M_prev, __next);
    x.__size() -= n;
        
    // Splice contents into *this.
    __link_nodes(__pos->_M_prev, __first);
    __link_nodes(__last, __pos);
    __size() += n;
}

#if 0 // TBD
template <typename _Tp, typename _Alloc>
    void list<_Tp,_Alloc>::remove(const _Tp& value);
template <typename _Tp, typename _Alloc>
    template <typename Pred> void remove_if(Pred pred);
    void list<_Tp,_Alloc>::unique();
template <typename _Tp, typename _Alloc>
    template <typename EqPredicate>
    void list<_Tp,_Alloc>::unique(EqPredicate binary_pred);
template <typename _Tp, typename _Alloc>
    void list<_Tp,_Alloc>::merge(list&& x);
template <typename _Tp, typename _Alloc>
    template <typename Compare>
      void list<_Tp,_Alloc>::merge(list&& x, Compare comp);
template <typename _Tp, typename _Alloc>
    void list<_Tp,_Alloc>::sort();
template <typename _Tp, typename _Alloc>
    template <typename Compare>
      void list<_Tp,_Alloc>::sort(Compare comp);
template <typename _Tp, typename _Alloc>
    void list<_Tp,_Alloc>::reverse();
#endif // TBD

template <typename _Tp, class _Alloc> inline
  bool operator==(const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y) {
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
  }
template <typename _Tp, class _Alloc> inline
  bool operator!=(const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y)
    { return ! (x == y); }

template <typename _Tp, class _Alloc> inline
  bool operator< (const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y) {
    return std::lexicographical_compare(x.begin(), x.end(),
                                        y.begin(), y.end());
  }

template <typename _Tp, class _Alloc> inline
  bool operator> (const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y)
    { return y < x; }

template <typename _Tp, class _Alloc> inline
  bool operator>=(const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y)
    { return ! (x < y); }

template <typename _Tp, class _Alloc> inline
  bool operator<=(const list<_Tp,_Alloc>& x, const list<_Tp,_Alloc>& y)
    { return ! (y < x); }

// specialized algorithms:
template <typename _Tp, class _Alloc>
void swap(list<_Tp,_Alloc>& x, list<_Tp,_Alloc>& y)
{
    x.swap(std::move(y));
}

template <typename _Tp, class _Alloc>
void swap(list<_Tp,_Alloc>&& x, list<_Tp,_Alloc>& y)
{
    y.swap(x);
}

template <typename _Tp, class _Alloc>
void swap(list<_Tp,_Alloc>& x, list<_Tp,_Alloc>&& y)
{
    x.swap(y);
}

END_NAMESPACE_XSTD

///////////////////////////////////////////////////////////////////////////////
// Implementation
///////////////////////////////////////////////////////////////////////////////

#endif // ! defined(INCLUDED_LIST_DOT_H)
