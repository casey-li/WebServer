#include "mysql_connection.h"

MysqlConnection::MysqlConnection()
{
    mysql_ = mysql_init(nullptr);
    if (!mysql_)
    {
        LOG_ERROR("MySQL init erroe!");
    }
    mysql_set_character_set(mysql_, "utf8");
}

MysqlConnection::~MysqlConnection()
{
    if (mysql_)
    {
        mysql_close(mysql_);
    }
    FreeResult();
}

bool MysqlConnection::Connect(const std::string &ip, const std::string &user, 
        const std::string &passward, const std::string db_name, const unsigned short port)
{
    mysql_ = mysql_real_connect(mysql_, ip.c_str(), user.c_str()
    , passward.c_str(), db_name.c_str(), port, nullptr, 0);
    if (!mysql_)
    {
        LOG_ERROR("MySQL connect erroe!");
        return false;
    }
    return true;
}

bool MysqlConnection::Update(const std::string &sql)
{
    return mysql_query(mysql_, sql.c_str()) == 0;
}

bool MysqlConnection::Query(const std::string &sql)
{
    FreeResult();
    if (mysql_query(mysql_, sql.c_str()))
    {
        return false;
    }
    result_ = mysql_store_result(mysql_);
    return true;
}

bool MysqlConnection::Next()
{
    if (result_)
    {
        if ((row_ = mysql_fetch_row(result_)) != nullptr)
        {
            return true;
        }
    }
    return false;
}

std::string MysqlConnection::GetValue(int index)
{
    int col_num = mysql_num_fields(result_);
    if (index < 0 || index >= col_num)
    {
        return "";
    }
    unsigned long len = mysql_fetch_lengths(result_)[index];
    return std::string(row_[index], len);
}

bool MysqlConnection::Transaction()
{
    return mysql_autocommit(mysql_, false);
}

bool MysqlConnection::Commit()
{
    return mysql_commit(mysql_);
}

bool MysqlConnection::Rollback()
{
    return mysql_rollback(mysql_);
}

void MysqlConnection::RefreshAliveTime()
{
    alive_time_ = std::chrono::steady_clock::now();
}

long long MysqlConnection::GetAliveTime()
{
    using namespace std::chrono;
    nanoseconds res = steady_clock::now() - alive_time_;
    milliseconds millsec = duration_cast<milliseconds>(res);
    return millsec.count();
}

void MysqlConnection::FreeResult()
{
    if (result_)
    {
        mysql_free_result(result_);
        result_ = nullptr;
    }
}

