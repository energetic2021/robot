package com.hospital.controller;

import com.hospital.common.Result;
import com.hospital.entity.MedicalRecord;
import com.hospital.service.MedicalRecordService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/medical-record")
@CrossOrigin
public class MedicalRecordController {
    
    @Autowired
    private MedicalRecordService medicalRecordService;
    
    @GetMapping("/patient/{patientId}")
    public Result<List<MedicalRecord>> getByPatientId(@PathVariable Long patientId) {
        List<MedicalRecord> records = medicalRecordService.findByPatientId(patientId);
        return Result.success(records);
    }
    
    @GetMapping("/doctor/{doctorId}")
    public Result<List<MedicalRecord>> getByDoctorId(@PathVariable Long doctorId) {
        List<MedicalRecord> records = medicalRecordService.findByDoctorId(doctorId);
        return Result.success(records);
    }
    
    @GetMapping("/{id}")
    public Result<MedicalRecord> getById(@PathVariable Long id) {
        MedicalRecord record = medicalRecordService.findById(id);
        return Result.success(record);
    }
    
    @GetMapping("/list")
    public Result<List<MedicalRecord>> getAll() {
        List<MedicalRecord> records = medicalRecordService.findAll();
        return Result.success(records);
    }
    
    @PostMapping
    public Result<MedicalRecord> create(@RequestBody MedicalRecord record) {
        try {
            MedicalRecord newRecord = medicalRecordService.create(record);
            return Result.success("创建成功", newRecord);
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }
    
    @PutMapping("/{id}")
    public Result<MedicalRecord> update(@PathVariable Long id, @RequestBody MedicalRecord record) {
        record.setId(id);
        MedicalRecord updatedRecord = medicalRecordService.update(record);
        return Result.success("更新成功", updatedRecord);
    }
    
    @DeleteMapping("/{id}")
    public Result<Void> delete(@PathVariable Long id) {
        medicalRecordService.delete(id);
        return Result.success("删除成功", null);
    }
}

