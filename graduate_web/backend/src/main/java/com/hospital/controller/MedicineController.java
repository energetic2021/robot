package com.hospital.controller;

import com.hospital.common.Result;
import com.hospital.entity.Medicine;
import com.hospital.service.MedicineService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/medicine")
@CrossOrigin
public class MedicineController {
    
    @Autowired
    private MedicineService medicineService;
    
    @GetMapping("/list")
    public Result<List<Medicine>> list() {
        List<Medicine> medicines = medicineService.findAll();
        return Result.success(medicines);
    }
    
    @GetMapping("/search")
    public Result<List<Medicine>> search(@RequestParam String keyword) {
        List<Medicine> medicines = medicineService.searchByName(keyword);
        return Result.success(medicines);
    }
    
    @GetMapping("/{id}")
    public Result<Medicine> getById(@PathVariable Long id) {
        Medicine medicine = medicineService.findById(id);
        return Result.success(medicine);
    }
    
    @PostMapping
    public Result<Medicine> create(@RequestBody Medicine medicine) {
        try {
            Medicine newMedicine = medicineService.create(medicine);
            return Result.success("创建成功", newMedicine);
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }
    
    @PutMapping("/{id}")
    public Result<Medicine> update(@PathVariable Long id, @RequestBody Medicine medicine) {
        medicine.setId(id);
        Medicine updatedMedicine = medicineService.update(medicine);
        return Result.success("更新成功", updatedMedicine);
    }
    
    @DeleteMapping("/{id}")
    public Result<Void> delete(@PathVariable Long id) {
        medicineService.delete(id);
        return Result.success("删除成功", null);
    }
}

