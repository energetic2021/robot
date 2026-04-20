package com.hospital.service;

import com.hospital.config.RosConfig;
import com.hospital.util.RosbridgeClient;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import javax.annotation.PostConstruct;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

/**
 * ROS 业务逻辑服务类
 * 该类是系统与机器人交互的核心“大脑”，负责：
 * 1. 订阅和缓存机器人的各种状态数据（位置、传感器、地图、任务进度）。
 * 2. 封装高层的控制指令（移动、导航、取物送物、抽屉控制）。
 * 3. 维护 ROS 连接的就绪状态。
 */
@Service
public class RosService {
    
    private static final Logger logger = LoggerFactory.getLogger(RosService.class);
    
    @Autowired
    private RosbridgeClient rosClient;
    
    @Autowired
    private RosConfig rosConfig;
    
    /** 标识 ROS 服务是否完全就绪（连接成功且完成初始订阅） */
    private boolean ready = false;

    /** 
     * 取物送物任务（Action）的最新状态、反馈和结果缓存。
     * 使用 volatile 确保多线程环境下的可见性。
     */
    private volatile JSONObject latestTransportStatus;
    private volatile JSONObject latestTransportFeedback;
    private volatile JSONObject latestTransportResult;

    /** 机器人实时位姿（来自 /amcl_pose） */
    private volatile JSONObject latestRobotStatusMsg;
    /** 激光雷达扫描数据（来自 /scan） */
    private volatile JSONObject latestSensorMsg;
    /** 栅格地图数据（来自 /map） */
    private volatile JSONObject latestMapMsg;

    /**
     * 组件初始化：设置连接监听器并订阅核心话题
     */
    @PostConstruct
    public void init() {
        rosClient.setConnectionListener(new RosbridgeClient.ConnectionListener() {
            @Override
            public void onConnected() {
                ready = true;
                logger.info("🎉 ROS 服务已就绪，开始自动订阅核心业务话题...");
                try {
                    subscribeTransportTopics(); // 订阅运输 Action 话题
                    subscribeRobotStatusTopics(); // 订阅位姿和传感器话题
                    subscribeMapTopic(); // 订阅地图话题
                } catch (Exception e) {
                    logger.warn("订阅 ROS 话题失败: {}", e.getMessage());
                }
            }
            
            @Override
            public void onDisconnected() {
                ready = false;
                logger.warn("😢 ROS 服务连接已断开");
            }
        });

        // 容错处理：如果连接已建立但监听器未触发（如重连成功），则手动触发订阅
        if (rosClient.isConnected()) {
            try {
                subscribeTransportTopics();
                subscribeRobotStatusTopics();
                subscribeMapTopic();
            } catch (Exception e) {
                logger.warn("初始化订阅失败: {}", e.getMessage());
            }
        }
    }

    /**
     * 订阅取物送物 Action 相关的三个话题：状态、反馈、结果
     */
    private void subscribeTransportTopics() {
        String statusTopic = "/hospital_transport/status";
        String feedbackTopic = "/hospital_transport/feedback";
        String resultTopic = "/hospital_transport/result";
        
        // 向 ROS 发送订阅请求
        rosClient.subscribe(statusTopic, "actionlib_msgs/GoalStatusArray");
        rosClient.subscribe(feedbackTopic, "hospital_transport/TransportActionFeedback");
        rosClient.subscribe(resultTopic, "hospital_transport/TransportActionResult");

        // 注册本地回调：每当话题有新消息，更新内存缓存
        rosClient.addTopicListener(statusTopic, (topic, msg) -> {
            latestTransportStatus = msg;
            logger.debug("更新任务队列状态: {}", msg);
        });
        rosClient.addTopicListener(feedbackTopic, (topic, msg) -> {
            latestTransportFeedback = msg;
            logger.debug("更新任务实时反馈: {}", msg);
        });
        rosClient.addTopicListener(resultTopic, (topic, msg) -> {
            latestTransportResult = msg;
            logger.debug("更新任务最终结果: {}", msg);
        });
    }

    /**
     * 订阅机器人基础状态：位姿（AMCL）和传感器（激光雷达）
     */
    private void subscribeRobotStatusTopics() {
        String robotStatusTopic = "/amcl_pose"; 
        String sensorTopic = "/scan"; 
        
        rosClient.subscribe(robotStatusTopic, "geometry_msgs/PoseWithCovarianceStamped");
        rosClient.subscribe(sensorTopic, "sensor_msgs/LaserScan");
        
        rosClient.addTopicListener(robotStatusTopic, (topic, msg) -> {
            latestRobotStatusMsg = msg;
            logger.debug("更新机器人实时位姿: {}", msg);
        });
        rosClient.addTopicListener(sensorTopic, (topic, msg) -> {
            latestSensorMsg = msg;
            logger.debug("更新激光雷达数据: {}", msg);
        });
    }
    
