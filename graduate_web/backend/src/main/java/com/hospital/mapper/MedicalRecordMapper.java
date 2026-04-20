package com.hospital.mapper;

import com.hospital.entity.MedicalRecord;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface MedicalRecordMapper {
    
    @Select("SELECT mr.*, u1.real_name as patient_name, u2.real_name as doctor_name " +
            "FROM medical_record mr " +
            "LEFT JOIN user u1 ON mr.patient_id = u1.id " +
            "LEFT JOIN user u2 ON mr.doctor_id = u2.id " +
            "WHERE mr.id = #{id}")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "patientId", column = "patient_id"),
        @Result(property = "doctorId", column = "doctor_id"),
        @Result(property = "visitDate", column = "visit_date"),
        @Result(property = "chiefComplaint", column = "chief_complaint"),
        @Result(property = "presentIllness", column = "present_illness"),
        @Result(property = "pastHistory", column = "past_history"),
        @Result(property = "physicalExam", column = "physical_exam"),
        @Result(property = "diagnosis", column = "diagnosis"),
        @Result(property = "treatmentPlan", column = "treatment_plan"),
        @Result(property = "remarks", column = "remarks"),
        @Result(property = "patient.realName", column = "patient_name"),
        @Result(property = "patient.id", column = "patient_id"),
        @Result(property = "doctor.realName", column = "doctor_name"),
        @Result(property = "doctor.id", column = "doctor_id")
    })
    MedicalRecord findById(Long id);
    
    @Select("SELECT mr.*, u1.real_name as patient_name, u2.real_name as doctor_name " +
            "FROM medical_record mr " +
            "LEFT JOIN user u1 ON mr.patient_id = u1.id " +
            "LEFT JOIN user u2 ON mr.doctor_id = u2.id " +
            "WHERE mr.patient_id = #{patientId} ORDER BY mr.visit_date DESC")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "patientId", column = "patient_id"),
        @Result(property = "doctorId", column = "doctor_id"),
        @Result(property = "visitDate", column = "visit_date"),
        @Result(property = "chiefComplaint", column = "chief_complaint"),
        @Result(property = "presentIllness", column = "present_illness"),
        @Result(property = "pastHistory", column = "past_history"),
        @Result(property = "physicalExam", column = "physical_exam"),
        @Result(property = "diagnosis", column = "diagnosis"),
        @Result(property = "treatmentPlan", column = "treatment_plan"),
        @Result(property = "remarks", column = "remarks"),
        @Result(property = "patient.realName", column = "patient_name"),
        @Result(property = "patient.id", column = "patient_id"),
        @Result(property = "doctor.realName", column = "doctor_name"),
        @Result(property = "doctor.id", column = "doctor_id")
    })
    List<MedicalRecord> findByPatientId(Long patientId);
    
    @Select("SELECT mr.*, u1.real_name as patient_name, u2.real_name as doctor_name " +
            "FROM medical_record mr " +
            "LEFT JOIN user u1 ON mr.patient_id = u1.id " +
            "LEFT JOIN user u2 ON mr.doctor_id = u2.id " +
            "WHERE mr.doctor_id = #{doctorId} ORDER BY mr.visit_date DESC")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "patientId", column = "patient_id"),
        @Result(property = "doctorId", column = "doctor_id"),
        @Result(property = "visitDate", column = "visit_date"),
        @Result(property = "chiefComplaint", column = "chief_complaint"),
        @Result(property = "presentIllness", column = "present_illness"),
        @Result(property = "pastHistory", column = "past_history"),
        @Result(property = "physicalExam", column = "physical_exam"),
        @Result(property = "diagnosis", column = "diagnosis"),
        @Result(property = "treatmentPlan", column = "treatment_plan"),
        @Result(property = "remarks", column = "remarks"),
        @Result(property = "patient.realName", column = "patient_name"),
        @Result(property = "patient.id", column = "patient_id"),
        @Result(property = "doctor.realName", column = "doctor_name"),
        @Result(property = "doctor.id", column = "doctor_id")
    })
    List<MedicalRecord> findByDoctorId(Long doctorId);
    
    @Select("SELECT mr.*, u1.real_name as patient_name, u2.real_name as doctor_name " +
            "FROM medical_record mr " +
            "LEFT JOIN user u1 ON mr.patient_id = u1.id " +
            "LEFT JOIN user u2 ON mr.doctor_id = u2.id " +
            "ORDER BY mr.visit_date DESC")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "patientId", column = "patient_id"),
        @Result(property = "doctorId", column = "doctor_id"),
        @Result(property = "visitDate", column = "visit_date"),
        @Result(property = "chiefComplaint", column = "chief_complaint"),
        @Result(property = "presentIllness", column = "present_illness"),
        @Result(property = "pastHistory", column = "past_history"),
        @Result(property = "physicalExam", column = "physical_exam"),
        @Result(property = "diagnosis", column = "diagnosis"),
        @Result(property = "treatmentPlan", column = "treatment_plan"),
        @Result(property = "remarks", column = "remarks"),
        @Result(property = "patient.realName", column = "patient_name"),
        @Result(property = "patient.id", column = "patient_id"),
        @Result(property = "doctor.realName", column = "doctor_name"),
        @Result(property = "doctor.id", column = "doctor_id")
    })
    List<MedicalRecord> findAll();
    
    @Insert("INSERT INTO medical_record (patient_id, doctor_id, visit_date, chief_complaint, present_illness, " +
            "past_history, physical_exam, diagnosis, treatment_plan, remarks) " +
            "VALUES (#{patientId}, #{doctorId}, #{visitDate}, #{chiefComplaint}, #{presentIllness}, " +
            "#{pastHistory}, #{physicalExam}, #{diagnosis}, #{treatmentPlan}, #{remarks})")
    @Options(useGeneratedKeys = true, keyProperty = "id")
    int insert(MedicalRecord record);
    
    @Update("UPDATE medical_record SET visit_date = #{visitDate}, chief_complaint = #{chiefComplaint}, " +
            "present_illness = #{presentIllness}, past_history = #{pastHistory}, physical_exam = #{physicalExam}, " +
            "diagnosis = #{diagnosis}, treatment_plan = #{treatmentPlan}, remarks = #{remarks} " +
            "WHERE id = #{id}")
    int update(MedicalRecord record);
    
    @Delete("DELETE FROM medical_record WHERE id = #{id}")
    int deleteById(Long id);
}

