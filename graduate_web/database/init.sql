-- 医院管理系统数据库初始化脚本

CREATE DATABASE IF NOT EXISTS hospital_management DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE hospital_management;

-- 用户表（包含超级用户、医生、患者）
CREATE TABLE IF NOT EXISTS `user` (
    `id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `username` VARCHAR(50) NOT NULL UNIQUE COMMENT '用户名',
    `password` VARCHAR(255) NOT NULL COMMENT '密码（加密）',
    `real_name` VARCHAR(50) NOT NULL COMMENT '真实姓名',
    `phone` VARCHAR(20) COMMENT '电话',
    `email` VARCHAR(100) COMMENT '邮箱',
    `role` ENUM('SUPER_ADMIN', 'DOCTOR', 'PATIENT') NOT NULL COMMENT '角色：超级管理员、医生、患者',
    `status` TINYINT DEFAULT 1 COMMENT '状态：1-启用，0-禁用',
    `create_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    `update_time` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX `idx_username` (`username`),
    INDEX `idx_role` (`role`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户表';

-- 医生信息表
CREATE TABLE IF NOT EXISTS `doctor` (
    `id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `user_id` BIGINT NOT NULL COMMENT '用户ID',
    `department` VARCHAR(50) NOT NULL COMMENT '科室',
    `title` VARCHAR(50) COMMENT '职称',
    `specialty` VARCHAR(200) COMMENT '专业特长',
    `license_number` VARCHAR(50) COMMENT '执业证书号',
    `introduction` TEXT COMMENT '简介',
    FOREIGN KEY (`user_id`) REFERENCES `user`(`id`) ON DELETE CASCADE,
    INDEX `idx_user_id` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='医生信息表';

-- 患者信息表
CREATE TABLE IF NOT EXISTS `patient` (
    `id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `user_id` BIGINT NOT NULL COMMENT '用户ID',
    `id_card` VARCHAR(18) COMMENT '身份证号',
    `gender` ENUM('MALE', 'FEMALE') COMMENT '性别',
    `birthday` DATE COMMENT '生日',
    `address` VARCHAR(200) COMMENT '地址',
    `emergency_contact` VARCHAR(50) COMMENT '紧急联系人',
    `emergency_phone` VARCHAR(20) COMMENT '紧急联系电话',
    `patient_type` ENUM('OUTPATIENT', 'INPATIENT') DEFAULT 'OUTPATIENT' COMMENT '患者类型：门诊、住院',
    `room_number` VARCHAR(50) COMMENT '房号（住院患者使用）',
    FOREIGN KEY (`user_id`) REFERENCES `user`(`id`) ON DELETE CASCADE,
    INDEX `idx_user_id` (`user_id`),
    INDEX `idx_id_card` (`id_card`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='患者信息表';

-- 病历表
CREATE TABLE IF NOT EXISTS `medical_record` (
    `id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `patient_id` BIGINT NOT NULL COMMENT '患者ID',
    `doctor_id` BIGINT NOT NULL COMMENT '医生ID',
    `visit_date` DATETIME NOT NULL COMMENT '就诊日期',
    `chief_complaint` TEXT COMMENT '主诉',
    `present_illness` TEXT COMMENT '现病史',
    `past_history` TEXT COMMENT '既往史',
    `physical_exam` TEXT COMMENT '体格检查',
    `diagnosis` TEXT COMMENT '诊断',
    `treatment_plan` TEXT COMMENT '治疗方案',
    `remarks` TEXT COMMENT '备注',
    `create_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    `update_time` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    FOREIGN KEY (`patient_id`) REFERENCES `user`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`doctor_id`) REFERENCES `user`(`id`) ON DELETE CASCADE,
    INDEX `idx_patient_id` (`patient_id`),
    INDEX `idx_doctor_id` (`doctor_id`),
    INDEX `idx_visit_date` (`visit_date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='病历表';

-- 会诊表
CREATE TABLE IF NOT EXISTS `consultation` (
    `id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `patient_id` BIGINT NOT NULL COMMENT '患者ID',
    `request_doctor_id` BIGINT NOT NULL COMMENT '申请医生ID',
    `target_doctor_id` BIGINT NOT NULL COMMENT '会诊医生ID',
    `medical_record_id` BIGINT COMMENT '关联病历ID',
    `consultation_type` ENUM('INPATIENT', 'OUTPATIENT') NOT NULL COMMENT '会诊类型：住院、门诊',
    `consultation_reason` TEXT COMMENT '会诊原因',
    `consultation_date` DATETIME NOT NULL COMMENT '会诊日期',
    `status` ENUM('PENDING', 'ACCEPTED', 'COMPLETED', 'CANCELLED') DEFAULT 'PENDING' COMMENT '状态：待处理、已接受、已完成、已取消',
    `consultation_result` TEXT COMMENT '会诊结果',
    `create_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    `update_time` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    FOREIGN KEY (`patient_id`) REFERENCES `user`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`request_doctor_id`) REFERENCES `user`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`target_doctor_id`) REFERENCES `user`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`medical_record_id`) REFERENCES `medical_record`(`id`) ON DELETE SET NULL,
    INDEX `idx_patient_id` (`patient_id`),
    INDEX `idx_request_doctor_id` (`request_doctor_id`),
    INDEX `idx_target_doctor_id` (`target_doctor_id`),
    INDEX `idx_status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='会诊表';

-- 药品表
CREATE TABLE IF NOT EXISTS `medicine` (
    `id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `name` VARCHAR(100) NOT NULL COMMENT '药品名称',
    `code` VARCHAR(50) UNIQUE COMMENT '药品编码',
    `specification` VARCHAR(100) COMMENT '规格',
    `unit` VARCHAR(20) COMMENT '单位',
    `price` DECIMAL(10,2) NOT NULL COMMENT '价格',
    `stock` INT DEFAULT 0 COMMENT '库存',
    `manufacturer` VARCHAR(100) COMMENT '生产厂家',
    `description` TEXT COMMENT '说明',
    `status` TINYINT DEFAULT 1 COMMENT '状态：1-可用，0-停用',
    `create_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    `update_time` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX `idx_name` (`name`),
    INDEX `idx_code` (`code`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='药品表';

-- 处方表
CREATE TABLE IF NOT EXISTS `prescription` (
    `id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `medical_record_id` BIGINT NOT NULL COMMENT '病历ID',
    `patient_id` BIGINT NOT NULL COMMENT '患者ID',
    `doctor_id` BIGINT NOT NULL COMMENT '医生ID',
    `prescription_date` DATETIME NOT NULL COMMENT '开方日期',
    `total_amount` DECIMAL(10,2) DEFAULT 0 COMMENT '总金额',
    `status` ENUM('PENDING', 'DISPENSED', 'CANCELLED') DEFAULT 'PENDING' COMMENT '状态：待发药、已发药、已取消',
    `remarks` TEXT COMMENT '备注',
    `create_time` DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    `update_time` DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    FOREIGN KEY (`medical_record_id`) REFERENCES `medical_record`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`patient_id`) REFERENCES `user`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`doctor_id`) REFERENCES `user`(`id`) ON DELETE CASCADE,
    INDEX `idx_medical_record_id` (`medical_record_id`),
    INDEX `idx_patient_id` (`patient_id`),
    INDEX `idx_doctor_id` (`doctor_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='处方表';

-- 处方明细表
CREATE TABLE IF NOT EXISTS `prescription_item` (
    `id` BIGINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    `prescription_id` BIGINT NOT NULL COMMENT '处方ID',
    `medicine_id` BIGINT NOT NULL COMMENT '药品ID',
    `quantity` INT NOT NULL COMMENT '数量',
    `dosage` VARCHAR(50) COMMENT '用法用量',
    `frequency` VARCHAR(50) COMMENT '频次',
    `unit_price` DECIMAL(10,2) NOT NULL COMMENT '单价',
    `total_price` DECIMAL(10,2) NOT NULL COMMENT '小计',
    FOREIGN KEY (`prescription_id`) REFERENCES `prescription`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`medicine_id`) REFERENCES `medicine`(`id`) ON DELETE CASCADE,
    INDEX `idx_prescription_id` (`prescription_id`),
    INDEX `idx_medicine_id` (`medicine_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='处方明细表';

-- 初始化超级管理员账号（密码：admin123，已使用BCrypt加密）
INSERT INTO `user` (`username`, `password`, `real_name`, `role`, `status`) VALUES
('admin', '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iwyL3B0VC', '系统管理员', 'SUPER_ADMIN', 1);

-- 初始化部分常用药品数据
INSERT INTO `medicine` (`name`, `code`, `specification`, `unit`, `price`, `stock`, `manufacturer`, `description`, `status`) VALUES
('对乙酰氨基酚片', 'MED001', '0.5g*10片', '盒', 12.50, 500, '扬子江药业', '用于普通感冒或流行性感冒引起的发热。', 1),
('布洛芬缓释胶囊', 'MED002', '0.3g*20粒', '盒', 25.00, 300, '中美史克', '用于缓解轻至中度疼痛。', 1),
('阿莫西林胶囊', 'MED003', '0.25g*24粒', '盒', 18.00, 400, '白云山', '青霉素类抗生素。', 1),
('奥美拉唑肠溶胶囊', 'MED004', '20mg*14粒', '盒', 35.00, 200, '阿斯利康', '用于胃溃疡、十二指肠溃疡。', 1),
('盐酸二甲双胍片', 'MED005', '0.5g*30片', '盒', 15.00, 600, '格华止', '用于单纯饮食控制不满意的2型糖尿病。', 1),
('阿司匹林肠溶片', 'MED006', '100mg*30片', '盒', 10.50, 800, '拜耳', '用于不稳定型心绞痛。', 1),
('多潘立酮片', 'MED007', '10mg*30片', '盒', 22.00, 350, '西安杨森', '用于消化不良、腹胀、嗳气。', 1),
('连花清瘟胶囊', 'MED008', '0.35g*24粒', '盒', 28.50, 450, '以岭药业', '清瘟解毒，宣肺泄热。', 1),
('维生素C泡腾片', 'MED009', '1g*10片', '管', 19.80, 550, '拜耳', '用于增强抵抗力，预防感冒。', 1),
('硝苯地平控释片', 'MED010', '30mg*7片', '盒', 42.00, 150, '拜耳', '用于治疗高血压。', 1);

