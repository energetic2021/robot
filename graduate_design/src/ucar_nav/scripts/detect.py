#!/usr/bin/env python3
# coding=utf-8
import rospy
from std_msgs.msg import String
from pygame import mixer

# 导航结果回调函数
def resultNavi(msg):
    rospy.loginfo("收到导航结果 = %s", msg.data)
    # 检查是否到达目标点
    if msg.data == "done":
        rospy.loginfo("检测到 done，播放到达音效")
        voice_play()
        # 播放完成后可以退出程序（可选）
        rospy.signal_shutdown("任务完成，程序退出")

def voice_play():
    try:
        mixer.init()
        mixer.music.load("/home/zj/graduate_design/src/voice_service/feedback_voice/finish.mp3")
        mixer.music.play()
        # 等待音频播放完成（可选，防止程序提前退出）
        while mixer.music.get_busy():
            rospy.sleep(0.1)
        rospy.loginfo("音效播放成功")
    except Exception as e:
        rospy.logerr("音效播放失败: %s", str(e))

if __name__ == "__main__":
    # 初始化节点
    rospy.init_node("navi_result_listener", anonymous=True)
    
    # 订阅导航结果话题
    result_sub = rospy.Subscriber("/waterplus/navi_result", String, resultNavi, queue_size=10)
    
    rospy.loginfo("开始监听 /waterplus/navi_result 话题，等待 'done' 消息...")
    
    # 保持节点运行，持续监听
    rospy.spin()