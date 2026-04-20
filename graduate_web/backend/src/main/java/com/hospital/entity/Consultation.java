package com.hospital.entity;

import lombok.Data;
import java.time.LocalDateTime;

@Data
public class Consultation {
    private Long id;
    private Long patientId;
    private Long requestDoctorId;
    private Long targetDoctorId;
    private Long medicalRecordId;
    private String consultationType; // INPATIENT, OUTPATIENT
    private String consultationReason;
    private LocalDateTime consultationDate;
    private String status; // PENDING, ACCEPTED, COMPLETED, CANCELLED
    private String consultationResult;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
    
    // 关联信息
    private User patient;
    private User requestDoctor;
    private User targetDoctor;
    private MedicalRecord medicalRecord;
}

