#!/usr/bin/env python3
# coding=utf-8

"""
医院运输任务服务器 (Hospital Transport Server)
功能：管理机器人在医院环境中的运输任务，包括导航到取物点、操作抽屉、导航到送达点等全流程控制。
"""

import rospy
from std_msgs.msg import String
from std_srvs.srv import Trigger, TriggerResponse
import actionlib
from hospital_transport.msg import TransportAction, TransportGoal, TransportResult

class TransportServer:
    def __init__(self):
        # 初始化 ROS 节点
        rospy.init_node("transport_server")
        
        # 从参数服务器获取配置参数
        self.target_drawer = rospy.get_param("~target_drawer", "1")  # 默认使用的抽屉编号
        self.drawer_open_duration = rospy.get_param("~drawer_open_duration", 30) # 抽屉开启等待时长
        self.stop_delay = rospy.get_param("~stop_delay", 2.0)  # 机器人到达后停稳的缓冲时间
        
        # 创建并启动 Action 服务器，用于接收和管理运输任务
        self.server = actionlib.SimpleActionServer(
            "hospital_transport", 
            TransportAction, 
            self.execute_callback, 
            auto_start=False
        )
        
        # 发布器：向导航模块发送目标航点名称
        self.navi_pub = rospy.Publisher("/waterplus/navi_waypoint", String, queue_size=10)
        
        # 订阅器：接收导航模块反馈的执行结果（done/failed）
        self.result_sub = rospy.Subscriber("/waterplus/navi_result", String, self.resultNavi, queue_size=10)
        
        # 内部状态管理变量
        self.current_state = "idle"        # 当前任务阶段
        self.current_goal = None           # 当前任务的目标信息
        self.drawer_open_time = None       # 记录抽屉打开的时刻
        
        self.server.start()
        rospy.loginfo("医院运输任务服务器已启动")
        rospy.loginfo("配置: 使用抽屉%s, 打开时间=%d秒, 停稳等待=%.1f秒", 
                     self.target_drawer, self.drawer_open_duration, self.stop_delay)
    
    def execute_callback(self, goal):
        """
        Action 执行回调函数：当收到新的运输请求时调用
        """
        self.current_goal = goal
        rospy.loginfo("收到运输任务: 取物点=%s, 送达点=%s", goal.pickup_point, goal.delivery_point)
        
        # 步骤1：下发导航指令，前往取物点
        self.current_state = "going_to_pickup"
        msg = String()
        msg.data = str(goal.pickup_point)
        self.navi_pub.publish(msg)
        rospy.loginfo("前往取物点: %s", goal.pickup_point)
        
        # 循环等待任务状态机在回调中完成各个步骤
        rate = rospy.Rate(1)
        while not rospy.is_shutdown():
            # 检查任务是否被客户端中途取消
            if self.server.is_preempt_requested():
                rospy.loginfo("任务被取消")
                self.server.set_preempted()
                return
            
            # 任务最终完成
            if self.current_state == "completed":
                result = TransportResult()
                result.success = True
                result.message = "运输任务完成"
                self.server.set_succeeded(result)
                return
            
            # 任务中途失败
            if self.current_state == "failed":
                result = TransportResult()
                result.success = False
                result.message = "运输任务失败"
                self.server.set_aborted(result)
                return
            
            rate.sleep()
    
    def resultNavi(self, msg):
        """
        导航结果回调：根据当前状态和导航结果，驱动任务进入下一阶段
        """
        if not self.current_goal:
            return
        
        if msg.data == "done":
            if self.current_state == "going_to_pickup":
                # 到达取物点：停稳 -> 开抽屉 -> 开始计时
                rospy.loginfo("到达取物点，等待%.1f秒后打开抽屉%s...", self.stop_delay, self.target_drawer)
                rospy.sleep(self.stop_delay)  
                self.open_drawer()  
                self.drawer_open_time = rospy.Time.now()
                self.current_state = "waiting_at_pickup"
                
            elif self.current_state == "going_to_delivery":
                # 到达送达点：停稳 -> 开抽屉 -> 开始计时
                rospy.loginfo("到达送达点，等待%.1f秒后打开抽屉%s...", self.stop_delay, self.target_drawer)
                rospy.sleep(self.stop_delay)  
                self.open_drawer()  
                self.drawer_open_time = rospy.Time.now()
                self.current_state = "waiting_at_delivery"
        
        elif msg.data == "failed":
            rospy.logerr("导航失败")
            self.current_state = "failed"
    
    def open_drawer(self):
        """
        调用底层硬件服务：打开指定的机器人抽屉
        """
        try:
            service_name = f'/drawers/open_drawer{self.target_drawer}'
            rospy.wait_for_service(service_name, timeout=1.0)
            open_drawer = rospy.ServiceProxy(service_name, Trigger)
            resp = open_drawer()
            if resp.success:
                rospy.loginfo("抽屉%s已打开", self.target_drawer)
                # 在参数服务器记录打开时间（可选，用于多节点同步）
                rospy.set_param("drawer_open_time", rospy.Time.now().to_sec())
            else:
                rospy.logerr("打开抽屉%s失败: %s", self.target_drawer, resp.message)
        except Exception as e:
            rospy.logerr("打开抽屉异常: %s", str(e))
    
    def close_all_drawers(self):
        """
        调用底层硬件服务：关闭机器人所有抽屉
        """
        try:
            rospy.wait_for_service('/drawers/close_all', timeout=1.0)
            close_all = rospy.ServiceProxy('/drawers/close_all', Trigger)
            resp = close_all()
            if resp.success:
                rospy.loginfo("所有抽屉已关闭")
            else:
                rospy.logerr("关闭所有抽屉失败: %s", resp.message)
        except Exception as e:
            rospy.logerr("关闭抽屉异常: %s", str(e))
    
    def update(self):
        """
        主逻辑轮询更新：负责处理基于时间的超时逻辑（如等待取货/卸货时间到）
        """
        # 情况A：在取物点等待超时
        if self.current_state == "waiting_at_pickup" and self.drawer_open_time:
            elapsed = (rospy.Time.now() - self.drawer_open_time).to_sec()
            if elapsed >= self.drawer_open_duration:
                rospy.loginfo("取物点等待时间到，关闭所有抽屉并前往送达点")
                self.close_all_drawers()
                self.drawer_open_time = None
                self.current_state = "going_to_delivery"
                msg = String()
                msg.data = str(self.current_goal.delivery_point)
                self.navi_pub.publish(msg)
                rospy.loginfo("前往送达点: %s", self.current_goal.delivery_point)
        
        # 情况B：在送达点等待超时
        elif self.current_state == "waiting_at_delivery" and self.drawer_open_time:
            elapsed = (rospy.Time.now() - self.drawer_open_time).to_sec()
            if elapsed >= self.drawer_open_duration:
                rospy.loginfo("送达点等待时间到，关闭所有抽屉，任务圆满完成")
                self.close_all_drawers()
                self.drawer_open_time = None
                self.current_state = "completed"

if __name__ == "__main__":
    server = TransportServer()
    # 以 10Hz 的频率运行主更新循环
    rate = rospy.Rate(10)
    while not rospy.is_shutdown():
        server.update()
        rate.sleep()
