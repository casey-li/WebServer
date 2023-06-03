这是在学习[牛客 c++ 项目](https://www.nowcoder.com/study/live/504)过程中跟随视频写的简易版WebServer，包含
- 线程池的实现
- 互斥锁，条件变量，信号量的封装
- 子线程对Http请求的解析（仅支持解析 GET 请求）以及生成响应（返回resource目录下的 index.html）