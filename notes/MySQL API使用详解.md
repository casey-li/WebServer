# 连接数据库的步骤

1. 初始化连接环境

2. 连接 mysql 的服务器，需要提供如下连接数据:

    - 服务器的 IP 地址
    - 服务器监听的端口（默认端口是 3306）
    - 连接服务器使用的用户名（默认是 root），和这个用户对应的密码
    - 要操作的数据库的名字

3. 连接已经建立，后续操作就是对数据库数据的添删查改

    - 这些操作都是需要通过 sql 语句来完成的
    - **数据查询**：通过调用 api 执行一个查询的 sql 语句
    - **数据修改**（添加 / 删除 / 更新）：通过调用 api 执行一个修改数据的 sql 语句

4. 如果要进行数据 **添加 / 删除 / 更新**，需要进行事务的处理

    - 需要对执行的结果进行判断
        - 成功：提交事务
        - 失败：数据回滚

5. 数据库的读操作 -> 查询 -> 得到结果集

6. 遍历结果集 -> 得到了要查询的数据

7. 释放资源

# MySQL C API

## 初始化连接环境
```c++
// 参数 mysql -> null
// 返回值: 该函数将分配、初始化、并返回新对象，通过返回的这个对象去连接MySQL的服务器
MYSQL *mysql_init(MYSQL *mysql) ;
```

## 连接 mysql 服务器
```c++
MYSQL *mysql_real_connect(
    MYSQL *mysql,           // mysql_init() 函数的返回值
    const char *host,       // mysql服务器的主机地址, 写IP地址 (localhost, null -> 代表本地连接)
    const char *user,       // 连接mysql服务器的用户名, 默认: root 
    const char *passwd,     // 连接mysql服务器用户对应的密码, root用户的密码
    const char *db,         // 要使用的数据库的名字
    unsigned int port,      // 连接的mysql服务器监听的端口
                            // 为0使用mysql的默认端口3306, !=0, 使用指定的这个端口
    const char *unix_socket,// 本地套接字, 不使用指定为 NULL
    unsigned long client_flag); // 通常指定为0

    返回值: 
        成功: 返回MYSQL*连接句柄, 返回值与第1个参数的值相同
        失败: NULL
        句柄: 是windows中的一个概念, 句柄可以理解为一个实例(或者对象)
```

## 执行 sql 语句
```c++
// 执行一个sql语句, 添删查改的sql语句都可以
int mysql_query(MYSQL *mysql, const char *query);

    参数:
        - mysql: mysql_real_connect() 的返回值
        - query: 一个可以执行的sql语句, 结尾的位置不需要加 ;
    返回值: 
        - 如果查询成功，返回0。如果是查询, 结果集在mysql 对象中
        - 如果出现错误，返回非0值。 
```

## 获取结果集
```c++
// 将结果集从 mysql(参数) 对象中取出
// MYSQL_RES 对应一块内存, 里边保存着这个查询之后得到的结果集
// 如何将行和列的数据从结果集中取出, 需要使用其他函数
MYSQL_RES *mysql_store_result(MYSQL *mysql);

返回值: 
    - 成功: 具有多个结果的MYSQL_RES结果集合
    - 失败: NULL 
```

## 得到结果集的列数
```c++
unsigned int mysql_num_fields(MYSQL_RES *result)

    参数: 
        调用 mysql_store_result() 得到的返回值
```

## 获取表头 -> 列名 (字段名)
```c++
MYSQL_FIELD *mysql_fetch_fields(MYSQL_RES *result);

    参数: 
        调用 mysql_store_result() 得到的返回值
    返回值: 
        MYSQL_FIELD* 指向一个结构体

typedef struct st_mysql_field {
    char *name;                 /* 列名-> 字段的名字 */
    char *org_name;             /* Original column name, if an alias */
    char *table;                /* Table of column if column was a field */
    char *org_table;            /* Org table name, if table was an alias */
    char *db;                   /* Database for table */
    char *catalog;              /* Catalog for table */
    char *def;                  /* Default value (set by mysql_list_fields) */
    unsigned long length;       /* Width of column (create length) */
    unsigned long max_length;   /* Max width for selected set */
    unsigned int name_length;
    unsigned int org_name_length;                                                                                        
    unsigned int table_length;
    unsigned int org_table_length;
    unsigned int db_length;
    unsigned int catalog_length;
    unsigned int def_length;
    unsigned int flags;         /* Div flags */
    unsigned int decimals;      /* Number of decimals in field */
    unsigned int charsetnr;     /* Character set */
    enum enum_field_types type; /* Type of field. See mysql_com.h for types */
    void *extension;
} MYSQL_FIELD;
```

**案例**

```c++
// 得到存储头信息的数组的地址
MYSQL_FIELD* fields = mysql_fetch_fields(res);
// 得到列数
int num = mysql_num_fields(res);
// 遍历得到每一列的列名
for(int i = 0; i < num; ++i)
{
    printf("当前列的名字: %s\n", fields[i].name);
}
```

## 得到结果集中字段的长度
```c++
/* 
返回结果集内当前行的列的长度:
    1. 如果打算复制字段值，使用该函数能避免调用 strlen()
    2. 如果结果集包含二进制数据，必须使用该函数来确定数据的大小，原因在于，对于包含Null字符的任何字段，strlen()将返回错误的结果。
*/
unsigned long *mysql_fetch_lengths(MYSQL_RES *result);

    参数: 
        - result: 通过查询得到的结果集
    返回值:
        - 无符号长整数的数组表示各列的大小
        - 出错返回NULL
```

**案例**

