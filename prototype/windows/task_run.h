#include <stdio.h>
#include <tchar.h>
#include <vector>
#include <stack>
#include <ppl.h>
#include <windows.h>

#ifdef _DEBUG
#define TRACE_ALLOCATIONS
#endif

class exception_list : public std::exception
{
public:
    typedef std::exception_ptr value_type;
    typedef const value_type& reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef std::vector<std::exception_ptr>::iterator iterator;
    typedef std::vector<std::exception_ptr>::const_iterator const_iterator;

    exception_list(std::vector<std::exception_ptr> exceptions) : exceptions_(std::move(exceptions)) {}

    size_t size() const {
        return exceptions_.size();
    }
    const_iterator begin() const {
        return exceptions_.begin();
    }
    const_iterator end() const {
        return exceptions_.end();
    }
private:
    std::vector<std::exception_ptr> exceptions_;
};

struct task_handler_node_base
{
#ifdef TRACE_ALLOCATIONS
    size_t                      size;
#endif
    task_handler_node_base*     next;
};

template<typename T>
struct task_handler_node : public task_handler_node_base
{
    concurrency::task_handle<T>  data;
};

class structured_task_groupEx : public concurrency::structured_task_group
{
    // Arena allocator that attempts to allocate task handles inside the extended task group
    static const size_t arena_size = 1024;
    template <std::size_t N>
    struct allocator
    {
        char data[N];
        void* p;
        std::size_t sz;
        allocator() : p(data), sz(N) {}
        template <typename Ty>
        Ty* aligned_alloc()
        {
            std::size_t a = __alignof(Ty);
            if (std::align(a, sizeof(Ty), p, sz))
            {
                Ty* result = reinterpret_cast<Ty*>(p);
                p = (char*)p + sizeof(Ty);
                sz -= sizeof(Ty);
                return result;
            }
            return nullptr;
        }
    };

    allocator<arena_size>           m_allocator;

    struct exception_list_node
    {
        std::exception_ptr data;
        exception_list_node* next;
        exception_list_node(const std::exception_ptr& node) : data(node), next(nullptr) {}
    };

    std::atomic<exception_list_node*>    m_exception_list_head = nullptr;
    task_handler_node_base*              m_task_list_head = nullptr;

public:

    void add_exception(const std::exception_ptr& ex)
    {
        auto new_node = new exception_list_node(ex);
        new_node->next = m_exception_list_head.load(std::memory_order_relaxed);

        while (!m_exception_list_head.compare_exchange_weak(new_node->next,
            new_node,
            std::memory_order_release,
            std::memory_order_relaxed))
            ;
    }

    bool have_exceptions()
    {
        return m_exception_list_head.load(std::memory_order_relaxed) != nullptr;
    }

    void throw_exception_list()
    {
        std::vector<std::exception_ptr> vec;
        auto node = m_exception_list_head.load();
        while (node) {
            vec.push_back(node->data);
            node = node->next;
        }
        exception_list el(std::move(vec));
        throw el;
    }

    template<typename Ty>
    task_handler_node<Ty>* alloc()
    {
        task_handler_node_base *p_node;
        p_node = m_allocator.aligned_alloc<task_handler_node<Ty>>();

        if (p_node == nullptr)
        {
            // fall back to ConcRT suballocator and record the pointer for future deallocation
            auto size = sizeof(task_handler_node<Ty>);
            p_node = (task_handler_node_base*)concurrency::Alloc(size);
#ifdef TRACE_ALLOCATIONS
            p_node->size = size;
            printf(":alloc %d bytes into 0x%p\n", size, p_node);
#endif
            p_node->next = m_task_list_head;
            m_task_list_head = p_node;
        }

        return static_cast<task_handler_node<Ty>*>(p_node);
    }

    ~structured_task_groupEx()
    {
        while (m_task_list_head != nullptr)
        {
            auto pdelete = m_task_list_head;
#ifdef TRACE_ALLOCATIONS
            auto size = m_task_list_head->size;
#endif
            m_task_list_head = m_task_list_head->next;
            concurrency::Free(pdelete);
#ifdef TRACE_ALLOCATIONS
            printf(":freed %d bytes at 0x%p\n", size, pdelete);
#endif
        }
    }
};

extern __declspec(thread) structured_task_groupEx* tls_current_task_group;

template<typename F>
void task_region(F && f)
{
    // this must be the last object to be destroyed
    struct restore_tls {
        structured_task_groupEx* m_old_state = tls_current_task_group;
        ~restore_tls()
        {
            tls_current_task_group = m_old_state;
        }
    }_;

    structured_task_groupEx stg;

    tls_current_task_group = &stg;

    try
    {
        f();
    }
    catch (...)
    {
        stg.add_exception(current_exception());
    }

    // This cannot throw, because task exceptions have been wrapped and handled
    stg.wait();

    if (stg.have_exceptions())
    {
        stg.throw_exception_list();
    }
}

template<typename F>
void task_run(F&& f) noexcept
{
    assert(tls_current_task_group != nullptr);

    auto current_task_group = tls_current_task_group;

    auto f_wrapped = [f, current_task_group] {
        try
        {
            f();
        }
        catch (...)
        {
            assert(current_task_group != nullptr);
            current_task_group->add_exception(current_exception());
        }
    };

    typedef decltype(f_wrapped) F_wrapped;

    auto ptask_handler_node = tls_current_task_group->alloc<F_wrapped>();

    // create a task handle in-place
    new(&ptask_handler_node->data)task_handle<F_wrapped>(make_task(f_wrapped));

    // Now run the task on the task group
    tls_current_task_group->run(ptask_handler_node->data);
}
