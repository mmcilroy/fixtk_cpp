#pragma once

#include "message.hpp"

#include <memory>
#include <experimental/memory>
#include <unordered_map>

namespace fix {

class session_id {
public:
    session_id( const message& );
    session_id( const string&, const string&, const string& );

    string get_protocol() const;
    string get_sender() const;
    string get_target() const;
    string get_id() const;

    bool operator==( const session_id& ) const;

private:
    string protocol_;
    string sender_;
    string target_;
    string id_;
};


// ---------------------------------------------------------------------------

session_id::session_id( const string& protocol, const string& sender, const string& target ) :
    protocol_( protocol ),
    sender_( sender ),
    target_( target ),
    id_( protocol + "." + sender + "." + target ) {
    ;
}

session_id::session_id( const message& m ) {
    for( auto& i : m ) {
        if( i.get_tag() == 8 ) {
            protocol_ = i.get_value();
        } else if( i.get_tag() == 49 ) {
            sender_ = i.get_value();
        } else if( i.get_tag() == 56 ) {
            target_ = i.get_value();
        }
        if( protocol_.size() && sender_.size() && target_.size() ) {
            break;
        }
    }
    if( protocol_.size() && sender_.size() && target_.size() ) {
        id_ = protocol_ + "." + sender_ + "." + target_;
    } else {
        // exception
    }
}

string session_id::get_protocol() const {
    return protocol_;
}

string session_id::get_sender() const {
    return sender_;
}

string session_id::get_target() const {
    return target_;
}

string session_id::get_id() const {
    return id_;
}

bool session_id::operator==( const session_id& rhs ) const {
    return id_ == rhs.id_;
}

}


// ---------------------------------------------------------------------------

namespace std {

template <>
struct hash< fix::session_id > {
    std::size_t operator()( const fix::session_id& id ) const {
        return hash< std::string >()( id.get_id() );
    }
};

}
