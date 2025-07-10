# Threads Cannot be Implemented as a Library

本项目是对 Hans-J. Boehm 于2004年发表的经典论文《Threads Cannot be Implemented as a Library》中核心基准测试的现代C++实现。

论文当年的核心论点是：在没有语言层面支持的情况下，纯粹通过库（如 Pthreads）来实现多线程是存在根本性缺陷的。编译器在不知晓并发语义的情况下进行的优化，可能导致难以察觉且致命的程序错误。

本项目旨在：

1. 重现论文中的“埃拉托斯特尼筛法”并行实验。

2. 展示如何使用现代C++标准库（`std::thread`, `std::mutex`, `std::atomic`）来安全、正确地实现不同的并发策略。

✨ 功能特性

1. `mutex`: 使用 `std::mutex` 进行传统的锁保护。
2. `spinlock`: 使用 `std::atomic_flag` 实现一个简单的自旋锁。
3. `atomic`: 使用 `std::atomic` 进行无锁（Lock-Free）操作，这在现代C++中是安全且高效的。
4. `unsafe`: 不使用任何同步措施，故意造成数据竞争（Data Race）作为性能基准。

使用方法：

```bash
./sieve_experiment <线程数> <版本>
```

version：

- `mutex`
- `spinlock`
- `atomic`
- `unsafe`

本项目的核心是 `.github/workflows/benchmark.yml` 中定义的工作流测试代码。

本项目采用 MIT 许可证。详情请见 `LICENSE` 文件。

感谢Hans-J. Boehm撰写了这篇富有洞察力的论文《Threads Cannot be Implemented as a Library》，它深刻地影响了C++等系统级编程语言的发展。
