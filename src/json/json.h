#ifndef JSON_H
#define JSON_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <sstream>
#include <iostream>
#include <memory>
#include <regex>
#include <fstream>


class Json
{
public:

    enum Type
    {
        JSON_NULL = 0,
        JSON_BOOL,
        JSON_INT,
        JSON_DOUBLE,
        JSON_STRING,
        JSON_ARRAY,
        JSON_OBJECT
    };


    Json();

    Json(bool value);

    Json(int value);

    Json(double value);

    Json(const char *value);

    Json(const std::string &value);

    Json(const std::vector<Json> &value);

    Json(const std::unordered_map<std::string, Json> &value);

    Json(Type type);

    Json(const Json & other);

    ~Json();

    Type GetType() const { return type_; }


    bool IsNull() const { return type_ == JSON_NULL; }

    bool IsBool() const { return type_ == JSON_BOOL; }

    bool IsInt() const { return type_ == JSON_INT; }

    bool IsDouble() const { return type_ == JSON_DOUBLE; }

    bool IsString() const { return type_ == JSON_STRING; }

    bool IsArray() const { return type_ == JSON_ARRAY; }

    bool IsObject() const { return type_ == JSON_OBJECT; }


    bool AsBool() const;

    int AsInt() const;

    double AsDouble() const;

    std::string AsString() const;

    std::vector<Json> AsArray() const;

    std::unordered_map<std::string, Json> AsObject() const;


    operator bool() const;

    operator int() const;

    operator double() const;

    operator std::string() const;

    operator std::vector<Json>() const;

    operator std::unordered_map<std::string, Json>() const;


    // 对数组操作，下标
    Json &operator [] (size_t index);

    Json &operator [] (size_t index) const;

    // 对对象操作，键值对
    Json &operator [] (const char *key);

    Json &operator [] (const char *key) const;

    Json &operator [] (const std::string &key);

    Json &operator [] (const std::string &key) const;


    Json &operator = (bool value);

    Json &operator = (int value);

    Json &operator = (double value);

    Json &operator = (const char *value);

    Json &operator = (const std::string &value);

    Json &operator = (const std::vector<Json> &value);

    Json &operator = (const std::unordered_map<std::string, Json> &value);

    Json &operator = (const Json &other);


    bool operator == (bool value) { return type_ == JSON_BOOL && value_.bool_ == value; }

    bool operator == (int value) { return type_ == JSON_INT && value_.int_ == value; }

    bool operator == (double value) { return type_ == JSON_DOUBLE && value_.double_ == value; }

    bool operator == (const char *value) { return (*this) == std::string(value); }

    bool operator == (const std::string &value) { return type_ == JSON_STRING && (*(value_.string_ptr_) == value); }

    bool operator == (const std::vector<Json> &value);

    bool operator == (const std::unordered_map<std::string, Json> &value);

    bool operator == (const Json &other);


    bool operator != (bool value) { return !((*this) == value); }

    bool operator != (int value) { return !((*this) == value); }

    bool operator != (double value) { return !((*this) == value); }

    bool operator != (const char *value) { return !((*this) == value); }

    bool operator != (const std::string &value) { return !((*this) == value); }

    bool operator != (const std::vector<Json> &value) { return !((*this) == value); }

    bool operator != (const std::unordered_map<std::string, Json> &value) { return !((*this) == value); }

    bool operator != (const Json &other) { return !((*this) == other); }


    std::vector<Json>::iterator ArrBegin();

    std::vector<Json>::const_iterator ArrBegin() const;

    std::vector<Json>::iterator ArrEnd();

    std::vector<Json>::const_iterator ArrEnd() const;

    std::unordered_map<std::string, Json>::iterator ObjBegin();

    std::unordered_map<std::string, Json>::const_iterator ObjBegin() const;

    std::unordered_map<std::string, Json>::iterator ObjEnd();

    std::unordered_map<std::string, Json>::const_iterator ObjEnd() const;


    void PushBack(const Json &other);

    void PopBack();

    bool Erase(size_t index);

    bool Erase(const char *key);

    bool Erase(const std::string &key);

    size_t Count(const Json &other) const;

    size_t Size() const;

    void Clear();

    void Copy(const Json &other);

    // 将整个 Json 对象转化为字符串
    std::string ToString() const;

    // 初始化要解析的字符串
    void InitializationParse(const std::string &str);

    // 解析字符串的内容，返回对象
    Json Parse();
    
    // 解析项目配置
    Json ParseConfig(const std::string path = "./config/config.json");

private:
    
    union Value
    {
        bool bool_;
        int int_;
        double double_;
        std::string *string_ptr_;
        std::vector<Json> *array_ptr_;
        std::unordered_map<std::string, Json> *object_ptr_;
    };

    void SkipWhiteSpaces();

    char GetCurrentChar();

    Json ParseNull();

    Json ParseBool();

    Json ParseNumber();

    Json ParseString();

    Json ParseArray();

    Json ParseObject();

    Type type_;

    Value value_;

    std::string str_;

    size_t index_; // 保存解析的字符串的下标
};

#endif