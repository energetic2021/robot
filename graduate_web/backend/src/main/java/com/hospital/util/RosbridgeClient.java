package com.hospital.util;

import com.hospital.config.RosConfig;
import org.java_websocket.client.WebSocketClient;
import org.java_websocket.handshake.ServerHandshake;
import org.json.JSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;
import java.net.URI;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.BiConsumer;

/**
 * rosbridge WebSocket 客户端
 * 负责与 ROS 系统（通过 rosbridge_suite）建立和维护 WebSocket 连接。
 * 该类实现了标准的 rosbridge 协议（v2.0），支持服务调用、话题发布和话题订阅。
 */
@Component
public class RosbridgeClient extends WebSocketClient {
    
    private static final Logger logger = LoggerFactory.getLogger(RosbridgeClient.class);
    
    /**
     * 存储等待响应的服务调用。
     * Key 为 call_id，Value 为用于异步获取结果的 CompletableFuture。
     */
    private final Map<String, CompletableFuture<JSONObject>> pendingCalls = new ConcurrentHashMap<>();

    /**
     * 话题监听器容器。
     * Key 为话题名称（Topic），Value 为消息处理回调（BiConsumer）。
     */
    private final Map<String, BiConsumer<String, JSONObject>> topicListeners = new ConcurrentHashMap<>();
    
    /**
     * 外部连接状态监听器，用于通知其他 Service 连接已建立或断开。
     */
    private ConnectionListener connectionListener;
    
    /**
     * ROS 配置对象，包含 WebSocket URL、重连间隔等信息。
     */
    private final RosConfig rosConfig;
    
    /**
     * 内部连接状态标识。
     */
    private boolean connected = false;
    
    @Autowired
    public RosbridgeClient(RosConfig rosConfig) {
        super(URI.create(rosConfig.getWebSocketUrl()));
        this.rosConfig = rosConfig;
        logger.info("初始化 RosbridgeClient, 连接地址: {}", rosConfig.getWebSocketUrl());
    }
    
    @PostConstruct
    public void init() {
        // 容器启动时自动发起连接
        connect();
    }
    
    /**
     * WebSocket 连接成功回调
     */
    @Override
    public void onOpen(ServerHandshake handshakedata) {
        connected = true;
        logger.info("✅ 已连接到 rosbridge 服务器: {}", uri);
        if (connectionListener != null) {
            connectionListener.onConnected();
        }
    }
    
    /**
     * 收到 WebSocket 消息回调
     * 根据 rosbridge 协议的 "op" 字段分发处理逻辑
     */
    @Override
    public void onMessage(String message) {
        try {
            JSONObject response = new JSONObject(message);
            String op = response.optString("op");
            
            // 情况 A：处理服务调用（Service Call）的响应结果
            if ("service_response".equals(op)) {
                String id = response.optString("id");
                CompletableFuture<JSONObject> future = pendingCalls.remove(id);
                if (future != null) {
                    future.complete(response);
                    logger.debug("收到服务响应, ID: {}, 结果: {}", id, response.getBoolean("result"));
                }
            }
            // 情况 B：处理订阅的话题（Topic Publish）消息推送
            else if ("publish".equals(op)) {
                String topic = response.optString("topic");
                JSONObject msg = response.optJSONObject("msg");
                logger.debug("收到话题消息 - 主题: {}, 内容: {}", topic, msg);
                
                // 查找并触发该话题注册的本地监听器
                BiConsumer<String, JSONObject> listener = topicListeners.get(topic);
                if (listener != null && msg != null) {
                    try {
                        listener.accept(topic, msg);
                    } catch (Exception ex) {
                        logger.error("处理话题 {} 消息回调出错: {}", topic, ex.getMessage());
                    }
                }
            }
        } catch (Exception e) {
            logger.error("处理消息时出错: {}", e.getMessage());
        }
    }
    
    /**
     * 连接关闭回调，包含异常断开
     */
    @Override
    public void onClose(int code, String reason, boolean remote) {
        connected = false;
        logger.warn("❌ ROS 连接关闭: {}, code: {}, remote: {}", reason, code, remote);
        
        if (connectionListener != null) {
            connectionListener.onDisconnected();
        }
        
        // 自动触发重连调度
        scheduleReconnect();
    }
    
