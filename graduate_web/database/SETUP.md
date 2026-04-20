# 数据库设置指南

## 方法一：安装MySQL客户端后导入（推荐）

### 1. 安装MySQL客户端

```bash
# 安装MySQL客户端
sudo apt install mysql-client-core-8.0

# 或者使用MariaDB客户端（兼容MySQL）
sudo apt install mariadb-client-core-10.3
```

### 2. 导入数据库脚本

```bash
# 方法1：使用mysql命令导入
mysql -u root -p < database/init.sql

# 方法2：先登录MySQL，然后导入
mysql -u root -p
source /home/zj/graduate_web/database/init.sql
exit
```

## 方法二：手动创建数据库（如果MySQL服务端已安装但客户端未安装）

### 1. 连接到MySQL服务器

如果MySQL服务已经运行，你可以通过其他方式连接（如phpMyAdmin、MySQL Workbench等）

### 2. 手动执行SQL语句

复制 `init.sql` 文件内容，在MySQL客户端中逐一执行。

## 方法三：使用Docker运行MySQL

### 1. 启动MySQL容器

```bash
docker run --name mysql-hospital \
  -e MYSQL_ROOT_PASSWORD=root \
  -e MYSQL_DATABASE=hospital_management \
  -p 3306:3306 \
  -d mysql:8.0
```

### 2. 导入数据

```bash
# 等待几秒让MySQL完全启动
sleep 10

# 导入数据库
docker exec -i mysql-hospital mysql -uroot -proot hospital_management < database/init.sql
```

### 3. 修改后端配置

编辑 `backend/src/main/resources/application.yml`：

```yaml
spring:
  datasource:
    url: jdbc:mysql://localhost:3306/hospital_management?useUnicode=true&characterEncoding=utf8&useSSL=false&serverTimezone=Asia/Shanghai&allowPublicKeyRetrieval=true
    username: root
    password: root
```

## 方法四：在MySQL Workbench或其他GUI工具中执行

1. 打开MySQL Workbench或其他MySQL图形界面工具
2. 连接到你的MySQL服务器
3. 打开 `database/init.sql` 文件
4. 执行整个脚本

## 验证数据库设置    数据库密码root

连接MySQL并验证：

```bash
mysql -u root -p

# 在MySQL提示符下执行：
USE hospital_management;
SHOW TABLES;
SELECT * FROM user;
```

应该能看到以下表：

- user
- doctor
- patient
- medical_record
- consultation
- medicine
- prescription
- prescription_item

## 默认管理员账号

- 用户名：`admin`
- 密码：`admin123`（已加密存储）

## 常见问题

### 问题1：MySQL服务未启动

```bash
# 检查MySQL服务状态
sudo systemctl status mysql

# 启动MySQL服务
sudo systemctl start mysql

# 设置开机自启
sudo systemctl enable mysql
```

### 问题2：忘记root密码

参考MySQL官方文档重置root密码。

### 问题3：权限问题

确保数据库用户有足够的权限创建数据库和表。

### 问题4：字符集问题

确保数据库使用UTF8MB4字符集（init.sql已设置）。

## 注意事项

1. 请根据实际情况修改数据库连接信息
2. 生产环境请使用强密码
3. 建议定期备份数据库
4. 默认密码仅用于开发环境，生产环境请修改
