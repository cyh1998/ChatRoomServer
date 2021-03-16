# ChatRoomServer
基于Epoll实现的聊天室服务器

## 编 译
```$xslt
mkdir bulid
cd bulid
cmake ..
make
```
**注：cmake使用的Clion自带的3.16版本**

## 运 行
运行服务器
```$xslt
./ChatRoomServer
```
运行多进程服务器(不包含定时器)
```$xslt
./MultiProcessServer
```
---
此项目基于对《Linux高性能服务器编程》(游双 著)的学习而完成，空闲之余会更据书中的内容进行代码的补充、更新和完善

更新日志会附上个人博客地址，可以配合代码一起学习
01. _2021.01.26_  Epoll基本内容的学习 [Linux epoll ET模式实现](https://www.jianshu.com/p/ca699516c2db)
02. _2021.02.02_  ChatRoomServer项目基础搭建 [基于Epoll实现的多人聊天室](https://www.jianshu.com/p/c5829b05cdf0)
03. _2021.02.04_  数据流大于自定义数据缓存区时，循环读取数据，保证数据完整
04. _2021.02.04_  统一事件源 [Linux 信号](https://www.jianshu.com/p/10383d4ac963)
05. _2021.02.09_  实现基于升序链表的定时器 [Linux 定时器(二) 基于升序链表实现](https://www.jianshu.com/p/5079184c4aeb)
06. _2021.02.18_  实现简单时间轮 [Linux 定时器(三) 时间轮](https://www.jianshu.com/p/df55c5a1f8c3)
07. _2021.02.20_  实现简单时间堆 [Linux 定时器(四) 时间堆](https://www.jianshu.com/p/e880f398530d)
08. _2021.02.23_  修改CMakeLists为多目录编译 [cmake 深入学习(一)](https://www.jianshu.com/p/41ffb634d30d)
09. _2021.03.03_  实现基于POXIS共享内存的多进程聊天室服务器 [Linux 多进程编程(二) 共享内存](https://www.jianshu.com/p/e41dff9cbec2)
10. _2021.03.12_  封装pthread [Linux pthread封装](https://www.jianshu.com/p/a154747d3f2d)
11. _2021.03.16_  用线程处理信号 [Linux 多线程处理信号](https://www.jianshu.com/p/f6d04653ff36)
