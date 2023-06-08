#include "mysql_connection_pool.h"
#include "mysql_connection.h"
#include <iostream>
#include <unistd.h>

using namespace std;
using namespace chrono;

void op1(int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        MysqlConnection conn;
        conn.Connect("localhost", "root", "root", "test_db");
        char sql[1024] = { 0 };
        sprintf(sql, "insert into person values(%d, 25, 'man', 'tom')", i);
        conn.Update(sql);
    }
}

void op2(MysqlConnectionPool* pool, int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        shared_ptr<MysqlConnection> conn = pool->GetConnection();
        char sql[1024] = { 0 };
        sprintf(sql, "insert into person values(%d, 25, 'man', 'tom')", i);
        conn->Update(sql);
    }
}

void test1()
{
#if 0
    // 原始的用时: 5140706125 纳秒, 5140 毫秒
    // 非连接池, 单线程, 用时: 5292357327 纳秒, 5292 毫秒
    steady_clock::time_point begin = steady_clock::now();
    op1(0, 5000);
    steady_clock::time_point end = steady_clock::now();
    auto length = end - begin;
    cout << "非连接池, 单线程, 用时: " << length.count() << " 纳秒, "
        << length.count() / 1000000 << " 毫秒" << endl;
#else
    // 原始的用时: 3596698861 纳秒, 3596 毫秒
    // 连接池, 单线程, 用时: 3665488863 纳秒, 3665 毫秒
    MysqlConnectionPool* pool = MysqlConnectionPool::GetInstance();
    steady_clock::time_point begin = steady_clock::now();
    op2(pool, 0, 5000);
    steady_clock::time_point end = steady_clock::now();
    auto length = end - begin;
    cout << "连接池, 单线程, 用时: " << length.count() << " 纳秒, "
        << length.count() / 1000000 << " 毫秒" << endl;

#endif
}

void test2()
{
#if 0
    // 原始的用时: 1845789579 纳秒, 1845 毫秒
    // 非连接池, 多单线程, 用时: 1889495965 纳秒, 1889 毫秒
    MysqlConnection conn;
    conn.Connect("localhost", "root", "root", "test_db");
    steady_clock::time_point begin = steady_clock::now();
    thread t1(op1, 0, 1000);
    thread t2(op1, 1000, 2000);
    thread t3(op1, 2000, 3000);
    thread t4(op1, 3000, 4000);
    thread t5(op1, 4000, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    steady_clock::time_point end = steady_clock::now();
    auto length = end - begin;
    cout << "非连接池, 多单线程, 用时: " << length.count() << " 纳秒, "
        << length.count() / 1000000 << " 毫秒" << endl;

#else
    // 原始的用时: 1289965695 纳秒, 1289 毫秒
    // 连接池, 多单线程, 用时: 1297562094 纳秒, 1297 毫秒
    MysqlConnectionPool* pool = MysqlConnectionPool::GetInstance();
    steady_clock::time_point begin = steady_clock::now();
    thread t1(op2, pool, 0, 1000);
    thread t2(op2, pool, 1000, 2000);
    thread t3(op2, pool, 2000, 3000);
    thread t4(op2, pool, 3000, 4000);
    thread t5(op2, pool, 4000, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    steady_clock::time_point end = steady_clock::now();
    auto length = end - begin;
    cout << "连接池, 多单线程, 用时: " << length.count() << " 纳秒, "
        << length.count() / 1000000 << " 毫秒" << endl;

#endif
}

int query()
{
    MysqlConnection conn;
    conn.Connect("localhost", "root", "root", "test_db");
    string sql = "insert into person values(6, 25, 'man', 'tom')";
    bool flag = conn.Update(sql);
    cout << "flag value:  " << flag << endl;

    sql = "select * from person";
    conn.Query(sql);
    while (conn.Next())
    {
        cout << conn.GetValue(0) << ", "
            << conn.GetValue(1) << ", "
            << conn.GetValue(2) << ", "
            << conn.GetValue(3) << endl;
    }

    sql = "select count(*) from person";
    conn.Query(sql);
    while (conn.Next())
    {
        cout << "contain " << conn.GetValue(0) << "row\n";
    }
    sql = "truncate table person";
    conn.Update(sql);
        sql = "select count(*) from person";
    conn.Query(sql);
    while (conn.Next())
    {
        cout << "contain " << conn.GetValue(0) << "row\n";
    }
    return 0;
}

void CheckAndDelete()
{
    string sql1 = "select count(*) from person";
    MysqlConnection conn;
    conn.Connect("localhost", "root", "root", "test_db");
    if (!conn.Query(sql1))
    {
        cout << "sql1 error!\n";
        return;
    }
    conn.Next();
    cout << "now contains " << conn.GetValue(0) << " row data\n";
    string sql2 = "truncate table person";
    if (!conn.Update(sql2))
    {
        cout << "sql1 error!\n";
        return;       
    }
    if (!conn.Query(sql1))
    {
        cout << "sql1 error!\n";
        return;
    }
    conn.Next();
    cout << "after delete, contains " << conn.GetValue(0) << " row data\n";
}

#if 0
int main()
{
    // query();
    test2();
    CheckAndDelete();
    return 0;
}
#endif
