cmake_minimum_required(VERSION 3.4.1)

add_library(
             NativeRecord
             SHARED
             src/main/cpp/JNI_NativeRecord.cpp
             src/main/cpp/NativeRecord.cpp
             src/main/cpp/ThreadQueue.cpp
              )

find_library(
              log-lib
              log )

target_link_libraries(
                       NativeRecord
                       android
                       mediandk
                       ${log-lib}
                      )