# 1. 指定 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 2. 设置项目名称
project(SFC_Deployment_GA VERSION 1.0)

# 3. 强制使用 C++11 标准 (网络空间安全底层开发标配)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. 告诉编译器去哪里找 .h 头文件
include_directories(include)

# 5. 自动搜索 src 目录下所有的 .cpp 源文件
aux_source_directory(src DIR_SRCS)

# 6. 设置编译好的可执行文件放在 bin 文件夹下
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 7. 生成可执行文件 sfc_engine
# (前提是你的 src 文件夹里必须有一个 main.cpp，哪怕里面只有个空的 main 函数)
add_executable(sfc_engine ${DIR_SRCS})

message(STATUS "🚀 Build configured for: ${PROJECT_NAME}")