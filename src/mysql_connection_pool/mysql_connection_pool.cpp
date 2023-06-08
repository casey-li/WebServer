#include "mysql_connection_pool.h"

MysqlConnectionPool *MysqlConnectionPool::GetInstance()
{
    static MysqlConnectionPool pool;
    return &pool;
}

std::shared_ptr<MysqlConnection> MysqlConnectionPool::GetConnection()
{
    std::unique_lock<std::mutex> locker(mtx_q_);
    while (connection_q_.empty())
    {
        if (std::cv_status::timeout == cond_.wait_for(locker, std::chrono::milliseconds(timeout_)))
        {
            // 做二次判断
            if (connection_q_.empty())
            {
                continue;
            }
        }
    }
    // 取出一个连接并设置析构器，以确保当对象析构时将连接放回连接池
    std::shared_ptr<MysqlConnection> ptr(connection_q_.front(), [this](MysqlConnection* p)
    {
        std::lock_guard<std::mutex> locker(mtx_q_);
        p->RefreshAliveTime();
        connection_q_.push(p);
        --user_count_;
    });
    connection_q_.pop();
    ++user_count_;
    cond_.notify_all();
    return ptr;
} 

void MysqlConnectionPool::CloseMysqlConnectionPool()
{
    is_closed_ = true;
    cond_.notify_all();
    while (!connection_q_.empty())
    {
        MysqlConnection *ptr = connection_q_.front();
        connection_q_.pop();
        delete ptr;
    }
}

MysqlConnectionPool::~MysqlConnectionPool()
{
    CloseMysqlConnectionPool();
}

MysqlConnectionPool::MysqlConnectionPool() : is_closed_(false)
{
    // 1、解析 json 文件获取连接数据库的信息
    if (!ParseJsonFile())
    {
        return;
    }
    // 2、创建 min_num 个连接
    while (connection_q_.size() < min_num_)
    {
        AddConnection();
    }
    user_count_ = 0;
    // 3、创建两个线程分别用于动态增加和减少Mysql连接数目
    std::thread(&MysqlConnectionPool::ProduceConnection, this).detach();
    std::thread(&MysqlConnectionPool::RecycleConnection, this).detach();
}

// TODO: json 解析器解析配置
bool MysqlConnectionPool::ParseJsonFile()
{
    ip_ = "localhost";
    user_ = "root";
    passward_ = "root";
    db_name_ = "WebServer";
    port_ = 3306;
    min_num_ = 10;
    max_num_ = 50;
    timeout_ = 200;
    max_idle_time_ = 1000;
    return true;
}

void MysqlConnectionPool::AddConnection()
{
    MysqlConnection *ptr = new MysqlConnection();
    if (ptr->Connect(ip_, user_, passward_, db_name_, port_))
    {
        ptr->RefreshAliveTime();
        connection_q_.push(ptr);
    }
}

void MysqlConnectionPool::ProduceConnection()
{
    while (!is_closed_)
    {
        std::unique_lock<std::mutex> locker(mtx_q_);
        while (!is_closed_ && connection_q_.size() >= min_num_) 
        {
            cond_.wait(locker);
        }
        if (is_closed_) 
        {
            cond_.notify_all();
            break;
        }
        if (user_count_ + connection_q_.size() < max_num_)
        {
            AddConnection();
            cond_.notify_all();
        }
    }
}

void MysqlConnectionPool::RecycleConnection()
{
    while (!is_closed_)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::lock_guard<std::mutex> locker(mtx_q_);
        while (!is_closed_ && connection_q_.size() > min_num_)
        {
            MysqlConnection *ptr = connection_q_.front();
            if (ptr->GetAliveTime() > max_idle_time_)
            {
                connection_q_.pop();
                delete ptr;
            }
            else
            {
                break;
            }
        }
        if (is_closed_) 
        {
            cond_.notify_all();
            break;
        }
    }
}

