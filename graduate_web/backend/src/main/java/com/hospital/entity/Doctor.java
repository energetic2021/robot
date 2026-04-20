package com.hospital.entity;

import lombok.Data;

@Data
public class Doctor {
    private Long id;
    private Long userId;
    private String department;
    private String title;
    private String specialty;
    private String licenseNumber;
    private String introduction;
    
    // 关联用户信息
    private User user;
}

