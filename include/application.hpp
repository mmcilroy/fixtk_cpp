#pragma once

#include "session.hpp"

namespace fix {

class application : public session::receiver {
public:
    void receive( session&, const message& ) override;
    bool is_logged_on() const;

protected:
    virtual void process( session&, const message&, const message_type& ) {}

private:
    void receive_while_logged_on( session&, const message& , const sequence& );
    void receive_while_logged_off( session&, const message& , const sequence& );

    bool queue( session&, const message&, const sequence& );
    bool is_pending_resend();

    void send_logon( session& );
    void send_resend( session&, sequence, sequence );

    void logoff( session& sess );

    std::list< std::pair< sequence, message > > queue_;

    bool logged_on_ = false;
};

struct alloc_application {
    session::receiver* operator()() {
        return new application;
    }
};

void application::receive( session& sess, const message& msg ) {
    auto seq_received = std::stoi( find_field( 34, msg ) );
    auto seq_expected = sess.get_receive_sequence();
    if( seq_received < seq_expected ) {
        // if the sequence is too low (ie the message has already been processed)
        // close the session immediately
        log_debug( "received sequence " << seq_received << " is too low. expected " << seq_expected );
        logoff( sess );
    } else {
        // attempt to process the message
        if( logged_on_ ) {
            receive_while_logged_on( sess, msg, seq_received );
        } else {
            receive_while_logged_off( sess, msg, seq_received );
        }
    }
}

bool application::is_logged_on() const {
    return logged_on_;
}

void application::receive_while_logged_on( session& sess, const message& msg, const sequence& seq_received ) {
    auto type = find_field( 35, msg );
    auto seq_expected = sess.get_receive_sequence();
    if( seq_received == seq_expected ) {
        process( sess, msg, type );
    } else {
        if( queue( sess, msg, seq_received ) ) {
            if( !is_pending_resend() ) {
              send_resend( sess, seq_expected, seq_received-1 );
            }
        }
    }
}

void application::receive_while_logged_off( session& sess, const message& msg, const sequence& seq_received ) {
    auto type = find_field( 35, msg );
    if( type == "A" ) {
        auto seq_expected = sess.get_receive_sequence();
        send_logon( sess );
        if( seq_received > seq_expected ) {
            send_resend( sess, seq_expected, seq_received-1 );
        }
    } else {
        logoff( sess );
    }
}

bool application::queue( session& sess, const message& msg, const sequence& seq_received ) {
    log_debug( "queueing message " << seq_received );
    if( queue_.size() ) {
        auto& last = queue_.back();
        if( seq_received - last.first != 1 ) {
            log_debug( "message sequence gap. " << last.first << " - " << seq_received );
            logoff( sess );
            return false;
        }
    }
    queue_.emplace_back( seq_received, msg );
    return true;
}

bool application::is_pending_resend() {
    return false;
}

void application::send_logon( session& sess ) {
    log_debug( "session logged on" );
    logged_on_ = true;
    sess.send( "A", {} );
}

void application::send_resend( session& sess, sequence low, sequence high ) {
    sess.send( "2", { { 7, low }, { 16, high } } );
}

void application::logoff( session& sess ) {
    logged_on_ = false;
    sess.disconnect();
}

}
