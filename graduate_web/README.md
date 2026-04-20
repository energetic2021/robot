原因
后端在 SecurityConfig 里配置了 CORS，只允许这两个来源：

http://localhost:3000
http://localhost:5173
你用 natapp 访问时，页面地址是 http://z6b78945.natappfree.cc，浏览器发请求时会带：

Origin: http://z6b78945.natappfree.cc

后端发现这个 Origin 不在白名单里，就返回 403。
所以本地用 http://localhost:3000/login 可以，换成 natapp 域名就不行。

修改内容
application.yml

新增配置项：cors.allowed-origins
在默认的 localhost 之外，加上了你当前用的 natapp 域名（http 和 https）：
http://z6b78945.natappfree.cc、https://z6b78945.natappfree.cc
SecurityConfig.java

使用 @Value("${cors.allowed-origins:...}") 读取上面的配置。
CORS 的允许来源改为从配置里按逗号分隔解析，这样以后换 natapp 域名只要改配置文件即可。
重启后端后再用 http://z6b78945.natappfree.cc/login 用 admin 登录，403 就会消失。

以后如果 natapp 换了域名（例如新的 xxxx.natappfree.cc），只需在 application.yml 里改这一行，例如：

cors:
  allowed-origins: http://localhost:3000,http://localhost:5173,http://新域名.natappfree.cc,https://新域名.natappfree.cc
然后重启后端即可。




数据库启用docker start mysql-hospital



















# 医院管理系统

一个完整的医院管理系统，基于 Spring Boot + Vue 3 + MySQL 构建。

## 功能特性

### 用户管理

- ✅ 超级用户管理
- ✅ 医生管理
- ✅ 患者管理

### 核心功能

- ✅ 会诊管理：申请会诊、接受会诊、完成会诊
- ✅ 病历编辑：创建、查看、编辑病历
- ✅ 开药管理：处方创建、药品管理、发药流程

## 技术栈

### 后端

- Spring Boot 2.7.14
- Spring Security + JWT
- MyBatis
- MySQL 8.0

### 前端

- Vue 3
- Element Plus
- Vue Router
- Pinia
- Axios
- Vite

## 快速开始

### 前置要求

- JDK 1.8+ (已检测到 Java 17 ✓)
- Maven 3.6+ ⚠️ 需要安装
- Node.js 16+ (前端需要)
- MySQL 8.0+

### 环境检查

运行快速检查脚本：

```bash
./quick-start.sh
```

### 安装必要工具

#### 安装 Maven

```bash
sudo apt update
sudo apt install maven
```

验证安装：

```bash
mvn -version
```

**详细安装说明请参考：** `BACKEND_SETUP.md`

### 数据库配置

#### 选项1：使用MySQL命令行客户端

1. 安装MySQL客户端（如果未安装）：

```bash
sudo apt install mysql-client-core-8.0
# 或
sudo apt install mariadb-client-core-10.3
```

2. 创建MySQL数据库：

```sql
CREATE DATABASE hospital_management;
```

3. 执行初始化脚本：

```bash
mysql -u root -p hospital_management < database/init.sql
```

#### 选项2：使用Docker运行MySQL

```bash
# 启动MySQL容器
docker run --name mysql-hospital \
  -e MYSQL_ROOT_PASSWORD=root \
  -e MYSQL_DATABASE=hospital_management \
  -p 3306:3306 \
  -d mysql:8.0

# 等待MySQL启动后导入数据
sleep 10
docker exec -i mysql-hospital mysql -uroot -proot hospital_management < database/init.sql
```

#### 选项3：使用图形化工具

使用MySQL Workbench、phpMyAdmin或其他MySQL图形界面工具：

1. 连接到MySQL服务器
2. 创建数据库 `hospital_management`
3. 打开并执行 `database/init.sql` 文件

**详细说明请参考：** `database/SETUP.md`

#### 默认管理员账号

- 用户名：`admin`
- 密码：`admin123`

### 后端启动

1. 进入后端目录：

```bash
cd backend
```

2. 修改数据库配置（如需要）：
   编辑 `src/main/resources/application.yml`，修改数据库连接信息。
3. 启动项目：

```bash
mvn spring-boot:run
```

后端服务将运行在 `http://localhost:8080`

### 前端启动

1. 进入前端目录：

```bash
cd frontend
```

2. 安装依赖：

```bash
npm install
```

3. 启动开发服务器：

```bash
npm run dev
```

前端应用将运行在 `http://localhost:3000`

### 访问系统

