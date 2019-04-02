// Harness stuff
#include <mutex>
#include <condition_variable>
#include <thread>
#include <random>
#include <cstdio>
#include <string>
#include <iostream>
#include <chrono>

// list implementations
#include <forward_list>
#include "lazy_list.hpp"

#define STD_LIST 1
//#define DEBUG

#ifdef STD_LIST
std::mutex list_lock;
std::forward_list<uintptr_t> list;
#else
LazyList lazy_list;
#endif

// Test synch stuff
bool ready = false;
std::mutex startup_lock;
std::condition_variable cv;

void l_insert(uintptr_t* values, bool* results, uint32_t num_values)
{
    std::unique_lock<std::mutex> cv_l(startup_lock);
    while(!ready) { cv.wait(cv_l); }

    for(uint32_t i = 0; i < num_values; i++)
    {
#ifdef STD_LIST
        list_lock.lock();
        // LazyList is sorted, do so here as well
#if 0
        list.push_front(values[i]);
        results[i] = true;
#endif
#if 0
        list.push_front(values[i]);
        list.sort();
        list.unique();
        results[i] = true;
#else
        for(auto itr = list.cbegin(); itr != list.cend(); itr++)
        {
            auto next = itr;
            next++;

            if(next != list.end() &&
               (*itr < values[i] && *next > values[i]))
            {
                list.insert_after(itr, values[i]);

                results[i] = true;
                break;
            }
        }
#endif
        list_lock.unlock();
#else
        results[i] = lazy_list.add(values[i]);
#endif
    }
}

void l_remove(uintptr_t* values, bool* results, uint32_t num_values)
{
    std::unique_lock<std::mutex> cv_l(startup_lock);
    while(!ready) { cv.wait(cv_l); }

    for(uint32_t i = 0; i < num_values; i++)
    {
#ifdef STD_LIST
        list_lock.lock();
        list.remove(values[i]);
        // std::forward_list::remove only has a useful return value in C++20...
        results[i] = false;
        list_lock.unlock();
#else
        Node* r;
        results[i] = lazy_list.remove(values[i], &r);
        if(results[i])
            delete r;
#endif
    }
}

void l_contains(uintptr_t* values, bool* results, uint32_t num_values)
{
    std::unique_lock<std::mutex> cv_l(startup_lock);
    while(!ready) { cv.wait(cv_l); }

    for(uint32_t i = 0; i < num_values; i++)
    {
#ifdef STD_LIST
        list_lock.lock();
        for(auto itr = list.cbegin(); itr != list.cend(); itr++)
        {
            if(*itr == values[i])
                results[i] = true;
        }
        list_lock.unlock();
#else
        Node* c;
        results[i] = lazy_list.contains(values[i], &c);
#endif
    }
}