    /**
     * 订阅地图话题
     */
    private void subscribeMapTopic() {
        String mapTopic = "/map";
        rosClient.subscribe(mapTopic, "nav_msgs/OccupancyGrid");
        rosClient.addTopicListener(mapTopic, (topic, msg) -> {
            latestMapMsg = msg;
            logger.debug("更新地图数据 (Map size: {} bytes)", msg.toString().length());
        });
    }

    /**
     * 获取缓存中的最新地图数据（供前端 Canvas 渲染）
     */
    public JSONObject getLatestMap() {
        if (!isReady()) throw new RuntimeException("ROS 服务未连接");
        if (latestMapMsg == null) throw new RuntimeException("暂无地图数据，请检查 ROS 地图服务是否开启");
        return latestMapMsg;
    }

    /**
     * 调用加法服务（演示用的标准 Service 调用例子）
     */
    public JSONObject addTwoInts(int a, int b) throws Exception {
        if (!isReady()) throw new RuntimeException("ROS 服务未连接");
        
        JSONObject args = new JSONObject();
        args.put("a", a);
        args.put("b", b);
        
        CompletableFuture<JSONObject> future = rosClient.callService(
            "/add_two_ints",           
            "roscpp_tutorials/TwoInts", 
            args
        );
        
        // 阻塞等待响应结果
        JSONObject response = future.get(rosConfig.getServiceTimeout(), TimeUnit.MILLISECONDS);
        
        if (response.getBoolean("result")) {
            return response.getJSONObject("values");
        } else {
            throw new RuntimeException("ROS 服务端处理加法失败");
        }
    }

    /**
     * 获取最新的机器人位姿（坐标和朝向）
     */
    public JSONObject getRobotStatus() {
        if (!isReady()) throw new RuntimeException("ROS 服务未连接");
        if (latestRobotStatusMsg == null) throw new RuntimeException("暂无机器人状态数据");
        return latestRobotStatusMsg;
    }
    
    /**
     * 获取最新的激光雷达扫描数据
     */
    public JSONObject getSensorData() {
        if (!isReady()) throw new RuntimeException("ROS 服务未连接");
        if (latestSensorMsg == null) throw new RuntimeException("暂无传感器数据");
        return latestSensorMsg;
    }
    
    /**
     * 控制机器人手动移动
     * @param direction 方向标识 (forward, backward, left, right)
     * @param speed 移动速度 (0.1 - 1.0)
     * @return 执行描述
     */
    public String moveRobot(String direction, double speed) throws Exception {
        if (!isReady()) throw new RuntimeException("ROS 服务未连接");
        
        String topic = rosConfig.getCmdVelTopic(); // 默认为 /mobile_base_controller/cmd_vel
        JSONObject twist = buildTwist(direction, speed); // 构造 Twist 消息
        rosClient.publish(topic, "geometry_msgs/Twist", twist);
        logger.info("📤 发送移动指令: 方向={}, 速度={}", direction, speed);
        return "已发送移动指令";
    }
    
    /**
     * 紧急停止机器人
     */
    public String stopRobot() throws Exception {
        if (!isReady()) throw new RuntimeException("ROS 服务未连接");
        
        String topic = rosConfig.getCmdVelTopic();
        JSONObject twist = buildTwist("stop", 0);
        rosClient.publish(topic, "geometry_msgs/Twist", twist);
        logger.warn("🚨 发送紧急停止指令！");
        return "机器人已停止";
    }
    
    /** 
     * 构建符合 ROS 标准的 geometry_msgs/Twist 消息结构 
     * linear.x 控制前后移动，angular.z 控制旋转
     */
    private JSONObject buildTwist(String direction, double speed) {
        JSONObject linear = new JSONObject();
        linear.put("x", 0.0); linear.put("y", 0.0); linear.put("z", 0.0);
        JSONObject angular = new JSONObject();
        angular.put("x", 0.0); angular.put("y", 0.0); angular.put("z", 0.0);
        
        switch (direction != null ? direction.toLowerCase() : "") {
            case "forward": linear.put("x", speed); break;
            case "backward": linear.put("x", -speed); break;
            case "left": angular.put("z", speed); break;
            case "right": angular.put("z", -speed); break;
            case "stop": default: break;
        }
        
        JSONObject msg = new JSONObject();
        msg.put("linear", linear);
        msg.put("angular", angular);
        return msg;
    }
    
