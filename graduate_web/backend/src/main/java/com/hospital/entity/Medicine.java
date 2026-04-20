package com.hospital.entity;

import lombok.Data;
import java.math.BigDecimal;
import java.time.LocalDateTime;

@Data
public class Medicine {
    private Long id;
    private String name;
    private String code;
    private String specification;
    private String unit;
    private BigDecimal price;
    private Integer stock;
    private String manufacturer;
    private String description;
    private Integer status; // 1-可用, 0-停用
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
}