int main(int argc, char** argv)
{
    if(argc < 4)
    {
        printf("Not enough arguments!\n");
        printf("Usage: ./bench <num threads> <num elements> <workload>\n");
        printf("Workload = 1: 50/50, insert/remove\n");
        printf("           2: ~80/20, contains/insert\n");
        printf("           3: ~20/80, contains/insert\n");
        printf("           4: 50/50, contains/insert\n");
        return 1;
    }

    unsigned int num_threads = std::stoul(argv[1]);
    unsigned int num_elements = std::stoul(argv[2]);
    unsigned int workload = std::stoul(argv[3]);

#ifdef DEBUG
    printf("Num threads: %u\n", num_threads);
    printf("Data set size: %u\n", num_elements);
    printf("Workload: %u\n", workload);
#endif

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> dist(1, num_elements - 1);

#ifdef DEBUG
    printf("Element Range: %u - %u\n", dist.min(), dist.max());
#endif

#ifdef STD_LIST
    list.push_front(std::numeric_limits<uintptr_t>::max());
    list.push_front(0);
#endif

#define WORK_TYPE_INS 0
#define WORK_TYPE_REM 1
#define WORK_TYPE_CON 2

    uint32_t *thread_work = new uint32_t[num_threads];
    switch(workload)
    {
        // 50/50
        case 1:
        {
            uint32_t num_ins = (num_threads / 2);
            for(uint32_t i = 0; i < num_ins; i++) { thread_work[i] = WORK_TYPE_INS; }
            for(uint32_t i = num_ins; i < num_threads; i++) { thread_work[i] = WORK_TYPE_REM; }
        }
        break;
        // 80/20, contains/inserts
        case 2:
        {
            uint32_t num_ins = (num_threads * 0.2);
            for(uint32_t i = 0; i < num_ins; i++) { thread_work[i] = WORK_TYPE_INS; }
            for(uint32_t i = num_ins; i < num_threads; i++) { thread_work[i] = WORK_TYPE_CON; }
        }
        break;
        // 20/80, contains/inserts
        case 3:
        {
            uint32_t num_ins = (num_threads * 0.8);
            for(uint32_t i = 0; i < num_ins; i++) { thread_work[i] = WORK_TYPE_INS; }
            for(uint32_t i = num_ins; i < num_threads; i++) { thread_work[i] = WORK_TYPE_CON; }
        }
        break;
        // 50/50, contains/inserts
        case 4:
        {
            uint32_t num_ins = (num_threads / 2);
            for(uint32_t i = 0; i < num_ins; i++) { thread_work[i] = WORK_TYPE_INS; }
            for(uint32_t i = num_ins; i < num_threads; i++) { thread_work[i] = WORK_TYPE_CON; }
        }
        break;
        default:
            printf("Invalid workload selection!\n");
            return 1;
            break;
    }

    // Preload the list
    for(uint32_t i = 0; i < (num_elements / 2); i++)
    {
#ifdef STD_LIST
        list.push_front(dist(generator));
#else
        lazy_list.add(dist(generator));
#endif
    }

#ifdef STD_LIST
    list.sort();
    list.unique();
#endif

    std::thread *ths = new std::thread[num_threads]();

    uintptr_t **value_set = new uintptr_t*[num_threads];
    bool **result_set = new bool*[num_threads];

    for(uint32_t i = 0; i < num_threads; i++)
    {
        // Allocate value set, fill with random values from [0, num_elements)
        value_set[i] = new uintptr_t[num_elements];
        for(uint32_t l = 0; l < num_elements; l++)
        {
            value_set[i][l] = dist(generator);
        }

        // Allocate result sets, ensure all set to zero
        result_set[i] = new bool[num_elements];
        for(uint32_t l = 0; l < num_elements; l++)
        {
            result_set[i][l] = false;
        }

        // Allocate thread object, initialize.
        switch(thread_work[i])
        {
            case WORK_TYPE_INS:
                ths[i] = std::thread(l_insert, value_set[i], result_set[i], num_elements);
                break;
            case WORK_TYPE_REM:
                ths[i] = std::thread(l_remove, value_set[i], result_set[i], num_elements);
                break;
            case WORK_TYPE_CON:
                ths[i] = std::thread(l_contains, value_set[i], result_set[i], num_elements);
                break;
            default:
                printf("Invalid thread work type!\n");
                return 1;
                break;
        }
    }

    printf("Starting benchmark: ");
#ifdef STD_LIST
    printf("std::forward_list (mutexed)\n");
#else
    printf("LazyList (lock-less)\n");
#endif
    fflush(stdout);

    startup_lock.lock();
    ready = true;
    auto start = std::chrono::steady_clock::now();
    cv.notify_all();
    startup_lock.unlock();

    for(uint32_t i = 0; i < num_threads; i++)
    {
        ths[i].join();
    }
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;

    std::cout << std::chrono::duration <double, std::milli> (diff).count() << " ms" << std::endl;

    int num_list_elems = 0;
#ifdef STD_LIST
    for(auto itr = list.cbegin(); itr != list.cend(); itr++)
    {
        num_list_elems++;
    }
#ifdef DEBUG
    printf("Number of elements in list at end of test: %d\n", num_list_elems);
#endif

    list.sort();
    list.unique();

    num_list_elems = 0;
    for(auto itr = list.cbegin(); itr != list.cend(); itr++)
    {
        num_list_elems++;
    }
#ifdef DEBUG
    printf("Number of elements in list after sort/unique: %d\n", num_list_elems);
#endif
#endif

    for(uint32_t i = 0; i < num_threads; i++)
    {
        int succ_op = 0;
        for(uint32_t l = 0; l < num_elements; l++)
        {
            if(result_set[i][l])
                succ_op++;
        }

#ifdef DEBUG
        printf("Thread %u(%d): %d successful operations\n", i, thread_work[i], succ_op);
#endif
    }

    // Clean-up
    for(uint32_t i = 0; i < num_threads; i++)
    {
        delete[] value_set[i];
        delete[] result_set[i];
    }

    delete[] thread_work;
    delete[] ths;
    delete[] value_set;
    delete[] result_set;

    return 0;
}
