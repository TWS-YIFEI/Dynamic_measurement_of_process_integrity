# Dynamic_measurement_of_process_integrity
关键词：进程完整性，动态度量，代码段，分页度量，ELF

### 简介
这是一个简单地内核模块，主要完成了以下几项工作：
* 分页读取ELF文件的代码段，进行hash运算后存储基准值。
* 根据/proc/pid/pagemap文件，读取运行进程的已经分配物理内存页的数据，进行hash运算并与基准值对比，判断进程的代码段是否被篡改。

### 环境
* centos 7

### 步骤
1.使用"make"进行编译。
2."make testload > tmp.txt"加载模块并打印日志。
3."make testunload"卸载模块。
4.如果重新编译模块，先执行"make clean"，再进行"make"。

