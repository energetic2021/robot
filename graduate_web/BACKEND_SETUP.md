# 后端环境设置指南

## Maven 安装

### 方法1：使用 apt 安装（推荐）

```bash
sudo apt update
sudo apt install maven
```

安装后验证：
```bash
mvn -version
```

### 方法2：手动安装 Maven

如果 apt 安装失败或需要特定版本：

1. 下载 Maven：
```bash
cd /tmp
wget https://dlcdn.apache.org/maven/maven-3/3.9.6/binaries/apache-maven-3.9.6-bin.tar.gz
```

2. 解压：
```bash
sudo tar -xzf apache-maven-3.9.6-bin.tar.gz -C /opt
```

3. 设置环境变量：
```bash
echo 'export PATH=/opt/apache-maven-3.9.6/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

4. 验证：
```bash
mvn -version
```

## 启动后端服务

### 使用 Maven 启动

```bash
cd backend
mvn spring-boot:run
```

### 使用 Maven Wrapper（如果可用）

如果项目中有 `mvnw` 文件：
```bash
cd backend
./mvnw spring-boot:run
```

### 编译项目

```bash
cd backend
mvn clean compile
```

### 打包项目

```bash
cd backend
mvn clean package
```

打包后会生成 `target/hospital-management-1.0.0.jar`，可以使用以下命令运行：
```bash
java -jar target/hospital-management-1.0.0.jar
```

## 常见问题

### 问题1：Maven 下载依赖很慢

可以使用国内镜像，编辑 `~/.m2/settings.xml`（如果不存在则创建）：

```xml
<?xml version="1.0" encoding="UTF-8"?>
<settings>
  <mirrors>
    <mirror>
      <id>aliyunmaven</id>
      <mirrorOf>*</mirrorOf>
      <name>阿里云公共仓库</name>
      <url>https://maven.aliyun.com/repository/public</url>
    </mirror>
  </mirrors>
</settings>
```

### 问题2：端口被占用

如果 8080 端口被占用，修改 `backend/src/main/resources/application.yml`：
```yaml
server:
  port: 8081  # 改为其他端口
```

### 问题3：数据库连接失败

检查：
1. MySQL 服务是否运行：`sudo systemctl status mysql`
2. 数据库配置是否正确：检查 `application.yml`
3. 数据库是否已创建：`mysql -u root -p -e "SHOW DATABASES;"`

### 问题4：Java 版本不兼容

项目使用 Java 1.8，如果系统是 Java 17：
- Java 17 向后兼容，应该可以运行
- 如果遇到问题，可以安装 Java 8：
```bash
sudo apt install openjdk-8-jdk
```

## 验证后端是否启动成功

启动后应该看到类似输出：
```
Started HospitalApplication in X.XXX seconds
```

访问测试接口（需要先登录获取token）：
```bash
curl http://localhost:8080/api/auth/login -X POST \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
```

## 开发工具推荐

- **IDE**: IntelliJ IDEA 或 Eclipse
- **API 测试**: Postman 或 curl
- **数据库工具**: DBeaver 或 MySQL Workbench