    /**
     * 抽屉控制：调用自定义的 ROS 服务（std_srvs/Trigger 类型）
     * 对应机器人上的抽屉执行机构控制
     */
    public String callDrawerService(String serviceName) throws Exception {
        if (!isReady()) throw new RuntimeException("ROS 服务未连接");
        
        logger.info("📤 调用抽屉控制服务: {}", serviceName);
        CompletableFuture<JSONObject> future = rosClient.callService(serviceName, "std_srvs/Trigger", new JSONObject());
        JSONObject response = future.get(rosConfig.getServiceTimeout(), TimeUnit.MILLISECONDS);
        
        boolean success = response.optBoolean("result", true);
        String message = response.optString("message", "ok");
        if (success) {
            return message;
        }
        throw new RuntimeException("抽屉操作失败: " + message);
    }

    /**
     * 多点导航：发布目标航点名称
     * 机器人端订阅此话题后会根据内置地图自动规划路径并导航
     */
    public String publishNaviWaypoint(String waypoint) throws Exception {
        if (!isReady()) throw new RuntimeException("ROS 服务未连接");
        if (waypoint == null || waypoint.trim().isEmpty()) throw new IllegalArgumentException("航点名称不能为空");
        
        String topic = rosConfig.getNaviWaypointTopic(); // 默认为 /waterplus/navi_waypoint
        JSONObject msg = new JSONObject();
        msg.put("data", waypoint.trim());
        rosClient.publish(topic, "std_msgs/String", msg);
        logger.info("📍 发送导航指令，目标航点: {}", waypoint);
        return "导航指令已发送";
    }

    /**
     * 取物送物自动化任务（Action Goal）
     * @param pickupPoint 起点名称（如 "药房"）
     * @param deliveryPoint 终点名称（如 "102房"）
     */
    public String startTransportTask(String pickupPoint, String deliveryPoint) {
        if (!isReady()) throw new RuntimeException("ROS 服务未连接");
        
        String topic = rosConfig.getTransportGoalTopic(); // /hospital_transport/goal
        JSONObject goal = new JSONObject();
        goal.put("pickup_point", pickupPoint.trim());
        goal.put("delivery_point", deliveryPoint.trim());
        
        JSONObject msg = new JSONObject();
        msg.put("goal", goal);
        
        // 发布 Action 目标，机器人将开始执行异步任务
        rosClient.publish(topic, "hospital_transport/TransportActionGoal", msg);
        logger.info("📦 启动自动化运输任务: {} -> {}", pickupPoint, deliveryPoint);
        return "运输任务已下达";
    }

    /**
     * 任务撤回：向 Action Server 发送取消指令
     */
    public String cancelAllTransportTasks() {
        if (!isReady()) throw new RuntimeException("ROS 服务未连接");
        
        String topic = rosConfig.getTransportCancelTopic(); // /hospital_transport/cancel
        rosClient.publish(topic, "actionlib_msgs/GoalID", new JSONObject());
        logger.warn("⏹️ 已向机器人发送：取消所有正在执行的任务");
        return "取消指令已发送";
    }

    // --- 缓存数据获取方法 ---

    public JSONObject getLatestTransportStatus() {
        return latestTransportStatus;
    }

    public JSONObject getLatestTransportFeedback() {
        return latestTransportFeedback;
    }

    public JSONObject getLatestTransportResult() {
        return latestTransportResult;
    }
    
    /**
     * 检查 ROS 全局服务健康状况
     */
    public boolean checkServiceHealth() {
        if (!rosClient.isConnected()) return false;

        String serviceName = rosConfig.getHealthServiceName();
        String serviceType = rosConfig.getHealthServiceType();
        if (serviceName == null || serviceName.isEmpty()) return true;

        try {
            CompletableFuture<JSONObject> future = rosClient.callService(
                serviceName, serviceType, new JSONObject());
            JSONObject response = future.get(rosConfig.getHealthServiceTimeout(), TimeUnit.MILLISECONDS);
            return response.optBoolean("result", true);
        } catch (Exception e) {
            logger.warn("ROS 健康检查异常: {}", e.getMessage());
            return false;
        }
    }
    
    public boolean isReady() {
        return ready && rosClient.isConnected();
    }
    
    public String getConnectionInfo() {
        return String.format("ROS 地址: %s, 连接: %s, 就绪: %s",
                rosConfig.getWebSocketUrl(),
                rosClient.isConnected() ? "已连接" : "断开",
                ready ? "是" : "否");
    }
    
    public void reconnect() {
        logger.info("正在执行手动重连...");
        rosClient.reconnect();
    }
}
