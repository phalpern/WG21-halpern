// simple_vec.h                  -*-C++-*-
#ifndef INCLUDED_SIMPLE_VEC
#define INCLUDED_SIMPLE_VEC

#include "destructive_move.h"
#include <memory>

namespace my {

template <class T, class A = std::allocator<T>>
class simple_vec
{
    A           m_alloc;
    T*          m_data;
    std::size_t m_capacity;
    std::size_t m_length;

    typedef std::allocator_traits<A> alloc_traits;

public:
    typedef T*       iterator;
    typedef const T* const_iterator;
    
    simple_vec(const A& a = A()) noexcept;

    simple_vec(const simple_vec& other);
    simple_vec(simple_vec&& other) noexcept;

    ~simple_vec() noexcept;

    std::size_t capacity() const { return m_capacity; }
    std::size_t size() const { return m_length; }

    T&       back()       noexcept { return m_data[m_length - 1]; }
    const T& back() const noexcept { return m_data[m_length - 1]; }

    iterator       begin()       noexcept { return m_data; }
    const_iterator begin() const noexcept { return m_data; }
    iterator       end()       noexcept { return m_data + m_length; }
    const_iterator end() const noexcept { return m_data + m_length; }

    void swap(simple_vec& other) noexcept;

    void push_back(const T& v);
};

template <class T, class A>
simple_vec<T, A>::simple_vec(const A& a) noexcept
    : m_alloc(a), m_data(nullptr), m_capacity(0), m_length(0)
{
}

template <class T, class A>
simple_vec<T, A>::simple_vec(const simple_vec& other)
    : m_alloc(alloc_traits::select_on_container_copy_construction(
                  other.m_alloc))
    , m_data(nullptr), m_capacity(0), m_length(0)
{
    simple_vec temp;
    for (const_iterator i = other.begin(); i != other.end(); ++i)
        temp.push_back(*i);
    temp.swap(*this);
}

template <class T, class A>
simple_vec<T, A>::simple_vec(simple_vec&& other) noexcept
    : m_alloc(other.m_alloc), m_data(nullptr), m_capacity(0), m_length(0)
{
    other.swap(*this);
}

template <class T, class A>
simple_vec<T, A>::~simple_vec() noexcept
{
    for (iterator i = begin(); i != end(); ++i)
        alloc_traits::destroy(m_alloc, &*i);
    if (m_data)
        alloc_traits::deallocate(m_alloc, m_data, m_capacity);
}

template <class T, class A>
void simple_vec<T, A>::swap(simple_vec& other) noexcept
{
    std::swap(m_data,     other.m_data);
    std::swap(m_capacity, other.m_capacity);
    std::swap(m_length,   other.m_length);
}

#ifdef USE_DESTRUCTIVE_MOVE
template <class T, class A>
void simple_vec<T, A>::push_back(const T& v)
{
    using std::experimental::destructive_move_array;

    if (m_length == m_capacity) {
        simple_vec temp(m_alloc);
        temp.m_capacity = (m_capacity ? 2 * m_capacity : 1);
        temp.m_data = alloc_traits::allocate(m_alloc, temp.m_capacity);

        // Exception-safe move from this->m_data to temp.m_data
        destructive_move_array(temp.m_data, m_data, m_length);

        temp.m_length = m_length;
        m_length = 0;
        temp.swap(*this);
    }

    alloc_traits::construct(m_alloc, &m_data[m_length], v);
    ++m_length;
}
#else
template <class T, class A>
void simple_vec<T, A>::push_back(const T& v)
{
    if (m_length == m_capacity) {
        simple_vec temp(m_alloc);
        temp.m_capacity = (m_capacity ? 2 * m_capacity : 1);
        temp.m_data = alloc_traits::allocate(m_alloc, temp.m_capacity);

        T *from = m_data, *to = temp.m_data;
        for (temp.m_length = 0; temp.m_length < m_length; ++temp.m_length)
            alloc_traits::construct(m_alloc, to++,
                                    std::move_if_noexcept(*from++));
        temp.swap(*this);
    }

    alloc_traits::construct(m_alloc, &m_data[m_length], v);
    ++m_length;
}
#endif

} // close namespace my

#endif // ! defined(INCLUDED_SIMPLE_VEC)

