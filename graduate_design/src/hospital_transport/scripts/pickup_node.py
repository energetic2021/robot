#!/usr/bin/env python3
# coding=utf-8

"""
取送物管理节点 (Pickup and Delivery Node)
功能：简化版的取送物逻辑实现，集成了音效反馈、导航目标下发以及抽屉自动控制。
"""

import rospy
from std_msgs.msg import String
from pygame import mixer
import time

class PickupNode:
    def __init__(self):
        # 初始化 ROS 节点
        rospy.init_node("pickup_node")
        
        # 发布器：下发导航目标点航点名称
        self.navi_pub = rospy.Publisher("/waterplus/navi_waypoint", String, queue_size=10)
        
        # 订阅器：监听导航执行状态
        self.result_sub = rospy.Subscriber("/waterplus/navi_result", String, self.resultNavi, queue_size=10)
        
        # 任务状态机变量
        # 状态包括: idle(空闲), going_to_pickup(前往取货), waiting_at_pickup(等待装货), 
        # going_to_delivery(前往送货), waiting_at_delivery(等待卸货), completed(任务完成)
        self.state = "idle"  
        self.pickup_point = None
        self.delivery_point = None
        
        # 业务逻辑参数
        self.drawer_open_duration = 30  # 抽屉保持打开的时间（秒）
        self.drawer_open_time = None    # 抽屉打开的起始时刻
        
        # 初始化音效反馈系统
        self.init_voice()
        
        rospy.loginfo("取送物节点初始化完成")
        
    def init_voice(self):
        """
        初始化 pygame mixer 用于播放本地 MP3 音效
        """
        try:
            mixer.init()
            # 音效文件存放路径
            self.voice_path = "/home/zj/graduate_design/src/voice_service/feedback_voice/"
            rospy.loginfo("音效系统初始化成功")
        except Exception as e:
            rospy.logerr("音效系统初始化失败: %s", str(e))
    
    def play_sound(self, sound_file):
        """
        播放指定的音效文件
        """
        try:
            mixer.music.load(self.voice_path + sound_file)
            mixer.music.play()
            rospy.loginfo("播放音效: %s", sound_file)
        except Exception as e:
            rospy.logerr("音效播放失败: %s", str(e))
    
    def resultNavi(self, msg):
        """
        导航结果回调：处理到达目标点后的业务逻辑（播报、开门、状态切换）
        """
        rospy.loginfo("导航结果 = %s, 当前状态 = %s", msg.data, self.state)
        
        if msg.data == "done":
            # 导航成功到达
            if self.state == "going_to_pickup":
                # 到达取物点逻辑
                rospy.loginfo("已到达取物点，开始打开抽屉")
                self.play_sound("arrive_pickup.mp3")
                self.open_drawer("all")  # 打开所有抽屉供装货
                self.drawer_open_time = time.time()
                self.state = "waiting_at_pickup"
                
            elif self.state == "going_to_delivery":
                # 到达送达点逻辑
                rospy.loginfo("已到达送达点，开始打开抽屉")
                self.play_sound("arrive_delivery.mp3")
                self.open_drawer("all")  # 打开所有抽屉供卸货
                self.drawer_open_time = time.time()
                self.state = "waiting_at_delivery"
        
        elif msg.data == "failed":
            # 导航失败处理
            rospy.logerr("导航失败，当前状态: %s", self.state)
            self.play_sound("navigation_failed.mp3")
            self.state = "idle"
    
    def open_drawer(self, drawer_id):
        """
        通过 ROS Service 调用底层驱动打开抽屉
        """
        try:
            if drawer_id == "all":
                # 开启所有抽屉的服务
                rospy.wait_for_service('/drawers/open_all', timeout=1.0)
                from std_srvs.srv import Trigger
                open_all = rospy.ServiceProxy('/drawers/open_all', Trigger)
                resp = open_all()
                if resp.success:
                    rospy.loginfo("所有抽屉已打开")
                    self.play_sound("drawer_open.mp3")
                else:
                    rospy.logerr("打开所有抽屉失败: %s", resp.message)
            else:
                # 开启指定编号抽屉的服务
                service_name = f'/drawers/open_drawer{drawer_id}'
                rospy.wait_for_service(service_name, timeout=1.0)
                from std_srvs.srv import Trigger
                open_drawer = rospy.ServiceProxy(service_name, Trigger)
                resp = open_drawer()
                if resp.success:
                    rospy.loginfo("抽屉 %s 已打开", drawer_id)
                    self.play_sound("drawer_open.mp3")
                else:
                    rospy.logerr("打开抽屉 %s 失败: %s", drawer_id, resp.message)
        except Exception as e:
            rospy.logerr("打开抽屉异常: %s", str(e))
    
    def close_drawer(self, drawer_id):
        """
        通过 ROS Service 调用底层驱动关闭抽屉
        """
        try:
            if drawer_id == "all":
                # 关闭所有抽屉的服务
                rospy.wait_for_service('/drawers/close_all', timeout=1.0)
                from std_srvs.srv import Trigger
                close_all = rospy.ServiceProxy('/drawers/close_all', Trigger)
                resp = close_all()
                if resp.success:
                    rospy.loginfo("所有抽屉已关闭")
                    self.play_sound("drawer_close.mp3")
                else:
                    rospy.logerr("关闭所有抽屉失败: %s", resp.message)
        except Exception as e:
            rospy.logerr("关闭抽屉异常: %s", str(e))
    
    def start_pickup_delivery(self, pickup_point, delivery_point):
        """
        启动一个完整的取送物任务流程
        """
        self.pickup_point = pickup_point
        self.delivery_point = delivery_point
        self.state = "going_to_pickup"
        
        rospy.loginfo("开始取送物任务: 取物点=%s, 送达点=%s", pickup_point, delivery_point)
        self.play_sound("task_start.mp3")
        
        # 向导航系统下发第一个目标点
        msg = String()
        msg.data = str(pickup_point)
        rospy.sleep(1)  # 简易延迟确保发布成功
        self.navi_pub.publish(msg)
        rospy.loginfo("已发送取物点: %s", pickup_point)
    
    def run(self):
        """
        主控制循环：处理超时自动关闭抽屉及阶段跳转
        """
        rate = rospy.Rate(1)  # 1Hz 检查频率
        
        while not rospy.is_shutdown():
            # 检查在取物点的等待时长
            if self.state == "waiting_at_pickup" and self.drawer_open_time:
                elapsed = time.time() - self.drawer_open_time
                if elapsed >= self.drawer_open_duration:
                    rospy.loginfo("取物点抽屉打开时间到，自动关闭并前往送货")
                    self.close_drawer("all")
                    self.drawer_open_time = None
                    
                    # 切换到送货阶段
                    self.state = "going_to_delivery"
                    msg = String()
                    msg.data = str(self.delivery_point)
                    self.navi_pub.publish(msg)
                    rospy.loginfo("已发送送达点: %s", self.delivery_point)
            
            # 检查在送达点的等待时长
            elif self.state == "waiting_at_delivery" and self.drawer_open_time:
                elapsed = time.time() - self.drawer_open_time
                if elapsed >= self.drawer_open_duration:
                    rospy.loginfo("送达点抽屉打开时间到，任务结束")
                    self.close_drawer("all")
                    self.drawer_open_time = None
                    self.state = "completed"
                    rospy.loginfo("取送物任务完成")
                    self.play_sound("task_complete.mp3")
            
            rate.sleep()

if __name__ == "__main__":
    try:
        node = PickupNode()
        rospy.sleep(2)
        
        # 从 ROS 参数中读取任务目标，方便外部动态配置
        pickup_point = rospy.get_param("~pickup_point", "1")
        delivery_point = rospy.get_param("~delivery_point", "2")
        
        node.start_pickup_delivery(pickup_point, delivery_point)
        node.run()
        
    except rospy.ROSInterruptException:
        pass