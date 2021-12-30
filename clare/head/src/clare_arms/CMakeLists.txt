cmake_minimum_required(VERSION 3.0.2)
project(clare_arms)

find_package(catkin REQUIRED COMPONENTS
  genmsg
  rospy
  std_msgs
  message_generation
)

catkin_python_setup()

add_message_files(
   FILES
   ArmMovement.msg
)

generate_messages(
  DEPENDENCIES
  std_msgs
)

catkin_package(
  CATKIN_DEPENDS
  rospy
  message_runtime
)

catkin_install_python(PROGRAMS
   nodes/arm_node.py
   DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
