#pragma once

#include "message.hpp"
#include "persistence.hpp"
#include "serialization.hpp"
#include "log.hpp"

#include <memory>
#include <experimental/memory>
#include <unordered_map>

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

class session_factory {
public:
    virtual session* get_session( const session_id& );

private:
    std::unordered_map< session_id, std::unique_ptr< session > > sessions_;
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


// ---------------------------------------------------------------------------

session* fix::session_factory::get_session( const session_id& id ) {
    auto it = sessions_.find( id );
    if( it == sessions_.end() ) {
        sessions_[ id ] = std::unique_ptr< session >( new session{ id, std::unique_ptr< persistence >( new in_memory_persistence ) } );
        return sessions_[ id ].get();
    } else {
        return it->second.get();
    }
}

}
