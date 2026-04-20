#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import rospy
import actionlib
import os
import subprocess
from move_base_msgs.msg import MoveBaseAction, MoveBaseGoal
from waterplus_map_tools.srv import GetNumOfWaypoints, GetWaypointByIndex
from std_msgs.msg import String

class MultiGoalNavigator:
    def __init__(self):
        rospy.init_node('multi_goal_navigator')

        # 参数配置（可通过roslaunch覆盖）
        self.qrcode_trigger_index = rospy.get_param('~qrcode_trigger_index', 1)
        self.qrcode_timeout = rospy.get_param('~qrcode_timeout', 30.0)
        self.save_dir = os.path.expanduser(rospy.get_param('~save_dir', '~/ucar_ws/src/waypoint_nav/data'))

        # 初始化服务
        rospy.wait_for_service('waterplus/get_num_waypoint')
        rospy.wait_for_service('waterplus/get_waypoint_index')
        self.num_srv = rospy.ServiceProxy('waterplus/get_num_waypoint', GetNumOfWaypoints)
        self.idx_srv = rospy.ServiceProxy('waterplus/get_waypoint_index', GetWaypointByIndex)

        # 加载航点
        self.waypoints = self._load_waypoints()
        self.client = actionlib.SimpleActionClient('move_base', MoveBaseAction)
        self.client.wait_for_server()

    def _load_waypoints(self):
        """从服务加载航点数据"""
        waypoints = []
        try:
            num = self.num_srv().num
            for idx in range(num):
                wp = self.idx_srv(idx)
                waypoints.append((
                    wp.name,
                    {
                        'position': {
                            'x': wp.pose.position.x,
                            'y': wp.pose.position.y,
                            'z': wp.pose.position.z,
                        },
                        'orientation': {
                            'x': wp.pose.orientation.x,
                            'y': wp.pose.orientation.y,
                            'z': wp.pose.orientation.z,
                            'w': wp.pose.orientation.w,
                        }
                    }
                ))
            rospy.loginfo(f"成功加载 {len(waypoints)} 个航点")
        except Exception as e:
            rospy.logerr(f"航点加载失败: {e}")
        return waypoints

    def send_goal(self, goal_name, pose):
        """发送导航目标"""
        goal = MoveBaseGoal()
        goal.target_pose.header.frame_id = "map"
        goal.target_pose.header.stamp = rospy.Time.now()
        
        # 设置目标位姿
        goal.target_pose.pose.position.x = pose['position']['x']
        goal.target_pose.pose.position.y = pose['position']['y']
        goal.target_pose.pose.position.z = pose['position']['z']
        goal.target_pose.pose.orientation.x = pose['orientation']['x']
        goal.target_pose.pose.orientation.y = pose['orientation']['y']
        goal.target_pose.pose.orientation.z = pose['orientation']['z']
        goal.target_pose.pose.orientation.w = pose['orientation']['w']

        self.client.send_goal(goal)
        if not self.client.wait_for_result(rospy.Duration(60)):
            rospy.logwarn(f"导航到 {goal_name} 超时")
            return False
        return self.client.get_result()

    def _qrcode_callback(self, msg):
        """二维码回调函数"""
        self.qrcode_content = msg.data
        rospy.loginfo(f"识别到二维码内容: {self.qrcode_content}")

    def handle_qrcode_event(self):
        """处理二维码识别（使用subprocess启动节点）"""
        self.qrcode_content = None
        
        # 先建立订阅
        sub = rospy.Subscriber('/qrcode/content', String, self._qrcode_callback)
        rospy.sleep(1)  # 确保订阅建立

        try:
            # 启动二维码识别节点
            proc = subprocess.Popen([
                'roslaunch', 
                'ucar_camera', 
                'qrcode.launch'
            ])
            
            # 等待结果
            start_time = rospy.Time.now()
            while not rospy.is_shutdown():
                if self.qrcode_content is not None:
                    break
                if (rospy.Time.now() - start_time).to_sec() > self.qrcode_timeout:
                    rospy.logwarn("二维码识别超时")
                    break
                rospy.sleep(0.1)

            # 保存结果
            if self.qrcode_content:
                os.makedirs(self.save_dir, exist_ok=True)
                save_path = os.path.join(self.save_dir, 'kind.txt')
                with open(save_path, 'w') as f:
                    f.write(self.qrcode_content)
                rospy.loginfo(f"二维码内容已保存到 {save_path}")

        finally:
            # 清理资源
            sub.unregister()
            if 'proc' in locals():
                proc.terminate()

    def run(self):
        """执行主导航逻辑"""
        try:
            for idx, (goal_name, pose) in enumerate(self.waypoints):
                if self.send_goal(goal_name, pose):
                    rospy.loginfo(f"成功到达航点: {goal_name}")
                    if idx == self.qrcode_trigger_index:
                        self.handle_qrcode_event()
                else:
                    rospy.logerr(f"导航到 {goal_name} 失败!")
                    break
        except KeyboardInterrupt:
            rospy.loginfo("用户中断导航")
        finally:
            rospy.loginfo("导航任务结束")

if __name__ == '__main__':
    try:
        navigator = MultiGoalNavigator()
        navigator.run()
    except rospy.ROSInterruptException:
        rospy.logerr("ROS中断!")
    except Exception as e:
        rospy.logerr(f"程序错误: {e}")