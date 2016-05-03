#ifndef INCLUDED_OPTIONAL_DOT_H
#define INCLUDED_OPTIONAL_DOT_H

#include <type_traits>
#include <memory>
#include <new>
#include <utility>

namespace std {
namespace experimental {
inline namespace pablo_v1 {

struct in_place { };
constexpr in_place_t in_place { };

template <class Tp>
class optional
{
    typedef void* (*manager_ptr)(opcode_t oc, optional* self, Tp* arg);

    class enum opcode {
        has_alloc,
        copy,
        destroy
    };

    static void* disengaged_manager(opcode oc, optional* self, Tp* arg);
    template <class Alloc>
    static void* disengaged_with_alloc_manager(opcode oc, optional* self,
                                               Tp* arg);
    static void* engaged_manager(opcode oc, optional* self, Tp* arg);
    template <class Alloc>
    static void* engaged_with_alloc_manager(opcode oc, optional* self,
                                            Tp* arg);


    manager_ptr m_manager;
    Tp*         m_object;

  public:
    optional() noexcept;

    template <class Alloc>
    optional(allocator_arg_t, const Alloc& a);

    template <class... Args>
    optional(in_place_t, Args&&... args);

    template <class Alloc, class... Args>
    optional(allocator_arg_t, const Alloc& ain_place_t, Args&&... args);

    template <class... Args>
    void emplace(Args&&... args);
};

template <class Tp>
optional<Tp>::optional() noexcept
    : m_manager(&disengaged_manager), m_object(nullptr) { } 

template <class Tp>
optional<Tp>::optional(allocator_arg_t, const Alloc& a)
    : m_manager(&disengaged_with_allocator_manager<Alloc>), m_object(nullptr)
{
    typedef tuple<Tp, Alloc> pair_t;
    typedef allocator_traits<Alloc>::rebind_traits<pair_t> alloc_traits;

    // Some little optimizations could go here, e.g.:
    if (is_same<alloc_traits, allocator_traits<allocator<pair_t>>>::value)
    {
        m_manager = &disengaged_manager;
        return;
    }

    pair_t* p = alloc_traits::allocate(a, 1);
    ::new(&p->second) Alloc(a);
    m_object = &p->first;
}

template <class Tp>
template <class... Args>
void optional<Tp>::emplace(Args&&... args)
{
    if (m_manager(opcode::has_alloc, this, nullptr))
    {
        Tp temp(std::forward<Args>(args)...);
        m_manager(opcode::move, this, temp);
    }
    else {
        Tp* p = new Tp(std::forward<Args>(args)...);
        if (m_object)
            m_object->~Tp();
        m_object = p;
    }
}

template <class Tp>
void* optional<Tp>::disengaged_manager(opcode oc, optional* self, Tp* arg)
{
    switch (oc)
    {
      opcode::has_alloc: return nullptr; break;
      opcode::move:
        m_object = new Tp(std::move(*arg));
        m_manager = &engaged_manager;
        break;
      opcode::destroy:
        break;
    }
}

template <class Alloc>
void* optional<Tp>::disengaged_with_alloc_manager(opcode oc, optional* self,
                                                  Tp* arg)
{
    typedef tuple<Tp, Alloc> pair_t;

    switch (oc)
    {
      opcode::has_alloc: return this; break;
      opcode::move:
        {
            Alloc& a = reinterpret_cast<pair_t*>(m_object)->second;
            uses_allocator_construction(a, m_object, std::move(*arg));
            m_manager = &engaged_with_alloc_manager<Alloc>;
        }
        break;
      opcode::destroy:
        break;
    }
}

template <class Tp>
void* optional<Tp>::engaged_manager(opcode oc, optional* self, Tp* arg)
{
    switch (oc)
    {
      opcode::has_alloc: return nullptr; break;
      opcode::move: *m_object = std::move(*arg); break;
      opcode::destroy:
        delete m_object;
        m_object = nullptr;
        m_manager = &disengaged_with;
        break;
    }
}

template <class Tp>
template <class Alloc>
void* optional<Tp>::engaged_with_alloc_manager(opcode oc, optional* self,
                                               Tp* arg)
{
    typedef tuple<Tp, Alloc> pair_t;

    switch (oc)
    {
      opcode::has_alloc: return this; break;
      opcode::move: *m_object = std::move(*arg); break;
      opcode::destroy:
        m_object->~tp();
        m_manager = &disengaged_with_alloc_manager<Alloc>;
        break;
    }
}


}
}
}

#endif // ! defined(INCLUDED_OPTIONAL_DOT_H)
