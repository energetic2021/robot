package com.hospital.service;

import com.hospital.entity.Doctor;
import com.hospital.entity.User;
import com.hospital.mapper.DoctorMapper;
import com.hospital.mapper.UserMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

@Service
public class DoctorService {
    
    @Autowired
    private DoctorMapper doctorMapper;
    
    @Autowired
    private UserMapper userMapper;
    
    public List<Doctor> findAll() {
        return doctorMapper.findAll();
    }
    
    public Doctor findById(Long id) {
        return doctorMapper.findById(id);
    }
    
    public Doctor findByUserId(Long userId) {
        return doctorMapper.findByUserId(userId);
    }

    public List<Doctor> searchByRealName(String name) {
        return doctorMapper.findByRealNameLike(name);
    }
    
    @Transactional
    public Doctor create(Doctor doctor, User user) {
        // 先创建用户
        user.setRole("DOCTOR");
        userMapper.insert(user);
        
        // 再创建医生信息
        doctor.setUserId(user.getId());
        doctorMapper.insert(doctor);
        
        return doctorMapper.findById(doctor.getId());
    }
    
    public Doctor update(Doctor doctor) {
        doctorMapper.update(doctor);
        return doctorMapper.findById(doctor.getId());
    }
    
    public void delete(Long id) {
        Doctor doctor = doctorMapper.findById(id);
        if (doctor != null) {
            doctorMapper.deleteById(id);
            userMapper.deleteById(doctor.getUserId());
        }
    }
}

