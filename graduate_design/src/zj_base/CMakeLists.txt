cmake_minimum_required(VERSION 3.0.2)
project(zj_base)

find_package(catkin REQUIRED COMPONENTS
  gazebo_plugins
  gazebo_ros
  gazebo_ros_control
  roscpp
  rospy
  std_msgs
  urdf
  xacro
  geometry_msgs
  tf2_ros
  message_generation
  controller_manager
  joint_state_controller
  position_controllers
  effort_controllers
  diff_drive_controller
)

generate_messages(
  DEPENDENCIES
  std_msgs
  geometry_msgs
)

catkin_package(
  CATKIN_DEPENDS 
  message_runtime 
  gazebo_plugins 
  gazebo_ros 
  gazebo_ros_control 
  roscpp 
  rospy 
  std_msgs 
  urdf 
  xacro
  geometry_msgs
  tf2_ros
  controller_manager
  joint_state_controller
  position_controllers
  effort_controllers
  diff_drive_controller
)

include_directories(
  ${catkin_INCLUDE_DIRS}
)

catkin_install_python(PROGRAMS
  script/drawer_control_node.py
  script/set_initial_position.py
  script/simple_drawer_control.py
  DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY launch urdf rviz config meshes
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)