// -*- c++ -*-

// Define zero or one of the macros OPTION_1, OPTION_2, or OPTION_3
// NON_AA:   inplace_vector<class T, size_t N>
// OPTION_0: inplace_vector<class T, size_t N, class Alloc = allocator<T>>
// OPTION_1: inplace_vector<class T, size_t N, class Alloc = void>
// OPTION_2: inplace_vector<class T, size_t N>  (allocator is deduced)
// OPTION_3: inplace_vector<class T, size_t N, class Alloc = deduced>

#include <array>
#include <type_traits>
#include <memory>

#ifdef VERBOSE
#include <iostream>
#endif

#define USE_CONCEPTS

// namespace std::experimental
// {

// struct no_allocator
// {
//   friend constexpr bool operator==(const no_allocator&, const no_allocator&)
//     { return true; }
// };

// } // Close namespace std::experimental

// namespace std
// {

// template <class T>
// struct uses_allocator<T, experimental::no_allocator> : false_type { };

// } // Close namespace std

namespace std::experimental
{

// exposition only trait `is-nothrow-ua-constructible-v`
template <class T, class... Y>
consteval bool ____is_nothrow_ua_constructible_check(tuple<Y...>&&)
{
  return is_nothrow_constructible_v<T, Y...>;
}

template <class T, class A, class... X>
constexpr bool __is_nothrow_ua_constructible_v =
  ____is_nothrow_ua_constructible_check<T>(
    uses_allocator_construction_args<T>(declval<const A&>(), declval<X>()...));

// non-allocator that meets the allocator requirements
template <class T>
struct __non_allocator
{
  using value_type = T;

  consteval T* allocate(std::size_t) { return nullptr; }
  consteval void deallocate(T*, std::size_t) { }
};

#if defined(OPTION_0) // Default allocator = std::allocator<T>

// Base class for `inplace_vector`, handling allocator use.
template <class T, class Alloc>
class __inplace_vector_base
{
  [[no_unique_address]] Alloc m_alloc;

protected:
  using AllocArg    = Alloc;
  using AllocTraits = allocator_traits<Alloc>;

#if VERBOSE
  __inplace_vector_base() { std::cout << "Option 1\n"; }
  explicit __inplace_vector_base(const AllocArg& a) : m_alloc(a)
    { std::cout << "Option 1\n"; }
#else
  constexpr __inplace_vector_base() = default;
  constexpr explicit __inplace_vector_base(const AllocArg& a) : m_alloc(a) { }
#endif

  template <class... Args>
  constexpr void construct_elem(T* elem, Args&&... args)
  {
    if constexpr (uses_allocator_v<T, Alloc>)
      uninitialized_construct_using_allocator(elem, m_alloc,
                                              std::forward<Args>(args)...);
    else
      construct_at(elem, std::forward<Args>(args)...);
  }

public:
  using allocator_type = Alloc;

  constexpr allocator_type get_allocator() const { return m_alloc; }
};

// Specialization for default allocator. Note that this base class is trivial
// even if `std::allocator` is not trivial (except in VERBOSE builds).
template <class T>
class __inplace_vector_base<T, allocator<T>>
{
protected:
  using AllocArg    = allocator<T>;
  using AllocTraits = allocator_traits<AllocArg>;

#if VERBOSE
  __inplace_vector_base() { std::cout << "Option 1 std::allocator\n"; }
  explicit __inplace_vector_base(const const AllocArg&&)
    { std::cout << "Option 1 std::allocator\n"; }
#else
  constexpr __inplace_vector_base() = default;
  constexpr explicit __inplace_vector_base(const AllocArg&) { }
#endif

  template <class... Args>
  constexpr void construct_elem(T* elem, Args&&... args)
    { construct_at(elem, std::forward<Args>(args)...); }

public:
  using allocator_type = allocator<T>;

  constexpr allocator_type get_allocator() const { return allocator<T>{}; }
};

#elif defined(OPTION_1) // Default allocator = void

// Base class for `inplace_vector`, handling allocator use.
template <class T, class Alloc>
class __inplace_vector_base
{
  [[no_unique_address]] Alloc m_alloc;

protected:
  using AllocArg    = Alloc;
  using AllocTraits = allocator_traits<Alloc>;

#if VERBOSE
  __inplace_vector_base() { std::cout << "Option 1\n"; }
  explicit __inplace_vector_base(const AllocArg& a) : m_alloc(a)
    { std::cout << "Option 1\n"; }
#else
  constexpr __inplace_vector_base() = default;
  constexpr explicit __inplace_vector_base(const AllocArg& a) : m_alloc(a) { }
#endif

