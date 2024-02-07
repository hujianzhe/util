# util工具库,C/C++代码分离,可运行于linux/windows/macOS平台,可直接集成到项目中,或抽取部分文件使用.  
设计原则:单一接口和模块尽可能做到绝对原子和功能分层设计，绝不做不属于它本身的事情(例如绝不把协程与io混为一个大模块，协程就是协程，io就是io).  
  
	util/  
		.gitignore					用于git忽略一些无用文件  
		lib_compile.sh				unix系统编译静态链接库脚本  
		so_compile.sh				unix系统编译动态链接库脚本  

		CPP部分  
		cpp_inc/  
			astar						基于格子和邻接点的A*寻路算法  
			coroutine_default_sche.h	基于C++20的无栈协程调度器,nodejs风格  
			coroutine_helper.h			基于C++20的无栈协程结构定义,nodejs风格  
			cpp_compiler_define			判断编译器当前指定的CPP版本  
			heap_timer					基于标准库堆结构实现的定时器  
			lexical_cast				简陋但可用的通用类型转换接口  
			misc						一些无法归类的,方便兼容C风格的  
			string_helper				一些简陋的字符串分割和to_string方法  

		纯C部分  
		inc/  
			all						自动include所有库内头文件  
			compiler_define			根据编译器不同,给出统一的关键字,定一缺失类型,必须的预处理语句,频闭不需要的警告等  
			platform_define			根据系统平台的不同,给出统一的关键字,定义缺失类型,必须的预处理语句,频闭不需要的警告等  
  
			component/  
				net_channel_rw		基于下面的reactor模块提供TCP/UDP传输,并发的可靠UDP传输与监听  
				dataqueue			用于线程间通信的消息队列  
				log					用于日志读写,支持异步/同步写入文件,且内置日志轮替机制  
				memheap_mt			基于共享内存的多进程/线程安全的内存管理  
				memref				基于引用计数实现的内存强引用和观察者  
				reactor				Reactor模型的网络套接字库  
				rbtimer				一个基于红黑树结构的定时器模块  
				stack_co_sche		基于系统平台API实现的有栈协程调度器  
				switch_co_sche		基于switch case语法的无栈协程调度器  
			crt/  
				geometry/  
					geometry_def	3D几何体定义(点,线段,平面,立方盒,球,多边形,多面体)  
					collision		3D几何体包围盒计算/相交检测/包含检测/扫掠检测接口  
				protocol/  
					hiredis_cli_protocol	基于hiredis代码的裁剪,只保留了客户端对RESP协议解析和构造部分  
					httpframe			用于解析与组装HTTP协议报文  
					websocketframe		用于解析与组装WebSocket协议报文(13版本)  
				cXML				用于解析XML  
				dynarr				模拟泛型的动态数组  
				json				用于解析JSON,沿用cJSON的命名风格,内部采用和cJSON不同的实现方式  
				math				一些实用的数学运算接口  
				math_matrix3		一些3d矩阵运算  
				math_vec3			一些3d向量运算  
				math_quat			一些3d四元数运算  
				octree				八叉树  
			datastruct/  
				arrheap				最小堆,最大堆  
				base64				提供base64编解码接口  
				bstree				类型无关的二叉搜索树  
				hash				提供一些常用的hash算法  
				hashtable			类型无关的哈希表  
				lengthfieldframe	用于解析与组装包含长度字段的协议报文  
				list				类型无关的双向链表,顺带支持栈/队列的PUSH/POP操作  
				md5					MD5编码生成  
				memfunc				不涉及分配释放的内存与字符串操作函数  
				memheap				简单内存堆分配与释放  
				random				随机数算法,提供rand48与MT19937算法  
				rbtree				类型无关的红黑树(内部基于linux内核红黑树代码)  
				serial_exec			任务串行执行队列结构  
				sha1				SHA1编解码,来源于Redis源码  
				transport_ctx		提供ACK确认与滑动窗口的传输控制接口,不包含OS对应的IO系统接口,并于标准库无关  
				tree				类型无关的普通树  
				url					URL解析与编解码接口  
			sysapi/  
				aio					提供统一AIO接口(基于iocp/io_uring)  
				assert				提供一个相对于assert的高级断言  
				atomic				提供统一的原子操作接口  
				error				提供统一的系统错误码接口  
				file				提供统一的文件与目录操作接口  
				io_overlapped		提供OVERLAPPED结构与接口,用于NIO/AIO  
				ipc					提供统一的OS锁接口  
				misc				杂项接口  
				mmap				提供统一的文件内存映射与共享内存接口  
				nio					提供统一网络NIO接口(基于iocp/epoll/kevent,reactor模式)  
				process				提供统一的进程/线程/协程接口  
				socket				提供统一的socket接口  
				statistics			一些杂项统计接口  
				terminal			提供终端控制台的操作接口  
				time				提供统一的线程安全的时间接口  
