# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.10.2)

# Declares and names the project.

project("nativedemo")

#native activity===========================
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
add_library(native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Werror")
# Export ANativeActivity_onCreate(),
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
add_library(native-activity SHARED main.cpp)

target_include_directories(native-activity PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(native-activity
        android
        native_app_glue
        EGL
        GLESv1_CM
        log)

#native activity============================

#native activity plasma demo============================
# 注意相关依赖不要重复
add_library(native-plasma SHARED plasma.cpp)

target_include_directories(native-plasma PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(native-plasma
        android
        native_app_glue
        log
        m)


#native activity plasma demo============================




# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             native-lib.cpp
             WIListener.cpp)

find_package(oboe REQUIRED CONFIG)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.
find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# 增加 ${ANDROID_NDK}/sources/android/native_app_glue（私有）
#target_include_directories(native-lib PRIVATE
#        ${ANDROID_NDK}/sources/android/native_app_glue)


# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib oboe::oboe

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib})