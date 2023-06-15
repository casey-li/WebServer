# JSON 解析器

## JSON 的数据类型

- Null
- 布尔
- 数字: 必须是整数或者浮点型
- 字符串: 必须用双引号包围
- 数组 (array): 有序的值的集合，以 '[' 开始，']' 结束，数组的值用逗号分割
- 对象（JSON 对象）: 是在 { } 中间插入的一组键值对。键必须是字符串且唯一，多个键值对由逗号分割

```json
{

    "name": "BeJson",
    "url": "http://www.bejson.com",
    "page": 88,
    "isNonProfit": true,
    "address": {
        "street": "科技园路.",
        "city": "江苏苏州",
        "country": "中国"
    },
    "links": [
        {
            "name": "Google",
            "url": "http://www.google.com"
        },
        {
            "name": "Baidu",
            "url": "http://www.baidu.com"
        },
        {
            "name": "SoSo",
            "url": "http://www.SoSo.com"
        }
    ]
}
```

## JSON 类的主要函数

为了节省空间，Json 类的对象的值采用联合体（共用内存，占用的内存大小有最大的元素决定），其中字符串，数组，对象都保存为指针

1. 构造函数

可以由基本类型，其他Json对象，Type 类型 构造Json对象

2. 类型转换运算符

用于将自定义类隐式转换为基本类型，如 `operator bool() const;`

显示转换，如 `bool AsBool() const;`

3. 数组类型的设计

可以按照索引下标添加元素，PushBack() 添加元素，同样支持用索引取元素，重载 [] 运算符
为了方便遍历数组元素，定义 ArrBegin(), ArrEnd() 函数返回迭代器

```c++
Json arr;
arr.PushBack(1.23);
arr.PushBack(false);
arr.PushBack(3);
arr[0] = false;
arr[1] = "sss";

bool flag = arr[0];
std::string str = arr[1].ToString();
int i = arr[2];

std::cout << arr.ToString() << std::endl;

for (auto it = arr.ArrBegin(); it != arr.ArrEnd(); ++it)
{
    std::cout << it->ToString() << std::endl;
}
```

4. 对象类型的设计

重载 [] 运算符，定义 ObjBegin(), ObjEnd() 函数返回迭代器，满足如下功能

```c++
Json obj;
obj["str"] = "ssss";
obj["int"] = 2;
obj["bool"] = true;
obj["hello"] = "world";
for (auto it = obj.ObjBegin(); it != obj.ObjEnd(); ++it)
{
    std::cout << it->second.ToString() << std::endl;
}
```

5. 解析函数

调用另一个解析器类的接口来解析内容
