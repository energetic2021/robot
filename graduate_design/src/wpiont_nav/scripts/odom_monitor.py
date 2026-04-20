#!/usr/bin/env python
import rospy
from nav_msgs.msg import Odometry
from tf.transformations import euler_from_quaternion
import math

class OdomMonitor:
    def __init__(self):
        rospy.init_node('odom_monitor_node')
        self.odom_sub = rospy.Subscriber('/odom', Odometry, self.odom_callback)
        rospy.loginfo("Odom monitor node started.")

    def odom_callback(self, msg):
        pos = msg.pose.pose.position
        ori = msg.pose.pose.orientation
        # 四元数转欧拉角
        _, _, yaw = euler_from_quaternion([ori.x, ori.y, ori.z, ori.w])
        yaw_deg = math.degrees(yaw)
        rospy.loginfo("Position: x = %.3f, y = %.3f | Yaw: %.1f°", pos.x, pos.y, yaw_deg)

if __name__ == '__main__':
    OdomMonitor()
    rospy.spin()