package com.hospital.service;

import com.hospital.entity.Patient;
import com.hospital.entity.Prescription;
import com.hospital.entity.PrescriptionItem;
import com.hospital.mapper.MedicineMapper;
import com.hospital.mapper.PrescriptionItemMapper;
import com.hospital.mapper.PrescriptionMapper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.math.BigDecimal;
import java.time.LocalDateTime;
import java.util.List;

@Service
public class PrescriptionService {
    
    private static final Logger logger = LoggerFactory.getLogger(PrescriptionService.class);
    
    @Autowired
    private PrescriptionMapper prescriptionMapper;
    
    @Autowired
    private PrescriptionItemMapper prescriptionItemMapper;
    
    @Autowired
    private MedicineMapper medicineMapper;
    
    @Autowired
    private PatientService patientService;
    
    @Autowired
    private RosService rosService;
    
    public Prescription findById(Long id) {
        Prescription prescription = prescriptionMapper.findById(id);
        if (prescription != null) {
            prescription.setItems(prescriptionItemMapper.findByPrescriptionId(id));
        }
        return prescription;
    }
    
    public List<Prescription> findByPatientId(Long patientId) {
        return prescriptionMapper.findByPatientId(patientId);
    }
    
    public Prescription findByMedicalRecordId(Long medicalRecordId) {
        Prescription prescription = prescriptionMapper.findByMedicalRecordId(medicalRecordId);
        if (prescription != null) {
            prescription.setItems(prescriptionItemMapper.findByPrescriptionId(prescription.getId()));
        }
        return prescription;
    }
    
    @Transactional
    public Prescription create(Prescription prescription, List<PrescriptionItem> items) {
        prescription.setPrescriptionDate(LocalDateTime.now());
        prescription.setStatus("PENDING");
        
        // 计算总金额
        BigDecimal totalAmount = BigDecimal.ZERO;
        for (PrescriptionItem item : items) {
            item.setUnitPrice(medicineMapper.findById(item.getMedicineId()).getPrice());
            item.setTotalPrice(item.getUnitPrice().multiply(BigDecimal.valueOf(item.getQuantity())));
            totalAmount = totalAmount.add(item.getTotalPrice());
        }
        prescription.setTotalAmount(totalAmount);
        
        prescriptionMapper.insert(prescription);
        
        // 插入处方明细
        for (PrescriptionItem item : items) {
            item.setPrescriptionId(prescription.getId());
            prescriptionItemMapper.insert(item);
        }
        
        prescription.setItems(items);
        return prescription;
    }
    
    public Prescription updateStatus(Long id, String status) {
        Prescription prescription = prescriptionMapper.findById(id);
        if (prescription == null) {
            throw new RuntimeException("处方不存在");
        }
        
        String oldStatus = prescription.getStatus();
        prescription.setStatus(status);
        prescriptionMapper.update(prescription);
        
        // 自动化逻辑：当状态变为“已发药” (DISPENSED) 时
        if ("DISPENSED".equals(status) && !"DISPENSED".equals(oldStatus)) {
            try {
                // 获取患者信息
                Patient patient = patientService.findByUserId(prescription.getPatientId());
                if (patient != null && "INPATIENT".equals(patient.getPatientType())) {
                    String roomNumber = patient.getRoomNumber();
                    if (roomNumber != null && !roomNumber.trim().isEmpty()) {
                        logger.info("检测到住院患者 {}，自动启动机器人送药任务：药房(1) -> 房号({})", 
                                prescription.getPatientId(), roomNumber);
                        
                        // 启动 ROS 任务：取物点为 "1" (药房)，送物点为房号
                        rosService.startTransportTask("1", roomNumber);
                    } else {
                        logger.warn("住院患者 {} 缺少房号，无法启动送药任务", prescription.getPatientId());
                    }
                }
            } catch (Exception e) {
                logger.error("自动启动送药任务失败: {}", e.getMessage());
                // 这里不抛出异常，以免影响发药状态的更新
            }
        }
        
        return prescription;
    }
    
    public void delete(Long id) {
        prescriptionItemMapper.deleteByPrescriptionId(id);
        prescriptionMapper.deleteById(id);
    }

    public List<Prescription> findAll() {
        return prescriptionMapper.findAll();
    }
}

