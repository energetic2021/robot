package com.hospital.service;

import com.hospital.common.JwtUtil;
import com.hospital.entity.User;
import com.hospital.mapper.UserMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;

import java.util.HashMap;
import java.util.Map;

@Service
public class AuthService {
    
    @Autowired
    private UserMapper userMapper;
    
    @Autowired
    private PasswordEncoder passwordEncoder;
    
    @Autowired
    private JwtUtil jwtUtil;
    
    @Autowired
    private UserService userService;
    
    public Map<String, Object> login(String username, String password) {
        User user = userMapper.findByUsername(username);
        if (user == null) {
            throw new RuntimeException("用户名或密码错误");
        }
        
        // 检查密码hash格式
        String storedPassword = user.getPassword();
        if (storedPassword == null || storedPassword.length() != 60) {
            throw new RuntimeException("密码格式错误，请联系管理员");
        }
        
        // 验证密码
        try {
            if (!passwordEncoder.matches(password, storedPassword)) {
                throw new RuntimeException("用户名或密码错误");
            }
        } catch (Exception e) {
            throw new RuntimeException("密码验证失败：" + e.getMessage());
        }
        
        if (user.getStatus() == 0) {
            throw new RuntimeException("账号已被禁用");
        }
        
        String token = jwtUtil.generateToken(user.getUsername(), user.getRole(), user.getId());
        
        Map<String, Object> result = new HashMap<>();
        result.put("token", token);
        result.put("user", user);
        return result;
    }
    
    public User register(User user) {
        user.setRole("PATIENT"); // 默认注册为患者
        return userService.create(user, null);
    }
}

