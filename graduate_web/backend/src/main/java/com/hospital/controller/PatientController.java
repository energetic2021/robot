package com.hospital.controller;

import com.hospital.common.Result;
import com.hospital.entity.Patient;
import com.hospital.entity.User;
import com.hospital.service.PatientService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.time.LocalDate;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/patient")
@CrossOrigin
public class PatientController {
    
    @Autowired
    private PatientService patientService;
    
    @GetMapping("/list")
    public Result<List<Patient>> list() {
        List<Patient> patients = patientService.findAll();
        return Result.success(patients);
    }
    
    @GetMapping("/{id}")
    public Result<Patient> getById(@PathVariable Long id) {
        Patient patient = patientService.findById(id);
        return Result.success(patient);
    }
    
    @PostMapping
    public Result<Patient> create(@RequestBody Map<String, Object> params) {
        try {
            User user = new User();
            user.setUsername((String) params.get("username"));
            user.setPassword((String) params.get("password"));
            user.setRealName((String) params.get("realName"));
            user.setPhone((String) params.get("phone"));
            user.setEmail((String) params.get("email"));
            
            Patient patient = new Patient();
            patient.setIdCard((String) params.get("idCard"));
            patient.setGender((String) params.get("gender"));
            if (params.get("birthday") != null) {
                patient.setBirthday(LocalDate.parse((String) params.get("birthday"), DateTimeFormatter.ISO_DATE));
            }
            patient.setAddress((String) params.get("address"));
            patient.setEmergencyContact((String) params.get("emergencyContact"));
            patient.setEmergencyPhone((String) params.get("emergencyPhone"));
            patient.setPatientType((String) params.get("patientType"));
            patient.setRoomNumber((String) params.get("roomNumber"));
            
            Patient newPatient = patientService.create(patient, user);
            return Result.success("创建成功", newPatient);
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }
    
    @PutMapping("/{id}")
    public Result<Patient> update(@PathVariable Long id, @RequestBody Patient patient) {
        patient.setId(id);
        Patient updatedPatient = patientService.update(patient);
        return Result.success("更新成功", updatedPatient);
    }
    
    @DeleteMapping("/{id}")
    public Result<Void> delete(@PathVariable Long id) {
        patientService.delete(id);
        return Result.success("删除成功", null);
    }
}

