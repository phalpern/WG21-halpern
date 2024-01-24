/* copy_swap_transaction.h                  -*-C++-*-
 *
 * Copyright (C) 2016 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * Implementation of P3091: Better lookups for `map` and `unordered_map`
 */

#include <map>

namespace std::experimental {

template <class Key, class Compare, class K>
concept _IsMapKeyType = is_convertible_v<K, Key> ||
  requires { typename Compare::is_transparent; };


template <class Key, class T, class Compare = less<Key>,
          class Allocator = allocator<pair<const Key, T>>>
class map : public ::std::map<Key, T, Compare, Allocator>
{
  using StdMap = ::std::map<Key, T, Compare, Allocator>;

  template <class R, class Self, class K, class... Args>
  static R do_get(Self& self, const K& k, Args&&... args);

  template <class R, class Self, class K, class Arg>
  static R do_get_ref(Self& self, const K& k, Arg& arg);

public:
  using key_type    = typename StdMap::key_type;
  using mapped_type = typename StdMap::mapped_type;
  using iterator    = typename StdMap::iterator;

  // Delegate all constructors to base class
  using StdMap::map;

  // get(K, Args...) -> mapped_type
  template <class K, class... Args>
  [[nodiscard]] auto get(const K& k, Args&&... args) const -> mapped_type
    requires _IsMapKeyType<Key, Compare, K>
    { return do_get<mapped_type>(*this, k, std::forward<Args>(args)...); }

  // get_ref(K, Arg&) -> ref
  template <class K, class Arg>
  [[nodiscard]] auto get_ref(const K& k, Arg& ref) -> decltype(auto)
    requires _IsMapKeyType<Key, Compare, K>
  {
    return do_get_ref<common_reference_t<mapped_type&, Arg&>>(*this, k, ref);
  }

  template <class K, class Arg>
  [[nodiscard]] auto get_ref(const K& k, Arg& ref) const -> decltype(auto)
    requires _IsMapKeyType<Key, Compare, K>
  {
    return do_get_ref<common_reference_t<const mapped_type&, Arg&>>(*this, k,
                                                                    ref);
  }

  // get_as<R>(K, Args...) -> R
  template <class R, class K, class... Args>
  requires (! is_lvalue_reference_v<R>)
  [[nodiscard]] auto get_as(const K& k, Args&&... args) -> R
    requires _IsMapKeyType<Key, Compare, K>
    { return do_get<R>(*this, k, std::forward<Args>(args)...); }

  template <class R, class K, class... Args>
  requires (is_lvalue_reference_v<R>)
  [[nodiscard]] auto get_as(const K& k, Args&&... args) -> R
    requires _IsMapKeyType<Key, Compare, K>
    { return do_get_ref<R>(*this, k, std::forward<Args>(args)...); }

  template <class R, class K, class... Args>
  requires (! is_lvalue_reference_v<R>)
  [[nodiscard]] auto get_as(const K& k, Args&&... args) const -> R
    requires _IsMapKeyType<Key, Compare, K>
    { return do_get<R>(*this, k, std::forward<Args>(args)...); }

  template <class R, class K, class... Args>
  requires (is_lvalue_reference_v<R>)
  [[nodiscard]] auto get_as(const K& k, Args&&... args) const -> R
    requires _IsMapKeyType<Key, Compare, K>
    { return do_get_ref<R>(*this, k, std::forward<Args>(args)...); }

};

// template <class R, class... Args>
// struct __unsafe_ref : std::false_type { };

// // Yield `true_type` if `Arg` is an xvalue and is convertible to `const R&`
// // without invoking a conversion operator (i.e., it is the same as `R` or
// // derived from `R`.)
// template <class R, class Arg>
// struct __unsafe_ref<const R&, Arg> :
//     std::bool_constant<(!std::is_lvalue_reference_v<Arg> &&
//                         std::is_base_of_v<R, std::remove_reference_t<Arg>>)>
// {
// };

// template <class R, class... Args>
// inline constexpr bool __unsafe_ref_v = __unsafe_ref<R, Args...>::value;

template <class Key, class T, class Compare, class Allocator>
template <class R, class Self, class K, class... Args>
R map<Key, T, Compare, Allocator>::do_get(Self& self, const K& k,
                                          Args&&... args)
{
  // static_assert(! __unsafe_ref_v<R, Args...>,
  //               "Can't bind return reference to rvalue");

  static_assert(! is_reference_v<R>, "Cannot return reference");

  auto iter = self.find(k);
  if (iter != self.end())
    return iter->second;
  else
    return R(std::forward<Args>(args)...);
}

template <class Key, class T, class Compare, class Allocator>
template <class R, class Self, class K, class Arg>
R map<Key, T, Compare, Allocator>::do_get_ref(Self& self, const K& k, Arg& arg)
{
  static_assert(is_lvalue_reference_v<R>, "Must return lvalue reference");

  auto iter = self.find(k);
  if (iter != self.end())
    return iter->second;
  else
    return arg;
}

}  // Close namespace std::experimental