  template <class... Args>
  constexpr void construct_elem(T* elem, Args&&... args)
  {
    if constexpr (uses_allocator_v<T, Alloc>)
      uninitialized_construct_using_allocator(elem, m_alloc, std::forward<Args>(args)...);
    else
      construct_at(elem, std::forward<Args>(args)...);
  }

public:
  using allocator_type = Alloc;

  constexpr allocator_type get_allocator() const { return m_alloc; }
};

// Specialization for `void` allocator
template <class T>
class __inplace_vector_base<T, void>
{
protected:
  using AllocArg    = __non_allocator<T>;
  using AllocTraits = allocator_traits<AllocArg>;

#if VERBOSE
  __inplace_vector_base() { std::cout << "Option 1 no allocator\n"; }
  explicit __inplace_vector_base(const AllocArg&) { std::cout << "Option 1 no allocator\n"; }
#else
  constexpr __inplace_vector_base() = default;
  constexpr explicit __inplace_vector_base(const AllocArg&) { }
#endif

  template <class... Args>
  constexpr void construct_elem(T* elem, Args&&... args)
    { construct_at(elem, std::forward<Args>(args)...); }
};

#elif defined(OPTION_2)  // `allocator_type` exists only if `T::allocator_type`

#ifdef USE_CONCEPTS
template <class T>
struct __deduced_alloc { using type = void; };

template <class T>
requires requires { typename T::allocator_type; }
struct __deduced_alloc<T> { using type = typename T::allocator_type; };
#else // Use SFINAE
template <class T, class = void>
struct __deduced_alloc { using type = void; };

template <class T>
struct __deduced_alloc<T, void_t<typename T::allocator_type>>
{
  using type = typename T::allocator_type;
};
#endif

// Base class for `inplace_vector`, handling allocator use.
template <class T, class Alloc>
class __inplace_vector_base
{
  [[no_unique_address]] Alloc m_alloc;

protected:
  using AllocArg    = Alloc;
  using AllocTraits = allocator_traits<Alloc>;

#if VERBOSE
  __inplace_vector_base() { std::cout << "Option 2 with allocator\n"; }
  explicit __inplace_vector_base(const AllocArg& a) : m_alloc(a) { std::cout << "Option 2 with allocator\n"; }
#else
  constexpr __inplace_vector_base() = default;
  constexpr explicit __inplace_vector_base(const AllocArg& a) : m_alloc(a) { }
#endif

  template <class... Args>
  constexpr void construct_elem(T* elem, Args&&... args)
    { uninitialized_construct_using_allocator(elem, m_alloc, std::forward<Args>(args)...); }

public:
  using allocator_type = Alloc;

  constexpr allocator_type get_allocator() const { return m_alloc; }
};

// Specialization for `void` allocator
template <class T>
class __inplace_vector_base<T, void>
{
protected:
  using AllocArg    = __non_allocator<T>;
  using AllocTraits = allocator_traits<AllocArg>;

#if VERBOSE
  __inplace_vector_base() { std::cout << "Option 2 no allocator\n"; }
  explicit __inplace_vector_base(const AllocArg&)
    { std::cout << "Option 2 no allocator\n"; }
#else
  constexpr __inplace_vector_base() = default;
  constexpr explicit __inplace_vector_base(const AllocArg&) { }
#endif

  template <class... Args>
  constexpr void construct_elem(T* elem, Args&&... args)
    { construct_at(elem, std::forward<Args>(args)...); }
};

#elif defined(OPTION_3) // Default allocator is `T::allocator_type`

template <class T>
struct __deduced_alloc { using type = void; };

template <class T>
requires requires { typename T::allocator_type; }
struct __deduced_alloc<T> { using type = typename T::allocator_type; };

// Base class for `inplace_vector`, handling allocator use.
template <class T, class Alloc>
class __inplace_vector_base
{
  [[no_unique_address]] Alloc m_alloc;

protected:
  using AllocArg    = Alloc;
  using AllocTraits = allocator_traits<Alloc>;

#if VERBOSE
  __inplace_vector_base() { std::cout << "Option 3 with allocator\n"; }
  explicit __inplace_vector_base(const AllocArg& a) : m_alloc(a)
    { std::cout << "Option 3 with allocator\n"; }
#else
  constexpr __inplace_vector_base() = default;
  constexpr explicit __inplace_vector_base(const AllocArg& a) : m_alloc(a) { }
#endif

