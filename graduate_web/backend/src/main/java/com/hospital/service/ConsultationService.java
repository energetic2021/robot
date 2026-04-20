package com.hospital.service;

import com.hospital.entity.Consultation;
import com.hospital.mapper.ConsultationMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class ConsultationService {
    
    @Autowired
    private ConsultationMapper consultationMapper;
    
    public List<Consultation> findByPatientId(Long patientId) {
        return consultationMapper.findByPatientId(patientId);
    }
    
    public List<Consultation> findByTargetDoctorId(Long doctorId) {
        return consultationMapper.findByTargetDoctorId(doctorId);
    }
    
    public Consultation findById(Long id) {
        return consultationMapper.findById(id);
    }
    
    public Consultation create(Consultation consultation) {
        consultation.setStatus("PENDING");
        consultationMapper.insert(consultation);
        return consultationMapper.findById(consultation.getId());
    }
    
    public Consultation update(Consultation consultation) {
        consultationMapper.update(consultation);
        return consultationMapper.findById(consultation.getId());
    }
    
    public void accept(Long id) {
        Consultation consultation = consultationMapper.findById(id);
        consultation.setStatus("ACCEPTED");
        consultationMapper.update(consultation);
    }
    
    public void complete(Long id, String result) {
        Consultation consultation = consultationMapper.findById(id);
        consultation.setStatus("COMPLETED");
        consultation.setConsultationResult(result);
        consultationMapper.update(consultation);
    }
    
    public void cancel(Long id) {
        Consultation consultation = consultationMapper.findById(id);
        consultation.setStatus("CANCELLED");
        consultationMapper.update(consultation);
    }
    
    public void delete(Long id) {
        consultationMapper.deleteById(id);
    }

    public List<Consultation> findAll() {
        return consultationMapper.findAll();
    }
}

