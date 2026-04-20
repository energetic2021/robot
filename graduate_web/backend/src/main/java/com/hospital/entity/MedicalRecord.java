package com.hospital.entity;

import lombok.Data;
import java.time.LocalDateTime;

@Data
public class MedicalRecord {
    private Long id;
    private Long patientId;
    private Long doctorId;
    private LocalDateTime visitDate;
    private String chiefComplaint;
    private String presentIllness;
    private String pastHistory;
    private String physicalExam;
    private String diagnosis;
    private String treatmentPlan;
    private String remarks;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
    
    // 关联信息
    private User patient;
    private User doctor;
}

