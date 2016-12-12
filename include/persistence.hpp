#pragma once

#include "message.hpp"
#include "log.hpp"
#include <unordered_map>

namespace fix {

class persistence {
public:
    virtual sequence load_send_sequence() = 0;
    virtual sequence load_receive_sequence() = 0;
    virtual string load_sent_message( sequence ) = 0;
    virtual void store_send_sequence( sequence ) = 0;
    virtual void store_receive_sequence( sequence ) = 0;
    virtual void store_sent_message( sequence, const string& ) = 0;
};


// ---------------------------------------------------------------------------

class in_memory_persistence : public persistence {
public:
    in_memory_persistence();
    sequence load_send_sequence() override;
    sequence load_receive_sequence() override;
    string load_sent_message( sequence ) override;

    void store_send_sequence( sequence ) override;
    void store_receive_sequence( sequence ) override;
    void store_sent_message( sequence, const string& ) override;

private:
    using message_map = std::unordered_map< sequence, string >;
    message_map messages_;
    sequence send_sequence_;
    sequence receive_sequence_;
};


// ---------------------------------------------------------------------------
in_memory_persistence::in_memory_persistence() :
    send_sequence_( 1 ),
    receive_sequence_( 1 ) {
    ;
}

sequence in_memory_persistence::load_send_sequence() {
    return send_sequence_;
}

sequence in_memory_persistence::load_receive_sequence()  {
    return receive_sequence_;
}

string in_memory_persistence::load_sent_message( sequence s )  {
    auto it = messages_.find( s );
    if( it != messages_.end() ) {
        return it->second;
    } else {
        throw std::runtime_error( "" );
    }
}

void in_memory_persistence::store_receive_sequence( sequence s )  {
    log_debug( "persist receive sequence: " << s );
    if( s > receive_sequence_ ) {
        receive_sequence_ = s;
    }
}

void in_memory_persistence::store_send_sequence( sequence s )  {
    log_debug( "persist send sequence: " << s );
    if( s > send_sequence_ ) {
        send_sequence_ = s;
    }
}

void in_memory_persistence::store_sent_message( sequence s, const string& m )  {
    log_debug( "persist sent message: " << s << ", " << m );
    messages_[ s ] = m;
}

}
