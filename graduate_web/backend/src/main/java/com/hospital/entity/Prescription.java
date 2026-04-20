package com.hospital.entity;

import lombok.Data;
import java.math.BigDecimal;
import java.time.LocalDateTime;
import java.util.List;

@Data
public class Prescription {
    private Long id;
    private Long medicalRecordId;
    private Long patientId;
    private Long doctorId;
    private LocalDateTime prescriptionDate;
    private BigDecimal totalAmount;
    private String status; // PENDING, DISPENSED, CANCELLED
    private String remarks;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
    
    // 关联信息
    private List<PrescriptionItem> items;
    private User patient;
    private User doctor;
    private MedicalRecord medicalRecord;
}

