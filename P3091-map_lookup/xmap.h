/* copy_swap_transaction.h                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * Implementation of P3091: Better lookups for `map` and `unordered_map`
 */

#include <map>

namespace std::experimental {

template <class Key, class T, class Compare = less<Key>,
          class Allocator = allocator<pair<const Key, T>>>
class map : public ::std::map<Key, T, Compare, Allocator>
{
  using StdMap = ::std::map<Key, T, Compare, Allocator>;

  template <class R, class Self, class K, class... Args>
  static R do_get_as(Self& self, const K& k, Args&&... args);

  template <class R, class Self, class K, class... Args>
  static R do_get_as(Self&& self, const K& k, Args&&... args);

public:
  using key_type    = typename StdMap::key_type;
  using mapped_type = typename StdMap::mapped_type;
  using iterator    = typename StdMap::iterator;

  // Delegate all constructors to base class
  using StdMap::map;

#ifdef __cpp_explicit_this_parameter

  // get(key_type, Args...) -> mapped_type
  template <class Self, class... Args> [[nodiscard]]
  auto get(this Self&& m, const key_type& k, Args&&... args) -> mapped_type
    { return do_get_as<mapped_type>(m, k, std::forward<Args>(args)...); }

  // get(K, Args...) -> mapped_type
  template <class Self, class K, class... Args> [[nodiscard]]
  auto get(this Self&& m, const K& k, Args&&... args) -> mapped_type
    requires requires { typename Compare::is_transparent; };
    { return do_get_as<mapped_type>(m, k, std::forward<Args>(args)...); }

  // get_as<R>(key_type, Args...) -> R
  template <class R, class Self, class... Args> [[nodiscard]]
  auto get_as(this Self&& m, const key_type& k, Args&&... args) -> R
    { return do_get_as<R>(m, k, std::forward<Args>(args)...); }

  // get_as<R>(K, Args...) -> R
  template <class R, class Self, class K, class... Args> [[nodiscard]]
  auto get_as(this Self&& m, const K& k, Args&&... args) -> R
    requires requires { typename Compare::is_transparent; };
    { return do_get_as<R>(m, k, std::forward<Args>(args)...); }

#else // if ! __cpp_explicit_this_parameter

  // get(key_type, Args...) -> mapped_type
  template <class... Args> [[nodiscard]]
  auto get(const key_type& k, Args&&... args) & -> mapped_type
    { return do_get_as<mapped_type>(*this, k, std::forward<Args>(args)...); }

  template <class... Args> [[nodiscard]]
  auto get(const key_type& k, Args&&... args) const & -> mapped_type
    { return do_get_as<mapped_type>(*this, k, std::forward<Args>(args)...); }

  template <class... Args> [[nodiscard]]
  auto get(const key_type& k, Args&&... args) && -> mapped_type
  {
    return do_get_as<mapped_type>(std::move(*this), k,
                                  std::forward<Args>(args)...);
  }

  // get(K, Args...) -> mapped_type
  template <class K, class... Args> [[nodiscard]]
  auto get(const K& k, Args&&... args) & -> mapped_type
    requires requires { typename Compare::is_transparent; }
    { return do_get_as<mapped_type>(*this, k, std::forward<Args>(args)...); }

  template <class K, class... Args> [[nodiscard]]
  auto get(const K& k, Args&&... args) const & -> mapped_type
    requires requires { typename Compare::is_transparent; }
    { return do_get_as<mapped_type>(*this, k, std::forward<Args>(args)...); }

  template <class K, class... Args> [[nodiscard]]
  auto get(const K& k, Args&&... args) && -> mapped_type
    requires requires { typename Compare::is_transparent; }
  {
    return do_get_as<mapped_type>(std::move(*this), k,
                                  std::forward<Args>(args)...);
  }

  // get_as<R>(key_type, Args...) -> R
  template <class R, class... Args> [[nodiscard]]
  auto get_as(const key_type& k, Args&&... args) & -> R
    { return do_get_as<R>(*this, k, std::forward<Args>(args)...); }

  template <class R, class... Args> [[nodiscard]]
  auto get_as(const key_type& k, Args&&... args) const & -> R
    { return do_get_as<R>(*this, k, std::forward<Args>(args)...); }

  template <class R, class... Args> [[nodiscard]]
  auto get_as(const key_type& k, Args&&... args) && -> R
  {
    return do_get_as<R>(std::move(*this), k,
                                  std::forward<Args>(args)...);
  }

  // get_as<R>(K, Args...) -> R
  template <class R, class K, class... Args> [[nodiscard]]
  auto get_as(const K& k, Args&&... args) & -> R
    requires requires { typename Compare::is_transparent; }
    { return do_get_as<R>(*this, k, std::forward<Args>(args)...); }

  template <class R, class K, class... Args> [[nodiscard]]
  auto get_as(const K& k, Args&&... args) const & -> R
    requires requires { typename Compare::is_transparent; }
    { return do_get_as<R>(*this, k, std::forward<Args>(args)...); }

  template <class R, class K, class... Args> [[nodiscard]]
  auto get_as(const K& k, Args&&... args) && -> R
    requires requires { typename Compare::is_transparent; }
    { return do_get_as<R>(std::move(*this), k, std::forward<Args>(args)...); }

#endif // else ! __cpp_explicit_this_parameter

};

template <class R, class... Args>
struct __unsafe_ref : std::false_type { };

// Yield `true_type` if `Arg` is an xvalue and is convertible to `const R&`
// without invoking a conversion operator (i.e., it is the same as `R` or
// derived from `R`.)
template <class R, class Arg>
struct __unsafe_ref<const R&, Arg> :
    std::bool_constant<(!std::is_lvalue_reference_v<Arg> &&
                        std::is_base_of_v<R, std::remove_reference_t<Arg>>)>
{
};

template <class R, class... Args>
inline constexpr bool __unsafe_ref_v = __unsafe_ref<R, Args...>::value;

template <class Key, class T, class Compare, class Allocator>
template <class R, class Self, class K, class... Args>
R map<Key, T, Compare, Allocator>::do_get_as(Self& self, const K& k,
                                             Args&&... args)
{
  static_assert(! __unsafe_ref_v<R, Args...>,
                "Can't bind return reference to rvalue");

  auto iter = self.find(k);
  if (iter != self.end())
    return iter->second;
  else
    return R(std::forward<Args>(args)...);
}

template <class Key, class T, class Compare, class Allocator>
template <class R, class Self, class K, class... Args>
R map<Key, T, Compare, Allocator>::do_get_as(Self&& self, const K& k,
                                             Args&&... args)
{
  static_assert(std::is_convertible_v<mapped_type&&, R>,
                "Mapped type must be convertible to return type");

  auto iter = self.find(k);
  if (iter != self.end())
    return std::move(iter->second);
  else
    return R(std::forward<Args>(args)...);
}

}  // Close namespace std::experimental
