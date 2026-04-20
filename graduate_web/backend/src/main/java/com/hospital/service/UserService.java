package com.hospital.service;

import com.hospital.entity.User;
import com.hospital.mapper.UserMapper;
import com.hospital.mapper.DoctorMapper;
import com.hospital.mapper.PatientMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;

import java.util.List;

import com.hospital.entity.Doctor;
import com.hospital.entity.Patient;

import java.util.Map;

@Service
public class UserService {
    
    @Autowired
    private UserMapper userMapper;
    
    @Autowired
    private PasswordEncoder passwordEncoder;

    @Autowired
    private DoctorMapper doctorMapper;

    @Autowired
    private PatientMapper patientMapper;
    
    public List<User> findAll(String role, Integer status) {
        return userMapper.findAll(role, status);
    }
    
    public User findById(Long id) {
        return userMapper.findById(id);
    }
    
    public User create(User user, Map<String, Object> params) {
        if (userMapper.findByUsername(user.getUsername()) != null) {
            throw new RuntimeException("用户名已存在");
        }
        user.setPassword(passwordEncoder.encode(user.getPassword()));
        // 不要在这里硬编码 status = 1，如果 params 里传了 status 就用传的
        if (user.getStatus() == null) {
            user.setStatus(1);
        }
        userMapper.insert(user);
        // 新增：如果是医生，自动插入doctor表
        if ("DOCTOR".equals(user.getRole())) {
            Doctor doctor = new Doctor();
            doctor.setUserId(user.getId());
            doctor.setDepartment("未分配"); // 可根据需要设置默认科室
            doctorMapper.insert(doctor);
        }
        // 新增：如果是患者，自动插入patient表
        if ("PATIENT".equals(user.getRole())) {
            Patient patient = new Patient();
            patient.setUserId(user.getId());
            patient.setAge(20); // 可根据需要设置默认年龄
            patient.setGender("MALE"); // 设置一个合法的默认值
            
            // 从参数中获取患者特定字段
            if (params != null) {
                patient.setPatientType((String) params.get("patientType"));
                patient.setRoomNumber((String) params.get("roomNumber"));
            }
            
            // 如果没有传，设置默认值
            if (patient.getPatientType() == null) {
                patient.setPatientType("OUTPATIENT");
            }
            
            patientMapper.insert(patient);
        }
        return user;
    }
    
    public User update(User user) {
        userMapper.update(user);
        return userMapper.findById(user.getId());
    }
    
    public void delete(Long id) {
        userMapper.deleteById(id);
    }
    
    public void updatePassword(Long id, String newPassword) {
        String encodedPassword = passwordEncoder.encode(newPassword);
        userMapper.updatePassword(id, encodedPassword);
    }
}

