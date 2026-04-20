##这是一个机器人项目

# AIUI ROS 节点使用指南

## 项目简介

这是一个基于讯飞AIUI SDK的ROS语音交互节点，将AIUI的语音唤醒、语音识别、语义理解、语音合成等功能完整地封装为ROS服务，方便在机器人项目中集成使用。

## 功能特性

* ✅ **语音唤醒** - 通过唤醒词激活系统
* ✅ **语音识别(ASR)** - 实时语音转文字
* ✅ **语义理解(NLP)** - 支持大模型的智能语义理解
* ✅ **语音合成(TTS)** - 文字转语音播放
* ✅ **录音控制** - 手动开始/停止录音
* ✅ **文本交互** - 直接文本输入交互
* ✅ **状态管理** - 唤醒、休眠、重置控制
* ✅ **实时状态反馈** - 通过话题发布系统状态

## 系统要求

* **操作系统**: Ubuntu 20.04 (ROS Noetic)
* **ROS版本**: Noetic Ninjemys
* **依赖包**: roscpp, std\_msgs, std\_srvs
* **硬件**: 麦克风、扬声器

## 安装与编译

### 1. 目录结构

**text**

```
voice_service/
├── CMakeLists.txt          # 编译配置
├── package.xml              # 包配置
├── include/                 # AIUI头文件
│   └── aiui/
├── lib/                     # AIUI库文件
│   ├── libaiui.so
│   └── libvtn_mic1.so
├── resources/               # AIUI资源文件
│   ├── cfg/                 # 配置文件
│   ├── assets/              # 唤醒词资源
│   └── audio/               # 测试音频
├── src/                     # 源代码
│   ├── aiui_node.cpp        # 主节点文件
│   └── utils/               # 工具类
├── launch/                  # 启动文件
│   └── aiui.launch
└── backup/                  # 原始demo备份
```

### 2. 编译

**bash**

```
# 进入工作空间
cd ~/graduate_design

# 仅编译voice_service包
catkin_make --only-pkg-with-deps voice_service

# 设置环境变量
source devel/setup.bash
```

## 快速开始

### 启动节点

**bash**

```
# 方法1：直接运行（推荐）
cd ~/graduate_design
source devel/setup.bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/graduate_design/src/voice_service/lib
~/graduate_design/build/voice_service/voice_service_node \
  _work_dir:=/home/zj/graduate_design/src/voice_service/resources

# 方法2：使用launch文件
roslaunch voice_service aiui.launch
```

### 基本测试

**bash**

```
# 新开终端
cd ~/graduate_design
source devel/setup.bash

# 唤醒AIUI
rosservice call /aiui/wakeup "{}"

# 开始录音
rosservice call /aiui/start_record "{}"
# 对着麦克风说话...

# 停止录音
rosservice call /aiui/stop_record "{}"

# 查看结果
rostopic echo /aiui/result -n 1
```

## ROS接口说明

### 发布的话题 (Topics)


| 话题名             | 消息类型          | 说明               |
| ------------------ | ----------------- | ------------------ |
| `/aiui/state`      | `std_msgs/String` | AIUI状态变化       |
| `/aiui/result`     | `std_msgs/String` | 最终的语义理解结果 |
| `/aiui/asr_result` | `std_msgs/String` | ASR中间识别结果    |
| `/aiui/nlp_result` | `std_msgs/String` | NLP/LLM中间结果    |

#### 状态说明


| 状态值              | 说明               |
| ------------------- | ------------------ |
| `idle`              | 空闲状态           |
| `ready`             | 就绪状态（待唤醒） |
| `working`           | 工作状态（已唤醒） |
| `wakeup`            | 正在唤醒           |
| `sleep`             | 正在休眠           |
| `connected`         | 已连接到服务器     |
| `disconnected`      | 服务器断开         |
| `error`             | 错误状态           |
| `recording_started` | 开始录音           |
| `recording_stopped` | 停止录音           |
| `vad_bos`           | 检测到说话开始     |
| `vad_eos`           | 检测到说话结束     |
| `tts_started`       | TTS开始播放        |
| `tts_stopped`       | TTS停止播放        |
| `tts_completed`     | TTS播放完成        |
| `tts_error`         | TTS错误            |

### 提供的服务 (Services)


