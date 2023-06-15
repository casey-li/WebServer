#if 0

#include <fstream>
#include <sstream>
#include "json.h"

int main()
{
    using std::cout, std::endl;
    // // 测试基本类型的构造函数
    // Json j1;
    // Json j2 = true;
    // Json j3 = 1;
    // Json j4 = 1.23;
    // Json j5 = "ssss";
    // Json j6(j5);

    // // 测试 Json 向基本类型的转换
    // bool flag = j2;
    // int i = j3;
    // double d = j4;
    // std::string str = j6;

    // // 测试数组类型
    // Json arr;
    // arr.PushBack(1.23);
    // arr.PushBack(false);
    // arr.PushBack(3);
    // arr[0] = false;
    // arr[1] = "sss";
    // std::string str = "aaa";
    // bool flag = arr[0];
    // str = arr[1].AsString();
    // int i = arr[2];
    // cout << arr.ToString() << endl;
    // for (auto it = arr.ArrBegin(); it != arr.ArrEnd(); ++it)
    // {
    //     cout << it->ToString() << endl;
    // }

    // Json arr1;
    // arr1.PushBack(false);
    // arr1.PushBack("sss");
    // arr1.PushBack(3);
    // cout << arr1.ToString() << endl;
    // cout << (arr == arr1) << endl;
    // cout << arr1.Size() << endl;
    // arr1.Erase(1);
    // cout << arr1.ToString() << endl;
    // arr1.PopBack();
    // cout << arr1.Size() << endl;
    // cout << arr1.ToString() << endl;

    // // 测试对象类型
    // Json obj;
    // obj["str"] = "ssss";
    // obj["int"] = 2;
    // obj["bool"] = true;
    // obj["hello"] = "world";
    // cout << obj.ToString() << endl;
    // for (auto it = obj.ObjBegin(); it != obj.ObjEnd(); ++it)
    // {
    //     cout << it->second.ToString() << endl;
    // }

    // Json obj1;
    // obj1["int"] = 2;
    // obj1["str"] = "ssss";
    // obj1["hello"] = "world";
    // obj1["bool"] = true;
    // cout << obj1.Size() << endl;
    // cout << obj1.ToString() << endl;
    // cout << (obj == obj1) << endl;
    // obj1.Erase("str");
    // cout << obj1.Size() << endl;
    // cout << obj1.Count("bool") << endl;
    // cout << obj1.Count("str") << endl;
    // obj1["arr"] = arr1;
    // obj1["bool"] = false;
    // cout << obj1.ToString() << endl;

    // 测试解析真实的 json 文件
    std::ifstream fin("./test.json");
    std::stringstream ss;
    ss << fin.rdbuf();
    const std::string &str = ss.str();
    Json j;
    j.InitializationParse(str);
    Json res = j.Parse();
    cout << res["RuleSet"][0]["Countries"].ToString() << endl;
    return 0;
}
#endif

