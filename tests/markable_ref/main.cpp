#include "markable_ref.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    int* t = new int;
    *t = 0xDEAD;

    int* s = new int;
    *s = 0xBEEF;

    MarkableReference<int> int_ref(0, false);
    try
    {
        int_ref.set(t, false);
    }
    catch(std::exception& e)
    {
        return 1;
    }

    if(!int_ref.mark(t, true))
    {
        return 1;
    }

    if(int_ref.compareAndSet(s, s, false, true))
    {
        return 1;
    }

    if(!int_ref.compareAndSet(t, s, true, false))
    {
        return 1;
    }

    delete t;

    return 0;
}
