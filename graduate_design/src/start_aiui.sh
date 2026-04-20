#!/bin/bash

# 设置环境
cd ~/graduate_design
source devel/setup.bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/graduate_design/src/voice_service/lib

# 确保目录存在
mkdir -p ~/graduate_design/devel/lib/voice_service/

# 如果devel目录没有可执行文件，从build复制
if [ ! -f ~/graduate_design/devel/lib/voice_service/voice_service_node ]; then
    echo "Copying executable to devel space..."
    cp ~/graduate_design/build/voice_service/voice_service_node \
       ~/graduate_design/devel/lib/voice_service/
    chmod +x ~/graduate_design/devel/lib/voice_service/voice_service_node
fi

# 启动节点
echo "Starting AIUI node..."
rosrun voice_service voice_service_node \
  _work_dir:=/home/zj/graduate_design/src/voice_service/resources