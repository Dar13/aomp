// Basic spinlock

#include <atomic>
#include <cstdint>

#include <cstdio>

class RawSpinlock
{
    public:
        RawSpinlock()
            : _mark(UNLOCK_VAL)
        {}

        void lock()
        {
            uint32_t expected = RawSpinlock::UNLOCK_VAL;
            while(!_mark.compare_exchange_strong(expected, LOCK_VAL))
            {
                asm volatile ("pause;");
                expected = RawSpinlock::UNLOCK_VAL;
            }
        }

        void unlock()
        {
            _mark.store(UNLOCK_VAL);
        }

    private:
        const uint32_t LOCK_VAL = 0x1;
        const uint32_t UNLOCK_VAL = 0x0;

        std::atomic<uint32_t> _mark;
};
