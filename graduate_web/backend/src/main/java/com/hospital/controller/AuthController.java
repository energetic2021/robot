package com.hospital.controller;

import com.hospital.common.Result;
import com.hospital.entity.User;
import com.hospital.service.AuthService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

@RestController
@RequestMapping("/auth")
@CrossOrigin
public class AuthController {
    
    @Autowired
    private AuthService authService;
    
    @PostMapping("/login")
    public Result<Map<String, Object>> login(@RequestBody Map<String, String> params) {
        try {
            String username = params.get("username");
            String password = params.get("password");
            Map<String, Object> result = authService.login(username, password);
            return Result.success(result);
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }
    
    @PostMapping("/register")
    public Result<User> register(@RequestBody User user) {
        try {
            User newUser = authService.register(user);
            return Result.success("注册成功", newUser);
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }
}

