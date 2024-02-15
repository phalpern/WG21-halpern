#include <vector>
#include <type_traits>
#include <cassert>

namespace std {

namespace __internal {

template <class T, size_t CAP>
struct sbo_buffer
{
  union TStorage {
    char m_c;
    T    m_data;
  };

  bool     m_sbo_used = false;
  TStorage m_storage[CAP];
};

template <class T, size_t CAP, class UPSTREAM = allocator<T>>
class sbo_buffer_alloc : private UPSTREAM
{
  using Base = UPSTREAM;
  using BufferType = sbo_buffer<T, CAP>;

  BufferType* m_buffer_p;

  template <class, size_t, class> friend class sbo_buffer_alloc;

public:
  using value_type = T;

  template <class U>
  struct rebind {
    using up = typename allocator_traits<UPSTREAM>::template rebind_alloc<U>;
    using other = sbo_buffer_alloc<U, CAP, up>;
  };

  sbo_buffer_alloc(BufferType *buf_p) : Base(), m_buffer_p(buf_p) { }
  sbo_buffer_alloc(const Base& alloc, BufferType *buf_p)
    : Base(alloc), m_buffer_p(buf_p) { }
  sbo_buffer_alloc(const sbo_buffer_alloc& other)
    : Base(upstream()), m_buffer_p(other.m_buffer_p) { }
  template <class U, class A>
  sbo_buffer_alloc(const sbo_buffer_alloc<U, CAP, A>& other)
    : Base(upstream()), m_buffer_p(other.m_buffer_p) { }

  T* allocate(size_t n) {
    if (m_buffer_p->m_sbo_used || CAP != n)
      return Base::allocate(n);
    else {
      m_buffer_p->m_sbo_used = true;
      return &m_buffer_p->m_storage[0].m_data;
    }
  }

  void deallocate(T* p, size_t n) {
    if (&m_buffer_p->m_storage[0].m_data == p) {
      assert(m_buffer_p->m_sbo_used);
      assert(CAP == n);
      m_buffer_p->m_sbo_used = false;
    }
    else
      Base::deallocate(p, n);
  }

  template <class U, class... Args>
  constexpr void construct(U* p, Args&&... args) {
    allocator_traits<UPSTREAM>::construct(upstream(), p,
                                          forward<Args>(args)...);
  }

  template <class U>
  constexpr void destroy(U* p) {
    allocator_traits<UPSTREAM>::destroy(upstream(), p);
  }

  constexpr typename allocator_traits<UPSTREAM>::size_type max_size() const {
    return max(CAP, allocator_traits<UPSTREAM>::max_size(upstream()));
  }

  Base      & upstream()       { return *this; }
  Base const& upstream() const { return *this; }

  friend
  bool operator==(const sbo_buffer_alloc& a, const sbo_buffer_alloc& b) {
    if (a.m_buffer_p == b.m_buffer_p)
      return true;       // Both are using the same buffer
    else if (a.m_buffer_p->m_sbo_used || b.m_buffer_p->m_sbo_used)
      return false;      // One or both are using non-sharable buffer
    else
      return a.upstream() == b.upstream();
  }
};

}  // close namespace __internal

template <class T, size_t CAP, class UPSTREAM = allocator<T>>
struct sbo_allocator
{
  using value_type = T;

  template <class U>
  struct rebind {
    using up = typename allocator_traits<UPSTREAM>::template rebind_alloc<U>;
    using other = sbo_allocator<U, CAP, up>;
  };
};

template <class T, size_t CAP, class ALLOC>
class vector<T, sbo_allocator<T, CAP, ALLOC>>
  : public vector<T, __internal::sbo_buffer_alloc<T, CAP, ALLOC>>
{
  using BaseAlloc  = __internal::sbo_buffer_alloc<T, CAP, ALLOC>;
  using Base       = vector<T, BaseAlloc>;
  using BufferType = __internal::sbo_buffer<T, CAP>;
  using AllocTraits= allocator_traits<ALLOC>;

  BufferType m_buffer;

  bool in_sbo() const {
    return this->data() == &m_buffer->m_storage[0].m_data;
  }

public:
  using allocator_type = ALLOC;  // Override base class

  vector() : Base(&m_buffer) { this->reserve(CAP); }
  explicit vector(const allocator_type& alloc)
    : Base(BaseAlloc(alloc, &m_buffer)) { this->reserve(CAP); }

  vector(const vector& other)
    : Base(other,
           BaseAlloc(
             AllocTraits::select_on_container_copy_construction(
               other.get_allocator()), &m_buffer)) { }
  vector(const vector& other, const allocator_type& alloc)
    : Base(other, BaseAlloc(alloc, &m_buffer)) { }

  vector(vector&& other) noexcept(is_nothrow_move_constructible_v<T>)
    : vector(std::move(other), other.get_allocator()) { }
  vector(vector&& other, const allocator_type& alloc)
    : Base(BaseAlloc(alloc, &m_buffer)) {
    if (other.size() <= CAP) {
      Base::reserve(CAP); // SBO
      for (auto& e : other)
        Base::emplace_back(std::move(e));
    }
    else if (alloc == other.get_allocator()) {
      Base::swap(other);
    }
    else {
      Base::operator=(std::move(other));
    }
  }

  vector& operator=(const vector& rhs) = default;

  vector& operator=(const vector&& rhs) {
    if (this == &rhs) return *this;

    if (this->get_allocator() != rhs.get_allocator() ||
        this->in_sbo() || rhs.in_sbo())
      this->operator=(rhs);  // Delegate to copy assignment
    else
      Base::operator=(std::move(rhs));  // Delegate to Base (pointer move)
    return *this;
  }

  void swap(vector& other) {
    assert(this->get_allocator() == other.get_allocator());
    if (in_sbo() || other.in_sbo())
      std::swap(*this, other);  // Do it the hard way
    else
      Base::swap(other);  // Pointer swap
  }

  allocator_type get_allocator() const {
    return Base::get_allocator().upstream();
  }
};

template <class T>
struct null_allocator
{
  using value_type = T;

  null_allocator() = default;
  template <class U> null_allocator(const null_allocator<U>&) { }

  T *allocate(std::size_t) { throw std::bad_alloc(); }
  void deallocate(T*, std::size_t) { }

  constexpr std::size_t max_size() const { return 0; }
};

template <class T, class U>
inline bool operator==(null_allocator<T>, null_allocator<U>)
{
    return true;
}

template <class T, class U>
inline bool operator!=(null_allocator<T>, null_allocator<U>)
{
    return false;
}

// ALIASES
template <class T, size_t CAP, class UPSTREAM = allocator<T>>
using sbo_vector = vector<T, sbo_allocator<T, CAP, UPSTREAM>>;

template <class T, size_t CAP>
using static_vector = vector<T, sbo_allocator<T, CAP, null_allocator<T>>>;

}  // close namespace std
