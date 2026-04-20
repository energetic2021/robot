package com.hospital.mapper;

import com.hospital.entity.Patient;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface PatientMapper {
    
    @Select("SELECT p.*, u.real_name, u.phone, u.email FROM patient p " +
            "LEFT JOIN user u ON p.user_id = u.id WHERE p.id = #{id}")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "userId", column = "user_id"),
        @Result(property = "idCard", column = "id_card"),
        @Result(property = "gender", column = "gender"),
        @Result(property = "birthday", column = "birthday"),
        @Result(property = "address", column = "address"),
        @Result(property = "emergencyContact", column = "emergency_contact"),
        @Result(property = "emergencyPhone", column = "emergency_phone"),
        @Result(property = "patientType", column = "patient_type"),
        @Result(property = "roomNumber", column = "room_number"),
        @Result(property = "user.realName", column = "real_name"),
        @Result(property = "user.phone", column = "phone"),
        @Result(property = "user.email", column = "email")
    })
    Patient findById(Long id);
    
    @Select("SELECT p.*, u.real_name, u.phone, u.email FROM patient p " +
            "LEFT JOIN user u ON p.user_id = u.id WHERE p.user_id = #{userId}")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "userId", column = "user_id"),
        @Result(property = "idCard", column = "id_card"),
        @Result(property = "gender", column = "gender"),
        @Result(property = "birthday", column = "birthday"),
        @Result(property = "address", column = "address"),
        @Result(property = "emergencyContact", column = "emergency_contact"),
        @Result(property = "emergencyPhone", column = "emergency_phone"),
        @Result(property = "patientType", column = "patient_type"),
        @Result(property = "roomNumber", column = "room_number"),
        @Result(property = "user.realName", column = "real_name"),
        @Result(property = "user.phone", column = "phone"),
        @Result(property = "user.email", column = "email")
    })
    Patient findByUserId(Long userId);
    
    @Select("SELECT p.*, u.real_name, u.phone, u.email FROM patient p " +
            "LEFT JOIN user u ON p.user_id = u.id")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "userId", column = "user_id"),
        @Result(property = "idCard", column = "id_card"),
        @Result(property = "gender", column = "gender"),
        @Result(property = "birthday", column = "birthday"),
        @Result(property = "address", column = "address"),
        @Result(property = "emergencyContact", column = "emergency_contact"),
        @Result(property = "emergencyPhone", column = "emergency_phone"),
        @Result(property = "patientType", column = "patient_type"),
        @Result(property = "roomNumber", column = "room_number"),
        @Result(property = "user.realName", column = "real_name"),
        @Result(property = "user.phone", column = "phone"),
        @Result(property = "user.email", column = "email")
    })
    List<Patient> findAll();
    
    @Insert("INSERT INTO patient (user_id, id_card, gender, birthday, address, emergency_contact, emergency_phone, patient_type, room_number) " +
            "VALUES (#{userId}, #{idCard}, #{gender}, #{birthday}, #{address}, #{emergencyContact}, #{emergencyPhone}, #{patientType}, #{roomNumber})")
    @Options(useGeneratedKeys = true, keyProperty = "id")
    int insert(Patient patient);
    
    @Update("UPDATE patient SET id_card = #{idCard}, gender = #{gender}, birthday = #{birthday}, " +
            "address = #{address}, emergency_contact = #{emergencyContact}, emergency_phone = #{emergencyPhone}, " +
            "patient_type = #{patientType}, room_number = #{roomNumber} " +
            "WHERE id = #{id}")
    int update(Patient patient);
    
    @Delete("DELETE FROM patient WHERE id = #{id}")
    int deleteById(Long id);
}

