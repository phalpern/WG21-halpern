/* ident.h                                                            -*-C++-*-
 *
 * Copyright (C) 2024 Pablo Halpern <phalpern@halpernwightsoftware.com>
 * Distributed under the Boost Software License - Version 1.0
 */

#ifndef INCLUDED_IDENT
#define INCLUDED_IDENT

/// Structural type usable as a template argument
struct T_ident
{
  char        m_data[32];
  std::size_t m_size;

  constexpr std::size_t max_size() const { return sizeof(m_data) - 1; }
  constexpr std::size_t size()     const { return m_size; }

  constexpr T_ident(const char* s, std::size_t n) : m_size(n) {
    if (n >= max_size())
      n = max_size() - 1;
    for (std::size_t i = 0; i < n; ++i)
      m_data[i] = s[i];
    for (std::size_t i = n; i <= max_size(); ++i)
      m_data[i] = '\0';
  }
};

#endif // ! defined(INCLUDED_IDENT)

// Local Variables:
// c-basic-offset: 2
// End:
