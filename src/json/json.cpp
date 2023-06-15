#include "json.h"

Json::Json() : type_(JSON_NULL), str_(""), index_(0)
{
}

Json::Json(bool value) : type_(JSON_BOOL), str_(""), index_(0)
{
    value_.bool_ = value;
}

Json::Json(int value) : type_(JSON_INT), str_(""), index_(0)
{
    value_.int_ = value;
}

Json::Json(double value) : type_(JSON_DOUBLE), str_(""), index_(0)
{
    value_.double_ = value;
}

Json::Json(const char *value) : type_(JSON_STRING), str_(""), index_(0)
{
    value_.string_ptr_ = new std::string(value);
}

Json::Json(const std::string &value) : type_(JSON_STRING), str_(""), index_(0)
{
    value_.string_ptr_ = new std::string(value);
}

Json::Json(const std::vector<Json> &value) : type_(JSON_ARRAY), str_(""), index_(0)
{
    value_.array_ptr_ = new std::vector<Json>(value);
}

Json::Json(const std::unordered_map<std::string, Json> &value) : type_(JSON_OBJECT), str_(""), index_(0)
{
    value_.object_ptr_ = new std::unordered_map<std::string, Json>(value);
}

Json::Json(Type type) : type_(type)
{
    switch (type_)
    {
        case JSON_NULL:
            break;
        case JSON_BOOL:
            value_.bool_ = true;
            break;
        case JSON_INT:
            value_.int_ = 0;
            break;
        case JSON_DOUBLE:
            value_.double_ = 0.0;
            break;    
        case JSON_STRING:
            value_.string_ptr_ = new std::string("");
            break;
        case JSON_ARRAY:
            value_.array_ptr_ = new std::vector<Json>();
            break;
        case JSON_OBJECT:
            value_.object_ptr_ = new std::unordered_map<std::string, Json>();
            break;
        default:
            break;
    }
}

Json::Json(const Json &other): type_(other.type_)
{
    Copy(other);
}

Json::~Json()
{
    Clear();
}

bool Json::AsBool() const
{
    assert (type_ == JSON_BOOL);
    return value_.bool_;
}

int Json::AsInt() const
{
    assert (type_ == JSON_INT);
    return value_.int_;
}

double Json::AsDouble() const
{
    assert (type_ == JSON_DOUBLE);
    return value_.double_;
}

std::string Json::AsString() const
{
    assert (type_ == JSON_STRING);
    return *(value_.string_ptr_);
}

std::vector<Json> Json::AsArray() const
{
    assert(type_ == JSON_ARRAY);
    return *(value_.array_ptr_);
}

std::unordered_map<std::string, Json> Json::AsObject() const
{
    assert(type_ == JSON_OBJECT);
    return *(value_.object_ptr_);
}

Json::operator bool() const
{
    assert(type_ == JSON_BOOL);
    return value_.bool_;
}

Json::operator int() const
{
    assert(type_ == JSON_INT);
    return value_.int_;
}
 
Json::operator double() const
{
    assert(type_ == JSON_DOUBLE);
    return value_.double_;
}

Json::operator std::string() const
{
    assert(type_ == JSON_STRING);
    return *(value_.string_ptr_);
}

Json::operator std::vector<Json>() const
{
    assert(type_ == JSON_ARRAY);
    return *(value_.array_ptr_);
}

Json::operator std::unordered_map<std::string, Json>() const
{
    assert(type_ == JSON_OBJECT);
    return *(value_.object_ptr_);
}

Json &Json::operator [](size_t index)
{
    assert(type_ == JSON_ARRAY || index < Size());
    return (value_.array_ptr_)->at(index);
}

Json &Json::operator [] (size_t index) const
{
    assert(type_ == JSON_ARRAY || index < Size());
    return (value_.array_ptr_)->at(index);
}

Json &Json::operator [](const char *key)
{
    return (*this)[std::string(key)];
}

Json &Json::operator [] (const char *key) const
{
    return (*this)[std::string(key)];
}

Json &Json::operator [](const std::string &key)
{
    if (type_ == JSON_NULL)
    {
        type_ = JSON_OBJECT;
        value_.object_ptr_ = new std::unordered_map<std::string, Json>();
    }
    assert(type_ == JSON_OBJECT);
    return (*(value_.object_ptr_))[key];
}

Json &Json::operator [] (const std::string &key) const
{
    assert(type_ == JSON_OBJECT);
    return (*(value_.object_ptr_))[key];
}

Json &Json::operator = (bool value)
{
    assert(type_ == JSON_BOOL || type_ == JSON_NULL);
    type_ = JSON_BOOL;
    value_.bool_ = value;
    return *this;
}

Json &Json::operator = (int value)
{
    assert(type_ == JSON_INT || type_ == JSON_NULL);
    type_ = JSON_INT;
    value_.int_ = value;
    return *this;
}

Json &Json::operator = (double value)
{
    assert(type_ == JSON_DOUBLE || type_ == JSON_NULL);
    type_ = JSON_DOUBLE;
    value_.double_ = value;
    return *this;
}

Json &Json::operator = (const char *value)
{
    return (*this) = std::string(value);
}

Json &Json::operator = (const std::string &value)
{
    assert(type_ == JSON_STRING || type_ == JSON_NULL);
    Clear();
    type_ = JSON_STRING;
    value_.string_ptr_ = new std::string(value);
    return *this;
}

Json &Json::operator = (const std::vector<Json> &value)
{
    assert(type_ == JSON_ARRAY || type_ == JSON_NULL);
    Clear();
    type_ = JSON_ARRAY;
    value_.array_ptr_ = new std::vector<Json>(value);
    return *this;    
}

Json &Json::operator = (const std::unordered_map<std::string, Json> &value)
{
    assert(type_ == JSON_OBJECT || type_ == JSON_NULL);
    Clear();
    type_ = JSON_OBJECT;
    value_.object_ptr_ = new std::unordered_map<std::string, Json>(value);
    return *this;    
}

Json &Json::operator = (const Json &other)
{
    Clear();
    Copy(other);
    return *this;
}

bool Json::operator == (const std::vector<Json> &value)
{
    Json tmp(value);
    return (*this) == tmp;
}

bool Json::operator == (const std::unordered_map<std::string, Json> &value)
{
    Json tmp(value);
    return (*this) == tmp;
}

bool Json::operator == (const Json &other)
{
    if (type_ != other.type_)
    {
        return false;
    }
    switch (other.type_)
    {
        case JSON_NULL:
            return true;
        case JSON_BOOL:
            return value_.bool_ == other.value_.bool_;
        case JSON_INT:
            return value_.int_ == other.value_.int_;
        case JSON_DOUBLE:
            return value_.double_ == other.value_.double_;
        case JSON_STRING:
            return *(value_.string_ptr_) == *(other.value_.string_ptr_);
        case JSON_ARRAY:
        {
            if (Size() != other.Size())
            {
                return false;
            }
            auto it2 = other.ArrBegin();
            for (auto it1 = ArrBegin(); it1 != ArrEnd(); ++it1, ++it2)
            {
                if ((*it1) != (*it2))
                {
                    return false;
                }
            }
            return true;
        }
        case JSON_OBJECT:
        {
            if (Size() != other.Size())
            {
                return false;
            }
            for (auto it = ObjBegin(); it != ObjEnd(); ++it)
            {
                if (!other.Count(it->first) || it->second != other[it->first])
                {
                    return false;
                }
            }
        }
        default:
            break;
    }
    return true;
}


std::vector<Json>::iterator Json::ArrBegin()
{
    assert(type_ == JSON_ARRAY);
    return value_.array_ptr_->begin();
}

std::vector<Json>::const_iterator Json::ArrBegin() const
{
    assert(type_ == JSON_ARRAY);
    return value_.array_ptr_->begin();
}

std::vector<Json>::iterator Json::ArrEnd()
{
    assert(type_ == JSON_ARRAY);
    return value_.array_ptr_->end();
}

std::vector<Json>::const_iterator Json::ArrEnd() const
{
    assert(type_ == JSON_ARRAY);
    return value_.array_ptr_->end();
}

std::unordered_map<std::string, Json>::iterator Json::ObjBegin()
{
    assert(type_ == JSON_OBJECT);
    return value_.object_ptr_->begin();
}

std::unordered_map<std::string, Json>::const_iterator Json::ObjBegin() const
{
    assert(type_ == JSON_OBJECT);
    return value_.object_ptr_->begin();
}

