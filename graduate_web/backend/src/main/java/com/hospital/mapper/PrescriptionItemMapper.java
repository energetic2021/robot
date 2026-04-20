package com.hospital.mapper;

import com.hospital.entity.PrescriptionItem;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface PrescriptionItemMapper {
    
    @Select("SELECT pi.*, m.name as medicine_name, m.specification, m.unit FROM prescription_item pi " +
            "LEFT JOIN medicine m ON pi.medicine_id = m.id WHERE pi.prescription_id = #{prescriptionId}")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "prescriptionId", column = "prescription_id"),
        @Result(property = "medicineId", column = "medicine_id"),
        @Result(property = "quantity", column = "quantity"),
        @Result(property = "dosage", column = "dosage"),
        @Result(property = "frequency", column = "frequency"),
        @Result(property = "unitPrice", column = "unit_price"),
        @Result(property = "totalPrice", column = "total_price"),
        @Result(property = "medicine.name", column = "medicine_name"),
        @Result(property = "medicine.specification", column = "specification"),
        @Result(property = "medicine.unit", column = "unit")
    })
    List<PrescriptionItem> findByPrescriptionId(Long prescriptionId);
    
    @Insert("INSERT INTO prescription_item (prescription_id, medicine_id, quantity, dosage, frequency, unit_price, total_price) " +
            "VALUES (#{prescriptionId}, #{medicineId}, #{quantity}, #{dosage}, #{frequency}, #{unitPrice}, #{totalPrice})")
    @Options(useGeneratedKeys = true, keyProperty = "id")
    int insert(PrescriptionItem item);
    
    @Delete("DELETE FROM prescription_item WHERE prescription_id = #{prescriptionId}")
    int deleteByPrescriptionId(Long prescriptionId);
    
    @Delete("DELETE FROM prescription_item WHERE id = #{id}")
    int deleteById(Long id);
}

