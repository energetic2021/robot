#!/usr/bin/env python3
import rospy
from std_msgs.msg import Float64
from std_srvs.srv import Trigger, TriggerResponse

class SimpleDrawerControl:
    def __init__(self):
        # 发布到Gazebo关节控制话题
        self.drawer1_pub = rospy.Publisher('/drawer1/command', Float64, queue_size=10)
        self.drawer2_pub = rospy.Publisher('/drawer2/command', Float64, queue_size=10)
        self.drawer3_pub = rospy.Publisher('/drawer3/command', Float64, queue_size=10)
        
        # 服务接口
        self.open_all_srv = rospy.Service('/drawers/open_all', Trigger, self.open_all_cb)
        self.close_all_srv = rospy.Service('/drawers/close_all', Trigger, self.close_all_cb)
        
        # 话题接口
        rospy.Subscriber('/drawer1/command', Float64, self.drawer1_cb)
        rospy.Subscriber('/drawer2/command', Float64, self.drawer2_cb)
        rospy.Subscriber('/drawer3/command', Float64, self.drawer3_cb)
        
        rospy.loginfo("简单抽屉控制节点已启动（使用Gazebo插件）")
    
    def drawer1_cb(self, msg):
        """控制抽屉1到指定位置"""
        target_pos = max(0.0, min(0.2, msg.data))
        self.drawer1_pub.publish(Float64(target_pos))
        rospy.loginfo(f"抽屉1移动到位置: {target_pos}")
    
    def drawer2_cb(self, msg):
        target_pos = max(0.0, min(0.2, msg.data))
        self.drawer2_pub.publish(Float64(target_pos))
        rospy.loginfo(f"抽屉2移动到位置: {target_pos}")
    
    def drawer3_cb(self, msg):
        target_pos = max(0.0, min(0.2, msg.data))
        self.drawer3_pub.publish(Float64(target_pos))
        rospy.loginfo(f"抽屉3移动到位置: {target_pos}")
    
    def open_all_cb(self, req):
        """打开所有抽屉服务"""
        try:
            self.drawer1_pub.publish(Float64(0.2))
            self.drawer2_pub.publish(Float64(0.2))
            self.drawer3_pub.publish(Float64(0.2))
            return TriggerResponse(True, "所有抽屉已打开")
        except Exception as e:
            return TriggerResponse(False, f"打开失败: {str(e)}")
    
    def close_all_cb(self, req):
        """关闭所有抽屉服务"""
        try:
            self.drawer1_pub.publish(Float64(0.0))
            self.drawer2_pub.publish(Float64(0.0))
            self.drawer3_pub.publish(Float64(0.0))
            return TriggerResponse(True, "所有抽屉已关闭")
        except Exception as e:
            return TriggerResponse(False, f"关闭失败: {str(e)}")

if __name__ == "__main__":
    rospy.init_node('simple_drawer_control')
    node = SimpleDrawerControl()
    rospy.spin()