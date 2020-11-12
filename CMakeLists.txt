cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
cmake_policy(VERSION 2.6)

project(Telnet_Module)

add_library(Telnet_Module "Telnet_Module.c"
   "libtelnet.c")

target_include_directories(Telnet_Module PRIVATE "include" "../../../include")

