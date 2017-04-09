#include "markable_ref.hpp"
#include <iostream>

struct test_type
{
    long val;
    short other_val;
};

int main()
{
    test_type tmp = { .val = 0xDEAD, .other_val = 0xFF};
    test_type tmp_other = { .val = 0xBABE, .other_val = 0xAA};

    MarkableReference<test_type> test_ref(&tmp, false);

    try
    {
        MarkableReference<test_type> test_except((test_type*)((uintptr_t)&tmp + 0x1), false);
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }

    std::cout << std::hex;

    std::cout << "Before mark: " << test_ref->val << ":" << test_ref->other_val << std::endl;
    test_ref.mark(&tmp, true);
    std::cout << "After mark: " << test_ref->val << ":" << test_ref->other_val << std::endl;

    if(!test_ref.compareAndSet(&tmp, &tmp_other, true, true))
    {
        std::cout << "Swap failed" << std::endl;
    }
    std::cout << "After swap: " << test_ref->val << ":" << test_ref->other_val << std::endl;

    test_ref.mark(&tmp_other, false);
    std::cout << "After mark: " << test_ref->val << ":" << test_ref->other_val << std::endl;

    return 0;
}
