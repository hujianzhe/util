# util工具库,C/C++代码分离,可运行于linux/windows/mac平台,提供实用类和方法,可直接集成到项目中,或抽取部分文件使用.  
作者QQ：976784480  
目录结构:  
util/  
	.gitignore					用于git忽略一些无用文件  
	lib_compile.sh				unix系统编译静态链接库脚本  
	so_compile.sh				unix系统编译动态链接库脚本  
	unzip.sh					unix根据文件后缀名自动解压缩脚本  
	inc/  
		all						自动include所有库内头文件  
		compiler_define			根据编译器不同,给出统一的关键字,定一缺失类型,必须的预处理语句,频闭不需要的警告等  
		platform_define			根据系统平台的不同,给出统一的关键字,定义缺失类型,必须的预处理语句,频闭不需要的警告等  
		-- component/  
			cJSON				用于解析JSON,修改了内部原cJSON的代码和一些BUG  
			cXML				用于解析XML  
			dataqueue			用于线程间通信的消息队列  
			db					用于与数据库交互的通用接口,屏蔽了不同数据库的CRUD接口,目前内部实现只支持MYSQL  
			httpfram			用于解析与组装HTTP协议报文  
			lengthfieldframe	用于解析与组装包含长度字段的协议报文  
			websocketframe		用于解析与组装WebSocket协议报文(13版本)  
			log					用于日志读写,支持异步/同步写入文件与控制台两种模式  
			niosocket			一个网络通讯框架,基于各平台NIO接口的事件机制,支持普通的TCP/UDP传输,可靠UDP传输,带ACK确认的TCP传输,  
								提供断线重连机制(可靠通讯模式下内部提供包缓存机制,重连后可以将之前未确认的数据包重传)  
			collision_detection	一个3D碰撞检测接口,支持射线/AABB/球/胶囊/平面/三角形之间的方向投射检测  
			rbtimer				一个基于红黑树结构的定时器模块接口(纯净的,比如内部不创建任何调度线程)  
		datastruct/  
				hash			提供一些常用的hash算法  
				hashtable		类型无关的哈希表  
				rbtree			类型无关的红黑树(内部基于linux内核红黑树代码)  
				list			类型无关的双向链表  
				random			随机数算法,提供rand48与MT19937算法  
				strings			一些安全的字符串操作接口  
				tree			类型无关的普通树  
				sort			合并有序数组,topN统计接口  
				url				URL解析  
				sha1			SHA1编解码,来源于Redis源码  
				value_type		通用类型结构  
		sysapi/  
				alloca			提供统一的内存对齐的分配释放接口  
				assert			提供一个相对于assert的高级断言  
				atomic			提供统一的原子操作接口  
				crypt			提供CRC32,MD5,SHA1,BASE64编解码(类unix平台需要基于openssl库)  
				error			提供统一的系统错误码接口  
				file			提供统一的文件与目录操作接口  
				io				提供统一的文件AIO接口,网络NIO接口(基于iocp/epoll/kevent,reactor模式)  
				ipc				提供统一的同步接口  
				math			一些实用的数学运算接口  
				mmap			提供统一的文件内存映射与共享内存接口  
				process			提供统一的进程/线程/协程接口  
				socket			提供统一的socket接口  
				statistics		一些杂项统计接口  
				terminal		提供获取终端名字,统一的kbhit/getch接口  
				time			提供统一的线程安全的时间接口  
				uuid			提供统一的UUID获取接口(类unix平台需要基于openssl库)  
		c++/  
			cpp_compiler_define	判断编译器当前指定的CPP版本,一些可以兼容98标准的关键字的定义  
			exception			包含文件名称,行号,出错语句的异常  
			nullptr				给98标准的编译器提供nullptr关键字  
			unique_ptr			给98标准的编译器提供unique_ptr  
			unordered_map		给98标准的编译器提供一个简单的哈希表键值对(与标准库相同的CRUD接口,但不支持其他STL接口风骚的用法)  
			unordered_set		给98标准的编译器提供一个简单的哈希表键集合(同上)  