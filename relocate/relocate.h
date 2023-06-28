#include <iostream>
#include <utility>
#include <type_traits>
#include <memory>
#include <cstring>
#include <cassert>

#ifndef __cpp_lib_constexpr_dynamic_alloc
namespace std {

template<class T, class... Args>
constexpr T* construct_at(T* location, Args&&... args)
{
  return ::new (static_cast<void*>(location)) T(std::forward<Args>(args)...);
}

}
#endif

namespace xstd {

template <class T> class Relocator;
template <class T> class MoveRelocator;
template <class T> class TrivialRelocator { };

template <class T>
struct is_explicitly_relocatable : std::is_convertible<Relocator<T>, T> {};

template <class T>
inline constexpr bool is_explicitly_relocatable_v =
  is_explicitly_relocatable<T>::value;

template <class T>
struct is_trivially_relocatable
  : std::bool_constant<std::is_trivially_move_constructible_v<T> ||
                       std::is_convertible_v<TrivialRelocator<T>, T>>
{};

template <class T>
inline constexpr bool is_trivially_relocatable_v =
  is_trivially_relocatable<T>::value;

template <class T>
requires is_explicitly_relocatable_v<T>
inline constexpr
Relocator<T> relocate(T& v) noexcept { return Relocator<T>(v); }

template <class T>
requires std::is_trivially_move_constructible_v<T>
inline constexpr T&& relocate(T& v) noexcept { return std::move(v); }

template <class T>
inline constexpr
MoveRelocator<T> relocate(T& v) noexcept { return MoveRelocator<T>(v); }

enum class RelocateState { engaged, exploded, released };

template <class T>
class Relocator
{
  T&            m_source;
  RelocateState m_state;

public:
  constexpr explicit Relocator(T& r) noexcept
    : m_source(r), m_state(RelocateState::engaged) { }

  Relocator(const Relocator&& dr) = delete; // move-only

  Relocator(Relocator&& dr) noexcept // move-only
    : m_source(dr.m_source)
    , m_state(dr.m_state) {
    dr.m_state = RelocateState::released;
  }

  constexpr ~Relocator()
    { if (m_state == RelocateState::engaged) m_source.~T(); }

  Relocator& operator=(const Relocator&) = delete;
  Relocator& operator=(Relocator&&) = delete;

  constexpr RelocateState state() const { return m_state; }
  constexpr T& explode() {
    assert(m_state != RelocateState::released);
    m_state = RelocateState::exploded;
    return m_source;
  }

  T& get() const {
    assert(m_state != RelocateState::released);
    return m_source;
  }

  T& release() {
    assert(m_state != RelocateState::exploded);
    m_state = RelocateState::released;
    return m_source;
  }
};

template <class T>
inline constexpr Relocator<T> relocate(Relocator<T>& r) noexcept
{
    return std::move(r);
}

template <class T>
struct MoveRelocator : Relocator<T>
{
  using Relocator<T>::Relocator;

  operator T&&() const { return std::move(reloc_get(*this)); }
};

template <class T>
inline constexpr MoveRelocator<T> relocate(MoveRelocator<T>& r) noexcept
{
    return std::move(r);
}

template <class TO, class FROM>
inline constexpr decltype(auto) fwd_relocate(FROM&& v)
{
  if constexpr (std::is_reference_v<TO>)
    return static_cast<TO>(v);
  else
    return relocate(static_cast<TO&>(v));
}

template <class T>
requires std::is_trivially_move_constructible_v<T>
inline constexpr T&& reloc_get(T&& r) { return std::forward<T>(r); }

template <class T>
inline T& reloc_get(const Relocator<T>& dr) { return dr.get(); }

template <class T>
inline T& reloc_get(const MoveRelocator<T>& dr) { return dr.get(); }

#define RELOCATE_MEMBER(R, MEMB_NAME) \
  xstd::fwd_relocate<decltype(R.get().MEMB_NAME)>(R.explode().MEMB_NAME)

template <class T>
class Relocatable
{
  union {
    char m_unused;
    T    m_value;
  };

  decltype(relocate(m_value)) m_relocator;

public:
  Relocatable() : m_relocator(relocate(m_value))
    { std::construct_at(&m_value); }

  template <class Arg>
  requires std::is_convertible_v<Arg, T>
  Relocatable(Arg&& arg) : m_relocator(relocate(m_value)) {
    std::construct_at(&m_value, std::forward<Arg>(arg));
  }

  template <class... Arg>
  explicit Relocatable(Arg&&... arg) : m_relocator(relocate(m_value)) {
    if constexpr (std::is_aggregate_v<T>)
      std::construct_at(&m_value, T{std::forward<Arg>(arg)...});
    else
      std::construct_at(&m_value, std::forward<Arg>(arg)...);
  }

  ~Relocatable() { }

  operator       T&()         { return reloc_get(m_value); }
  operator const T&()   const { return reloc_get(m_value); }
  T&       operator*()        { return reloc_get(m_value); }
  T const& operator*()  const { return reloc_get(m_value); }
  T*       operator->()       { return &reloc_get(m_value); }
  T const* operator->() const { return &reloc_get(m_value); }

  constexpr decltype(auto) do_relocate() noexcept
    { return relocate(m_relocator); }
};

template <class T>
requires std::is_trivially_move_constructible_v<T>
class Relocatable<T>
{
  T m_value;

public:
  Relocatable() = default;

  template <class Arg>
  requires std::is_convertible_v<Arg, T>
  Relocatable(Arg&& arg) : m_value(std::forward<Arg>(arg)) { }

  template <class... Arg>
  explicit Relocatable(Arg&&... arg) : m_value{std::forward<Arg>(arg)...} { }

  operator       T&()         { return m_value; }
  operator const T&()   const { return m_value; }
  T&       operator*()        { return m_value; }
  T const& operator*()  const { return m_value; }
  T*       operator->()       { return &m_value; }
  T const* operator->() const { return &m_value; }

  constexpr T&& do_relocate() noexcept { return std::move(m_value); }
};

template <class T>
constexpr decltype(auto) relocate(Relocatable<T>& r) noexcept
{
  return r.do_relocate();
}

template <class T>
void uninitialized_relocate(T* src, T* dest, std::size_t n = 1)
  noexcept(noexcept(T(xstd::relocate(*src))))
{
  if constexpr (is_trivially_relocatable_v<T>)
    std::memcpy(dest, src, n * sizeof(*src));
  else {
    for ( ; n > 0; --n)
      std::construct_at(dest++, xstd::relocate(*src++));
  }
}

template <class T>
void uninitialized_relocate(Relocatable<T>* src, T* dest, std::size_t n = 1)
  noexcept(noexcept(T(xstd::relocate(*src))))
{
  if constexpr (std::is_trivially_move_constructible_v<T>)
    std::memcpy(dest, src, n * sizeof(*src));
  else {
    for ( ; n > 0; --n)
      if constexpr (is_trivially_relocatable_v<T>) {
        std::memcpy(dest++, &*src, sizeof(T));
        xstd::relocate(*src++);
      }
      else
        std::construct_at(dest++, xstd::relocate(*src++));
  }
}

}  // Close namespace xstd.
