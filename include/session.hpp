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

    struct listener {
        virtual void on_connected( session& ) {}
        virtual void on_disconnected( session& ) {}
        virtual void on_message( session&, const message& ) {}
    };

    session( const session_id& );
    session( const session_id&, std::unique_ptr< listener >, std::unique_ptr< persistence > );

    const session_id& get_id() const;
    listener* get_listener() const;

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
    std::unique_ptr< listener > listener_;
    std::unique_ptr< persistence > persistence_;
};


// ---------------------------------------------------------------------------

session::session( const session_id& id ) :
    id_( id ),
    send_sequence_( 1 ),
    receive_sequence_( 1 ) {
    ;
}

session::session( const session_id& id, std::unique_ptr< listener > r, std::unique_ptr< persistence > p ) :
    id_( id ),
    send_sequence_( p->load_send_sequence() ),
    receive_sequence_( p->load_receive_sequence() ),
    listener_( std::move( r ) ),
    persistence_( std::move( p ) ) {
    ;
}

const session_id& session::get_id() const {
    return id_;
}

session::listener* session::get_listener() const {
    return listener_.get();
}

void session::connect( const std::shared_ptr< sender >& s ) {
    sender_ = s;
    if( listener_ ) {
        listener_->on_connected( *this );
    }
}

void session::disconnect() {
    auto s = sender_.lock();
    if( s ) {
        s->close( *this );
        sender_.reset();
    }
    if( listener_ ) {
        listener_->on_disconnected( *this );
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
    if( listener_ ) {
        listener_->on_message( *this, m );
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
