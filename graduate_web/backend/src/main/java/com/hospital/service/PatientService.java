package com.hospital.service;

import com.hospital.entity.Patient;
import com.hospital.entity.User;
import com.hospital.mapper.PatientMapper;
import com.hospital.mapper.UserMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

@Service
public class PatientService {
    
    @Autowired
    private PatientMapper patientMapper;
    
    @Autowired
    private UserMapper userMapper;
    
    public List<Patient> findAll() {
        return patientMapper.findAll();
    }
    
    public Patient findById(Long id) {
        return patientMapper.findById(id);
    }
    
    public Patient findByUserId(Long userId) {
        return patientMapper.findByUserId(userId);
    }
    
    @Transactional
    public Patient create(Patient patient, User user) {
        // 先创建用户
        user.setRole("PATIENT");
        userMapper.insert(user);
        
        // 再创建患者信息
        patient.setUserId(user.getId());
        patientMapper.insert(patient);
        
        return patientMapper.findById(patient.getId());
    }
    
    public Patient update(Patient patient) {
        patientMapper.update(patient);
        return patientMapper.findById(patient.getId());
    }
    
    public void delete(Long id) {
        Patient patient = patientMapper.findById(id);
        if (patient != null) {
            patientMapper.deleteById(id);
            userMapper.deleteById(patient.getUserId());
        }
    }
}