| 服务名               | 服务类型           | 说明         |
| -------------------- | ------------------ | ------------ |
| `/aiui/wakeup`       | `std_srvs/Trigger` | 唤醒AIUI     |
| `/aiui/sleep`        | `std_srvs/Trigger` | 休眠AIUI     |
| `/aiui/tts`          | `std_srvs/SetBool` | 触发语音合成 |
| `/aiui/reset`        | `std_srvs/Trigger` | 重置AIUI     |
| `/aiui/start_record` | `std_srvs/Trigger` | 开始录音     |
| `/aiui/stop_record`  | `std_srvs/Trigger` | 停止录音     |
| `/aiui/write_text`   | `std_srvs/SetBool` | 文本交互     |

### 参数 (Parameters)


| 参数名             | 类型   | 默认值         | 说明             |
| ------------------ | ------ | -------------- | ---------------- |
| `pcm_device_index` | int    | -1             | 音频输出设备索引 |
| `work_dir`         | string | "."            | AIUI资源文件目录 |
| `cfg_path`         | string | "cfg/aiui.cfg" | 配置文件路径     |
| `debug`            | bool   | false          | 调试模式         |

## 使用示例

### 1. 基础对话流程

**bash**

```
# 终端1：启动节点
cd ~/graduate_design
source devel/setup.bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/graduate_design/src/voice_service/lib
~/graduate_design/build/voice_service/voice_service_node \
  _work_dir:=/home/zj/graduate_design/src/voice_service/resources

# 终端2：控制交互
# 唤醒
rosservice call /aiui/wakeup "{}"

# 开始录音
rosservice call /aiui/start_record "{}"
# 说话："今天天气怎么样"

# 停止录音
rosservice call /aiui/stop_record "{}"

# 查看结果
rostopic echo /aiui/result -n 1
```

### 2. 连续对话

**bash**

```
# 唤醒
rosservice call /aiui/wakeup "{}"

# 第一次对话
rosservice call /aiui/start_record "{}"
# 说话...
rosservice call /aiui/stop_record "{}"

# 第二次对话（无需重新唤醒）
rosservice call /aiui/start_record "{}"
# 说话...
rosservice call /aiui/stop_record "{}"

# 结束对话
rosservice call /aiui/sleep "{}"
```

### 3. 文本交互

**bash**

```
# 唤醒
rosservice call /aiui/wakeup "{}"

# 直接发送文本
rosservice call /aiui/write_text "data: true"

# 查看结果
rostopic echo /aiui/result -n 1
```

### 4. 语音播报

**bash**

```
# 触发默认TTS
rosservice call /aiui/tts "data: true"
```

### 5. 监控状态

**bash**

```
# 实时查看状态变化
rostopic echo /aiui/state
```




## 配置文件说明

资源文件目录 `resources/cfg/aiui.cfg` 配置说明：

**json**

```
{
    "login": {
        "appid": "9449f0a6"  // 讯飞应用ID
    },
    "tts": {
        "voice_name": "x5_lingxiaoyue_flow"  // 发音人
    },
    "ivw": {
        "res_path": "AIUI/assets/vtn/vtn.ini",  // 唤醒词资源
        "mic_type": "mic1"  // 麦克风类型
    }
}
```

## 常见问题

### Q1: 找不到可执行文件

**bash**

```
# 手动创建链接
mkdir -p ~/graduate_design/devel/lib/voice_service/
cp ~/graduate_design/build/voice_service/voice_service_node \
   ~/graduate_design/devel/lib/voice_service/
source ~/graduate_design/devel/setup.bash
```

### Q2: 没有声音输出

**bash**

```
# 查看可用设备，选择正确索引
# 启动时添加参数 _pcm_device_index:=3
```

### Q3: 录音没反应

**bash**

```
# 测试麦克风
arecord -d 5 test.wav
aplay test.wav
```

### Q4: 连接服务器失败

**bash**

```
# 检查 appid 是否正确
cat ~/graduate_design/src/voice_service/resources/cfg/aiui.cfg
```

## 注意事项

1. 需要有效的讯飞应用ID（当前使用示例ID `9449f0a6`）
2. 确保麦克风和扬声器正常工作
3. 首次运行需要联网下载模型
4. 节点运行需要 `roscore` 在后台运行

## 更新日志

### v0.1.0 (2024-03-11)

* 初始版本发布
* 完成基础语音交互功能
* 提供完整的ROS接口

## 许可证

本项目基于讯飞AIUI SDK开发，遵循原SDK的许可证要求。
