#ifndef SQL_CONNECTION_POOL
#define SQL_CONNECTION_POOL

#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "mysql_connection.h"
#include "../json/json.h"

class MysqlConnectionPool
{
public:
    static MysqlConnectionPool *GetInstance();

    MysqlConnectionPool(const MysqlConnectionPool &obj) = delete;

    MysqlConnectionPool &operator = (const MysqlConnectionPool &obj) = delete;

    // 从连接池中取出一个连接，用shared_ptr管理连接并设置析构器
    std::shared_ptr<MysqlConnection> GetConnection(); 

    void CloseMysqlConnectionPool();

    std::string GetDatabaseName()
    {
        return db_name_;
    }

    ~MysqlConnectionPool();

private:
    MysqlConnectionPool();
    
    bool InitializationParameters(const Json &config);

    // 增加一个连接
    void AddConnection();

    // 子线程工作函数，用于动态增加连接数目
    void ProduceConnection();

    // 子线程工作函数，用于动态减少连接数目
    void RecycleConnection();

    std::string ip_, user_, passward_, db_name_;

    unsigned short port_;

    int user_count_; 

    size_t min_num_, max_num_;

    int timeout_;

    int max_idle_time_; // 最大空闲时间

    bool is_closed_;

    std::queue<MysqlConnection*> connection_q_;

    std::mutex mtx_q_;//, mtx_u_;

    std::condition_variable cond_;
};

#endif