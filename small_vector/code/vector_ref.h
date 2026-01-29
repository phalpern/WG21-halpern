/* vector_ref.h                                                       -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_VECTOR_REF
#define INCLUDED_VECTOR_REF

#include <iterator>

namespace xstd
{

using namespace std;

template <class T>
class vector_ref
{
public:
  // types

  /// Common implementation details of vector-like types. Any type that can be
  /// proxied by a `vector_ref` must be able to express its current data using
  /// this `struct` type.
  struct data_rep
  {
    size_t size     = 0;
    size_t capacity = 0;
    T*     data     = nullptr;
  };

  /// Reference type for a function that returns a new data block for the
  /// vector-like object pointed to by `vp`. Return a `data_rep` having a
  /// `capacity >= mincap`, and a `data` member pointing to the allocated
  /// block. If the vector-like object's current capacity is already at least
  /// `mincap`, the referenced function may return a `data_rep` unchanged from
  /// the previous one, in which case the return value's `size` is the current
  /// container size, otherwise the return value's `size` is 0. The referenced
  /// function is allowed to throw and may use the `maxcap_hint` (which might
  /// be smaller than the current capacity, i.e., during `shrink_to_fit`) to
  /// choose a maximum capacity.
  using new_rep_func           = data_rep (&)(void*       vp,
                                              std::size_t mincap,
                                              std::size_t maxcap_hint);

  /// Reference type for a function that replaces the data representation
  /// within a vector-like object pointed to by `vp` to match `new_rep`. If
  /// `old_rep.data` is not null, it should be deallocated appropriately for
  /// the vector-like object. If `new_rep.data` is null, make no changes to the
  /// container rep. Returns the new representation (which is expected to be
  /// `new_rep`, but that might change in the future). The referenced function
  /// throws nothing. Precondition: `old_rep.size == 0`.
  using replace_rep_func       = data_rep (&)(void*    vp,
                                              data_rep old_rep,
                                              data_rep new_rep);

  using value_type             = T;
  using pointer                = T*;
  using const_pointer          = T const *;
  using reference              = value_type&;
  using const_reference        = const value_type&;
  using size_type              = size_t;
  using difference_type        = ptrdiff_t;
  using iterator               = T*;        // TBD: Wrap this
  using const_iterator         = T const*;  // TBD: Wrap this
  using reverse_iterator       = reverse_iterator<iterator>;
  using const_reverse_iterator = reverse_iterator<const_iterator>;

  // Construct/copy/destroy

  /// Construct a `vector_ref` from a vector-like object `v`, providing an
  /// initial representation, a maximum size for growing the vector, and
  /// functions for creating and replacing the representation.
  template <class VecLike>
  constexpr vector_ref(VecLike& v, data_rep rep, std::size_t max_size
                       new_rep_func new_rep_f, replace_rep_func replace_rep_f);

  constexpr ~vector_ref() = default;

  /// Move only
  vector_ref(const vector_ref&) = delete;
  constexpr vector_ref(vector_ref&& other);

  /// Copy or move to/from proxied vector-like type
  constexpr vector_ref& operator=(const vector_ref& rhs);
  constexpr vector_ref& operator=(vector_ref&&      rhs);  // might throw

  constexpr vector_ref& operator=(initializer_list<T>);

  template<class InputIterator>
  constexpr void assign(InputIterator first, InputIterator last);

  template <class R>
  constexpr void assign_range(R&& rg);

  constexpr void assign(size_type n, const T& u);
  constexpr void assign(initializer_list<T>);

  // iterators
  constexpr iterator begin() noexcept;
  constexpr const_iterator begin() const noexcept;
  constexpr iterator end() noexcept;
  constexpr const_iterator end() const noexcept;
  constexpr reverse_iterator rbegin() noexcept;
  constexpr const_reverse_iterator rbegin() const noexcept;
  constexpr reverse_iterator rend() noexcept;
  constexpr const_reverse_iterator rend() const noexcept;
  constexpr const_iterator cbegin() const noexcept;
  constexpr const_iterator cend() const noexcept;
  constexpr const_reverse_iterator crbegin() const noexcept;
  constexpr const_reverse_iterator crend() const noexcept;

  // capacity
  constexpr bool empty() const noexcept;
  constexpr size_type size() const noexcept;
  constexpr size_type max_size() const noexcept;
  constexpr size_type capacity() const noexcept;
  constexpr void resize(size_type sz);
  constexpr void resize(size_type sz, const T& c);
  constexpr void reserve(size_type n);
  constexpr void shrink_to_fit();

  // element access
  constexpr reference operator[](size_type n);
  constexpr const_reference operator[](size_type n) const;
  constexpr reference at(size_type n);
  constexpr const_reference at(size_type n) const;
  constexpr reference front();
  constexpr const_reference front() const;
  constexpr reference back();
  constexpr const_reference back() const;

  // data access
  constexpr T* data() noexcept;
  constexpr const T* data() const noexcept;

  // modifiers
  template<class... Args> constexpr reference emplace_back(Args&&... args);
  constexpr void push_back(const T& x);
  constexpr void push_back(T&& x);
  template<container-compatible-range <T> R>
  constexpr void append_range(R&& rg);
  constexpr void pop_back();
  template<class... Args>
  constexpr iterator emplace(const_iterator position, Args&&... args);
  constexpr iterator insert(const_iterator position, const T& x);
  constexpr iterator insert(const_iterator position, T&& x);
  constexpr iterator insert(const_iterator position, size_type n, const T& x);
  template<class InputIterator>
  constexpr iterator insert(const_iterator position,
                            InputIterator first, InputIterator last);
  template<class R>
  constexpr iterator insert_range(const_iterator position, R&& rg);
  constexpr iterator insert(const_iterator position, initializer_list<T> il);
  constexpr iterator erase(const_iterator position);
  constexpr iterator erase(const_iterator first, const_iterator last);
  constexpr void swap(vector&)
    noexcept(allocator_traits<Allocator>::propagate_on_container_swap::value ||
             allocator_traits<Allocator>::is_always_equal::value);
  constexpr void clear() noexcept;

private:
  void*        const m_veclike_p;
  data_rep           m_rep;
  std::size_t  const m_max_size;
  new_rep_func       m_new_rep_func;
  replace_rep_func   m_replace_rep_func;

  /// Commit changes from `r`, allocate a larger block, and move elements from
  /// `r` into the new block, then deallocate `r` and return the new block.  If
  /// an exception is thrown, the original value of `r` remains valid, but some
  /// of the elements in `r.data` might be left a moved-from state. On
  /// successful return, the original value of `r` becomes invalid.
  data_rep grow(data_rep r);
};

// Private implementation
template<class T>
data_rep vector_ref<T>::grow(data_rep r)
{
  // Commit the changes from `r`
  m_replace_rep(m_veclike_p, r.data == m_rep.data ? data_rep{} : m_rep, r);
  auto ret = m_new_rep(m_veclike_p, r.capacity + 1, m_maxsize);
  if constexpr (is_nothrow_relocatable_v<T>) {
    relocate(r.data, r.data + r.size, ret);
    ret.size = r.size;
  }
  else {
    try {
      for (std::size i = 0; i < r.size; ++i) {
        m_construct(m_veclike_p, &ret.data[i], std::move(r.data[i]));
        ret.size = i;
      }
    }
    catch (...) {
      std::destroy(ret.data, ret.data + ret.size);
      m_replace_rep(m_veclike_p, ret, r);
      throw;
    }
  }
  return ret;
}

// Implementation
// Construct/copy/destroy

/// Construct a `vector_ref` from a vector-like object `v`, providing an
/// initial representation and a maximum size for growing the vector.
template<class T>
  template <class VecLike>
constexpr vector_ref<T>::vector_ref(VecLike&     v,
                                    data_rep     rep,
                                    std::size_t  max_size
                                    new_rep_func new_rep_f,
                                    replace_rep_func replace_rep_f)
  : m_veclike_p(&v)
  , m_rep(rep)
  , m_max_size(max_size)
  , m_new_rep_func(new_rep_f)
  , m_replace_rep_func(replace_rep_f)
{
}

template<class T>
constexpr vector_ref<T>::vector_ref(vector_ref&& other)
  : m_veclike_p(other.m_veclike_p)
  , m_rep(other.m_rep)
  , m_max_size(other.m_max_size)
  , m_new_rep_func(other.m_new_rep_func)
  , m_replace_rep_func(other.m_replace_rep_func)
{
  other.m_veclike_p        = nullptr;
  other.m_rep              = data_rep{};
  other.m_max_size         = 0;
  other.m_new_rep_func     = nullptr;
  other.m_replace_rep_func = nullptr;
}

/// Copy or move to/from proxied vector-like type
template<class T>
constexpr vector_ref& vector_ref<T>::operator=(const vector_ref& rhs)
{


  return *this;
}

template<class T>
constexpr vector_ref& vector_ref<T>::operator=(vector_ref&&      rhs);  // might throw

template<class T>
constexpr vector_ref& vector_ref<T>::operator=(initializer_list<T>);

template<class T>
  template<class InputIterator>
constexpr void vector_ref<T>::assign(InputIterator first, InputIterator last)
{
  data_rep old_rep;  // starts out null
  data_rep work_rep = m_rep;

  if constexpr (forward_iterator<InputIterator>)
  {
    auto d = distance(first, last);
    if (d > m_max_size)
      throw std::size_error{};
    std::size_t n = static_cast<std::size_t>(d);

    if (n > capacity())
    {
      // Allocate new array and copy-append [first, last) into it.
      data_rep work_rep = m_new_rep(m_veclike_p, n, m_max_size);

      // If allocation succeeded, delete old data.
      old_rep = m_rep;
      std::destroy(old_rep.data, old_rep.data + old_rep.size());
      old_rep.size = 0;
    }
  }

  try {
    // Assign to existing elements, until we run out of either existing
    // elements or input elements.
    std::size_t i = 0;
    for ( ; first != last && i < m_rep.size; ++i)
      m_rep.data[i] = *first++;

    // If ran out of input elements, destroy any remaining existing elements.
    if (i < work_rep.size)
    {
      std::destroy(work_rep.data + i, work_rep.data + work_rep.size);
      work_rep.size = i;
    }

    // If there is any more input, copy-append it.
    while (first != last)
    {
      if constexpr (! forward_iterator<InputIterator>) {
        // Only if we we have an input-only iterator (not a forward iterator)
        // can we run out of capacity here.
        if (work_rep.size == work_rep.capacity)
          work_rep = grow(work_rep);
      }
      m_construct(m_veclike, &work_rep.data[work_rep.size], *first++);
      ++work_rep.size;
    }

  }
  catch (...) {
    m_rep = work_rep;
    m_replace_rep(m_veclike, old_rep, work_rep);
    throw;
  }

  m_rep = work_rep;
  m_replace_rep(m_veclike, old_rep, work_rep);
}

template<class T>
  template <class R>
constexpr void vector_ref<T>::assign_range(R&& rg);

template<class T>
constexpr void vector_ref<T>::assign(size_type n, const T& u);
template<class T>
constexpr void vector_ref<T>::assign(initializer_list<T>);

// iterators
template<class T>
constexpr iterator vector_ref<T>::begin() noexcept;
template<class T>
constexpr const_iterator vector_ref<T>::begin() const noexcept;
template<class T>
constexpr iterator vector_ref<T>::end() noexcept;
template<class T>
constexpr const_iterator vector_ref<T>::end() const noexcept;
template<class T>
constexpr reverse_iterator vector_ref<T>::rbegin() noexcept;
template<class T>
constexpr const_reverse_iterator vector_ref<T>::rbegin() const noexcept;
template<class T>
constexpr reverse_iterator vector_ref<T>::rend() noexcept;
template<class T>
constexpr const_reverse_iterator vector_ref<T>::rend() const noexcept;
template<class T>
constexpr const_iterator vector_ref<T>::cbegin() const noexcept;
template<class T>
constexpr const_iterator vector_ref<T>::cend() const noexcept;
template<class T>
constexpr const_reverse_iterator vector_ref<T>::crbegin() const noexcept;
template<class T>
constexpr const_reverse_iterator vector_ref<T>::crend() const noexcept;

// capacity
template<class T>
constexpr bool vector_ref<T>::empty() const noexcept;
template<class T>
constexpr size_type vector_ref<T>::size() const noexcept;
template<class T>
constexpr size_type vector_ref<T>::max_size() const noexcept;
template<class T>
constexpr size_type vector_ref<T>::capacity() const noexcept;
template<class T>
constexpr void vector_ref<T>::resize(size_type sz);
template<class T>
constexpr void vector_ref<T>::resize(size_type sz, const T& c);
template<class T>
constexpr void vector_ref<T>::reserve(size_type n);
template<class T>
constexpr void vector_ref<T>::shrink_to_fit();

// element access
template<class T>
constexpr reference vector_ref<T>::operator[](size_type n);
template<class T>
constexpr const_reference vector_ref<T>::operator[](size_type n) const;
template<class T>
constexpr reference vector_ref<T>::at(size_type n);
template<class T>
constexpr const_reference vector_ref<T>::at(size_type n) const;
template<class T>
constexpr reference vector_ref<T>::front();
template<class T>
constexpr const_reference vector_ref<T>::front() const;
template<class T>
constexpr reference vector_ref<T>::back();
template<class T>
constexpr const_reference vector_ref<T>::back() const;

// data access
template<class T>
constexpr T* vector_ref<T>::data() noexcept;
template<class T>
constexpr const T* vector_ref<T>::data() const noexcept;

// modifiers
template<class T>
  template<class... Args> constexpr reference vector_ref<T>::emplace_back(Args&&... args);
template<class T>
constexpr void vector_ref<T>::push_back(const T& x);
template<class T>
constexpr void vector_ref<T>::push_back(T&& x);
template<class T>
  template<container-compatible-range <T> R>
template<class T>
constexpr void vector_ref<T>::append_range(R&& rg);
template<class T>
constexpr void vector_ref<T>::pop_back();
template<class T>
  template<class... Args>
constexpr iterator vector_ref<T>::emplace(const_iterator position, Args&&... args);
template<class T>
constexpr iterator vector_ref<T>::insert(const_iterator position, const T& x);
template<class T>
constexpr iterator vector_ref<T>::insert(const_iterator position, T&& x);
template<class T>
constexpr iterator vector_ref<T>::insert(const_iterator position, size_type n, const T& x);
template<class T>
  template<class InputIterator>
constexpr iterator vector_ref<T>::insert(const_iterator position,
                          InputIterator first, InputIterator last);
template<class T>
  template<class R>
constexpr iterator vector_ref<T>::insert_range(const_iterator position, R&& rg);
template<class T>
constexpr iterator vector_ref<T>::insert(const_iterator position, initializer_list<T> il);
template<class T>
constexpr iterator vector_ref<T>::erase(const_iterator position);
template<class T>
constexpr iterator vector_ref<T>::erase(const_iterator first, const_iterator last);
template<class T>
constexpr void vector_ref<T>::swap(vector&)
  noexcept(allocator_traits<Allocator>::propagate_on_container_swap::value ||
           allocator_traits<Allocator>::is_always_equal::value);
template<class T>
constexpr void vector_ref<T>::clear() noexcept;

}

#endif // ! defined(INCLUDED_VECTOR_REF)

// Local Variables:
// c-basic-offset: 2
// End:
