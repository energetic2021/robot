package com.hospital.entity;

import lombok.Data;
import java.time.LocalDateTime;

@Data
public class User {
    private Long id;
    private String username;
    private String password;
    private String realName;
    private String phone;
    private String email;
    private String role; // SUPER_ADMIN, DOCTOR, PATIENT
    private Integer status; // 1-启用, 0-禁用
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
}

