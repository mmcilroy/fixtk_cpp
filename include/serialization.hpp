#pragma once

#include "message.hpp"
#include "session_id.hpp"

#include <boost/algorithm/string.hpp>

namespace fix {

message parse( const string& b ) {
    message m;
    std::vector< std::string > p0;
    boost::split( p0, b, boost::is_any_of( delim_str.c_str() ) );
    for( auto& i : p0 ) {
        std::vector< std::string > p1;
        boost::split( p1, i, boost::is_any_of( "=" ) );
        if( p1.size() == 2 ) {
            m.emplace_back( std::atoi( p1[0].c_str() ), p1[1] );
        }
    }
    return m;
}

string serialize( 
    const session_id& id,
    const message_type& type,
    sequence seq,
    const message& body ) {
    std::stringstream ss;
    ss <<  "8=" << id.get_protocol() << delim
       <<  "9=??" << delim
       << "35=" << type << delim
       << "34=" << seq << delim
       << "49=" << id.get_sender() << delim
       << "56=" << id.get_target() << delim
       <<  body
       << "10=??" << delim;
    return ss.str();
}

}
