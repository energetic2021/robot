#!/bin/bash

# 设置AIUI环境变量
export VOICE_SERVICE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export LD_LIBRARY_PATH="$VOICE_SERVICE_ROOT/lib:$LD_LIBRARY_PATH"
export AIUI_WORK_DIR="$VOICE_SERVICE_ROOT/resources"
export AIUI_ROOT="$VOICE_SERVICE_ROOT"

# 创建必要的目录
mkdir -p "$AIUI_WORK_DIR/log"
mkdir -p "$AIUI_WORK_DIR/msc"

echo "Voice Service environment setup complete"
echo "----------------------------------------"
echo "VOICE_SERVICE_ROOT: $VOICE_SERVICE_ROOT"
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
echo "AIUI_WORK_DIR: $AIUI_WORK_DIR"
echo "----------------------------------------"