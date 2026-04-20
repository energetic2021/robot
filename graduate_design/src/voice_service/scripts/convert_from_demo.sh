#!/bin/bash

# 转换脚本：从备份的demo复制文件到正确位置
echo "Converting AIUI demo to ROS package structure..."

DEMO_DIR="backup/Linux_aiui6.8.0003.0008_9449f0a6"
CURRENT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# 复制头文件
echo "Copying header files..."
mkdir -p $CURRENT_DIR/include/aiui
cp -r $DEMO_DIR/include/aiui/* $CURRENT_DIR/include/aiui/

# 复制库文件
echo "Copying library files..."
mkdir -p $CURRENT_DIR/lib
cp $DEMO_DIR/bin_x64/libaiui.so $CURRENT_DIR/lib/
cp $DEMO_DIR/bin_x64/libvtn_mic1.so $CURRENT_DIR/lib/

# 复制资源文件
echo "Copying resource files..."
mkdir -p $CURRENT_DIR/resources
cp -r $DEMO_DIR/bin_x64/AIUI/* $CURRENT_DIR/resources/

# 复制示例代码（作为参考）
echo "Copying sample code for reference..."
mkdir -p $CURRENT_DIR/src/reference
cp $DEMO_DIR/samples/src/demo.cpp $CURRENT_DIR/src/reference/
cp -r $DEMO_DIR/samples/src/utils $CURRENT_DIR/src/reference/
cp -r $DEMO_DIR/samples/src/jsoncpp $CURRENT_DIR/src/reference/

echo "Conversion complete!"
echo "Original demo files are preserved in backup/Linux_aiui6.8.0003.0008_9449f0a6"