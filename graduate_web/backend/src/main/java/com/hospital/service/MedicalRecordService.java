package com.hospital.service;

import com.hospital.entity.MedicalRecord;
import com.hospital.mapper.MedicalRecordMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.time.LocalDateTime;
import java.util.List;

@Service
public class MedicalRecordService {
    
    @Autowired
    private MedicalRecordMapper medicalRecordMapper;
    
    public List<MedicalRecord> findByPatientId(Long patientId) {
        return medicalRecordMapper.findByPatientId(patientId);
    }
    
    public List<MedicalRecord> findByDoctorId(Long doctorId) {
        return medicalRecordMapper.findByDoctorId(doctorId);
    }
    
    public MedicalRecord findById(Long id) {
        return medicalRecordMapper.findById(id);
    }
    
    public MedicalRecord create(MedicalRecord record) {
        if (record.getVisitDate() == null) {
            record.setVisitDate(LocalDateTime.now());
        }
        medicalRecordMapper.insert(record);
        return medicalRecordMapper.findById(record.getId());
    }
    
    public MedicalRecord update(MedicalRecord record) {
        medicalRecordMapper.update(record);
        return medicalRecordMapper.findById(record.getId());
    }
    
    public void delete(Long id) {
        medicalRecordMapper.deleteById(id);
    }

    public List<MedicalRecord> findAll() {
        return medicalRecordMapper.findAll();
    }
}

