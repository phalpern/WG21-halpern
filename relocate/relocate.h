// relocate.h                                                         -*-C++-*-

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
template <class T> class TrivialRelocator;
template <class T> class MoveRelocator;

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
requires (is_trivially_relocatable_v<T> &&
          ! std::is_trivially_move_constructible_v<T>)
inline constexpr
TrivialRelocator<T> relocate(T& v) noexcept { return TrivialRelocator<T>(v); }

template <class T>
requires std::is_trivially_move_constructible_v<T>
inline constexpr T&& relocate(T& v) noexcept { return std::move(v); }

template <class T>
inline constexpr
MoveRelocator<T> relocate(T& v) noexcept { return MoveRelocator<T>(v); }

// Track state of a relcator. Functionally, a `bool` would have been
// sufficient, but tracking three states allows for better defensive checks
// (`assert`s).
enum class RelocateState {
  owned,    // Owned by relocator
  exploded, // Not owned, but some members might still be owned
  released  // Not owned at all
};

// Base class for constructor-argument types.  Acts an an owning reference
template <class T>
class RelocatorBase
{
  T&            m_source;
  RelocateState m_state;

  friend class Relocator<T>;

public:
  constexpr explicit RelocatorBase(T& r) noexcept
    : m_source(r), m_state(RelocateState::owned) { }

  RelocatorBase(const RelocatorBase& dr) = delete; // move-only

  RelocatorBase(RelocatorBase&& dr) noexcept // move-only
    : m_source(dr.m_source)
    , m_state(dr.m_state) {
    dr.m_state = RelocateState::released;
  }

  constexpr ~RelocatorBase()
    { if (RelocateState::owned == m_state) m_source.~T(); }

  RelocatorBase& operator=(const RelocatorBase&) = delete;
  RelocatorBase& operator=(RelocatorBase&&) = delete;

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

// Partial specialization for trivially destructible types does not track
// ownership state or call destructor.
template <class T>
requires std::is_trivially_destructible_v<T>
class RelocatorBase<T>
{
  T&            m_source;

  friend class Relocator<T>;

public:
  constexpr explicit RelocatorBase(T& r) noexcept : m_source(r) { }

  RelocatorBase(const RelocatorBase& dr) = delete; // move-only

  RelocatorBase(RelocatorBase&& dr) noexcept // move-only
    : m_source(dr.m_source) { }

  constexpr ~RelocatorBase() = default;

  RelocatorBase& operator=(const RelocatorBase&) = delete;
  RelocatorBase& operator=(RelocatorBase&&) = delete;

  T& get() const { return m_source; }
  T& release() { return m_source; }
};

// Owning reference argument to explicitly relocatable class types. A
// constructor of type `T` takes an argument of type `Relocator<T>`, and
// performs the equivalent of a move-destroy in one step.
template <class T>
class Relocator : public RelocatorBase<T>
{
public:
  using RelocatorBase<T>::RelocatorBase;

  constexpr T& explode() {
    if constexpr (! std::is_trivially_destructible_v<T>) {
      assert(this->m_state != RelocateState::released);
      this->m_state = RelocateState::exploded;
    }
    return this->m_source;
  }
};

// Owning reference to non-explicitly relocatable class types. This class never
// shows up directly in an interface.  When passed to a constructor, it is
// convertible to an rvalue reference, hence invoking the move constructor.
// The destructor then destroys the moved-from object, completing the
// relocation as a two-step process.
template <class T>
class MoveRelocator : public RelocatorBase<T>
{
public:
  using RelocatorBase<T>::RelocatorBase;

  operator T&&() const { return std::move(this->get()); }
};

// Owning reference to trivially relocatable class type.  A constructor of `T`
// takes an argument of type `TrivialRelocator<T>`, then calls the
// `relocateTo` method to `memcpy` the source object to the destination.
template <class T>
class TrivialRelocator : public RelocatorBase<T>
{
public:
  using RelocatorBase<T>::RelocatorBase;

  void relocateTo(T* dest) noexcept
    { std::memcpy(dest, std::addressof(this->release()), sizeof(T)); }
};

// Relocating a relocator is idempotent to relocating the source object
template <class T>
inline constexpr Relocator<T> relocate(Relocator<T>& r) noexcept
{
    return std::move(r);
}

template <class T>
inline constexpr MoveRelocator<T> relocate(MoveRelocator<T>& r) noexcept
{
    return std::move(r);
}

template <class T>
inline constexpr TrivialRelocator<T> relocate(TrivialRelocator<T>& r) noexcept
{
    return std::move(r);
}

// Forward `v` as `TO` reference.  If `TO` is a reference type, simply cast `v`
// to `TO`, otherwise, return a relocator reference to `v` after casing it to
// `TO&`.
template <class TO, class FROM>
inline constexpr decltype(auto) forward_relocate(FROM&& v)
{
  if constexpr (std::is_reference_v<TO>)
    return static_cast<TO>(v);
  else
    return relocate(static_cast<TO&>(v));
}

#define RELOCATE_MEMBER(R, MEMB_NAME) \
  xstd::forward_relocate<decltype(R.get().MEMB_NAME)>(R.explode().MEMB_NAME)

// Hold a `T` object in such a way that it can be relocated from. Useful for
// stack-based objects that you want to relocate from.
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

  operator       T&()         { return m_relocator.get(); }
  operator const T&()   const { return m_relocator.get(); }
  T&       operator*()        { return m_relocator.get(); }
  T const& operator*()  const { return m_relocator.get(); }
  T*       operator->()       { return &m_relocator.get(); }
  T const* operator->() const { return &m_relocator.get(); }

  constexpr decltype(auto) do_relocate() noexcept
    { return relocate(m_relocator); }

  T& release() { return m_relocator.release(); }
};

// Partial specialization for trivially move-constructible types. Because
// trivially move-constructible types are automatically trivially relocatable
// and trivially destructible, we do not need to track ownership.  Critically,
// `sizeof(Relocatable<T>) == sizeof(T)` for this specialization, allowing us
// to `memcpy` from an array of `Relocatable<T>` to an array of `T`.
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

  T& release() { return m_value; }
};

// Relocating a `relocatable` object actually relocates its contents.
template <class T>
constexpr decltype(auto) relocate(Relocatable<T>& r) noexcept
{
  return r.do_relocate();
}

// Library function to perform an efficient relocation of `n` objects from
// `src` to `dest`.
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

// This overload of `uninitialized_relocate` relocates `Relocatable<T>` objects
// to `T` objects.
template <class T>
void uninitialized_relocate(Relocatable<T>* src, T* dest, std::size_t n = 1)
  noexcept(noexcept(T(xstd::relocate(*src))))
{
  if constexpr (std::is_trivially_move_constructible_v<T>) {
    // Use `memcpy` for the entire array
    static_assert(sizeof(*src) == sizeof(*dest));
    std::memcpy(dest, src, n * sizeof(T));
  }
  else {
    for ( ; n > 0; --n) {
      if constexpr (is_trivially_relocatable_v<T>)
        // Use `memcpy` for each element (non-contiguous)
        std::memcpy(dest++, &src++->release(), sizeof(T));
      else
        std::construct_at(dest++, xstd::relocate(*src++));
    }
  }
}

}  // Close namespace xstd.

/*
 * Local Variables:
 * c-basic-offset: 2
 * End:
 */
