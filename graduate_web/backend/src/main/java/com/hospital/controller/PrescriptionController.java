package com.hospital.controller;

import com.hospital.common.Result;
import com.hospital.entity.Prescription;
import com.hospital.entity.PrescriptionItem;
import com.hospital.service.PrescriptionService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/prescription")
@CrossOrigin
public class PrescriptionController {
    
    @Autowired
    private PrescriptionService prescriptionService;
    
    @GetMapping("/patient/{patientId}")
    public Result<List<Prescription>> getByPatientId(@PathVariable Long patientId) {
        List<Prescription> prescriptions = prescriptionService.findByPatientId(patientId);
        return Result.success(prescriptions);
    }
    
    @GetMapping("/medical-record/{medicalRecordId}")
    public Result<Prescription> getByMedicalRecordId(@PathVariable Long medicalRecordId) {
        Prescription prescription = prescriptionService.findByMedicalRecordId(medicalRecordId);
        return Result.success(prescription);
    }
    
    @GetMapping("/{id}")
    public Result<Prescription> getById(@PathVariable Long id) {
        Prescription prescription = prescriptionService.findById(id);
        return Result.success(prescription);
    }
    
    @GetMapping("/list")
    public Result<List<Prescription>> getAll() {
        List<Prescription> prescriptions = prescriptionService.findAll();
        return Result.success(prescriptions);
    }

    @PostMapping
    public Result<Prescription> create(@RequestBody Map<String, Object> params) {
        try {
            Prescription prescription = new Prescription();
            prescription.setMedicalRecordId(Long.valueOf(params.get("medicalRecordId").toString()));
            prescription.setPatientId(Long.valueOf(params.get("patientId").toString()));
            prescription.setDoctorId(Long.valueOf(params.get("doctorId").toString()));
            prescription.setRemarks((String) params.get("remarks"));
            
            @SuppressWarnings("unchecked")
            List<Map<String, Object>> itemsData = (List<Map<String, Object>>) params.get("items");
            List<PrescriptionItem> items = itemsData.stream().map(itemData -> {
                PrescriptionItem item = new PrescriptionItem();
                item.setMedicineId(Long.valueOf(itemData.get("medicineId").toString()));
                item.setQuantity(Integer.valueOf(itemData.get("quantity").toString()));
                item.setDosage((String) itemData.get("dosage"));
                item.setFrequency((String) itemData.get("frequency"));
                return item;
            }).collect(java.util.stream.Collectors.toList());
            
            Prescription newPrescription = prescriptionService.create(prescription, items);
            return Result.success("创建成功", newPrescription);
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }
    
    @PutMapping("/{id}/status")
    public Result<Prescription> updateStatus(@PathVariable Long id, @RequestBody Map<String, String> params) {
        String status = params.get("status");
        Prescription prescription = prescriptionService.updateStatus(id, status);
        return Result.success("更新成功", prescription);
    }
    
    @DeleteMapping("/{id}")
    public Result<Void> delete(@PathVariable Long id) {
        prescriptionService.delete(id);
        return Result.success("删除成功", null);
    }
}

