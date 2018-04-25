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
// __attribute__((noinline))

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

static void add_cmp_jump()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x40000000;

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

static void xor_xor_add_cmp_jump()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x40000000;

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

static void mul_add_cmp_jump()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x40000000;

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

static void div_add_cmp_jump()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x4000000;

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

static void dbl_mul()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x4000000;

    double test = 1.;
    double factor = 1. + 1. / COUNT;
    for (size_t i = 0; i < COUNT; i++)
        test *= factor;
    now_t stop = now();
    side_effect += test > 2 ? 1 : 0;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void dbl_div()
{
    now_t start = now();
    const volatile uint64_t COUNT = 0x4000000;

    double test = 1.;
    double factor = 1. + 1. / COUNT;
    for (size_t i = 0; i < COUNT; i++)
        test /= factor;
    now_t stop = now();
    side_effect += test > 2 ? 1 : 0;

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void same_addr_access()
{
    arr[0].next_value = 0;

    now_t start = now();
    const size_t COUNT = SIZE;
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

static void sequential_access()
{
    for (uint32_t i = 0; i < SIZE; i++)
    {
        arr[i].next_value = i + 1;
    }

    now_t start = now();
    const size_t COUNT = SIZE;
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

static void random_access()
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
    const size_t COUNT = SIZE;
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

static void predictable_branch()
{
    for (uint32_t i = 0; i < SIZE; i++)
    {
        arr[i].value = rand() % 1000;
    }

    now_t start = now();
    size_t COUNT = SIZE;
    uint32_t res = 0;
    for (uint32_t i = 0; i < COUNT; i++)
    {
        if (arr[i].value < 1000)
        {
            res++;
            COUNT--;
            i++;
        }
    }
    side_effect += res % 2;
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static void unpredictable_branch()
{
    for (uint32_t i = 0; i < SIZE; i++)
    {
        arr[i].value = rand() % 1000;
    }

    now_t start = now();
    size_t COUNT = SIZE;
    uint32_t res = 0;
    for (uint32_t i = 0; i < SIZE; i++)
    {
        if (arr[i].value < 500)
        {
            res++;
            COUNT--;
            i++;
        }
    }
    side_effect += res % 2;
    now_t stop = now();

    std::chrono::duration<double> diff = stop - start;
    double ns_per_op = diff.count() * 1000 * 1000 * 1000 / COUNT;
    double Mrps = COUNT / diff.count() / 1000 / 1000;
    std::cout << NAME << ": " << ns_per_op << "ns per operation,\t"
              << (ns_per_op / unit) << " ticks,\t"
              << Mrps << " Mrps" << std::endl;
}

static volatile bool must_throw = false;

static void try_reference()
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

static void try_nothrow()
{
    now_t start = now();
    const size_t COUNT = SIZE;
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

static void try_throw()
{
    must_throw = true;
    now_t start = now();
    const size_t COUNT = SIZE / 10;
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

static void func_call_reference(size_t n)
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

static void func_call_inline(size_t n)
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

size_t __attribute__((noinline)) f_noinline(size_t &x, size_t n)
{
    size_t r = x;
    x += n;
    return r;
}

static void func_call_noinline(size_t n)
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

static void func_call_virtual(size_t n)
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

static void __attribute__((noinline)) func_call_by_pointer()
{
    f_t f = (f_t)f1;
    now_t start = now();
    const size_t COUNT = 0x10000000;
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

static void func_call_by_ptr_arr()
{
    f_t f[8] = {f1, f2, f3, f4, f5, f6, f7, f8};
    for (uint32_t i = 0; i < SIZE; i++)
    {
        arr[i].value = rand();
    }

    now_t start = now();
    const size_t COUNT = SIZE;
    size_t res = 0;
    for (uint32_t i = 0; i < SIZE; i++)
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

static void atomic_one_user()
{
    std::atomic<uint64_t> c;
    now_t start = now();
    const size_t COUNT = 0x10000000;
    for (uint32_t i = 0; i < COUNT; i++)
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

static void mutex_one_user()
{
    std::mutex m;
    now_t start = now();
    const size_t COUNT = 0x10000000;
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

static void sys_call()
{
    int fd[2];
    if (pipe(fd))
        std::cout << "Pipe failed!!" << std::endl;
    int wr = fd[1];
    int rd = fd[0];

    now_t start = now();
    const size_t COUNT = SIZE / 2;
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

static void thread_start_stop()
{
    now_t start = now();
    const size_t COUNT = SIZE / 100;
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

    std::cout << " --- floating points ---" << std::endl;
    dbl_mul();
    dbl_div();

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
    func_call_noinline(n);
    func_call_virtual(n);
    func_call_by_pointer();
    func_call_by_ptr_arr();
    atomic_one_user();
    mutex_one_user();
    sys_call();
    thread_start_stop();


    std::cout << "Side effect (ignore it) " << side_effect % 2 << std::endl;
};