1. 打开浏览器访问：`http://localhost:3000`
2. 使用默认管理员账号登录：
   - 用户名：`admin`
   - 密码：`admin123`

## 项目结构

```
graduate_web/
├── backend/                 # Spring Boot后端
│   ├── src/
│   │   └── main/
│   │       ├── java/
│   │       │   └── com/hospital/
│   │       │       ├── controller/    # 控制器
│   │       │       ├── service/       # 服务层
│   │       │       ├── mapper/        # MyBatis映射
│   │       │       ├── entity/        # 实体类
│   │       │       ├── security/      # 安全配置
│   │       │       └── common/        # 通用类
│   │       └── resources/
│   │           └── application.yml    # 配置文件
│   └── pom.xml
├── frontend/                # Vue前端
│   ├── src/
│   │   ├── views/          # 页面组件
│   │   ├── layouts/        # 布局组件
│   │   ├── router/         # 路由配置
│   │   ├── stores/         # Pinia状态管理
│   │   ├── utils/          # 工具函数
│   │   └── main.js         # 入口文件
│   └── package.json
└── database/               # 数据库脚本
    └── init.sql           # 初始化SQL
```

## API接口说明

### 认证相关

- `POST /api/auth/login` - 用户登录
- `POST /api/auth/register` - 用户注册

### 用户管理

- `GET /api/user/list` - 用户列表
- `POST /api/user` - 创建用户
- `PUT /api/user/{id}` - 更新用户
- `DELETE /api/user/{id}` - 删除用户

### 医生管理

- `GET /api/doctor/list` - 医生列表
- `POST /api/doctor` - 创建医生
- `PUT /api/doctor/{id}` - 更新医生
- `DELETE /api/doctor/{id}` - 删除医生

### 患者管理

- `GET /api/patient/list` - 患者列表
- `POST /api/patient` - 创建患者
- `PUT /api/patient/{id}` - 更新患者
- `DELETE /api/patient/{id}` - 删除患者

### 病历管理

- `GET /api/medical-record/patient/{patientId}` - 获取患者病历
- `GET /api/medical-record/doctor/{doctorId}` - 获取医生病历
- `POST /api/medical-record` - 创建病历
- `PUT /api/medical-record/{id}` - 更新病历
- `DELETE /api/medical-record/{id}` - 删除病历

### 会诊管理

- `GET /api/consultation/patient/{patientId}` - 获取患者会诊
- `GET /api/consultation/doctor/{doctorId}` - 获取医生会诊
- `POST /api/consultation` - 申请会诊
- `PUT /api/consultation/{id}/accept` - 接受会诊
- `PUT /api/consultation/{id}/complete` - 完成会诊
- `PUT /api/consultation/{id}/cancel` - 取消会诊

### 药品管理

- `GET /api/medicine/list` - 药品列表
- `GET /api/medicine/search?keyword={keyword}` - 搜索药品
- `POST /api/medicine` - 创建药品
- `PUT /api/medicine/{id}` - 更新药品
- `DELETE /api/medicine/{id}` - 删除药品

### 处方管理

- `GET /api/prescription/patient/{patientId}` - 获取患者处方
- `GET /api/prescription/{id}` - 获取处方详情
- `POST /api/prescription` - 创建处方
- `PUT /api/prescription/{id}/status` - 更新处方状态

## 开发说明

### 后端开发

- 使用MyBatis进行数据持久化
- 使用JWT进行身份认证
- 使用Spring Security进行权限控制

### 前端开发

- 使用Vue 3 Composition API
- 使用Element Plus作为UI组件库
- 使用Pinia进行状态管理
- 使用Axios进行HTTP请求

## 注意事项

1. 首次运行前请确保MySQL服务已启动
2. 请根据实际情况修改数据库连接配置
3. 默认管理员密码已使用BCrypt加密存储在数据库中
4. 前端代理配置在 `vite.config.js` 中，后端地址为 `http://localhost:8080`

## 许可证

MIT License











数据库启动

zj@TP:~/graduate_web$ docker start mysql-hospital
mysql-hospital

zj@TP:~/graduate_web$ docker ps | grep mysql-hospital
7ebef9a46a40   mysql:8.0   "docker-entrypoint.s…"   4 months ago   Up 4 minutes   0.0.0.0:3306->3306/tcp, :::3306->3306/tcp, 33060/tcp   mysql-hospital


数据库密码：root



管理员账户：admin 密码：admin123
医生账户：555 密码：555
患者账户：111 密码：111
