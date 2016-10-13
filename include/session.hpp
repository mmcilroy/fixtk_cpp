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
        virtual void send( session&, const string& ) = 0;
        virtual void close( session& ) = 0;
    };

    struct receiver {
        virtual void receive( session&, const message& ) = 0;
    };

    session( const session_id& );
    session( const session_id&, std::unique_ptr< receiver >, std::unique_ptr< persistence > );

    const session_id& get_id() const;
    receiver* get_receiver() const;

    void connect( const std::shared_ptr< sender >& );
    void disconnect();
    bool is_connected() const;

    void send( const message_type&, const message& );
    void set_send_sequence( sequence );
    message get_sent( sequence ) const;

    void receive( const message& );
    void set_receive_sequence( sequence );
    sequence get_receive_sequence() const;
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

session::session( const session_id& id ) :
    id_( id ),
    send_sequence_( 1 ),
    receive_sequence_( 1 ) {
    ;
}

session::session( const session_id& id, std::unique_ptr< receiver > r, std::unique_ptr< persistence > p ) :
    id_( id ),
    send_sequence_( p->load_send_sequence() ),
    receive_sequence_( p->load_receive_sequence() ),
    receiver_( std::move( r ) ),
    persistence_( std::move( p ) ) {
    ;
}

const session_id& session::get_id() const {
    return id_;
}

session::receiver* session::get_receiver() const {
    return receiver_.get();
}

void session::connect( const std::shared_ptr< sender >& s ) {
    sender_ = s;
}

void session::disconnect() {
    auto s = sender_.lock();
    if( s ) {
        s->close( *this );
        sender_.reset();
    }
}

bool session::is_connected() const {
    auto s = sender_.lock();
    return s.get() != nullptr;
}

void session::send( const message_type& type, const message& body ) {
    string msg = serialize( id_, type, send_sequence_, body );
    log_debug( "send:" << msg );
    if( persistence_ ) {
        persistence_->store_send_sequence( send_sequence_ );
        persistence_->store_sent_message( send_sequence_, msg );
    }
    auto s = sender_.lock();
    if( s ) {
        s->send( *this, msg );
    } else {
        log_debug( "no sender" );
    }
    send_sequence_++;
}

void session::set_send_sequence( sequence s ) {
    send_sequence_ = s;
}

message session::get_sent( sequence s ) const {
    return parse( persistence_->load_sent_message( s ) );
}

void session::receive( const message& m ) {
    log_debug( "recv: " << id_ << " | " << m );
    if( receiver_ ) {
        receiver_->receive( *this, m );
    }
}

void session::set_receive_sequence( sequence s ) {
    receive_sequence_ = s;
}

sequence session::get_receive_sequence() const {
    return receive_sequence_;
}

void session::confirm_receipt( sequence s ) {
    log_debug( "confirm receipt: " << s );
    persistence_->store_receive_sequence( s );
}

}