  template <class... Args>
  constexpr void construct_elem(T* elem, Args&&... args)
  {
    if constexpr (uses_allocator_v<T, Alloc>)
      uninitialized_construct_using_allocator(elem, m_alloc, std::forward<Args>(args)...);
    else
      construct_at(elem, std::forward<Args>(args)...);
  }

public:
  using allocator_type = Alloc;

  constexpr allocator_type get_allocator() const { return m_alloc; }
};

// Specialization for `std::allocator`
template <class T>
class __inplace_vector_base<T, allocator<T> >
{
protected:
  using AllocArg    = allocator<T>;
  using AllocTraits = allocator_traits<AllocArg>;

#if VERBOSE
  __inplace_vector_base() { std::cout << "Option 3 std::allocator\n"; }
  explicit __inplace_vector_base(const AllocArg&)
    { std::cout << "Option 2 no allocator\n"; }
#else
  constexpr __inplace_vector_base() = default;
  constexpr explicit __inplace_vector_base(const AllocArg&) { }
#endif

  template <class... Args>
  constexpr void construct_elem(T* elem, Args&&... args)
    { construct_at(elem, std::forward<Args>(args)...); }

public:
  constexpr allocator<T> get_allocator() const { return {}; }
};

#else // if non-AA `inplace_vector`

# if ! defined(NON_AA)
#  define NON_AA
# endif

class __inplace_vector_base
{
protected:
  using AllocArg    = __non_allocator<T>;
  using AllocTraits = allocator_traits<AllocArg>;

#if VERBOSE
  __inplace_vector_base() { std::cout << "Non-AA `inplace_vector`\n"; }
#endif
};

#endif // NON_AA

template <class Tp>
union __uninitialized
{
  char trivial;
  Tp   object;

