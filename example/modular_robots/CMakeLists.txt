# Copyright: (C) 2010 RobotCub Consortium
# Author: Paul Fitzpatrick
# CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT

# Make sure that YARP_DIR is set in your environment to wherever you 
# compiled YARP.

cmake_minimum_required(VERSION 2.8.9)

FIND_PACKAGE(YARP REQUIRED)

IF (NOT YARP_HAS_MATH_LIB)
  MESSAGE(FATAL_ERROR "Please compile YARP with math library turned on")
ENDIF()

INCLUDE_DIRECTORIES(${YARP_INCLUDE_DIRS})
ADD_EXECUTABLE(yarpwide main.cpp)
TARGET_LINK_LIBRARIES(yarpwide ${YARP_LIBRARIES})

