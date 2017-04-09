#include <type_traits>
#include <cstdint>
#include <atomic>
#include <exception>
#include <stdexcept>

template <typename T>
class MarkableReference
{
public:
    MarkableReference(T* ref, bool init_mark)
        : m_reference(reinterpret_cast<uintptr_t>(ref))
    {
        // Most pointers are aligned to some degree, make sure this one is
        // at least 8-byte aligned.
        // This is needed as we're going to be stealing the bottom bit.
        if(m_reference.load() & 0x7)
        {
            throw std::invalid_argument("Pointer given to MarkableReference is not aligned correctly! "
                                        "Must be at least 8-byte aligned!");
        }

        if(init_mark)
        {
            // The mark bit is the least-significant bit
            m_reference |= 0x1;
        }
    }

    //
    // We use a trick provided by the C++ type system where bool is promoted to int 
    // when used in integral expressions. Just a heads-up.
    //
    // This class is modeled after the Java AtomicMarkableReference atomic primitive.
    //

    bool mark(T* expected_ref, bool new_mark)
    {
        uintptr_t expected = reinterpret_cast<uintptr_t>(expected_ref) | (m_reference & 0x1);
        uintptr_t new_value = reinterpret_cast<uintptr_t>(expected_ref) | new_mark;

        return m_reference.compare_exchange_strong(expected, new_value);
    }

    bool compareAndSet(T* expected_ref, T* new_ref, bool expected_mark, bool new_mark)
    {
        uintptr_t expected_value = reinterpret_cast<uintptr_t>(expected_ref) | expected_mark;
        uintptr_t new_value = reinterpret_cast<uintptr_t>(new_ref) | new_mark;

        return m_reference.compare_exchange_strong(expected_value, new_value);
    }

    T* get(bool& mark)
    {
        uintptr_t value = m_reference.load();
        mark = value & 0x1;
        return reinterpret_cast<T*>(value ^ mark);
    }

    void set(T* ref, bool mark)
    {
        uintptr_t value = reinterpret_cast<uintptr_t>(ref) & mark;
        m_reference.store(value);
    }

    T* operator->() const
    {
        return reinterpret_cast<T*>(m_reference.load() ^ (m_reference & 0x1));
    }
protected:
    std::atomic<uintptr_t> m_reference;
};