  constexpr __uninitialized() {}
};

#if defined(NON_AA)
template <class T, size_t N>
class inplace_vector : public __inplace_vector_base
{
  using Base = __inplace_vector_base;
#elif defined(OPTION_0)
template <class T, size_t N, class Alloc = allocator<T>>
class inplace_vector : public __inplace_vector_base<T, Alloc>
{
  using Base = __inplace_vector_base<T, Alloc>;
#elif defined(OPTION_1)
template <class T, size_t N, class Alloc = void>
class inplace_vector : public __inplace_vector_base<T, Alloc>
{
  using Base = __inplace_vector_base<T, Alloc>;
#elif defined(OPTION_2)
template <class T, size_t N>
class inplace_vector :
    public __inplace_vector_base<T, typename __deduced_alloc<T>::type>
{
  using Base = __inplace_vector_base<T, typename __deduced_alloc<T>::type>;
#elif defined(OPTION_3)
template <class T, size_t N, class Alloc = typename __deduced_alloc<T>::type>
class inplace_vector : public __inplace_vector_base<T, Alloc>
{
  using Base = __inplace_vector_base<T, Alloc>;
#endif

  using AllocArg    = typename Base::AllocArg;
  using AllocTraits = typename Base::AllocTraits;

  using ArrayType = array<T, N>;

  union Data {
    array<__uninitialized<T>, N> raw_mem;
    ArrayType                    value;

    constexpr Data() { }
    constexpr ~Data() { }
  };

  Data   m_data;
  size_t m_size = 0;

  static constexpr void check_size(size_t n) { if (n > N) throw bad_alloc{}; }

public:
  // types:
  using value_type             = T;
  using pointer                = T*;
  using const_pointer          = const T*;
  using reference              = value_type&;
  using const_reference        = const value_type&;
  using size_type              = size_t;
  using difference_type        = ptrdiff_t;
  using iterator               = typename ArrayType::iterator;
  using const_iterator         = typename ArrayType::const_iterator;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // [containers.sequences.inplace.vector.cons], construct/copy/destroy
  constexpr inplace_vector() noexcept = default;

  constexpr explicit inplace_vector(size_type n)
  {
    check_size(n);
    while (n--)
      unchecked_emplace_back();
  }

  constexpr inplace_vector(size_type n, const T& value);
  template <class InputIterator>
  constexpr inplace_vector(InputIterator first, InputIterator last);
  // template <container-compatible-range<T> R>
  // constexpr inplace_vector(from_range_t, R&& rg);
  constexpr inplace_vector(const inplace_vector& rhs)
    : Base(AllocTraits::select_on_container_copy_construction(rhs.get_allocator()))
    { *this = rhs; };
  constexpr inplace_vector(const inplace_vector& rhs,
                           const type_identity_t<AllocArg>& a) : Base(a)
    { *this = rhs; };
  constexpr inplace_vector(inplace_vector&& rhs)
    noexcept(N == 0 || __is_nothrow_ua_constructible_v<T, Alloc, T&&>)
    : Base(rhs.get_allocator()) { *this = std::move(rhs); };
  constexpr inplace_vector(inplace_vector&& rhs,
                           const type_identity_t<AllocArg>& a)
    noexcept(N == 0 || __is_nothrow_ua_constructible_v<T, Alloc, T&&>)
    : Base(a) { *this = std::move(rhs); };
  constexpr inplace_vector(initializer_list<T> il);

  constexpr ~inplace_vector() { this->clear(); }

  constexpr inplace_vector& operator=(const inplace_vector& other)
  {
    size_t i = 0;
    for ( ; i < std::min(m_size, other.m_size); ++i)
      m_data.value[i] = other.m_data.value[i];
    for ( ; i < other.m_size; ++i)
      unchecked_push_back(other.m_data.value[i]);
    while (m_size > i)
      pop_back();
    return *this;
  }

  constexpr inplace_vector& operator=(inplace_vector&& other)
              noexcept(N == 0 || is_nothrow_move_assignable_v<T>)
  {
    size_t i = 0;
    for ( ; i < std::min(m_size, other.m_size); ++i)
      m_data.value[i] = std::move(other.m_data.value[i]);
    for ( ; i < other.m_size; ++i)
      unchecked_push_back(std::move(other.m_data.value[i]));
    while (m_size > i)
      pop_back();
    return *this;
  }

  constexpr inplace_vector& operator=(initializer_list<T> il)
    { check_size(il.size()); for (const T& elem : il) unchecked_push_back(elem); }

  template <class InputIterator>
  constexpr void assign(InputIterator first, InputIterator last)
    { while (first != last) push_back(*first++); }

  // template<container-compatible-range<T> R>
  // constexpr void assign_range(R&& rg);
  constexpr void assign(size_type n, const T& u)
    { check_size(n); while (n--) unchecked_push_back(u); }
  constexpr void assign(initializer_list<T> il) { *this = il; }

  // iterators
  constexpr iterator               begin()         noexcept
    { return m_data.value.begin(); }
  constexpr const_iterator         begin()   const noexcept
    { return m_data.value.begin(); }
  constexpr iterator               end()           noexcept
    { return begin() + m_size; }
  constexpr const_iterator         end()     const noexcept
    { return begin() + m_size; }
  constexpr reverse_iterator       rbegin()        noexcept
    { return rend() - m_size; }
  constexpr const_reverse_iterator rbegin()  const noexcept
    { return rend() - m_size; }
  constexpr reverse_iterator       rend()          noexcept
    { return m_data.value.rend(); }
  constexpr const_reverse_iterator rend()    const noexcept
    { return m_data.value.rend(); }

  constexpr const_iterator         cbegin()  const noexcept { return begin(); }
  constexpr const_iterator         cend()    const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin();}
  constexpr const_reverse_iterator crend()   const noexcept { return rend(); }

  // [containers.sequences.inplace.vector.members] size/capacity
  [[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }
  constexpr size_type size() const noexcept { return m_size; }
  static constexpr size_type max_size() noexcept { return N; }
  static constexpr size_type capacity() noexcept { return N; }
  constexpr void resize(size_type sz) { resize(sz, T{}); }
  constexpr void resize(size_type sz, const T& c)
  {
    check_size(sz);
    while (m_size > sz) pop_back();
    while (m_size < sz) unchecked_push_back(c);
  }
  static constexpr void reserve(size_type n) { check_size(n); }
  static constexpr void shrink_to_fit() { }

  // element access
  constexpr reference       operator[](size_type n) { return m_data.value[n]; }
  constexpr const_reference operator[](size_type n) const
    { return m_data.value[n]; }
  constexpr const_reference at(size_type n) const;
  constexpr reference       at(size_type n);
  constexpr reference       front()       { return m_data.value[0]; }
  constexpr const_reference front() const { return m_data.value[0]; }
  constexpr reference       back()       { return m_data.value[m_size - 1]; }
  constexpr const_reference back() const { return m_data.value[m_size - 1]; }

  // [containers.sequences.inplace.vector.data], data access
  constexpr       T* data()       noexcept;
  constexpr const T* data() const noexcept;

  // [containers.sequences.inplace.vector.modifiers], modifiers
  template <class... Args> constexpr T& emplace_back(Args&&... args)
  {
    check_size(m_size + 1);
#ifdef NON_AA
    construct_at(&m_data.value[m_size], std::forward<Args>(args)...);
#else
    this->construct_elem(&m_data.value[m_size], std::forward<Args>(args)...);
#endif
    return m_data.value[m_size++];
  }

  constexpr T& push_back(const T& x) { return emplace_back(x); }
  constexpr T& push_back(T&& x) { return emplace_back(std::move(x)); }
  // template<container-compatible-range<T> R>
  // constexpr void append_range(R&& rg);
  constexpr void pop_back()
  {
    m_data.value[--m_size].~T();
  }

  template<class... Args>
  constexpr T* try_emplace_back(Args&&... args);
  constexpr T* try_push_back(const T& x);
  constexpr T* try_push_back(T&& x);
  // template<container-compatible-range<T> R>
  // constexpr ranges::borrowed_iterator_t<R> try_append_range(R&& rg);

  template<class... Args>
  constexpr T& unchecked_emplace_back(Args&&... args)
  {
#ifdef NON_AA
    construct_at(&m_data.value[m_size], std::forward<Args>(args)...);
#else
    this->construct_elem(&m_data.value[m_size], std::forward<Args>(args)...);
#endif
    return m_data.value[m_size++];
  }
  constexpr T& unchecked_push_back(const T& x)
    { return unchecked_emplace_back(x); }
  constexpr T& unchecked_push_back(T&& x)
    { return unchecked_emplace_back(std::move(x)); }

  template <class... Args>
  constexpr iterator emplace(const_iterator position, Args&&... args);
  constexpr iterator insert(const_iterator position, const T& x);
  constexpr iterator insert(const_iterator position, T&& x);
  constexpr iterator insert(const_iterator position, size_type n, const T& x);
  template <class InputIterator>
  constexpr iterator insert(const_iterator position, InputIterator first, InputIterator last);
  // template<container-compatible-range<T> R>
  // constexpr iterator insert_range(const_iterator position, R&& rg); //

  constexpr iterator insert(const_iterator position, initializer_list<T> il);
  constexpr iterator erase(const_iterator position)
    { return erase(position, position + 1); }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    size_type n = last - first;
    iterator first2 = begin() + (first - cbegin());
    iterator last2  = begin() + (last  - cbegin());
    iterator ebeg   = move(last2, end(), first2);
    for (iterator i = ebeg; i != end(); ++i)
      i->~T();
    m_size -= n;
    return first2;
  }
  constexpr void swap(inplace_vector& x)
    noexcept(N == 0 || (is_nothrow_swappable_v<T> && is_nothrow_move_constructible_v<T>))
  {
    if (size() > x.size())
    {
      x.swap(*this);
      return;
    }

    size_type n = size();

#ifndef NON_AA
    // conditionally swap allocators
    using AllocTraits = typename Base::AllocTraits;
    if constexpr (AllocTraits::propagate_on_container_swap::value &&
                  ! AllocTraits::always_compare_equal::value)
      swap(this->m_alloc, x.m_alloc);
#endif

    // Swap first n elements
    swap_ranges(begin(), begin() + n, x.begin());

    // Move remaining elements (increases size of smaller container).
    for (auto i = x.begin() + n; i != x.end(); ++i)
      unchecked_push_back(std::move(*i));

    // Erase moved-from elements (decreases size of formerly larger container)
    x.erase(x.begin() + n, x.end());
  }

  constexpr void clear() noexcept { erase(begin(), end()); }

  constexpr friend bool operator==(const inplace_vector& x, const inplace_vector& y)
  {
    if (x.m_size != y.m_size) return false;
    for (size_t i = 0; i < x.m_size; ++i)
      if (x[i] != y[i])
        return false;
    return true;
  }
  // constexpr friend synth-three-way-result<T>
  // operator<=>(const inplace_vector& x, const inplace_vector& y);
  constexpr friend void swap(inplace_vector& x, inplace_vector& y)
    noexcept(N == 0 || (is_nothrow_swappable_v<T> && is_nothrow_move_constructible_v<T>))
    { x.swap(y); }
};

}  // close namespace std::experimental

// Local Variables:
// c-basic-offset: 2
// End:
