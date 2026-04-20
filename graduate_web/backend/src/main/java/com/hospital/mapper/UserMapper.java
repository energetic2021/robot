package com.hospital.mapper;

import com.hospital.entity.User;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface UserMapper {
    
    @Select("SELECT * FROM user WHERE username = #{username}")
    User findByUsername(String username);
    
    @Select("SELECT * FROM user WHERE id = #{id}")
    User findById(Long id);
    
    @Select("SELECT * FROM user WHERE role = #{role}")
    List<User> findByRole(String role);
    
    @Insert("INSERT INTO user (username, password, real_name, phone, email, role, status) " +
            "VALUES (#{username}, #{password}, #{realName}, #{phone}, #{email}, #{role}, #{status})")
    @Options(useGeneratedKeys = true, keyProperty = "id")
    int insert(User user);
    
    @Update("UPDATE user SET real_name = #{realName}, phone = #{phone}, email = #{email}, status = #{status} WHERE id = #{id}")
    int update(User user);
    
    @Update("UPDATE user SET password = #{password} WHERE id = #{id}")
    int updatePassword(Long id, String password);
    
    @Delete("DELETE FROM user WHERE id = #{id}")
    int deleteById(Long id);
    
    @Select("<script>" +
            "SELECT * FROM user WHERE 1=1 " +
            "<if test='role != null'> AND role = #{role} </if>" +
            "<if test='status != null'> AND status = #{status} </if>" +
            "ORDER BY create_time DESC" +
            "</script>")
    List<User> findAll(@Param("role") String role, @Param("status") Integer status);
}

