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

  template <class R, class Self, class K, class U>
  static R do_get_ref(Self& self, const K& k, U& ref);

public:
  using key_type    = typename StdMap::key_type;
  using mapped_type = typename StdMap::mapped_type;
  using iterator    = typename StdMap::iterator;

  // Delegate all constructors to base class
  using StdMap::map;

  // mapped_type get(K, Args...)
  template <class K, class... Args>
  [[nodiscard]] mapped_type get(const K& k, Args&&... args) const
    requires _IsMapKeyType<Key, Compare, K>
    { return do_get<mapped_type>(*this, k, std::forward<Args>(args)...); }

  // ref get_ref(K, U&)
  template <class K, class U>
  [[nodiscard]] common_reference_t<mapped_type&, U&> get_ref(const K& k, U& ref)
    requires _IsMapKeyType<Key, Compare, K>
  {
    return do_get_ref<common_reference_t<mapped_type&, U&>>(*this, k, ref);
  }

  template <class K, class U>
  [[nodiscard]] common_reference_t<const mapped_type&, U&> get_ref(const K& k, U& ref) const
    requires _IsMapKeyType<Key, Compare, K>
  {
    return do_get_ref<common_reference_t<const mapped_type&, U&>>(*this, k,
                                                                  ref);
  }

  // R get_as<R>(K, Args...)
  template <class R, class K, class... Args>
  requires (! is_lvalue_reference_v<R>)
  [[nodiscard]] R get_as(const K& k, Args&&... args)
    requires _IsMapKeyType<Key, Compare, K>
    { return do_get<R>(*this, k, std::forward<Args>(args)...); }

  template <class R, class K, class... Args>
  requires (is_lvalue_reference_v<R>)
  [[nodiscard]] R get_as(const K& k, Args&&... args)
    requires _IsMapKeyType<Key, Compare, K>
    { return do_get_ref<R>(*this, k, std::forward<Args>(args)...); }

  template <class R, class K, class... Args>
  requires (! is_lvalue_reference_v<R>)
  [[nodiscard]] R get_as(const K& k, Args&&... args) const
    requires _IsMapKeyType<Key, Compare, K>
    { return do_get<R>(*this, k, std::forward<Args>(args)...); }

  template <class R, class K, class... Args>
  requires (is_lvalue_reference_v<R>)
  [[nodiscard]] R get_as(const K& k, Args&&... args) const
    requires _IsMapKeyType<Key, Compare, K>
    { return do_get_ref<R>(*this, k, std::forward<Args>(args)...); }

};

template <class Key, class T, class Compare, class Allocator>
template <class R, class Self, class K, class... Args>
R map<Key, T, Compare, Allocator>::do_get(Self& self, const K& k,
                                          Args&&... args)
{
  static_assert(! is_reference_v<R>, "Cannot return reference");

  auto iter = self.find(k);
  if (iter != self.end())
    return iter->second;
  else
    return R(std::forward<Args>(args)...);
}

template <class Key, class T, class Compare, class Allocator>
template <class R, class Self, class K, class U>
R map<Key, T, Compare, Allocator>::do_get_ref(Self& self, const K& k, U& ref)
{
  static_assert(is_lvalue_reference_v<R>, "No common reference type");

  auto iter = self.find(k);
  if (iter != self.end())
    return iter->second;
  else
    return ref;
}

}  // Close namespace std::experimental
