#!/usr/bin/env python3
import rospy
import actionlib
from hospital_transport.msg import TransportAction, TransportGoal
import sys

if len(sys.argv) != 3:
    print("用法: rosrun hospital_transport send_task.py <取物点> <送达点>")
    print("示例: rosrun hospital_transport send_task.py 3 4")
    sys.exit(1)

rospy.init_node('send_task')
client = actionlib.SimpleActionClient('hospital_transport', TransportAction)
client.wait_for_server()

goal = TransportGoal()
goal.pickup_point = sys.argv[1]
goal.delivery_point = sys.argv[2]

rospy.loginfo(f"发送任务: 取物点={goal.pickup_point}, 送达点={goal.delivery_point}")
client.send_goal(goal)
client.wait_for_result()

result = client.get_result()
if result.success:
    rospy.loginfo("✅ 任务成功完成！")
else:
    rospy.logerr(f"❌ 任务失败: {result.message}")