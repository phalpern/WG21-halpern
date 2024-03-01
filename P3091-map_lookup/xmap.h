/* xmap.h                  -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 *
 * Implementation of P3091: Better lookups for `map` and `unordered_map`
 */

#ifndef INCLUDED_XMAP
#define INCLUDED_XMAP

#include <map>
#include <xoptional.h>

namespace std::experimental {

template <class Key, class Compare, class K>
concept _IsMapKeyType = is_convertible_v<K, Key> ||
  requires { typename Compare::is_transparent; };


template <class Key, class T, class Compare = less<Key>,
          class Allocator = allocator<pair<const Key, T>>>
class map : public ::std::map<Key, T, Compare, Allocator>
{
  using StdMap = ::std::map<Key, T, Compare, Allocator>;

public:
  using key_type    = typename StdMap::key_type;
  using mapped_type = typename StdMap::mapped_type;
  using iterator    = typename StdMap::iterator;

  // Delegate all constructors to base class
  using StdMap::map;

  template <class K, class... Args>
  [[nodiscard]] optional<mapped_type&> get(const K& k)
    requires _IsMapKeyType<Key, Compare, K>
  {
    auto iter = this->find(k);
    if (iter != this->end())
      return { iter->second };
    else
      return nullopt;
  }

  template <class K, class... Args>
  [[nodiscard]] optional<const mapped_type&> get(const K& k) const
    requires _IsMapKeyType<Key, Compare, K>
  {
    auto iter = this->find(k);
    if (iter != this->end())
      return iter->second;
    else
      return nullopt;
  }
};

}  // Close namespace std::experimental

#endif // ! defined(INCLUDED_XMAP)

// Local Variables:
// c-basic-offset: 2
// End:
