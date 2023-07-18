#include <stdlib.h>
#include <sys/mman.h>

int main() {
    for (int i = 0; i < 1000 * 1000; i++)
        munmap(mmap(0, i + 1, PROT_NONE, MAP_PRIVATE, 0, 0), i + 1);
    return 0;
}
// mmap()用于创建新映射内存； munmap()用于释放映射的内存；
//这段代码在每次迭代中都会分配一块不同大小的内存，并立即释放它。这样的行为可能会导致系统上的内存分配和释放频繁进行，可能对系统的性能产生一定的影响。
