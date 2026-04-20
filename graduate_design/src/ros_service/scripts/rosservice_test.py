#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import rospy
import sys
import json
from std_srvs.srv import Trigger, TriggerRequest, TriggerResponse
from std_srvs.srv import Empty, EmptyRequest, EmptyResponse

class RosServiceNode:
    """
    ROS服务端节点类
    提供各种可以被Java后端调用的ROS服务
    """
    
    def __init__(self):
        # 初始化节点
        rospy.init_node('ros_service_node', anonymous=False)
        
        self.services = []
        self.init_services()
        
        rospy.loginfo("=" * 50)
        rospy.loginfo("ROS服务端节点已启动")
        rospy.loginfo("节点名称: ros_service_node")
        rospy.loginfo("等待Java后端调用...")
        rospy.loginfo("=" * 50)
    
    def init_services(self):
        """初始化所有提供的服务"""
        
        # 1. 加法服务
        try:
            from ros_service.srv import AddTwoInts, AddTwoIntsResponse
            self.services.append(
                rospy.Service('/add_two_ints', AddTwoInts, self.handle_add_two_ints)
            )
            rospy.loginfo("✅ 服务已注册: /add_two_ints (自定义服务)")
        except ImportError:
            rospy.logwarn("⚠️ 未找到AddTwoInts服务，跳过加法服务注册")
        
        # 2. 机器人状态服务 (使用Trigger)
        self.services.append(
            rospy.Service('/robot/status', Trigger, self.handle_robot_status)
        )
        rospy.loginfo("✅ 服务已注册: /robot/status (std_srvs/Trigger)")
        
        # 3. 机器人移动控制 (使用Trigger)
        self.services.append(
            rospy.Service('/robot/move', Trigger, self.handle_robot_move)
        )
        rospy.loginfo("✅ 服务已注册: /robot/move (std_srvs/Trigger)")
        
        # 4. 传感器数据服务 (使用Trigger)
        self.services.append(
            rospy.Service('/sensor/data', Trigger, self.handle_sensor_data)
        )
        rospy.loginfo("✅ 服务已注册: /sensor/data (std_srvs/Trigger)")
        
        # 5. 系统健康检查 (使用Trigger)
        self.services.append(
            rospy.Service('/system/health', Trigger, self.handle_system_health)
        )
        rospy.loginfo("✅ 服务已注册: /system/health (std_srvs/Trigger)")
        
        # 6. 机器人停止服务 (使用Empty)
        self.services.append(
            rospy.Service('/robot/stop', Empty, self.handle_robot_stop)
        )
        rospy.loginfo("✅ 服务已注册: /robot/stop (std_srvs/Empty)")
    
    def handle_add_two_ints(self, req):
        """
        处理加法服务请求
        """
        rospy.loginfo("=" * 40)
        rospy.loginfo("收到加法服务请求")
        rospy.loginfo(f"请求参数: a = {req.a}, b = {req.b}")
        
        # 计算和
        result_sum = req.a + req.b
        
        rospy.loginfo(f"计算结果: {req.a} + {req.b} = {result_sum}")
        rospy.loginfo("=" * 40)
        
        # 返回响应
        from ros_service.srv import AddTwoIntsResponse
        return AddTwoIntsResponse(result_sum)
    
    def handle_robot_status(self, req):
        """
        处理机器人状态查询服务
        """
        rospy.loginfo("收到机器人状态查询请求")
        
        # 构建状态数据
        status = {
            'battery': 85,
            'position': {
                'x': 1.2,
                'y': 3.4,
                'z': 0.0
            },
            'speed': 0.5,
            'mode': 'auto',
            'status': 'running',
            'timestamp': rospy.get_time()
        }
        
        # 转换为JSON字符串
        status_json = json.dumps(status)
        rospy.loginfo(f"返回状态: {status_json}")
        
        # TriggerResponse需要返回(success, message)
        return TriggerResponse(
            success=True,
            message=status_json
        )
    
    def handle_robot_move(self, req):
        """
        处理机器人移动控制服务
        """
        rospy.loginfo("收到机器人移动控制请求")
        
        # 这里可以添加实际的移动控制逻辑
        # req是TriggerRequest，没有数据字段
        
        return TriggerResponse(
            success=True,
            message="移动指令已执行"
        )
    
    def handle_sensor_data(self, req):
        """
        处理传感器数据查询服务
        """
        rospy.loginfo("收到传感器数据查询请求")
        
        # 模拟传感器数据
        sensor_data = {
            'laser': [1.2, 3.4, 5.6, 7.8],
            'ultrasonic': 2.3,
            'temperature': 25.6,
            'humidity': 60.5,
            'imu': {
                'accel': [0.1, 0.2, 9.8],
                'gyro': [0.01, 0.02, 0.03],
                'mag': [0.5, 0.6, 0.7]
            },
            'timestamp': rospy.get_time()
        }
        
        sensor_json = json.dumps(sensor_data)
        
        return TriggerResponse(
            success=True,
            message=sensor_json
        )
    
    def handle_system_health(self, req):
        """
        处理系统健康检查服务
        """
        rospy.loginfo("收到系统健康检查请求")
        
        # 检查系统状态
        health_status = {
            'cpu_usage': 23.5,
            'memory_usage': 45.2,
            'disk_usage': 32.1,
            'ros_node_count': len(rospy.get_node_list()),
            'uptime': rospy.get_time(),
            'status': 'healthy'
        }
        
        # 尝试获取真实的系统信息
        try:
            import psutil
            health_status['cpu_usage'] = psutil.cpu_percent()
            health_status['memory_usage'] = psutil.virtual_memory().percent
            health_status['disk_usage'] = psutil.disk_usage('/').percent
        except ImportError:
            rospy.logwarn("未安装psutil，使用模拟数据")
        
        health_json = json.dumps(health_status)
        rospy.loginfo(f"系统健康状态: {health_json}")
        
        return TriggerResponse(
            success=True,
            message=health_json
        )
    
    def handle_robot_stop(self, req):
        """
        处理机器人停止服务 (Empty类型)
        """
        rospy.loginfo("收到机器人停止请求")
        
        # Empty服务不需要返回值，但需要返回EmptyResponse
        return EmptyResponse()
    
    def run(self):
        """运行节点"""
        rospy.loginfo("ROS服务端节点正在运行...")
        rospy.loginfo("已注册的服务:")
        
        # 列出所有服务
        try:
            services = rospy.get_service_list()
            for service in sorted(services):
                rospy.loginfo(f"  📌 {service}")
        except:
            # 如果get_service_list不可用，使用rosservice命令
            import subprocess
            result = subprocess.run(['rosservice', 'list'], 
                                   capture_output=True, text=True)
            for line in result.stdout.split('\n'):
                if line.strip():
                    rospy.loginfo(f"  📌 {line}")
        
        rospy.loginfo("=" * 50)
        rospy.loginfo("等待请求...")
        rospy.spin()


def main():
    """主函数"""
    try:
        node = RosServiceNode()
        node.run()
    except rospy.ROSInterruptException:
        rospy.loginfo("ROS服务端节点已关闭")
    except Exception as e:
        rospy.logerr(f"发生错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()