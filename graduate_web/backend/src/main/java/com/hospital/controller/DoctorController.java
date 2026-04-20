package com.hospital.controller;

import com.hospital.common.Result;
import com.hospital.entity.Doctor;
import com.hospital.entity.User;
import com.hospital.service.DoctorService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/doctor")
@CrossOrigin
public class DoctorController {
    
    @Autowired
    private DoctorService doctorService;
    
    @GetMapping("/list")
    public Result<List<Doctor>> list() {
        List<Doctor> doctors = doctorService.findAll();
        return Result.success(doctors);
    }
    
    @GetMapping("/{id}")
    public Result<Doctor> getById(@PathVariable Long id) {
        Doctor doctor = doctorService.findById(id);
        return Result.success(doctor);
    }

    /** 按医生姓名（user.real_name）模糊查询 */
    @GetMapping("/search")
    public Result<List<Doctor>> searchByName(@RequestParam String name) {
        try {
            String keyword = name == null ? "" : name.trim();
            if (keyword.isEmpty()) {
                return Result.error("name 不能为空");
            }
            return Result.success(doctorService.searchByRealName(keyword));
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }
    
    @PostMapping
    public Result<Doctor> create(@RequestBody Map<String, Object> params) {
        try {
            User user = new User();
            user.setUsername((String) params.get("username"));
            user.setPassword((String) params.get("password"));
            user.setRealName((String) params.get("realName"));
            user.setPhone((String) params.get("phone"));
            user.setEmail((String) params.get("email"));
            
            Doctor doctor = new Doctor();
            doctor.setDepartment((String) params.get("department"));
            doctor.setTitle((String) params.get("title"));
            doctor.setSpecialty((String) params.get("specialty"));
            doctor.setLicenseNumber((String) params.get("licenseNumber"));
            doctor.setIntroduction((String) params.get("introduction"));
            
            Doctor newDoctor = doctorService.create(doctor, user);
            return Result.success("创建成功", newDoctor);
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }
    
    @PutMapping("/{id}")
    public Result<Doctor> update(@PathVariable Long id, @RequestBody Doctor doctor) {
        doctor.setId(id);
        Doctor updatedDoctor = doctorService.update(doctor);
        return Result.success("更新成功", updatedDoctor);
    }
    
    @DeleteMapping("/{id}")
    public Result<Void> delete(@PathVariable Long id) {
        doctorService.delete(id);
        return Result.success("删除成功", null);
    }
}

