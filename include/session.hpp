#pragma once

#include "message.hpp"
#include "persistence.hpp"
#include "serialization.hpp"
#include "log.hpp"

#include <memory>
#include <experimental/memory>

namespace fix {

class session {
public:
    struct sender {
        virtual void send( const string& ) = 0;
        virtual void close() = 0;
    };

    struct receiver {
        virtual void receive( const message& ) = 0;
    };

    session( const session_id& );
    session( const session_id&, std::unique_ptr< persistence > );

    void connect( const std::shared_ptr< sender >& );
    void disconnect();
    void send( const message_type&, const message& );
    void receive( const message& );
    message get_sent( sequence ) const;
    void confirm_receipt( sequence );

private:
    session_id id_;
    sequence send_sequence_;
    sequence receive_sequence_;

    std::weak_ptr< sender > sender_;
    std::unique_ptr< receiver > receiver_;
    std::unique_ptr< persistence > persistence_;
};


// ---------------------------------------------------------------------------

fix::session::session( const session_id& id ) :
    id_( id ),
    send_sequence_( 1 ),
    receive_sequence_( 1 ) {
    ;
}

fix::session::session( const session_id& id, std::unique_ptr< persistence > p ) :
    id_( id ),
    send_sequence_( 1 ),
    receive_sequence_( 1 ),
    persistence_( std::move( p ) ) {
    ;
}

void fix::session::connect( const std::shared_ptr< sender >& s ) {
    sender_ = s;
}

void fix::session::disconnect() {
    auto s = sender_.lock();
    if( s ) {
        s->close();
    }
}

void fix::session::send( const message_type& type, const message& body ) {
    string msg = serialize( id_, type, body );
    log_debug( "send:" << msg );
    if( persistence_ ) {
        persistence_->store_send_sequence( send_sequence_ );
        persistence_->store_sent_message( send_sequence_, msg );
    }
    auto s = sender_.lock();
    if( s ) {
        s->send( msg );
    } else {
        log_debug( "no sender" );
    }
    send_sequence_++;
}

void fix::session::receive( const message& m ) {
    log_debug( "recv: " << m );
    if( receiver_ ) {
        receiver_->receive( m );
    }
}

}
