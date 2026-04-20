#!/usr/bin/env python3

import rospy
import json
import os
from geometry_msgs.msg import PoseWithCovarianceStamped

class PoseCollector:
    def __init__(self, json_path):
        self.json_path = json_path
        self.current_pose = None
        self.data = self.initialize_data()
        self.running = True
        self.confirming = False  # 用于跟踪确认状态
        
        # 创建目录（如果不存在）
        os.makedirs(os.path.dirname(json_path), exist_ok=True)
        
        # 订阅initialpose话题
        rospy.Subscriber("/initialpose", PoseWithCovarianceStamped, self.pose_callback)
        
        # 注册关闭时的保存函数
        rospy.on_shutdown(self.save_data_on_exit)
        
        rospy.loginfo("位姿采集器已启动，等待/initialpose话题数据...")
        self.print_data_status()  # 初始时打印数据状态

    def initialize_data(self):
        """初始化或加载JSON数据"""
        try:
            if os.path.exists(self.json_path):
                with open(self.json_path, 'r') as f:
                    data = json.load(f)
                    rospy.loginfo(f"已加载现有JSON文件: {self.json_path}")
            else:
                data = {str(i): None for i in range(1, 16)}
                rospy.loginfo(f"创建新的JSON文件: {self.json_path}")
        except Exception as e:
            rospy.logerr(f"加载JSON文件失败: {e}")
            data = {str(i): None for i in range(1, 16)}
        
        # 确保有15个位置
        for i in range(1, 16):
            key = str(i)
            if key not in data:
                data[key] = None
        
        return data

    def pose_callback(self, msg):
        """处理接收到的位姿数据"""
        self.current_pose = msg
        pos = msg.pose.pose.position
        orient = msg.pose.pose.orientation
        
        rospy.loginfo("\n" + "="*50)
        rospy.loginfo("收到新位姿:")
        rospy.loginfo(f"位置: x={pos.x:.4f}, y={pos.y:.4f}, z={pos.z:.4f}")
        rospy.loginfo(f"姿态: x={orient.x:.4f}, y={orient.y:.4f}, z={orient.z:.4f}, w={orient.w:.4f}")
        
        self.prompt_user()

    def print_data_status(self):
        """打印所有位置的数据状态"""
        print("\n" + "="*50)
        print("当前数据状态:")
        for i in range(1, 16):
            status = "有数据" if self.data[str(i)] else "空"
            print(f"位置 {i}: {status}", end='\t')
            if i % 5 == 0: print()
        print("="*50)

    def prompt_user(self):
        """提示用户输入位置编号"""
        self.print_data_status()
        print("\n请选择保存位置 (1-15) 或输入 'q' 退出:")
        print("> ", end='', flush=True)

    def save_pose(self, position):
        """保存位姿到指定位置"""
        if self.current_pose is None:
            rospy.logwarn("没有可保存的位姿数据!")
            return False
        
        pos = self.current_pose.pose.pose.position
        orient = self.current_pose.pose.pose.orientation
        
        self.data[position] = {
            "position": {"x": pos.x, "y": pos.y, "z": pos.z},
            "orientation": {"x": orient.x, "y": orient.y, "z": orient.z, "w": orient.w},
            "timestamp": rospy.Time.now().to_sec()
        }
        
        try:
            with open(self.json_path, 'w') as f:
                json.dump(self.data, f, indent=4)
            rospy.loginfo(f"位姿已保存到位置 {position}!")
            self.print_data_status()  # 保存后立即更新状态显示
            return True
        except Exception as e:
            rospy.logerr(f"保存数据失败: {e}")
            return False

    def save_data_on_exit(self):
        """节点关闭时保存数据"""
        if self.running:
            self.running = False
            try:
                with open(self.json_path, 'w') as f:
                    json.dump(self.data, f, indent=4)
                rospy.loginfo("数据已保存到JSON文件")
                self.print_data_status()  # 退出时显示最终状态
            except Exception as e:
                rospy.logerr(f"退出时保存失败: {e}")
            rospy.loginfo("结束信息采集")

    def run(self):
        """主运行循环"""
        try:
            while not rospy.is_shutdown() and self.running:
                if self.current_pose:
                    user_input = input().strip()
                    
                    if self.confirming:
                        # 处理覆盖确认
                        if user_input.lower() == 'y':
                            self.save_pose(self.selected_position)
                            self.current_pose = None
                            self.confirming = False
                            rospy.loginfo("等待下一个位姿数据...")
                        elif user_input.lower() == 'n':
                            rospy.loginfo("取消保存，请重新选择位置")
                            self.confirming = False
                            self.prompt_user()
                        else:
                            rospy.logwarn("无效输入! 请输入 'y' 或 'n'")
                            print("是否覆盖? (y/n) > ", end='', flush=True)
                        continue
                    
                    # 正常输入处理
                    if user_input.lower() == 'q':
                        rospy.signal_shutdown("用户退出")
                        break
                    
                    if user_input in [str(i) for i in range(1, 16)]:
                        # 检查位置是否已有数据
                        if self.data[user_input] is not None:
                            print(f"\n位置 {user_input} 已有数据! 是否覆盖? (y/n)")
                            print("> ", end='', flush=True)
                            self.selected_position = user_input
                            self.confirming = True
                        else:
                            self.save_pose(user_input)
                            self.current_pose = None
                            rospy.loginfo("等待下一个位姿数据...")
                    else:
                        rospy.logwarn("无效输入! 请输入1-15或q")
                        self.prompt_user()
                else:
                    rospy.sleep(0.1)
        except KeyboardInterrupt:
            pass
        finally:
            self.save_data_on_exit()

if __name__ == '__main__':
    rospy.init_node('pose_collector_node')
    
    # 设置JSON文件路径
    json_path = "../data/waypoint.json"
    
    collector = PoseCollector(json_path)
    collector.run()
    rospy.loginfo("节点已关闭")