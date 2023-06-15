> 保存服务器，线程池，数据库连接池以及日志的参数信息
可自由修改

```c++
{
    // 服务器参数
    "Server": {
        "port": 9999,               // 端口
        "timeout": 60000,           // 连接超时时间，用于清理超时非活动连接
        "ET_mode" : 3,              // ET 模式
        "open_linger" : true        // 是否开启 open_linger
    },
    // 线程池参数
    "ThreadPool":{
        "thread_num": 3             // 线程池线程数目
    },
    // 数据库连接池参数
    "MysqlPool":{
        "ip": "localhost",          // Mysql ip
        "user": "root",             // 登陆的用户名
        "passward": "root",         // 登陆密码
        "db_name": "WebServer",     // 连接的数据库库名
        "port": 3306,               // 数据库端口
        "min_thread_num": 3,        // 最小线程数目
        "max_thread_num": 6,        // 最大线程数目
        "timeout": 1000,            // 无可用连接时，阻塞等待的时间长度
        "max_idle_time": 10000,     // 最大空闲时间，用于销毁多余连接
        "min_thread_num": 2,        // 最大线程数目
        "max_thread_num": 5         // 最小线程数目
    },
    "Log":{
        "open_log": true,           // 是否开启日志
        "log_level": 1,             // 日志等级
        "block_queue_size": 1024    // 阻塞队列大小
    }
}
```