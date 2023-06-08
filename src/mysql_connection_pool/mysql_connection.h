#ifndef MYSQL_CONNECTION
#define MYSQL_CONNECTION

#include <mysql/mysql.h>
#include <string>
#include <chrono>
#include "../log/log.h"

class MysqlConnection
{
public:

    MysqlConnection();

    ~MysqlConnection();

    bool Connect(const std::string &ip, const std::string &user, const std::string &passward, const std::string db_name, const unsigned short port = 3306);

    // 更新数据库: insert, update, delete
    bool Update(const std::string &sql);

    // 查询数据库涉及到了结果集的获取操作，单独写
    bool Query(const std::string &sql);

    // 遍历查询得到的结果集
    bool Next();

    // 得到结果集中的字段值
    std::string GetValue(const int index);

    bool Transaction();

    bool Commit();

    bool Rollback();

    // 刷新起始的空闲时间点
    void RefreshAliveTime();

    // 计算连接存活的总时长，毫秒
    long long GetAliveTime();

private:
    void FreeResult();

    MYSQL *mysql_ = nullptr;

    MYSQL_RES *result_ = nullptr;

    MYSQL_ROW row_ = nullptr;

    std::chrono::steady_clock::time_point alive_time_;
};

#endif