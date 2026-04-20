const express = require('express');
const mysql = require('mysql2/promise');
const app = express();
const port = 3322;

// 数据库配置
const dbConfig = {
  host: '127.0.0.1',  // 使用 127.0.0.1 而不是 localhost
  user: 'readonly',
  password: 'readonly123',
  database: 'hospital_management',
  port: 3306,
  socketPath: null,  // 强制使用 TCP
  connectTimeout: 60000,
  acquireTimeout: 60000,
  timeout: 60000
};

// 查询所有医生信息
// 带参数的医生查询
app.get('/api/doctors/query', async (req, res) => {
  try {
    const { department, page = 1, limit = 10 } = req.query;
    const connection = await mysql.createConnection(dbConfig);
    
    let sql = 'SELECT * FROM doctor WHERE 1=1';
    const params = [];
    
    // 按科室筛选
    if (department) {
      sql += ' AND department = ?';
      params.push(department);
    }
    
    // 分页
    const offset = (page - 1) * limit;
    sql += ' LIMIT ? OFFSET ?';
    params.push(parseInt(limit), offset);
    
    const [rows] = await connection.execute(sql, params);
    
    // 获取总数
    let countSql = 'SELECT COUNT(*) as total FROM doctor WHERE 1=1';
    const countParams = [];
    
    if (department) {
      countSql += ' AND department = ?';
      countParams.push(department);
    }
    
    const [countResult] = await connection.execute(countSql, countParams);
    const total = countResult[0].total;
    
    await connection.end();
    
    res.json({
      success: true,
      data: rows,
      pagination: {
        page: parseInt(page),
        limit: parseInt(limit),
        total,
        pages: Math.ceil(total / limit)
      }
    });
  } catch (error) {
    console.error('数据库查询错误:', error);
    res.status(500).json({
      success: false,
      message: '查询失败',
      error: error.message
    });
  }
});


// 查询所有医生信息
app.get('/api/doctors', async (req, res) => {
  try {
    const connection = await mysql.createConnection(dbConfig);
    const [rows] = await connection.execute('SELECT * FROM doctor');
    await connection.end();
    
    res.json({
      success: true,
      data: rows,
      count: rows.length
    });
  } catch (error) {
    console.error('数据库查询错误:', error);
    res.status(500).json({
      success: false,
      message: '查询失败',
      error: error.message
    });
  }
});


// 根据ID查询医生
// 使用查询字符串的ID查询
app.get('/api/doctors/id', async (req, res) => {
  try {
    const { id } = req.query;
    if (!id) {
      return res.status(400).json({
        success: false,
        message: '缺少ID参数'
      });
    }
    
    const connection = await mysql.createConnection(dbConfig);
    const [rows] = await connection.execute('SELECT * FROM doctor WHERE id = ?', [id]);
    await connection.end();
    
    if (rows.length === 0) {
      return res.status(404).json({
        success: false,
        message: '医生不存在'
      });
    }
    
    res.json({
      success: true,
      data: rows[0]
    });
  } catch (error) {
    console.error('数据库查询错误:', error);
    res.status(500).json({
      success: false,
      message: '查询失败',
      error: error.message
    });
  }
});


// 多条件查询医生
app.get('/api/doctors/search', async (req, res) => {
  try {
    const { department, title, specialty, page = 1, limit = 10 } = req.query;
    const connection = await mysql.createConnection(dbConfig);
    
    let sql = 'SELECT * FROM doctor WHERE 1=1';
    const params = [];
    
    if (department) {
      sql += ' AND department LIKE ?';
      params.push(`%${department}%`);
    }
    
    if (title) {
      sql += ' AND title LIKE ?';
      params.push(`%${title}%`);
    }
    
    if (specialty) {
      sql += ' AND specialty LIKE ?';
      params.push(`%${specialty}%`);
    }
    
    // 分页
    const offset = (page - 1) * limit;
    sql += ' LIMIT ? OFFSET ?';
    params.push(parseInt(limit), offset);
    
    const [rows] = await connection.execute(sql, params);
    await connection.end();
    
    res.json({
      success: true,
      data: rows,
      count: rows.length
    });
  } catch (error) {
    console.error('数据库查询错误:', error);
    res.status(500).json({
      success: false,
      message: '查询失败',
      error: error.message
    });
  }
});



app.listen(port, () => {
  console.log(`API 服务器运行在 http://localhost:${port}`);
});