package com.hospital.controller;

import com.hospital.service.RosService;
import org.json.JSONObject;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;
import org.springframework.http.ResponseEntity;

/**
 * 机器人控制控制器
 * 暴露 RESTful 接口供前端调用，实现对机器人的远程监控和指令下发。
 */
@RestController
@RequestMapping("/robot")
public class RobotController {
    
    @Autowired
    private RosService rosService;
    
    /**
     * 获取 ROS 系统整体健康状态
     * 包括 WebSocket 连接状态和核心 Service 的响应情况
     */
    @GetMapping("/health")
    public ResponseEntity<String> health() {
        try {
            boolean healthy = rosService.checkServiceHealth();
            String info = rosService.getConnectionInfo();
            if (healthy) {
                return ResponseEntity.ok("✅ ROS 服务运行正常\n" + info);
            } else {
                return ResponseEntity.ok("❌ ROS 服务连接异常\n" + info);
            }
        } catch (Exception e) {
            return ResponseEntity.ok("❌ 系统错误: " + e.getMessage());
        }
    }
    
    /**
     * 获取机器人当前实时位姿（AMCL 坐标）
     */
    @GetMapping("/status")
    public ResponseEntity<String> getStatus() {
        try {
            JSONObject result = rosService.getRobotStatus();
            return ResponseEntity.ok(result.toString(2));
        } catch (Exception e) {
            return ResponseEntity.ok("{\"error\":\"" + e.getMessage() + "\"}");
        }
    }
    
    /**
     * 获取传感器原始数据（激光雷达扫描）
     */
    @GetMapping("/sensor")
    public ResponseEntity<String> getSensorData() {
        try {
            JSONObject result = rosService.getSensorData();
            return ResponseEntity.ok(result.toString(2));
        } catch (Exception e) {
            return ResponseEntity.ok("{\"error\":\"" + e.getMessage() + "\"}");
        }
    }
    
    /**
     * 加法服务测试接口（调用 ROS 端的 RPC 服务）
     */
    @GetMapping("/add")
    public ResponseEntity<String> add(@RequestParam int a, @RequestParam int b) {
        try {
            JSONObject result = rosService.addTwoInts(a, b);
            JSONObject response = new JSONObject();
            response.put("success", true);
            response.put("a", a);
            response.put("b", b);
            response.put("sum", result.getLong("sum"));
            return ResponseEntity.ok(response.toString(2));
        } catch (Exception e) {
            JSONObject error = new JSONObject();
            error.put("success", false);
            error.put("error", e.getMessage());
            return ResponseEntity.ok(error.toString(2));
        }
    }
    
    /**
     * 手动遥控：发布移动指令
     * @param direction 方向 (forward/backward/left/right)
     * @param speed 线速度或角速度
     */
    @PostMapping("/move")
    public ResponseEntity<String> move(@RequestParam String direction, @RequestParam(defaultValue="0.5") double speed) {
        try {
            String result = rosService.moveRobot(direction, speed);
            JSONObject response = new JSONObject();
            response.put("success", true);
            response.put("message", result);
            return ResponseEntity.ok(response.toString(2));
        } catch (Exception e) {
            JSONObject error = new JSONObject();
            error.put("success", false);
            error.put("error", e.getMessage());
            return ResponseEntity.ok(error.toString(2));
        }
    }
    
    /**
     * 紧急停止指令
     */
    @PostMapping("/stop")
    public ResponseEntity<String> stop() {
        try {
            String result = rosService.stopRobot();
            JSONObject response = new JSONObject();
            response.put("success", true);
            response.put("message", result);
            return ResponseEntity.ok(response.toString(2));
        } catch (Exception e) {
            JSONObject error = new JSONObject();
            error.put("success", false);
            error.put("error", e.getMessage());
            return ResponseEntity.ok(error.toString(2));
        }
    }
    
    /**
     * 多点导航：发布目标航点
     * @param waypoint 航点标识（如 1, 2, 3 或 A, B, C）
     */
    @PostMapping("/navigate")
    public ResponseEntity<String> navigate(@RequestParam String waypoint) {
        try {
            String result = rosService.publishNaviWaypoint(waypoint);
            JSONObject response = new JSONObject();
            response.put("success", true);
            response.put("message", result);
            return ResponseEntity.ok(response.toString(2));
        } catch (IllegalArgumentException e) {
            JSONObject error = new JSONObject();
            error.put("success", false);
            error.put("error", e.getMessage());
            return ResponseEntity.badRequest().body(error.toString(2));
        } catch (Exception e) {
            JSONObject error = new JSONObject();
            error.put("success", false);
            error.put("error", e.getMessage());
            return ResponseEntity.ok(error.toString(2));
        }
    }

