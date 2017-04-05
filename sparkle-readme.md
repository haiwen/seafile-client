## Setup WinSparkle environment

[WinSparkle](https://github.com/vslavik/winsparkle) 是 Mac 上的 Sparkle 框架在 windows 上的实现，用于软件自动更新.

* 下载 winsparkle 发布包  https://github.com/vslavik/winsparkle/releases/download/v0.5.3/WinSparkle-0.5.3.zip, 并解压
* 把 include 下的文件拷贝到 /usr/local/lib
* 把 Release/winsparkle.dll 拷贝到 /mingw32/bin
* 把 winsparkle.lib 拷贝到 seafile-client 目录下

在编译时需要加上 `BUILD_SPARKLE_SUPPORT` flag:
```sh
cmake -DBUILD_SPARKLE_SUPPORT=ON .
```


## 更新 appcast.xml

winsparkle 根据下载下来的 appcast.xml 中的数据:

- 判断当前是否有更新版本
- 新版本的下载地址
- 新版本的 release notes (展示给用户看)

发布新的版本时，需要更新appcast中的哪些字段:

- pubDate 字段
- enclosure 中新版本的下载地址 url 字段
- enclosure 中新版本的版本号 sparkle:version 字段

### 本地开发时如何搭建一个简单的 server 来让 seafile-client 去获取 appcast.xml

- 将 appcast.example.xml 修改一下
- 然后本地启动一个 nginx 服务器
- 设置 `SEAFILE_CLIENT_APPCAST_URI` 环境变量, 然后启动 seafile-applet.

```sh
export SEAFILE_CLIENT_APPCAST_URI=http://localhost:8888/appcast.xml
./seafile-applet.exe
```
