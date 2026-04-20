#!/usr/bin/env python3
import rospy
from std_msgs.msg import Float64
from std_srvs.srv import Trigger, TriggerResponse

class DrawerControlNode:
    def __init__(self):
        # 发布到标准控制器话题
        self.drawer1_pub = rospy.Publisher('/drawer1_controller/command', Float64, queue_size=10)
        self.drawer2_pub = rospy.Publisher('/drawer2_controller/command', Float64, queue_size=10)
        self.drawer3_pub = rospy.Publisher('/drawer3_controller/command', Float64, queue_size=10)
        
        # 服务接口
        self.open_all_srv = rospy.Service('/drawers/open_all', Trigger, self.open_all_cb)
        self.close_all_srv = rospy.Service('/drawers/close_all', Trigger, self.close_all_cb)
        self.open_drawer1_srv = rospy.Service('/drawers/open_drawer1', Trigger, self.open_drawer1_cb)
        self.open_drawer2_srv = rospy.Service('/drawers/open_drawer2', Trigger, self.open_drawer2_cb)
        self.open_drawer3_srv = rospy.Service('/drawers/open_drawer3', Trigger, self.open_drawer3_cb)
        
        # 话题接口
        rospy.Subscriber('/drawer1/command', Float64, self.drawer1_cb)
        rospy.Subscriber('/drawer2/command', Float64, self.drawer2_cb)
        rospy.Subscriber('/drawer3/command', Float64, self.drawer3_cb)
        
        # 抽屉位置限制
        self.drawer_limits = {
            'min': 0.0,
            'max': 0.2
        }
        
        rospy.loginfo("抽屉控制节点已启动（使用effort控制器）")
        rospy.loginfo("可用服务:")
        rospy.loginfo("  /drawers/open_all - 打开所有抽屉")
        rospy.loginfo("  /drawers/close_all - 关闭所有抽屉")
        rospy.loginfo("  /drawers/open_drawer1 - 打开抽屉1")
        rospy.loginfo("  /drawers/open_drawer2 - 打开抽屉2") 
        rospy.loginfo("  /drawers/open_drawer3 - 打开抽屉3")
        rospy.loginfo("可用话题:")
        rospy.loginfo("  /drawer1/command - 控制抽屉1位置 (0.0-0.2)")
        rospy.loginfo("  /drawer2/command - 控制抽屉2位置 (0.0-0.2)")
        rospy.loginfo("  /drawer3/command - 控制抽屉3位置 (0.0-0.2)")
    
    def limit_position(self, position):
        """限制位置在有效范围内"""
        return max(self.drawer_limits['min'], min(self.drawer_limits['max'], position))
    
    def drawer1_cb(self, msg):
        """控制抽屉1到指定位置"""
        target_pos = self.limit_position(msg.data)
        self.drawer1_pub.publish(Float64(target_pos))
        rospy.loginfo(f"抽屉1移动到位置: {target_pos}")
    
    def drawer2_cb(self, msg):
        target_pos = self.limit_position(msg.data)
        self.drawer2_pub.publish(Float64(target_pos))
        rospy.loginfo(f"抽屉2移动到位置: {target_pos}")
    
    def drawer3_cb(self, msg):
        target_pos = self.limit_position(msg.data)
        self.drawer3_pub.publish(Float64(target_pos))
        rospy.loginfo(f"抽屉3移动到位置: {target_pos}")
    
    def open_all_cb(self, req):
        """打开所有抽屉服务"""
        try:
            open_pos = self.drawer_limits['max']
            self.drawer1_pub.publish(Float64(open_pos))
            self.drawer2_pub.publish(Float64(open_pos))
            self.drawer3_pub.publish(Float64(open_pos))
            rospy.loginfo("所有抽屉已打开")
            return TriggerResponse(True, "所有抽屉已打开")
        except Exception as e:
            rospy.logerr(f"打开所有抽屉失败: {str(e)}")
            return TriggerResponse(False, f"打开失败: {str(e)}")
    
    def close_all_cb(self, req):
        """关闭所有抽屉服务"""
        try:
            close_pos = self.drawer_limits['min']
            self.drawer1_pub.publish(Float64(close_pos))
            self.drawer2_pub.publish(Float64(close_pos))
            self.drawer3_pub.publish(Float64(close_pos))
            rospy.loginfo("所有抽屉已关闭")
            return TriggerResponse(True, "所有抽屉已关闭")
        except Exception as e:
            rospy.logerr(f"关闭所有抽屉失败: {str(e)}")
            return TriggerResponse(False, f"关闭失败: {str(e)}")
    
    def open_drawer1_cb(self, req):
        """单独打开抽屉1"""
        try:
            self.drawer1_pub.publish(Float64(self.drawer_limits['max']))
            rospy.loginfo("抽屉1已打开")
            return TriggerResponse(True, "抽屉1已打开")
        except Exception as e:
            rospy.logerr(f"打开抽屉1失败: {str(e)}")
            return TriggerResponse(False, f"打开抽屉1失败: {str(e)}")
    
    def open_drawer2_cb(self, req):
        """单独打开抽屉2"""
        try:
            self.drawer2_pub.publish(Float64(self.drawer_limits['max']))
            rospy.loginfo("抽屉2已打开")
            return TriggerResponse(True, "抽屉2已打开")
        except Exception as e:
            rospy.logerr(f"打开抽屉2失败: {str(e)}")
            return TriggerResponse(False, f"打开抽屉2失败: {str(e)}")
    
    def open_drawer3_cb(self, req):
        """单独打开抽屉3"""
        try:
            self.drawer3_pub.publish(Float64(self.drawer_limits['max']))
            rospy.loginfo("抽屉3已打开")
            return TriggerResponse(True, "抽屉3已打开")
        except Exception as e:
            rospy.logerr(f"打开抽屉3失败: {str(e)}")
            return TriggerResponse(False, f"打开抽屉3失败: {str(e)}")

if __name__ == "__main__":
    rospy.init_node('drawer_control_node')
    node = DrawerControlNode()
    rospy.spin()