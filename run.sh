#!/bin/bash

echo "⚙️  开始自动化构建流程..."

# 1. 确保 build 文件夹存在，并安全进入
mkdir -p build
cd build || exit 1  # 如果进不去 build 目录，立刻停止，绝不往下执行删库命令！

# 2. 只有确认在 build 目录下，才执行原力清空
if [ "$(basename "$PWD")" = "build" ]; then
    echo "🧹 清理旧缓存..."
    rm -rf *
else
    echo "❌ 目录错误，安全退出！"
    exit 1
fi

# 3. 编译
echo "🔨 正在编译..."
cmake ..
make

# 4. 检查编译是否成功
if [ $? -eq 0 ]; then
    echo "✅ 编译成功！🚀 启动引擎..."
    echo "------------------------------------------------"
    # 退回根目录并执行程序
    cd ..
    ./bin/sfc_engine
else
    echo "❌ 编译失败，请检查代码报错！"
fi
