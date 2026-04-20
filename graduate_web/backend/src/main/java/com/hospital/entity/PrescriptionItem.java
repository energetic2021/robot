package com.hospital.entity;

import lombok.Data;
import java.math.BigDecimal;

@Data
public class PrescriptionItem {
    private Long id;
    private Long prescriptionId;
    private Long medicineId;
    private Integer quantity;
    private String dosage;
    private String frequency;
    private BigDecimal unitPrice;
    private BigDecimal totalPrice;
    
    // 关联信息
    private Medicine medicine;
}

