#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include <numeric>
#include <memory>

// --- 常量定义，与论文保持一致 ---
constexpr int SIEVE_LIMIT = 100'000'000;
constexpr int PRIME_LIMIT = 10'000;
// 论文中提到每256个条目一个锁
constexpr int LOCK_GRANULARITY = 256;

// 一个简单的自旋锁实现，用于复现实验中的Spinlock版本
class Spinlock {
public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            // 自旋等待
        }
    }
    void unlock() {
        flag.clear(std::memory_order_release);
    }
private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

// --- 抽象基类，处理通用逻辑 ---
class Sieve {
public:
    // 虚析构函数
    virtual ~Sieve() = default;

    // 运行实验并计时
    void run(int num_threads) {
        std::cout << "Running with " << num_threads << " threads..." << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads; // <<-- 修改点1：使用 std::thread
        threads.reserve(num_threads);

        // 将2到PRIME_LIMIT的数字范围分配给各个线程
        int range_per_thread = (PRIME_LIMIT - 2) / num_threads;
        for (int i = 0; i < num_threads; ++i) {
            int start_prime = 2 + i * range_per_thread;
            int end_prime = (i == num_threads - 1) ? PRIME_LIMIT : start_prime + range_per_thread;
            threads.emplace_back(&Sieve::run_thread, this, start_prime, end_prime);
        }

        // <<-- 修改点2：手动等待所有线程完成
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;
        std::cout << "Execution time: " << elapsed.count() << " seconds" << std::endl;
    }

protected:
    // 每个派生类需要实现自己的线程工作函数
    virtual void run_thread(int start_prime, int end_prime) = 0;
};


// --- 版本1: Mutex (互斥锁) ---
class MutexSieve : public Sieve {
public:
    MutexSieve() : bit_array(SIEVE_LIMIT, 0), locks(SIEVE_LIMIT / LOCK_GRANULARITY) {}
protected:
    void run_thread(int start_prime, int end_prime) override {
        for (int p = start_prime; p < end_prime; ++p) {
            if (!get(p)) {
                for (long long i = (long long)p * p; i < SIEVE_LIMIT; i += p) {
                    if (!get(i)) {
                        set(i);
                    }
                }
            }
        }
    }
private:
    bool get(long long index) {
        std::lock_guard<std::mutex> guard(locks[index / LOCK_GRANULARITY]);
        return bit_array[index];
    }
    void set(long long index) {
        std::lock_guard<std::mutex> guard(locks[index / LOCK_GRANULARITY]);
        bit_array[index] = 1;
    }

    std::vector<char> bit_array;
    std::vector<std::mutex> locks;
};

// --- 版本2: Spinlock (自旋锁) ---
class SpinlockSieve : public Sieve {
public:
    SpinlockSieve() : bit_array(SIEVE_LIMIT, 0), locks(SIEVE_LIMIT / LOCK_GRANULARITY) {}
protected:
    void run_thread(int start_prime, int end_prime) override {
        for (int p = start_prime; p < end_prime; ++p) {
            if (!get(p)) {
                for (long long i = (long long)p * p; i < SIEVE_LIMIT; i += p) {
                    if (!get(i)) {
                        set(i);
                    }
                }
            }
        }
    }
private:
    bool get(long long index) {
        std::lock_guard<Spinlock> guard(locks[index / LOCK_GRANULARITY]);
        return bit_array[index];
    }
    void set(long long index) {
        std::lock_guard<Spinlock> guard(locks[index / LOCK_GRANULARITY]);
        bit_array[index] = 1;
    }

    std::vector<char> bit_array;
    std::vector<Spinlock> locks;
};

// --- 版本3: Atomic (原子操作) ---
class AtomicSieve : public Sieve {
public:
    // 初始化atomic vector
    AtomicSieve() : bit_array(SIEVE_LIMIT) {
        for(size_t i = 0; i < SIEVE_LIMIT; ++i) {
            bit_array[i].store(0, std::memory_order_relaxed);
        }
    }
protected:
    void run_thread(int start_prime, int end_prime) override {
        for (int p = start_prime; p < end_prime; ++p) {
            if (bit_array[p].load(std::memory_order_relaxed) == 0) {
                for (long long i = (long long)p * p; i < SIEVE_LIMIT; i += p) {
                   // 避免重复写入，与论文逻辑一致
                   if (bit_array[i].load(std::memory_order_relaxed) == 0) {
                        bit_array[i].store(1, std::memory_order_relaxed);
                   }
                }
            }
        }
    }
private:
    std::vector<std::atomic<char>> bit_array;
};

// --- 版本4: Unsafe (无任何同步) ---
class UnsafeSieve : public Sieve {
public:
    UnsafeSieve() : bit_array(SIEVE_LIMIT, 0) {}
protected:
    void run_thread(int start_prime, int end_prime) override {
        for (int p = start_prime; p < end_prime; ++p) {
            if (bit_array[p] == 0) {
                for (long long i = (long long)p * p; i < SIEVE_LIMIT; i += p) {
                    if (bit_array[i] == 0) {
                         bit_array[i] = 1;
                    }
                }
            }
        }
    }
private:
    std::vector<char> bit_array;
};

// --- main函数，用于解析参数并启动实验 ---
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <num_threads> <version>" << std::endl;
        std::cerr << "Versions: mutex, spinlock, atomic, unsafe" << std::endl;
        return 1;
    }

    int num_threads = std::stoi(argv[1]);
    std::string version = argv[2];

    std::unique_ptr<Sieve> sieve;

    if (version == "mutex") {
        std::cout << "Selected version: Mutex" << std::endl;
        sieve = std::make_unique<MutexSieve>();
    } else if (version == "spinlock") {
        std::cout << "Selected version: Spinlock" << std::endl;
        sieve = std::make_unique<SpinlockSieve>();
    } else if (version == "atomic") {
        std::cout << "Selected version: Atomic" << std::endl;
        sieve = std::make_unique<AtomicSieve>();
    } else if (version == "unsafe") {
        std::cout << "Selected version: Unsafe" << std::endl;
        sieve = std::make_unique<UnsafeSieve>();
    } else {
        std::cerr << "Unknown version: " << version << std::endl;
        return 1;
    }

    if (sieve) {
        sieve->run(num_threads);
    }

    return 0;
}