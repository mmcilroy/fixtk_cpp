cmake_minimum_required( VERSION 2.8 )
project( doodle )
set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++17 -g" )

include_directories( include ../Catch/single_include )

#add_executable( test_store test/test_store.cpp )
#add_executable( test_application test/test_application.cpp )
#add_executable( test_field test/test_field.cpp )
#add_executable( test_session test/test_session.cpp )
#add_executable( test_serialization test/test_serialization.cpp )
#add_executable( test_acceptor test/test_acceptor.cpp )

add_executable( test test/test.cpp )
target_link_libraries( test boost_system pthread )
