# AllSkyCameraCal: CMakeLists.txt

# Copyright (C) 2022  Johnathan K Burchill

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.


PROJECT(AllSkyCameraCal)

CMAKE_MINIMUM_REQUIRED(VERSION 3.3)

ADD_DEFINITIONS(-DHAVE_GETOPT_H)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    message( "-- Debugging enabled")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg -std=gnu99 ")
else (CMAKE_BUILD_TYPE STREQUAL Debug)
    message( "-- Optimization enabled")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=gnu99 ")
endif(CMAKE_BUILD_TYPE STREQUAL Debug)

INCLUDE_DIRECTORIES(include)

# GSL
FIND_PACKAGE(GSL REQUIRED)

FIND_LIBRARY(MATH libm.a)
FIND_LIBRARY(GSLSTATIC libgsl.a)
FIND_LIBRARY(GSLBLASSTATIC libgslcblas.a)

FIND_LIBRARY(READSAVE libredsafe.a)
FIND_LIBRARY(CDF libcdf.a)
FIND_LIBRARY(LIBC libc.a)

INCLUDE_DIRECTORIES(${INCLUDE_DIRS} ${GSL_INCLUDE_DIRS})

ADD_EXECUTABLE(allskycameracal main.c import.c analysis.c export.c util.c options.c info.c)
TARGET_LINK_LIBRARIES(allskycameracal -static ${CDF} ${LIBC} ${GSLSTATIC} ${GSLBLASSTATIC} ${READSAVE} ${MATH})

ADD_EXECUTABLE(testsiteimport test_site_import.c import.c)
TARGET_LINK_LIBRARIES(testsiteimport -static ${LIBC} ${CDF} ${READSAVE} ${MATH})

install(TARGETS allskycameracal DESTINATION $ENV{HOME}/bin)
