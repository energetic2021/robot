package com.hospital.mapper;

import com.hospital.entity.Medicine;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface MedicineMapper {
    
    @Select("SELECT * FROM medicine WHERE id = #{id}")
    Medicine findById(Long id);
    
    @Select("SELECT * FROM medicine WHERE status = 1 ORDER BY name")
    List<Medicine> findAll();
    
    @Select("SELECT * FROM medicine WHERE name LIKE CONCAT('%', #{keyword}, '%') AND status = 1")
    List<Medicine> searchByName(String keyword);
    
    @Insert("INSERT INTO medicine (name, code, specification, unit, price, stock, manufacturer, description, status) " +
            "VALUES (#{name}, #{code}, #{specification}, #{unit}, #{price}, #{stock}, #{manufacturer}, #{description}, #{status})")
    @Options(useGeneratedKeys = true, keyProperty = "id")
    int insert(Medicine medicine);
    
    @Update("UPDATE medicine SET name = #{name}, code = #{code}, specification = #{specification}, " +
            "unit = #{unit}, price = #{price}, stock = #{stock}, manufacturer = #{manufacturer}, " +
            "description = #{description}, status = #{status} WHERE id = #{id}")
    int update(Medicine medicine);
    
    @Delete("DELETE FROM medicine WHERE id = #{id}")
    int deleteById(Long id);
}

