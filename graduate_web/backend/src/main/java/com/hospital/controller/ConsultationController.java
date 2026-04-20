package com.hospital.controller;

import com.hospital.common.Result;
import com.hospital.entity.Consultation;
import com.hospital.service.ConsultationService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/consultation")
@CrossOrigin
public class ConsultationController {
    
    @Autowired
    private ConsultationService consultationService;
    
    @GetMapping("/patient/{patientId}")
    public Result<List<Consultation>> getByPatientId(@PathVariable Long patientId) {
        List<Consultation> consultations = consultationService.findByPatientId(patientId);
        return Result.success(consultations);
    }
    
    @GetMapping("/doctor/{doctorId}")
    public Result<List<Consultation>> getByTargetDoctorId(@PathVariable Long doctorId) {
        List<Consultation> consultations = consultationService.findByTargetDoctorId(doctorId);
        return Result.success(consultations);
    }
    
    @GetMapping("/{id}")
    public Result<Consultation> getById(@PathVariable Long id) {
        Consultation consultation = consultationService.findById(id);
        return Result.success(consultation);
    }
    
    @GetMapping("/list")
    public Result<List<Consultation>> getAll() {
        List<Consultation> consultations = consultationService.findAll();
        return Result.success(consultations);
    }
    
    @PostMapping
    public Result<Consultation> create(@RequestBody Consultation consultation) {
        try {
            Consultation newConsultation = consultationService.create(consultation);
            return Result.success("创建成功", newConsultation);
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }
    
    @PutMapping("/{id}/accept")
    public Result<Consultation> accept(@PathVariable Long id) {
        consultationService.accept(id);
        Consultation consultation = consultationService.findById(id);
        return Result.success("接受成功", consultation);
    }
    
    @PutMapping("/{id}/complete")
    public Result<Consultation> complete(@PathVariable Long id, @RequestBody Map<String, String> params) {
        String result = params.get("result");
        consultationService.complete(id, result);
        Consultation consultation = consultationService.findById(id);
        return Result.success("完成成功", consultation);
    }
    
    @PutMapping("/{id}/cancel")
    public Result<Consultation> cancel(@PathVariable Long id) {
        consultationService.cancel(id);
        Consultation consultation = consultationService.findById(id);
        return Result.success("取消成功", consultation);
    }
    
    @DeleteMapping("/{id}")
    public Result<Void> delete(@PathVariable Long id) {
        consultationService.delete(id);
        return Result.success("删除成功", null);
    }
}

