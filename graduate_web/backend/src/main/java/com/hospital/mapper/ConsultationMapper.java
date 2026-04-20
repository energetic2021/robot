package com.hospital.mapper;

import com.hospital.entity.Consultation;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface ConsultationMapper {
    
    @Select("SELECT c.*, u1.real_name as patient_name, u2.real_name as request_doctor_name, " +
            "u3.real_name as target_doctor_name FROM consultation c " +
            "LEFT JOIN user u1 ON c.patient_id = u1.id " +
            "LEFT JOIN user u2 ON c.request_doctor_id = u2.id " +
            "LEFT JOIN user u3 ON c.target_doctor_id = u3.id " +
            "WHERE c.id = #{id}")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "patientId", column = "patient_id"),
        @Result(property = "requestDoctorId", column = "request_doctor_id"),
        @Result(property = "targetDoctorId", column = "target_doctor_id"),
        @Result(property = "medicalRecordId", column = "medical_record_id"),
        @Result(property = "consultationType", column = "consultation_type"),
        @Result(property = "consultationReason", column = "consultation_reason"),
        @Result(property = "consultationDate", column = "consultation_date"),
        @Result(property = "status", column = "status"),
        @Result(property = "consultationResult", column = "consultation_result"),
        @Result(property = "patient.realName", column = "patient_name"),
        @Result(property = "requestDoctor.realName", column = "request_doctor_name"),
        @Result(property = "targetDoctor.realName", column = "target_doctor_name")
    })
    Consultation findById(Long id);
    
    @Select("SELECT c.*, u1.real_name as patient_name, u2.real_name as request_doctor_name, " +
            "u3.real_name as target_doctor_name FROM consultation c " +
            "LEFT JOIN user u1 ON c.patient_id = u1.id " +
            "LEFT JOIN user u2 ON c.request_doctor_id = u2.id " +
            "LEFT JOIN user u3 ON c.target_doctor_id = u3.id " +
            "WHERE c.patient_id = #{patientId} ORDER BY c.consultation_date DESC")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "patientId", column = "patient_id"),
        @Result(property = "requestDoctorId", column = "request_doctor_id"),
        @Result(property = "targetDoctorId", column = "target_doctor_id"),
        @Result(property = "medicalRecordId", column = "medical_record_id"),
        @Result(property = "consultationType", column = "consultation_type"),
        @Result(property = "consultationReason", column = "consultation_reason"),
        @Result(property = "consultationDate", column = "consultation_date"),
        @Result(property = "status", column = "status"),
        @Result(property = "consultationResult", column = "consultation_result"),
        @Result(property = "patient.realName", column = "patient_name"),
        @Result(property = "requestDoctor.realName", column = "request_doctor_name"),
        @Result(property = "targetDoctor.realName", column = "target_doctor_name")
    })
    List<Consultation> findByPatientId(Long patientId);
    
    @Select("SELECT c.*, u1.real_name as patient_name, u2.real_name as request_doctor_name, " +
            "u3.real_name as target_doctor_name FROM consultation c " +
            "LEFT JOIN user u1 ON c.patient_id = u1.id " +
            "LEFT JOIN user u2 ON c.request_doctor_id = u2.id " +
            "LEFT JOIN user u3 ON c.target_doctor_id = u3.id " +
            "WHERE c.target_doctor_id = #{doctorId} ORDER BY c.consultation_date DESC")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "patientId", column = "patient_id"),
        @Result(property = "requestDoctorId", column = "request_doctor_id"),
        @Result(property = "targetDoctorId", column = "target_doctor_id"),
        @Result(property = "medicalRecordId", column = "medical_record_id"),
        @Result(property = "consultationType", column = "consultation_type"),
        @Result(property = "consultationReason", column = "consultation_reason"),
        @Result(property = "consultationDate", column = "consultation_date"),
        @Result(property = "status", column = "status"),
        @Result(property = "consultationResult", column = "consultation_result"),
        @Result(property = "patient.realName", column = "patient_name"),
        @Result(property = "requestDoctor.realName", column = "request_doctor_name"),
        @Result(property = "targetDoctor.realName", column = "target_doctor_name")
    })
    List<Consultation> findByTargetDoctorId(Long doctorId);
    
    @Select("SELECT c.*, u1.real_name as patient_name, u2.real_name as request_doctor_name, " +
            "u3.real_name as target_doctor_name FROM consultation c " +
            "LEFT JOIN user u1 ON c.patient_id = u1.id " +
            "LEFT JOIN user u2 ON c.request_doctor_id = u2.id " +
            "LEFT JOIN user u3 ON c.target_doctor_id = u3.id " +
            "ORDER BY c.consultation_date DESC")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "patientId", column = "patient_id"),
        @Result(property = "requestDoctorId", column = "request_doctor_id"),
        @Result(property = "targetDoctorId", column = "target_doctor_id"),
        @Result(property = "medicalRecordId", column = "medical_record_id"),
        @Result(property = "consultationType", column = "consultation_type"),
        @Result(property = "consultationReason", column = "consultation_reason"),
        @Result(property = "consultationDate", column = "consultation_date"),
        @Result(property = "status", column = "status"),
        @Result(property = "consultationResult", column = "consultation_result"),
        @Result(property = "patient.realName", column = "patient_name"),
        @Result(property = "requestDoctor.realName", column = "request_doctor_name"),
        @Result(property = "targetDoctor.realName", column = "target_doctor_name")
    })
    List<Consultation> findAll();
    
    @Insert("INSERT INTO consultation (patient_id, request_doctor_id, target_doctor_id, medical_record_id, " +
            "consultation_type, consultation_reason, consultation_date, status) " +
            "VALUES (#{patientId}, #{requestDoctorId}, #{targetDoctorId}, #{medicalRecordId}, " +
            "#{consultationType}, #{consultationReason}, #{consultationDate}, #{status})")
    @Options(useGeneratedKeys = true, keyProperty = "id")
    int insert(Consultation consultation);
    
    @Update("UPDATE consultation SET status = #{status}, consultation_result = #{consultationResult} WHERE id = #{id}")
    int update(Consultation consultation);
    
    @Delete("DELETE FROM consultation WHERE id = #{id}")
    int deleteById(Long id);
}

