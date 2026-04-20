package com.hospital.entity;

import lombok.Data;
import java.time.LocalDate;

@Data
public class Patient {
    private Long id;
    private Long userId;
    private String idCard;
    private String gender; // MALE, FEMALE
    private LocalDate birthday;
    private String address;
    private String emergencyContact;
    private String emergencyPhone;
    private String patientType; // OUTPATIENT, INPATIENT
    private String roomNumber;
    
    private Integer age; // 新增年龄字段，支持 setAge/getAge
    // 关联用户信息
    private User user;
}