    @Override
    public void onError(Exception ex) {
        logger.error("⚠️ WebSocket 错误: {}", ex.getMessage());
    }
    
    /**
     * 调度自动重连线程
     */
    private void scheduleReconnect() {
        new Thread(() -> {
            try {
                logger.info("{}ms 后尝试重连 ROS...", rosConfig.getReconnectInterval());
                Thread.sleep(rosConfig.getReconnectInterval());
                if (!isOpen()) {
                    reconnect(); // 继承自 WebSocketClient 的重连方法
                }
            } catch (InterruptedException e) {
                logger.error("重连被中断");
            }
        }).start();
    }
    
    /**
     * 调用 ROS 服务（RPC 模式）
     * @param serviceName 服务名称，如 "/add_two_ints"
     * @param serviceType 服务类型，如 "roscpp_tutorials/TwoInts"
     * @param args 服务参数
     * @return 异步结果 CompletableFuture
     */
    public CompletableFuture<JSONObject> callService(String serviceName, String serviceType, JSONObject args) {
        // 生成唯一调用 ID，用于在 onMessage 中匹配响应
        String callId = "call_" + System.currentTimeMillis() + "_" + (int)(Math.random() * 1000);
        CompletableFuture<JSONObject> future = new CompletableFuture<>();
        pendingCalls.put(callId, future);
        
        // 构造 rosbridge 协议消息
        JSONObject callMessage = new JSONObject();
        callMessage.put("op", "call_service");
        callMessage.put("service", serviceName);
        callMessage.put("type", serviceType);
        callMessage.put("id", callId);
        callMessage.put("args", args);
        
        send(callMessage.toString());
        logger.info("📤 服务调用已发送: {}, ID: {}", serviceName, callId);
        logger.debug("服务参数: {}", args);
        
        return future;
    }
    
    /**
     * 发布话题消息（Pub 模式）
     * @param topic 话题名称
     * @param messageType 消息类型，如 "geometry_msgs/Twist"
     * @param msg 消息内容
     */
    public void publish(String topic, String messageType, JSONObject msg) {
        JSONObject publishMsg = new JSONObject();
        publishMsg.put("op", "publish");
        publishMsg.put("topic", topic);
        publishMsg.put("type", messageType);
        publishMsg.put("msg", msg);
        send(publishMsg.toString());
        logger.debug("📤 已发布到主题 {}: {}", topic, msg);
    }
    
    /**
     * 订阅话题（Sub 模式）
     * 仅发送订阅指令给 ROS，实际消息接收在 onMessage 中处理
     */
    public void subscribe(String topic, String messageType) {
        JSONObject subscribeMsg = new JSONObject();
        subscribeMsg.put("op", "subscribe");
        subscribeMsg.put("topic", topic);
        subscribeMsg.put("type", messageType);
        send(subscribeMsg.toString());
        logger.info("📥 已向 ROS 订阅主题: {}", topic);
    }

    /**
     * 注册本地话题消息监听器
     * 当收到已订阅话题的消息时，会调用此监听器
     */
    public void addTopicListener(String topic, BiConsumer<String, JSONObject> listener) {
        if (listener != null) {
            topicListeners.put(topic, listener);
            logger.info("已注册本地话题监听器: {}", topic);
        }
    }
    
    /**
     * 取消订阅话题
     */
    public void unsubscribe(String topic) {
        JSONObject unsubscribeMsg = new JSONObject();
        unsubscribeMsg.put("op", "unsubscribe");
        unsubscribeMsg.put("topic", topic);
        send(unsubscribeMsg.toString());
        logger.info("📤 已取消订阅主题: {}", topic);
    }
    
    /**
     * 设置连接状态回调，由业务 Service 调用
     */
    public void setConnectionListener(ConnectionListener listener) {
        this.connectionListener = listener;
    }
    
    public boolean isConnected() {
        return connected && isOpen();
    }
    
    @PreDestroy
    public void destroy() {
        logger.info("正在关闭 RosbridgeClient 连接...");
        close();
    }
    
    /**
     * 连接状态监听器接口
     */
    public interface ConnectionListener {
        /** 连接建立时触发 */
        void onConnected();
        /** 连接断开时触发 */
        void onDisconnected();
    }
}