```c++
MYSQL_ROW row;
unsigned long *lengths;
unsigned int num_fields;

row = mysql_fetch_row(result);
if (row)
{
    num_fields = mysql_num_fields(result);
    lengths = mysql_fetch_lengths(result);
    for(int i = 0; i < num_fields; i++)
    {
         printf("Column %u is %lu bytes in length.\n", i, lengths[i]);
    }
}
```

## 遍历结果集
```c++
typedef char** MYSQL_ROW;
// 遍历结果集的下一行 
// 如果想遍历整个结果集, 需要对该函数进行循环调用
// 返回值是二级指针, char**, 指向一个指针数组, 类型是数组,里边的每个元素都是指针, char* 类型

// 需要对 MYSQL_ROW 遍历就可以得到每一列的值
// 如果要遍历整个结果集, 需要循环调用这个函数
MYSQL_ROW mysql_fetch_row(MYSQL_RES *result);

参数: 
    - result: 通过查询得到的结果集
返回值: 
    - 成功: 得到了当前记录中每个字段的值
    - 失败: NULL, 说明数据已经读完了
```

## 资源回收
```c++
// 释放结果集
void mysql_free_result(MYSQL_RES *result);

// 关闭mysql实例
void mysql_close(MYSQL *mysql);
```

## 字符编码
```c++
// 获取api默认使用的字符编码
// 为当前连接返回默认的字符集。
const char *mysql_character_set_name(MYSQL *mysql) 
    
    返回值: 默认字符集。 

// 设置api使用的字符集
// 第二个参数 csname 就是要设置的字符集 -> 支持中文: utf8
int mysql_set_character_set(MYSQL *mysql, char *csname);
```

## 事务操作
```c++
// mysql中默认会进行事务的提交
// 因为自动提交事务, 会对我们的操作造成影响
// 如果我们操作的步骤比较多, 集合的开始和结束需要用户自己去设置, 需要改为手动方式提交事务
my_bool mysql_autocommit(MYSQL *mysql, my_bool mode) 

    参数:
        1 表示启用autocommit模式；0 表示禁止autocommit模式
    返回值
        成功返回 0，失败返回 非0值。

// 事务提交
my_bool mysql_commit(MYSQL *mysql);

    返回值: 
        成功: 0, 失败: 非0
    
// 数据回滚
my_bool mysql_rollback(MYSQL *mysql) 

    返回值: 
        成功: 0, 失败: 非0
```

## 打印错误信息
```c++
// 返回错误的描述
const char *mysql_error(MYSQL *mysql);

// 返回错误的编号
unsigned int mysql_errno(MYSQL *mysql);
```
# 实例代码

```c++
#include <stdio.h>
#include <mysql.h>

int main()
{
    // 1. 初始化连接环境
    MYSQL* mysql = mysql_init(NULL);
    if(mysql == NULL)
    {
        printf("mysql_init() error\n");
        return -1;
    }

    // 2. 连接数据库服务器
    mysql = mysql_real_connect(mysql, "localhost", "root", "root", "db_name", 0, NULL, 0);
    if(mysql == NULL)
    {
        printf("mysql_real_connect() error\n");
        return -1;
    }

    printf("mysql api使用的默认编码: %s\n", mysql_character_set_name(mysql));

    // 设置编码为utf8
    mysql_set_character_set(mysql, "utf8");

    printf("mysql api使用的修改之后的编码: %s\n", mysql_character_set_name(mysql));

    printf("恭喜, 连接数据库服务器成功了...\n");

    // 3. 执行一个sql语句
    // 查询数据库下的dept部门表
    const char* sql = "select * from dept";
    // 执行这个sql语句
    int ret = mysql_query(mysql, sql);
    if(ret != 0)
    {
        printf("mysql_query() 失败了, 原因: %s\n", mysql_error(mysql));
        return -1;
    }

    // 4. 取出结果集
    MYSQL_RES* res = mysql_store_result(mysql);
    if(res == NULL)
    {
        printf("mysql_store_result() 失败了, 原因: %s\n", mysql_error(mysql));
        return -1;
    }

    // 5. 得到结果集中的列数
    int num = mysql_num_fields(res);

    // 6. 得到所有列的名字, 并且输出
    MYSQL_FIELD * fields = mysql_fetch_fields(res);
    for(int i = 0; i < num; ++i)
    {
        printf("%s\t\t", fields[i].name);
    }
    printf("\n");

    // 7. 遍历结果集中所有的行
    MYSQL_ROW row;
    while( (row = mysql_fetch_row(res)) != NULL)
    {
        // 将当前行中的每一列信息读出
        for(int i = 0; i < num; ++i)
        {
            printf("%s\t\t", row[i]);
        }
        printf("\n");
    }

    // 8. 释放资源 - 结果集
    mysql_free_result(res);

    // 9. 写数据库
    // 以下三条是一个完整的操作, 对应的是一个事务
    // 设置事务为手动提交
    mysql_autocommit(mysql, 0); 
    int ret1 = mysql_query(mysql, "insert into dept values(61, '海军', '圣地玛丽乔亚')");
    int ret2 = mysql_query(mysql, "insert into dept values(62, '七武海', '世界各地')");
    int ret3 = mysql_query(mysql, "insert into dept values(63, '四皇', '新世界')");
    printf("ret1 = %d, ret2 = %d, ret3 = %d\n", ret1, ret2, ret3);

    if(ret1 == 0 && ret2 == 0 && ret3 == 0)
    {
        // 提交事务
        mysql_commit(mysql);
    }
    else
    {
        mysql_rollback(mysql);
    }

    // 释放数据库资源
    mysql_close(mysql);
    
  return 0;
}
```