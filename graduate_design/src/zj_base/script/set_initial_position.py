#!/usr/bin/env python3
import rospy
from std_msgs.msg import Float64

def set_initial_position():
    rospy.init_node('set_initial_drawer_position')
    rospy.sleep(2.0)  # 等待控制器启动
    
    # 创建发布器
    drawer1_pub = rospy.Publisher('/drawer1_controller/command', Float64, queue_size=10)
    drawer2_pub = rospy.Publisher('/drawer2_controller/command', Float64, queue_size=10)
    drawer3_pub = rospy.Publisher('/drawer3_controller/command', Float64, queue_size=10)
    
    # 设置初始位置为关闭状态
    drawer1_pub.publish(Float64(0.0))
    drawer2_pub.publish(Float64(0.0))
    drawer3_pub.publish(Float64(0.0))
    
    rospy.loginfo("抽屉初始位置已设置为关闭状态")
    
    # 短暂等待确保消息发布
    rospy.sleep(0.5)

if __name__ == "__main__":
    set_initial_position()