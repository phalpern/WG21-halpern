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

  template <class Self, class K, class... Args>
  static T do_get(Self&& self, const K&, Args&&... args);

  template <class K, class... Args>
  auto do_replace(K&& k, Args&&... args) -> StdMap::iterator;

public:
  using key_type    = typename StdMap::key_type;
  using mapped_type = typename StdMap::mapped_type;
  using iterator    = typename StdMap::iterator;

  // Delegate all constructors to base class
  using StdMap::map;

#ifdef __cpp_explicit_this_parameter
  template <class Self, class... Args>
  [[nodiscard]] T get(this Self&& m, const key_type& k, Args&&... args)
    { return do_get(m, k, std::forward<Args>(args)...); }

  template <class K, class... Args>
  [[nodiscard]] T get(this auto&& m, const K& k, Args&&... args)
    requires requires { typename Compare::is_transparent; };
    { return do_get(m, k, std::forward<Args>(args)...); }
#else
  template <class... Args>
  [[nodiscard]] T get(const key_type& k, Args&&... args) &
    { return do_get(*this, k, std::forward<Args>(args)...); }
  template <class... Args>
  [[nodiscard]] T get(const key_type& k, Args&&... args) const &
    { return do_get(*this, k, std::forward<Args>(args)...); }
  template <class... Args>
  [[nodiscard]] T get(const key_type& k, Args&&... args) &&
    { return do_get(std::move(*this), k, std::forward<Args>(args)...); }

  template <class K, class... Args>
  [[nodiscard]] T get(const K& k, Args&&... args) &
    requires requires { typename Compare::is_transparent; }
    { return do_get(*this, k, std::forward<Args>(args)...); }
  template <class K, class... Args>
  [[nodiscard]] T get(const K& k, Args&&... args) const &
    requires requires { typename Compare::is_transparent; }
    { return do_get(*this, k, std::forward<Args>(args)...); }
  template <class K, class... Args>
  [[nodiscard]] T get(const K& k, Args&&... args) &&
    requires requires { typename Compare::is_transparent; }
    { return do_get(std::move(*this), k, std::forward<Args>(args)...); }
#endif

  [[nodiscard]] const T& get_ref(const key_type& k, const T& dflt) const &;
  [[nodiscard]] const T& get_ref(const key_type& k, T&& dflt) const & = delete;

  template <class K> [[nodiscard]] const T&
  get_ref(const K& k, const T& dflt) const &
    requires requires { typename Compare::is_transparent; };
  template <class K> [[nodiscard]] const T&
  get_ref(const K& k, T&& dflt) const & = delete;

  template <class... Args>
  iterator replace(const key_type& k, Args&&... args)
    { return do_replace(k, std::forward<Args>(args)...); }

  template <class... Args>
  iterator replace(key_type&& k, Args&&... args)
    { return do_replace(std::move(k), std::forward<Args>(args)...); }

  template <class K, class... Args>
  iterator replace(K&& k, Args&&... args)
    requires requires { typename Compare::is_transparent; }
    { return do_replace(std::forward<K>(k), std::forward<Args>(args)...); }
};

template <class Key, class T, class Compare, class Allocator>
template <class Self, class K, class... Args>
T map<Key, T, Compare, Allocator>::do_get(Self&& m, const K& k, Args&&... args)
{
  auto iter = m.find(k);
  if (iter != m.end())
  {
    if constexpr (is_lvalue_reference_v<Self>)
      return iter->second;
    else
      return std::move(iter->second);
  }
  else
    return T(std::forward<Args>(args)...);
}

template <class Key, class T, class Compare, class Allocator>
template <class K, class... Args>
auto map<Key, T, Compare, Allocator>::do_replace(K&& k, Args&&... args)
  -> iterator
{
  auto [ iter, is_new ] =
    this->try_emplace(std::forward<K>(k), std::forward<Args>(args)...);

  if (! is_new)
    iter->second = mapped_type(std::forward<Args>(args)...);

  return iter;
}

template <class Key, class T, class Compare, class Allocator>
const T& map<Key, T, Compare, Allocator>::get_ref(const key_type& k,
                                                  const T& dflt) const &
{
  auto iter = this->find(k);
  if (iter != this->end())
    return iter->second;
  else
    return dflt;
}

template <class Key, class T, class Compare, class Allocator>
template <class K>
const T& map<Key, T, Compare, Allocator>::get_ref(const K& k,
                                                  const T& dflt) const &
  requires requires { typename Compare::is_transparent; }
{
  auto iter = this->find(k);
  if (iter != this->end())
    return iter->second;
  else
    return dflt;
}



}  // Close namespace std::experimental
