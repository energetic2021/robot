package com.hospital.mapper;

import com.hospital.entity.Doctor;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface DoctorMapper {
    
    @Select("SELECT d.*, u.real_name, u.phone, u.email FROM doctor d " +
            "LEFT JOIN user u ON d.user_id = u.id WHERE d.id = #{id}")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "userId", column = "user_id"),
        @Result(property = "department", column = "department"),
        @Result(property = "title", column = "title"),
        @Result(property = "specialty", column = "specialty"),
        @Result(property = "licenseNumber", column = "license_number"),
        @Result(property = "introduction", column = "introduction"),
        @Result(property = "user.realName", column = "real_name"),
        @Result(property = "user.phone", column = "phone"),
        @Result(property = "user.email", column = "email")
    })
    Doctor findById(Long id);
    
    @Select("SELECT d.*, u.real_name, u.phone, u.email FROM doctor d " +
            "LEFT JOIN user u ON d.user_id = u.id WHERE d.user_id = #{userId}")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "userId", column = "user_id"),
        @Result(property = "department", column = "department"),
        @Result(property = "title", column = "title"),
        @Result(property = "specialty", column = "specialty"),
        @Result(property = "licenseNumber", column = "license_number"),
        @Result(property = "introduction", column = "introduction"),
        @Result(property = "user.realName", column = "real_name"),
        @Result(property = "user.phone", column = "phone"),
        @Result(property = "user.email", column = "email")
    })
    Doctor findByUserId(Long userId);
    
    @Select("SELECT d.*, u.real_name, u.phone, u.email FROM doctor d " +
            "LEFT JOIN user u ON d.user_id = u.id")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "userId", column = "user_id"),
        @Result(property = "department", column = "department"),
        @Result(property = "title", column = "title"),
        @Result(property = "specialty", column = "specialty"),
        @Result(property = "licenseNumber", column = "license_number"),
        @Result(property = "introduction", column = "introduction"),
        @Result(property = "user.realName", column = "real_name"),
        @Result(property = "user.phone", column = "phone"),
        @Result(property = "user.email", column = "email")
    })
    List<Doctor> findAll();

    @Select("SELECT d.*, u.real_name, u.phone, u.email FROM doctor d " +
            "LEFT JOIN user u ON d.user_id = u.id " +
            "WHERE u.real_name LIKE CONCAT('%', #{name}, '%')")
    @Results({
        @Result(property = "id", column = "id"),
        @Result(property = "userId", column = "user_id"),
        @Result(property = "department", column = "department"),
        @Result(property = "title", column = "title"),
        @Result(property = "specialty", column = "specialty"),
        @Result(property = "licenseNumber", column = "license_number"),
        @Result(property = "introduction", column = "introduction"),
        @Result(property = "user.realName", column = "real_name"),
        @Result(property = "user.phone", column = "phone"),
        @Result(property = "user.email", column = "email")
    })
    List<Doctor> findByRealNameLike(String name);
    
    @Insert("INSERT INTO doctor (user_id, department, title, specialty, license_number, introduction) " +
            "VALUES (#{userId}, #{department}, #{title}, #{specialty}, #{licenseNumber}, #{introduction})")
    @Options(useGeneratedKeys = true, keyProperty = "id")
    int insert(Doctor doctor);
    
    @Update("UPDATE doctor SET department = #{department}, title = #{title}, specialty = #{specialty}, " +
            "license_number = #{licenseNumber}, introduction = #{introduction} WHERE id = #{id}")
    int update(Doctor doctor);
    
    @Delete("DELETE FROM doctor WHERE id = #{id}")
    int deleteById(Long id);
}

