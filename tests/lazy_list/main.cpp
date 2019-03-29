#include "lazy_list.hpp"

#include <cstdio>

#include <thread>
#include <mutex>
#include <condition_variable>

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    std::mutex lock;
    std::condition_variable cv;
    bool ready = false;
    LazyList ll;

    auto list_insert = [&](int num_ops, uintptr_t* ops) {
        std::unique_lock<std::mutex> l(lock);

        while(!ready) { cv.wait(l); }

        for(int i = 0; i < num_ops; i++)
        {
            ll.add(ops[i]);
        }

        printf("a done\n");
    };

    auto list_remove = [&](int num_ops, uintptr_t* ops) {
        std::unique_lock<std::mutex> l(lock);

        while(!ready) { cv.wait(l); }

        for(int i = 0; i < num_ops; i++)
        {
            Node* l;
            ll.remove(ops[i], &l);
        }

        printf("r done\n");
    };

    uintptr_t ops1[] = { 1, 2, 3, 4, 5 };
    uintptr_t ops2[] = { 6, 7, 8, 9, 10};
    uintptr_t ops3[] = { 2, 3, 8, 10, 1};
    uintptr_t ops4[] = { 13, 2, 12, 4, 11};

    std::thread a1(list_insert, 5, ops1);
    std::thread a2(list_insert, 5, ops2);
    std::thread a3(list_insert, 5, ops3);
    std::thread a4(list_insert, 5, ops4);

    std::thread d1(list_remove, 5, ops4);
    std::thread d2(list_remove, 5, ops3);
    std::thread d3(list_remove, 5, ops2);
    std::thread d4(list_remove, 5, ops1);

    lock.lock();
    ready = true;
    cv.notify_all();
    lock.unlock();

    a1.join();
    a2.join();
    a3.join();
    a4.join();

    d1.join();
    d2.join();
    d3.join();
    d4.join();

    ll.print();

    return 0;
}
