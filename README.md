# MediaMaster 多媒体影音管理系统
基于Qt6开发的跨平台多媒体影音管理工具，支持本地音视频扫描、播放、格式转换、文件管理等功能。

## 功能特性
- 多线程文件夹扫描，实时展示进度
- 主流音视频格式播放（MP4/AVI/MP3/WAV等）
- 基于FFmpeg的格式转换，后台异步执行
- 简单的文件重命名、删除管理
- 跨平台兼容（Windows/Linux/macOS）

## 开发环境
- Qt 6.5+
- C++17
- FFmpeg 5.1+
- CMake 3.16+

## 编译运行
```bash
# 克隆仓库
git clone https://github.com/你的用户名/MediaMaster.git
# 构建项目
mkdir build && cd build
cmake ..
make # Windows用mingw32-make/VS编译