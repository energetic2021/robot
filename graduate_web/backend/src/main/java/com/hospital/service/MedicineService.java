package com.hospital.service;

import com.hospital.entity.Medicine;
import com.hospital.mapper.MedicineMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class MedicineService {
    
    @Autowired
    private MedicineMapper medicineMapper;
    
    public List<Medicine> findAll() {
        return medicineMapper.findAll();
    }
    
    public List<Medicine> searchByName(String keyword) {
        return medicineMapper.searchByName(keyword);
    }
    
    public Medicine findById(Long id) {
        return medicineMapper.findById(id);
    }
    
    public Medicine create(Medicine medicine) {
        medicine.setStatus(1);
        medicineMapper.insert(medicine);
        return medicineMapper.findById(medicine.getId());
    }
    
    public Medicine update(Medicine medicine) {
        medicineMapper.update(medicine);
        return medicineMapper.findById(medicine.getId());
    }
    
    public void delete(Long id) {
        medicineMapper.deleteById(id);
    }
}

