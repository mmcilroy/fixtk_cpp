cmake_minimum_required( VERSION 2.8 )
project( doodle )
set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g" )

include_directories( include ../Catch/single_include )

add_executable( test_application test/test_application.cpp )
target_link_libraries( test_application pthread )

add_executable( test_message test/test_message.cpp )
target_link_libraries( test_message pthread )

add_executable( test_persistence test/test_persistence.cpp )
target_link_libraries( test_persistence pthread )

add_executable( test_session test/test_session.cpp )
target_link_libraries( test_session pthread )

add_executable( test_tcp test/test_tcp.cpp )
target_link_libraries( test_tcp pthread boost_system )
