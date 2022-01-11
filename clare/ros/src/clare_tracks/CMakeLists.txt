cmake_minimum_required(VERSION 3.0.2)
project(clare_tracks)

find_package(catkin REQUIRED COMPONENTS
  genmsg
  rospy
  std_msgs
  message_generation
)

catkin_python_setup()

add_message_files(
   FILES
   EncoderMessage.msg
   JoystickInput.msg
   MotorSpeeds.msg
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
   DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