std::unordered_map<std::string, Json>::iterator Json::ObjEnd()
{
    assert(type_ == JSON_OBJECT);
    return value_.object_ptr_->end();
}

std::unordered_map<std::string, Json>::const_iterator Json::ObjEnd() const
{
    assert(type_ == JSON_OBJECT);
    return value_.object_ptr_->end();
}

void Json::PushBack(const Json &other)
{
    if (type_ == JSON_NULL)
    {
        type_ = JSON_ARRAY;
        value_.array_ptr_ = new std::vector<Json>();
    }
    assert(type_ == JSON_ARRAY);
    (value_.array_ptr_)->push_back(other);
}

void Json::PopBack()
{
    assert(type_ == JSON_ARRAY || Size() > 0);
    (value_.array_ptr_)->pop_back();
}

bool Json::Erase(size_t index)
{
    if (type_ != JSON_ARRAY)
    {
        return false;
    }
    assert(index >= 0 && index < Size());
    (*value_.array_ptr_)[index].Clear();
    value_.array_ptr_->erase(ArrBegin() + index);
    return true;
}

bool Json::Erase(const char *key)
{
    return Erase(std::string(key));
}

bool Json::Erase(const std::string &key)
{
    if (type_ != JSON_OBJECT || !Count(key))
    {
        return false;
    }
    (*value_.object_ptr_)[key].Clear();
    value_.object_ptr_->erase(key);
    return true;
}

size_t Json::Count(const Json &other) const
{
    assert(type_ == JSON_OBJECT);
    return value_.object_ptr_->count(other);
}

size_t Json::Size() const
{
    assert(type_ != JSON_NULL);
    switch (type_)
    {
    case JSON_ARRAY:
        return value_.array_ptr_->size();
    case JSON_OBJECT:
        return value_.object_ptr_->size();
    default:
        return 1;
    }
}

void Json::Clear()
{
    switch (type_)
    {
        case JSON_STRING:
        {
            if (value_.string_ptr_)
            {
                delete value_.string_ptr_;
            }
            break;
        }
        case JSON_ARRAY:
        {
            if (value_.array_ptr_)
            {
                for (auto it = ArrBegin(); it != ArrEnd(); ++it)
                {
                    it->Clear();
                }
                delete value_.array_ptr_;
            }
            break;
        }
        case JSON_OBJECT:
        {
            if (value_.object_ptr_)
            {
                for (auto it = ObjBegin(); it != ObjEnd(); ++it)
                {
                    (it->second).Clear();
                }
                delete value_.object_ptr_;
            }
            break;
        }
        default:
            break;
    }
    type_ = JSON_NULL;
    if (str_.size()) str_ = "";
    index_ = 0;
}

void Json::Copy(const Json &other)
{
    type_ = other.type_;
    switch (other.type_)
    {
        case JSON_NULL:
            break;
        case JSON_BOOL:
            value_.bool_ = other.value_.bool_;
            break;
        case JSON_INT:
            value_.int_ = other.value_.int_;
            break;
        case JSON_DOUBLE:
            value_.double_ = other.value_.double_;
            break;    
        case JSON_STRING:
            value_.string_ptr_ = new std::string(*other.value_.string_ptr_);
            break;
        case JSON_ARRAY:
            value_.array_ptr_ = new std::vector<Json>(*other.value_.array_ptr_);
            break;
        case JSON_OBJECT:
            value_.object_ptr_ = new std::unordered_map<std::string, Json>(*other.value_.object_ptr_);
            break;
        default:
            break;
    }
}

std::string Json::ToString() const
{
    std::stringstream ss;
    switch (type_)
    {
        case JSON_NULL:
        {
            ss << "null";
            break;
        }
        case JSON_BOOL:
        {
            ss << (value_.bool_ ? "true" : "false");
            break;
        }
        case JSON_INT:
        {
            ss << value_.int_;
            break;
        }
        case JSON_DOUBLE:
        {
            ss << value_.double_;
            break;
        }
        // 字符串用 " " 扩起来
        case JSON_STRING:
        {
            ss << '\"' << *(value_.string_ptr_) << '\"';
            break;
        }
        // 数组的话递归调用自己即可，用 [ ] 扩起来
        case JSON_ARRAY:
        {
            ss << '[';
            for (auto it = ArrBegin(); it != ArrEnd(); ++it)
            {
                if (it != ArrBegin())
                {
                    ss << ',';
                }
                ss << it->ToString();
            }
            ss << ']';
            break;
        }
        // 对象的话用 { } 扩起来
        case JSON_OBJECT:
        {
            ss << '{';
            for (auto it = ObjBegin(); it != ObjEnd(); ++it)
            {
                // 每个 it 指向的是一个键值对，键为字符串，键和值之间用 ':' 隔开
                if (it != ObjBegin())
                {
                    ss << ',';
                }
                ss << '\"' << it->first << '\"' << ':' << it->second.ToString();
            }
            ss << "}";
            break;
        }
        default:
            break;
    }
    return ss.str();
}

void Json::InitializationParse(const std::string &str)
{
    str_ = str;
    index_ = 0;
}

Json Json::ParseConfig(const std::string path)
{
    std::ifstream ifs(path);
    std::stringstream ss;
    ss << ifs.rdbuf();
    InitializationParse(ss.str());
    *this = Parse();
    return *this;
}

Json Json::Parse()
{
    SkipWhiteSpaces();
    char ch = GetCurrentChar();

    // 是数字
    if (ch == '-' || isdigit(ch))
    {
        return ParseNumber();
    }
    // 其他类型
    switch (ch)
    {
        case 'n':
            return ParseNull();
        case 't':
        case 'f':
            return ParseBool();
        case '"':
            return ParseString();
        case '[':
            return ParseArray();
        case '{':
            return ParseObject();
        default:
            throw std::logic_error("unexpected character in parse json");
    }
    return Json();
}

void Json::SkipWhiteSpaces()
{
    while (index_ < str_.size() && std::isspace(str_[index_]))
    {
        ++index_;
    }
}

char Json::GetCurrentChar()
{
    SkipWhiteSpaces();
    assert(index_ < str_.size());
    return str_[index_];
}

Json Json::ParseNull()
{
    if (!str_.compare(index_, 4, "null"))
    {
        index_ += 4;
        return Json();
    }
    throw std::logic_error("parse null error");
}

Json Json::ParseBool()
{
    if (!str_.compare(index_, 4, "true"))
    {
        index_ += 4;
        return Json(true);
    }
    if (!str_.compare(index_, 5, "false"))
    {
        index_ += 5;
        return Json(false);
    }
    throw std::logic_error("parse bool error");
}

Json Json::ParseNumber()
{
    std::regex pattern("[+-]?0(\\.\\d+)?|[+-]?[1-9]\\d*(\\.\\d+)?");
    std::smatch res;
    std::string tmp = str_.substr(index_);
    if (std::regex_search(tmp, res, pattern))
    {
        std::string number = res.str();
        index_ += number.size();
        if (number.find('.') != std::string::npos)
        {
            return Json(std::stod(number));
        }
        return Json(stoi(number));
    }
    throw std::logic_error("parse number error");
}

Json Json::ParseString()
{
    std::string res;
    ++index_; // 跳过 "
    auto pos = str_.find('"', index_);
    assert(pos != std::string::npos);
    std::string tmp = str_.substr(index_, pos - index_);
    index_ = pos + 1;
    return Json(std::move(tmp));
}

// 以 , 分割; ] 结束
Json Json::ParseArray()
{
    Json arr(JSON_ARRAY);
    ++index_;
    while (GetCurrentChar() != ']')
    {
        arr.PushBack(Parse());
        if (GetCurrentChar() == ']')
        {
            break;
        }
        assert(GetCurrentChar() == ',');
        ++index_;
    }
    ++index_;
    return arr;
}

// key:value 键值对，key 为字符串，不同键值对之间以 , 分割
Json Json::ParseObject()
{
    Json obj(JSON_OBJECT);
    ++index_;
    while (GetCurrentChar() != '}')
    {
        Json key = ParseString();
        assert(GetCurrentChar() == ':');
        ++index_;
        obj[key.AsString()] = Parse();
        if (GetCurrentChar() == '}')
        {
            break;
        }
        assert(GetCurrentChar() == ',');
        ++index_;
    }
    ++index_;
    return obj;
}



