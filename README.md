# util工具库,C/C++代码分离,可运行于linux/windows/mac平台,提供实用类和方法,可直接集成到项目中,或抽取部分文件使用.  
# 侧重网络通讯,对使用者屏蔽普通TCP/UDP传输,并发的可靠UDP传输与监听,带ACK确认的TCP传输,数据包缓存等实现细节.  
作者QQ：976784480  
  
	util/  
		.gitignore					用于git忽略一些无用文件  
		lib_compile.sh				unix系统编译静态链接库脚本  
		so_compile.sh				unix系统编译动态链接库脚本  
		unzip.sh					unix根据文件后缀名自动解压缩脚本  
		inc/  
			all						自动include所有库内头文件  
			compiler_define			根据编译器不同,给出统一的关键字,定一缺失类型,必须的预处理语句,频闭不需要的警告等  
			platform_define			根据系统平台的不同,给出统一的关键字,定义缺失类型,必须的预处理语句,频闭不需要的警告等  
  
			component/  
				channel				在下面的reactor,transport_ctx模块的基础上扩展封装,可自定义组包/解包格式,对使用者屏蔽普通TCP/UDP传输,并发的可靠UDP传输与监听,带ACK确认的TCP传输,数据包缓存等实现细节  
				cJSON				用于解析JSON,修改了内部原cJSON的代码和一些BUG  
				cXML				用于解析XML  
				dataqueue			用于线程间通信的消息队列  
				db					用于与数据库交互的通用接口,屏蔽了不同数据库的CRUD接口,目前内部实现只支持MYSQL  
				httpfram			用于解析与组装HTTP协议报文  
				lengthfieldframe	用于解析与组装包含长度字段的协议报文  
				websocketframe		用于解析与组装WebSocket协议报文(13版本)  
				log					用于日志读写,支持异步/同步写入文件与控制台两种模式  
				reactor				Reactor模型的事件通知库,支持I/O多路复用,定时/自定义事件,对使用者屏蔽TCP发送,TCP断线重连重发缓存包等细节问题  
				collision_detection	一个3D碰撞检测接口,支持射线/AABB/球/胶囊/平面/三角形之间的方向投射检测  
				rbtimer				一个基于红黑树结构的定时器模块接口(纯净的,比如内部不创建任何调度线程)  
			datastruct/  
					base64			提供base64编解码接口  
					bstree			类型无关的二叉搜索树  
					hash			提供一些常用的hash算法  
					hashtable		类型无关的哈希表  
					rbtree			类型无关的红黑树(内部基于linux内核红黑树代码)  
					list			类型无关的双向链表,顺带支持栈/队列的PUSH/POP操作  
					random			随机数算法,提供rand48与MT19937算法  
					strings			一些安全的字符串操作接口  
					transport_ctx	提供ACK确认与滑动窗口的传输控制接口,不包含OS对应的IO系统接口,并于标准库无关  
					tree			类型无关的普通树  
					sort			合并有序数组,topN统计接口  
					url				URL解析与编解码接口  
					sha1			SHA1编解码,来源于Redis源码  
			sysapi/  
					alloca			提供统一的内存对齐的分配释放接口  
					assert			提供一个相对于assert的高级断言  
					atomic			提供统一的原子操作接口  
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
			c++/  
				cpp_compiler_define	判断编译器当前指定的CPP版本,一些可以兼容98标准的关键字的定义  
				exception			包含文件名称,行号,出错语句的异常  
				lexical_cast		简陋但可用的通用类型转换接口  
				nullptr				给98标准的编译器提供nullptr关键字  
				unique_ptr			给98标准的编译器提供unique_ptr  
				unordered_map		给98标准的编译器提供一个简单的哈希表键值对(与标准库相同的CRUD接口,但不支持其他STL接口风骚的用法)  
				unordered_set		给98标准的编译器提供一个简单的哈希表键集合(同上)  
