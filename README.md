# Dynamic_measurement_of_process_integrity
关键词：进程完整性，动态度量，可信计算，代码段，分页度量，ELF

### 简介
这是一个简单地内核模块，主要完成了以下几项工作：
* 分页读取ELF文件的代码段，进行hash运算后存储基准值。
* 根据/proc/pid/pagemap文件，判断该进程虚拟地址与物理地址的映射情况。
* 如果该虚拟地址已经分配了物理地址，则读取该物理页的数据，进行hash运算并与基准值对比，判断进程的代码段是否被篡改。

### 环境
* centos 7

### 步骤
1.使用"make"进行编译。
```
make
```
2.加载模块并打印日志。
```
make testload > tmp.txt"
```
3.卸载模块。
```
make testunload
```
4.如果重新编译模块，先执行"make clean"，再进行"make"。
```
make clean
make
```

### 参考
* [对于结构体指针+、-常数的理解(page_to_pfn和pfn_to_page)](http://blog.chinaunix.net/uid-20564848-id-72855.html?utm_source=jiancool)
* [Linux用户程序如何访问物理内存](http://ilinuxkernel.com/?p=1248x	)
* [Linux中的虚拟地址、物理地址和内存管理方式](https://blog.csdn.net/avrmcu1/article/details/16939063 )
* [利用mmap /dev/mem 读写Linux内存](https://blog.csdn.net/zhanglei4214/article/details/6653568 )
* [判断内存地址是否缺页](https://blog.csdn.net/demonshir/article/details/50414839 )
* [写自己的内核模块——获取一个进程的物理地址](https://blog.csdn.net/sium__/article/details/49700971 )
* [copy_from_user分析](https://www.cnblogs.com/rongpmcu/p/7662749.html )
