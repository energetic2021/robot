package com.hospital.mapper;

import com.hospital.entity.Prescription;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface PrescriptionMapper {
    
    @Select("SELECT * FROM prescription WHERE id = #{id}")
    Prescription findById(Long id);
    
    @Select("SELECT * FROM prescription WHERE patient_id = #{patientId} ORDER BY prescription_date DESC")
    List<Prescription> findByPatientId(Long patientId);
    
    @Select("SELECT * FROM prescription WHERE medical_record_id = #{medicalRecordId}")
    Prescription findByMedicalRecordId(Long medicalRecordId);
    
    @Select("SELECT * FROM prescription ORDER BY prescription_date DESC")
    List<Prescription> findAll();
    
    @Insert("INSERT INTO prescription (medical_record_id, patient_id, doctor_id, prescription_date, total_amount, status, remarks) " +
            "VALUES (#{medicalRecordId}, #{patientId}, #{doctorId}, #{prescriptionDate}, #{totalAmount}, #{status}, #{remarks})")
    @Options(useGeneratedKeys = true, keyProperty = "id")
    int insert(Prescription prescription);
    
    @Update("UPDATE prescription SET status = #{status}, total_amount = #{totalAmount}, remarks = #{remarks} WHERE id = #{id}")
    int update(Prescription prescription);
    
    @Delete("DELETE FROM prescription WHERE id = #{id}")
    int deleteById(Long id);
}

