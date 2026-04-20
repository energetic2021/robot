package com.hospital.config;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.PropertySource;

/**
 * ROS连接配置类
 * 从application.properties读取配置参数
 */
@Configuration
@PropertySource("classpath:application.properties")
public class RosConfig {
    
    @Value("${ros.bridge.host:127.0.0.1}")
    private String host;
    
    @Value("${ros.bridge.port:9090}")
    private int port;
    
    @Value("${ros.bridge.connection-timeout:5000}")
    private int connectionTimeout;
    
    @Value("${ros.bridge.reconnect-interval:5000}")
    private int reconnectInterval;
    
    @Value("${ros.bridge.service-timeout:5000}")
    private int serviceTimeout;

    // 可选：ROS 健康检查用来调用的“真实服务”
    // 为空时，不发起任何 ROS service 调用，仅基于 rosbridge 连接状态展示健康信息
    @Value("${ros.bridge.health-service-name:}")
    private String healthServiceName;

    @Value("${ros.bridge.health-service-type:}")
    private String healthServiceType;

    @Value("${ros.bridge.health-service-timeout:2000}")
    private int healthServiceTimeout;
    
    @Value("${ros.cmd-vel-topic:/mobile_base_controller/cmd_vel}")
    private String cmdVelTopic;
    
    @Value("${ros.navi-waypoint-topic:/waterplus/navi_waypoint}")
    private String naviWaypointTopic;

    @Value("${ros.transport-goal-topic:/hospital_transport/goal}")
    private String transportGoalTopic;

    @Value("${ros.transport-cancel-topic:/hospital_transport/cancel}")
    private String transportCancelTopic;
    
    // Getter方法
    public String getHost() { return host; }
    public int getPort() { return port; }
    public int getConnectionTimeout() { return connectionTimeout; }
    public int getReconnectInterval() { return reconnectInterval; }
    public int getServiceTimeout() { return serviceTimeout; }

    public String getHealthServiceName() { return healthServiceName; }
    public String getHealthServiceType() { return healthServiceType; }
    public int getHealthServiceTimeout() { return healthServiceTimeout; }
    public String getCmdVelTopic() { return cmdVelTopic; }
    public String getNaviWaypointTopic() { return naviWaypointTopic; }
    public String getTransportGoalTopic() { return transportGoalTopic; }
    public String getTransportCancelTopic() { return transportCancelTopic; }
    
    // 获取完整的WebSocket URL
    public String getWebSocketUrl() {
        return String.format("ws://%s:%d", host, port);
    }
    
    @Override
    public String toString() {
        return String.format("RosConfig{url='%s', timeout=%d, reconnect=%d}", 
                getWebSocketUrl(), connectionTimeout, reconnectInterval);
    }
}