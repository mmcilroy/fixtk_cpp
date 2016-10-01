#pragma once

#include <cstring>
#include <thread>
#include <pthread.h>
#include <iostream>

#define log_debug( m ) { \
	std::cout << "DBG t=" << pthread_self() << " o=" << this << " " \
	          << m << " ("  \
	          << __filename( __FILE__ ) << ":" << __LINE__ << ")" << std::endl; }

inline const char* __filename( const char* s ) {
    return strrchr( s, '/' ) ? strrchr( s, '/' ) + 1 : s;
}
