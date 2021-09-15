#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

const size_t LOG_S = 24;
const size_t SIZE = 1ull << LOG_S;
size_t side_effect = 0;
double unit = 1;

struct Entry
{
    uint32_t value;
    uint32_t next_value;
    bool operator<(const Entry& a) const { return value < a.value; }
};

Entry arr[SIZE];

typedef std::chrono::time_point<std::chrono::high_resolution_clock> now_t;
static now_t now() { return std::chrono::high_resolution_clock::now(); }

const char *expand_name(const char *n)
{
    static char buf[128];
    std::strcpy(buf, n);
    size_t len = strlen(n);
    while (len < 20)
        buf[len++] = ' ';
    buf[len] = 0;
    return buf;
}
#define NAME (expand_name(__func__))

static void warmup(int n)
{
    now_t start = now();
    n++;
    const size_t COUNT = 1024ull * 1024 * 1024;
    for (size_t i = 0; i < COUNT; i++)
        n = n * 13 + 17;
    side_effect += n % 2;
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double milliseconds = diff.count() * 1000;
    std::cout << "Warmup for " << milliseconds << "ms" << std::endl;
}

static void __attribute__((noinline))
add_cmp_jump(uint64_t COUNT = 0x40000000)
{
    now_t start = now();

    __asm__ volatile(
    "label0:\n"
        "add    $1, %%rbx\n"
        "cmp    %%rcx, %%rbx\n" // COUNT
        "jne    label0\n"
    :: "a"(1), "b"(0), "c"(COUNT), "d"(1)
    );

    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    unit = ns_per_op;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
xor_xor_add_cmp_jump(uint64_t COUNT = 0x40000000)
{
    now_t start = now();

    __asm__ volatile(
    "label1:\n"
        "xor    %%rax,%%rax\n"
        "xor    %%rax,%%rax\n"
        "add    $1, %%rbx\n"
        "cmp    %%rcx, %%rbx\n" // COUNT
        "jne    label1\n"
    :: "a"(1), "b"(0), "c"(COUNT), "d"(1)
    );

    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
mul_add_cmp_jump(uint64_t COUNT = 0x40000000)
{
    now_t start = now();

    __asm__ volatile(
    "label2:\n"
        "mulq   %%rax\n"
        "add    $1, %%rbx\n"
        "cmp    %%rcx, %%rbx\n" // COUNT
        "jne    label2\n"
    :: "a"(13), "b"(0), "c"(COUNT), "d"(1)
    );

    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
div_add_cmp_jump(uint64_t COUNT = 0x4000000)
{
    now_t start = now();

    __asm__ volatile(
    "label3:\n"
        "divq   %%rax\n"
        "add    $1, %%rbx\n"
        "cmp    %%rcx, %%rbx\n" // COUNT
        "jne    label3\n"
    :: "a"(1), "b"(0), "c"(COUNT), "d"(0)
    );

    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static uint64_t __attribute__((noinline)) int_mul_act(uint64_t factor, uint64_t COUNT)
{
    uint64_t test = 1;
    for (size_t i = 0; i < COUNT; i++)
        test *= factor;
    return test;
}

static void int_mul()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x4000000;
    uint64_t test = int_mul_act(13, COUNT);
    now_t stop = now();
    side_effect += test > 2 ? 1 : 0;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static uint64_t __attribute__((noinline)) int_div_act(uint64_t factor, uint64_t COUNT)
{
    uint64_t test = INT64_MAX;
    for (size_t i = 0; i < COUNT; i++)
    {
        test /= factor;
        test <<= 4;
    }
    return test;
}

static void int_div()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x4000000;
    uint64_t test = int_div_act(4, COUNT);
    now_t stop = now();
    side_effect += test > 2 ? 1 : 0;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

typedef __uint128_t uint128_t;

static uint128_t __attribute__((noinline)) int128_mul_act(uint128_t factor, uint64_t COUNT)
{
    uint128_t test = 1;
    for (size_t i = 0; i < COUNT; i++)
        test *= factor;
    return test;
}

static void int128_mul()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x4000000;
    uint128_t test = int128_mul_act(13, COUNT);
    now_t stop = now();
    side_effect += test > 2 ? 1 : 0;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static uint128_t __attribute__((noinline)) int128_div_act(uint128_t factor, uint64_t COUNT)
{
    uint128_t test = INT64_MAX;
    for (size_t i = 0; i < COUNT; i++)
    {
        test /= factor;
        test <<= 4;
    }
    return test;
}

static void int128_div()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x4000000;
    uint128_t test = int128_div_act(4, COUNT);
    now_t stop = now();
    side_effect += test > 2 ? 1 : 0;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static double __attribute__((noinline)) dbl_mul_act(uint64_t COUNT)
{
    double test = 1.;
    double factor = 1. + 1. / COUNT;
    for (size_t i = 0; i < COUNT; i++)
        test *= factor;
    return test;
}

static void dbl_mul()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x4000000;
    double test = dbl_mul_act(COUNT);
    now_t stop = now();
    side_effect += test > 2 ? 1 : 0;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static double __attribute__((noinline)) dbl_div_act(uint64_t COUNT)
{
    double test = 1.;
    double factor = 1. + 1. / COUNT;
    for (size_t i = 0; i < COUNT; i++)
        test /= factor;
    return test;
}

static void dbl_div()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x4000000;
    double test = dbl_div_act(COUNT);
    now_t stop = now();
    side_effect += test > 2 ? 1 : 0;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static double __attribute__((noinline)) dbl128_mul_act(uint64_t COUNT)
{
    __float128 test = 1.;
    __float128 factor = 1. + 1. / COUNT;
    for (size_t i = 0; i < COUNT; i++)
        test *= factor;
    return test;
}

static void dbl128_mul()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x4000000;
    __float128 test = dbl128_mul_act(COUNT);
    now_t stop = now();
    side_effect += test > 2 ? 1 : 0;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static double __attribute__((noinline)) dbl128_div_act(uint64_t COUNT)
{
    __float128 test = 1.;
    __float128 factor = 1. + 1. / COUNT;
    for (size_t i = 0; i < COUNT; i++)
        test /= factor;
    return test;
}

static void dbl128_div()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x400000;
    __float128 test = dbl128_div_act(COUNT);
    now_t stop = now();
    side_effect += test > 2 ? 1 : 0;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
same_addr_access(const size_t COUNT = SIZE)
{
    arr[0].next_value = 0;

    now_t start = now();
    uint32_t pos = 0;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        pos = arr[pos].next_value;
    }
    side_effect += pos;
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
sequential_access(const size_t COUNT = SIZE)
{
    for (uint32_t i = 0; i < SIZE; i++)
    {
        arr[i].next_value = i + 1;
    }

    now_t start = now();
    uint32_t pos = 0;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        pos = arr[pos].next_value;
    }
    side_effect += pos;
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
random_access(const size_t COUNT = SIZE)
{
    for (uint32_t i = 0; i < SIZE; i++)
    {
        arr[i].value = i - 1;
        arr[i].next_value = i + 1;
    }
    for (uint32_t i = 0; i < SIZE; i++)
    {
        uint32_t a = std::rand() % (SIZE - 2) + 1;
        uint32_t b = std::rand() % (SIZE - 2) + 1;
        Entry ea = arr[a];
        Entry eb = arr[b];
        arr[ea.value].next_value = b;
        arr[ea.next_value].value = b;
        arr[eb.value].next_value = a;
        arr[eb.next_value].value = a;
        std::swap(arr[a], arr[b]);
    }

    now_t start = now();
    uint32_t pos = 0;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        pos = arr[pos].next_value;
    }
    if (pos != SIZE)
        std::cout << "Wrong shuffle!" << std::endl;
    side_effect += pos % 2;
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
predictable_branch(const size_t COUNT = SIZE)
{
    for (uint32_t i = 0; i < SIZE; i++)
    {
        arr[i].value = rand() % 1000;
    }

    now_t start = now();
    uint32_t res = 0;
    size_t made_count = COUNT;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        if (arr[i].value < 1000)
        {
            res++;
            made_count--;
            i++;
        }
    }
    side_effect += res % 2;
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / made_count;
    double Mrps = made_count / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
unpredictable_branch(const size_t COUNT = SIZE)
{
    for (uint32_t i = 0; i < SIZE; i++)
    {
        arr[i].value = rand() % 1000;
    }

    now_t start = now();
    uint32_t res = 0;
    size_t made_count = COUNT;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        if (arr[i].value < 500)
        {
            res++;
            made_count--;
            i++;
        }
    }
    side_effect += res % 2;
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / made_count;
    double Mrps = made_count / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static volatile bool must_throw = false;

static void __attribute__((noinline))
try_reference()
{
    now_t start = now();
    const size_t COUNT = SIZE;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        if (must_throw)
            throw 0;
    }
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;

}

static void __attribute__((noinline))
try_nothrow(const size_t COUNT = SIZE)
{
    now_t start = now();
    for (uint32_t i = 0; i < COUNT; i++)
    {
        try {
            if (must_throw)
                throw 0;
        } catch (int e) {
            std::cout << "Can't be" << std::endl;
        }
    }
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
try_throw(const size_t COUNT = SIZE / 10)
{
    must_throw = true;
    now_t start = now();
    for (uint32_t i = 0; i < COUNT; i++)
    {
        try {
            if (must_throw)
                throw 0;
            std::cout << "Can't be" << std::endl;
        } catch (int e) {
        }
    }
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
func_call_reference(size_t n)
{
    size_t res = 0;
    now_t start = now();
    const size_t COUNT = 0x10000000;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        size_t x = res;
        res += n;
        res += x;
    }
    now_t stop = now();
    side_effect += res % 2;


    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

size_t f_inline(size_t &x, size_t n)
{
    size_t r = x;
    x += n;
    return r;
}

static void __attribute__((noinline))
func_call_inline(size_t n)
{
    size_t res = 0;
    now_t start = now();
    const size_t COUNT = 0x10000000;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        size_t x = f_inline(res, n);
        res += x;
    }
    now_t stop = now();
    side_effect += res % 2;


    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

struct SomeSingleton
{
    SomeSingleton() { side_effect++; }
    ~SomeSingleton() { side_effect++; }
};

size_t f_with_static_variable(size_t &x, size_t n)
{
    static SomeSingleton instance;
    size_t r = x;
    x += n;
    return r;
}

static void __attribute__((noinline))
func_call_static_var(size_t n)
{
    size_t res = 0;
    now_t start = now();
    const size_t COUNT = 0x10000000;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        size_t x = f_with_static_variable(res, n);
        res += x;
    }
    now_t stop = now();
    side_effect += res % 2;


    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

size_t __attribute__((noinline)) f_noinline(size_t &x, size_t n)
{
    size_t r = x;
    x += n;
    return r;
}

static void __attribute__((noinline))
func_call_noinline(size_t n)
{
    size_t res = 0;
    now_t start = now();
    const size_t COUNT = 0x10000000;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        size_t x = f_noinline(res, n);
        res += x;
    }
    now_t stop = now();
    side_effect += res % 2;


    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

struct A
{
    virtual size_t f(size_t &x, size_t n) = 0;
    virtual ~A() {}

};

struct B : A
{
    virtual size_t f(size_t &x, size_t n)
    {
        size_t r = x;
        x += n;
        return r;
    }
};

static void __attribute__((noinline))
func_call_virtual(size_t n)
{
    size_t res = 0;
    now_t start = now();
    const size_t COUNT = 0x10000000;
    A *a = new B;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        size_t x = a->f(res, n);
        res += x;
    }
    now_t stop = now();
    side_effect += res % 2;
    delete a;


    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

typedef void* (*f_t)(size_t &);

static void *f1(size_t& x);
static void *f2(size_t& x);
static void *f3(size_t& x);
static void *f4(size_t& x);
static void *f5(size_t& x);
static void *f6(size_t& x);
static void *f7(size_t& x);
static void *f8(size_t& x);

static void *f1(size_t& x) { x++; return (void*)f2; }
static void *f2(size_t& x) { x--; return (void*)f3; }
static void *f3(size_t& x) { (void)x; return (void*)f4; }
static void *f4(size_t& x) { x ^= 12; return (void*)f5; }
static void *f5(size_t& x) { x ^= 13; return (void*)f6; }
static void *f6(size_t& x) { x ^= 14; return (void*)f7; }
static void *f7(size_t& x) { x ^= 15; return (void*)f8; }
static void *f8(size_t& x) { x ^= 16; return (void*)f1; }

static void __attribute__((noinline))
func_call_by_pointer(const size_t COUNT = 0x10000000)
{
    f_t f = (f_t)f1;
    now_t start = now();
    size_t res = 0;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        f = (f_t)f(res);
    }
    now_t stop = now();
    side_effect += res % 2;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
func_call_by_ptr_arr(const size_t COUNT = SIZE)
{
    f_t f[8] = {f1, f2, f3, f4, f5, f6, f7, f8};
    for (uint32_t i = 0; i < COUNT; i++)
    {
        arr[i].value = rand();
    }

    now_t start = now();
    size_t res = 0;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        f[(res + arr[i].value) % 8](res);
    }
    now_t stop = now();
    side_effect += res % 2;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
atomic_inc_stl(const size_t COUNT = 0x10000000)
{
    std::atomic<uint64_t> c;
    now_t start = now();
    for (size_t i = 0; i < COUNT; i++)
    {
        ++c;
    }
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
atomic_inc_acq_rel(const size_t COUNT = 0x10000000)
{
    uint64_t c;
    now_t start = now();
    for (size_t i = 0; i < COUNT; i++)
    {
        __atomic_add_fetch(&c, 1, __ATOMIC_ACQ_REL);
    }
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
atomic_inc_relax(const size_t COUNT = 0x10000000)
{
    uint64_t c;
    now_t start = now();
    for (size_t i = 0; i < COUNT; i++)
    {
        __atomic_add_fetch(&c, 1, __ATOMIC_RELAXED);
    }
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
mutex_lock_unlock(const size_t COUNT = 0x1000000)
{
    std::mutex m;
    now_t start = now();
    for (uint32_t i = 0; i < COUNT; i++)
    {
        std::lock_guard<std::mutex> l(m);
    }
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void __attribute__((noinline))
sys_call(const size_t COUNT = SIZE / 16)
{
    int fd[2];
    if (pipe(fd))
        std::cout << "Pipe failed!!" << std::endl;
    int wr = fd[1];
    int rd = fd[0];

    now_t start = now();
    for (uint32_t i = 0; i < COUNT; i += 2) // 2 sys calls per cycle
    {
        char x = '?';
        int r = write(wr, &x, 1);
        (void)r;
        r = read(rd, &x, 1);
        (void)r;
    }
    now_t stop = now();

    close(fd[0]);
    close(fd[1]);

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void thread_start_stop_func()
{
}

static void __attribute__((noinline))
thread_start_stop(const size_t COUNT = SIZE / 1024)
{
    now_t start = now();
    for (uint32_t i = 0; i < COUNT; i++)
    {
        std::thread t(thread_start_stop_func);
        t.join();
    }
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

int main(int n, const char**)
{
    std::cout << "Array size: " << sizeof(arr) / 1024 / 1024 << "MB" << std::endl;
    std::srand(time(0));

    warmup(n);

    std::cout << " --- simple operations ---" << std::endl;
    add_cmp_jump();
    xor_xor_add_cmp_jump();
    mul_add_cmp_jump();
    div_add_cmp_jump();
    int_mul();
    int_div();
    int128_mul();
    int128_div();

    std::cout << " --- floating points ---" << std::endl;
    dbl_mul();
    dbl_div();
    dbl128_mul();
    dbl128_div();

    std::cout << " --- memory access ---" << std::endl;
    same_addr_access();
    sequential_access();
    random_access();

    std::cout << " --- branching ---" << std::endl;
    predictable_branch();
    unpredictable_branch();

    std::cout << " --- C++ ---" << std::endl;
    try_reference();
    try_nothrow();
    try_throw();

    std::cout << " --- other ---" << std::endl;
    func_call_reference(n);
    func_call_inline(n);
    func_call_static_var(n);
    func_call_noinline(n);
    func_call_virtual(n);
    func_call_by_pointer();
    func_call_by_ptr_arr();
    atomic_inc_stl();
    atomic_inc_acq_rel();
    atomic_inc_relax();
    mutex_lock_unlock();
    sys_call();
    thread_start_stop();

    std::cout << "Side effect (ignore it) " << side_effect % 2 << std::endl;
}