    /**
     * 启动自动化取物送物任务（Action 模式）
     * @param pickupPoint 起点
     * @param deliveryPoint 终点
     */
    @PostMapping("/transport/start")
    public ResponseEntity<String> startTransport(@RequestParam String pickupPoint,
                                                 @RequestParam String deliveryPoint) {
        try {
            String result = rosService.startTransportTask(pickupPoint, deliveryPoint);
            return jsonSuccess(result);
        } catch (IllegalArgumentException e) {
            return jsonError(e.getMessage());
        } catch (Exception e) {
            return jsonError(e.getMessage());
        }
    }

    /**
     * 取消当前所有运输任务
     */
    @PostMapping("/transport/cancel")
    public ResponseEntity<String> cancelTransport() {
        try {
            String result = rosService.cancelAllTransportTasks();
            return jsonSuccess(result);
        } catch (Exception e) {
            return jsonError(e.getMessage());
        }
    }

    /** 
     * 获取任务队列实时状态
     */
    @GetMapping("/transport/status")
    public ResponseEntity<String> getTransportStatus() {
        try {
            JSONObject status = rosService.getLatestTransportStatus();
            if (status == null) {
                return jsonError("暂无任务队列状态信息");
            }
            return ResponseEntity.ok(status.toString(2));
        } catch (Exception e) {
            return jsonError(e.getMessage());
        }
    }

    /** 
     * 获取任务执行过程中的实时反馈（位置进度等）
     */
    @GetMapping("/transport/feedback")
    public ResponseEntity<String> getTransportFeedback() {
        try {
            JSONObject fb = rosService.getLatestTransportFeedback();
            if (fb == null) {
                return jsonError("暂无任务实时反馈");
            }
            return ResponseEntity.ok(fb.toString(2));
        } catch (Exception e) {
            return jsonError(e.getMessage());
        }
    }

    /** 
     * 获取最近一次任务的最终执行结果
     */
    @GetMapping("/transport/result")
    public ResponseEntity<String> getTransportResult() {
        try {
            JSONObject rs = rosService.getLatestTransportResult();
            if (rs == null) {
                return jsonError("任务尚未完成，无结果记录");
            }
            return ResponseEntity.ok(rs.toString(2));
        } catch (Exception e) {
            return jsonError(e.getMessage());
        }
    }

    /**
     * 获取最新地图数据（栅格占用地图）
     */
    @GetMapping("/map")
    public ResponseEntity<String> getMap() {
        try {
            JSONObject map = rosService.getLatestMap();
            return ResponseEntity.ok(map.toString(2));
        } catch (Exception e) {
            return ResponseEntity.ok("{\"error\":\"" + e.getMessage() + "\"}");
        }
    }

    /** 
     * 抽屉控制接口：一键打开所有抽屉
     */
    @PostMapping("/drawers/open-all")
    public ResponseEntity<String> drawersOpenAll() {
        try {
            String result = rosService.callDrawerService("/drawers/open_all");
            return jsonSuccess(result);
        } catch (Exception e) {
            return jsonError(e.getMessage());
        }
    }

    /** 
     * 抽屉控制接口：一键关闭所有抽屉
     */
    @PostMapping("/drawers/close-all")
    public ResponseEntity<String> drawersCloseAll() {
        try {
            String result = rosService.callDrawerService("/drawers/close_all");
            return jsonSuccess(result);
        } catch (Exception e) {
            return jsonError(e.getMessage());
        }
    }

    /** 
     * 抽屉控制接口：打开指定的 1/2/3 号抽屉
     */
    @PostMapping("/drawers/open/1")
    public ResponseEntity<String> drawerOpen1() {
        try {
            String result = rosService.callDrawerService("/drawers/open_drawer1");
            return jsonSuccess(result);
        } catch (Exception e) {
            return jsonError(e.getMessage());
        }
    }

    @PostMapping("/drawers/open/2")
    public ResponseEntity<String> drawerOpen2() {
        try {
            String result = rosService.callDrawerService("/drawers/open_drawer2");
            return jsonSuccess(result);
        } catch (Exception e) {
            return jsonError(e.getMessage());
        }
    }

    @PostMapping("/drawers/open/3")
    public ResponseEntity<String> drawerOpen3() {
        try {
            String result = rosService.callDrawerService("/drawers/open_drawer3");
            return jsonSuccess(result);
        } catch (Exception e) {
            return jsonError(e.getMessage());
        }
    }

    private static ResponseEntity<String> jsonSuccess(String message) {
        JSONObject o = new JSONObject();
        o.put("success", true);
        o.put("message", message);
        return ResponseEntity.ok(o.toString(2));
    }

    private static ResponseEntity<String> jsonError(String error) {
        JSONObject o = new JSONObject();
        o.put("success", false);
        o.put("error", error);
        return ResponseEntity.ok(o.toString(2));
    }
}
