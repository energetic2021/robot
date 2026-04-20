cmake_minimum_required(VERSION 3.10)
project(voice_service)

# 设置编译选项
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找ROS依赖
find_package(catkin REQUIRED COMPONENTS
  roscpp
  std_msgs
  std_srvs
)

# 查找系统jsoncpp
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 设置AIUI路径
set(AIUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(AIUI_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(AIUI_RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# 包含目录
include_directories(
  ${catkin_INCLUDE_DIRS}
  ${JSONCPP_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include  # 这是AIUI头文件的根目录
  ${CMAKE_CURRENT_SOURCE_DIR}/include/aiui  # 也可以直接包含aiui目录
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)

# 源文件列表
set(SOURCES
  src/aiui_node.cpp
  src/utils/Base64Util.cpp
  src/utils/ConvertUtil.cpp
  src/utils/IatResultUtil.cpp
  src/utils/StreamNlpTtsHelper.cpp
)

# 生成可执行文件
add_executable(${PROJECT_NAME}_node ${SOURCES})

# 为所有源文件添加宏定义，将 aiui_va 映射到 aiui_v2
target_compile_definitions(${PROJECT_NAME}_node PRIVATE
  aiui=aiui_v2
)


# 设置运行时库路径
set_target_properties(${PROJECT_NAME}_node PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib:${AIUI_LIB_DIR}"
  BUILD_RPATH "${AIUI_LIB_DIR}:${CMAKE_CURRENT_BINARY_DIR}"
)

# 链接库
target_link_libraries(${PROJECT_NAME}_node
  ${catkin_LIBRARIES}
  ${JSONCPP_LIBRARIES}
  ${AIUI_LIB_DIR}/libaiui.so
  dl
  pthread
  rt
)

# 声明catkin包 - 这一行非常重要！
catkin_package(
  CATKIN_DEPENDS roscpp std_msgs std_srvs
)

install(TARGETS ${PROJECT_NAME}_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
)

# 安装库文件
install(FILES
  ${AIUI_LIB_DIR}/libaiui.so
  DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
)

# 安装头文件
install(DIRECTORY ${AIUI_INCLUDE_DIR}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
  PATTERN "*.h"
)

# 安装资源文件
install(DIRECTORY ${AIUI_RESOURCE_DIR}/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/resources
  USE_SOURCE_PERMISSIONS
)

# 安装启动文件
install(DIRECTORY launch/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
)

# 安装配置文件
install(DIRECTORY config/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/config
)