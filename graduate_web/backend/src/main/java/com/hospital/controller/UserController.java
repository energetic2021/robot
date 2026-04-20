package com.hospital.controller;

import com.hospital.common.Result;
import com.hospital.entity.User;
import com.hospital.service.UserService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/user")
@CrossOrigin
public class UserController {
    
    @Autowired
    private UserService userService;
    
    @GetMapping("/list")
    public Result<List<User>> list(@RequestParam(required = false) String role,
                                    @RequestParam(required = false) Integer status) {
        List<User> users = userService.findAll(role, status);
        return Result.success(users);
    }
    
    @GetMapping("/{id}")
    public Result<User> getById(@PathVariable Long id) {
        User user = userService.findById(id);
        return Result.success(user);
    }
    
    @PostMapping
    public Result<User> create(@RequestBody Map<String, Object> params) {
        try {
            User user = new User();
            user.setUsername((String) params.get("username"));
            user.setPassword((String) params.get("password"));
            user.setRealName((String) params.get("realName"));
            user.setPhone((String) params.get("phone"));
            user.setEmail((String) params.get("email"));
            user.setRole((String) params.get("role"));
            user.setStatus((Integer) params.get("status"));
            
            User newUser = userService.create(user, params);
            return Result.success("创建成功", newUser);
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }
    
    @PutMapping("/{id}")
    public Result<User> update(@PathVariable Long id, @RequestBody User user) {
        user.setId(id);
        User updatedUser = userService.update(user);
        return Result.success("更新成功", updatedUser);
    }
    
    @DeleteMapping("/{id}")
    public Result<Void> delete(@PathVariable Long id) {
        userService.delete(id);
        return Result.success("删除成功", null);
    }
    
    @PutMapping("/{id}/password")
    public Result<Void> updatePassword(@PathVariable Long id, @RequestBody Map<String, String> params) {
        String newPassword = params.get("newPassword");
        userService.updatePassword(id, newPassword);
        return Result.success("密码修改成功", null);
    }
}

