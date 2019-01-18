# util工具库,C/C++代码分离,可运行于linux,windows,mac平台，提供实用类和方法.

C提供基本数据结构(list,rbtree,hashtable,tree),哈希算法,URL解析随机数,可靠UDP接口,json格式解析,数据库接口,基本3D形状投射的碰撞查询(射线、线段、平面、三角形、立方体、球体、胶囊体)

各个系统平台API/crt封装以实现跨平台(包括文件,套接字,进程线程操作,原子操作,锁,io通知,内存,时间,字符串,错误码统一,少量常用的编解码,杂项信息统计).
在此基础上实现定时器,消息队列,网络NIO,日志,协议格式解析

C++部分实现了部分C++11的库以兼容C++98,包括nullptr,std::unique_ptr,std::unordered_map,std::unordered_set,exception macro
如果开启C++11支持,则调用C++标准库,否则调用上述实现.

可直接集成到项目中,不与其他部分冲突.

编译需注意:

linux下需要安装一下uuid,openssl.

作者QQ：976784480
