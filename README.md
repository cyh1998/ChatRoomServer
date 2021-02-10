# ChatRoomServer
基于Epoll实现的聊天室服务器

#### 编 译
```$xslt
mkdir bulid
cd bulid
cmake ..
make
```
**注：cmake使用的clion自带的3.16版本**

#### 运 行
编译成功后，运行
```$xslt
./ChatRoomServer
```
---
此项目基于对《Linux高性能服务器编程》(游双 著)的学习而完成，空闲之余会更据书中的内容进行代码的补充、更新和完善

更新日志会附上个人博客地址，可以配合代码一起学习
1. _2021.01.26_  Epoll基本内容的学习 [Linux epoll ET模式实现](https://www.jianshu.com/p/ca699516c2db)
2. _2021.02.02_  ChatRoomServer项目基础搭建 [基于Epoll实现的多人聊天室](https://www.jianshu.com/p/c5829b05cdf0)
3. _2021.02.04_  数据流大于自定义数据缓存区时，循环读取数据，保证数据完整
4. _2021.02.04_  统一事件源 [Linux 信号](https://www.jianshu.com/p/10383d4ac963)
5. _2021.02.09_  实现基于升序链表的定时器 [Linux 定时器(二) 基于升序链表实现](https://www.jianshu.com/p/5079184c4aeb)
