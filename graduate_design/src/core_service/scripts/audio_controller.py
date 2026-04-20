#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
音频控制器 (Audio Controller)
功能：监控系统音频输出状态，在扬声器播放时自动静音麦克风，防止语音交互中的自激和回声干扰。
"""

import rospy
import time
import threading
import subprocess

class AudioController:
    def __init__(self):
        # 初始化 ROS 节点
        rospy.init_node('audio_controller', anonymous=True)
        
        # 自动获取当前系统的麦克风（录音源）索引
        self.mic_source_index = self._get_mic_source_index()
        # 记录上一次的播放状态，用于实现状态切换逻辑，避免重复发送指令
        self.last_playing_state = None  
        
        # 开启后台监控线程，daemon=True 表示主程序退出时线程自动销毁
        self.audio_control_thread = threading.Thread(target=self.audio_monitor_loop)
        self.audio_control_thread.daemon = True
        self.audio_control_thread.start()
        
        rospy.loginfo("音频控制节点启动成功，麦克风音量自动控制已开启")

    def _get_mic_source_index(self):
        """
        获取麦克风输入源的索引
        原理：调用 pactl list sources short 命令，解析出 alsa_input 且非 monitor 的设备
        """
        try:
            output = subprocess.check_output(["pactl", "list", "sources", "short"]).decode('utf-8')
            for line in output.splitlines():
                # 寻找包含 'alsa_input' 且不是 'monitor' 的行
                if 'alsa_input' in line and 'monitor' not in line:
                    return line.split()[0]
            # 如果没找到，默认返回 1
            return "1"
        except Exception as e:
            rospy.logwarn("无法获取麦克风索引: %s", e)
            return "1"

    def audio_monitor_loop(self):
        """
        核心监控循环：持续监控是否有真实的音频电平输出，并据此控制麦克风音量
        """
        loop_count = 0
        
        # 获取系统默认的输出设备（Sink）名称
        try:
            sink_info = subprocess.check_output(["pactl", "info"]).decode('utf-8')
            for line in sink_info.splitlines():
                if "Default Sink" in line or "默认信宿" in line:
                    self.default_sink_name = line.split(": ")[1].strip()
                    break
            else:
                self.default_sink_name = "@DEFAULT_SINK@"
        except:
            self.default_sink_name = "@DEFAULT_SINK@"

        while not rospy.is_shutdown():
            try:
                # 检查所有音频输入流（sink-inputs）的状态
                # 真正有声音播放时，必有 sink-input，且其“抑制”状态（Corked）必为“否”
                input_details = subprocess.check_output(["pactl", "list", "sink-inputs"]).decode('utf-8')
                
                # 过滤逻辑：解析每一个音频流，排除掉系统控制类流，只保留真实的应用播放流
                is_playing = False
                # 根据不同系统的输出格式进行分割解析
                streams = input_details.split("\n\n") if "\n\n" in input_details else input_details.split("信宿输入 #")
                
                for stream in streams:
                    # 检查流是否处于非抑制状态 (Corked: no)
                    if ("抑制: 否" in stream or "Corked: no" in stream):
                        # 排除掉音量控制面板等不产生真实媒体音频的常驻流
                        if "application.name = \"PulseAudio Volume Control\"" in stream:
                            continue
                        is_playing = True
                        break

                # 每隔 50 次循环（约 10 秒）打印一次状态日志，用于调试和确认系统存活
                loop_count += 1
                if loop_count % 50 == 0:
                    rospy.loginfo("正在监控真实音频流... 当前判定: %s", "播放中" if is_playing else "空闲")
                
                # 状态切换逻辑：仅在“播放”和“空闲”状态发生切换时，发送一次 pactl 指令
                if is_playing != self.last_playing_state:
                    if is_playing:
                        # 播放中：静音麦克风，音量设为 0%
                        subprocess.call(["pactl", "set-source-mute", self.mic_source_index, "1"])
                        subprocess.call(["pactl", "set-source-volume", self.mic_source_index, "0%"])
                        rospy.loginfo(">>> 【真·播放开始】：已静音麦克风")
                    else:
                        # 停止播放：取消静音，恢复 100% 音量
                        subprocess.call(["pactl", "set-source-mute", self.mic_source_index, "0"])
                        subprocess.call(["pactl", "set-source-volume", self.mic_source_index, "100%"])
                        rospy.loginfo(">>> 【真·播放结束】：已恢复麦克风")
                    
                    self.last_playing_state = is_playing
                    
            except Exception as e:
                rospy.logwarn("音频监控循环出错: %s", e)
            
            # 设置采样间隔为 0.2 秒，在保证响应速度的同时降低 CPU 占用
            time.sleep(0.2)

    def run(self):
        # 维持 ROS 节点运行
        rospy.spin()

if __name__ == '__main__':
    try:
        controller = AudioController()
        controller.run()
    except rospy.ROSInterruptException:
        rospy.loginfo("音频控制节点已关闭")
