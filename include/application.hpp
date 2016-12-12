#pragma once

#include "session_factory.hpp"

namespace fix {

class application : public session::listener {
public:
    struct listener {
        virtual void on_connected( session& ) {}
        virtual void on_disconnected( session& ) {}
        virtual void on_message( session&, const message& ) {}
    };

    application( bool acceptor );

    bool is_logged_on() const;

    void on_connected( session& ) override;
    void on_disconnected( session& ) override;
    void on_message( session&, const message& ) override;

protected:
    void process( session&, const message&, const message_type&, const sequence& );

private:
    void logon( session& );
    void logoff( session& );
    void resend( session&, sequence low, sequence high );
    bool queue( session&, const message&, const sequence& );

    bool acceptor_;
    bool logged_on_;
    std::list< std::pair< sequence, message > > queue_;
};


// ---------------------------------------------------------------------------

application::application( bool acceptor ) :
    acceptor_( acceptor ),
    logged_on_( false ) {
    ;
}

bool application::is_logged_on() const {
    return logged_on_;
}

void application::on_connected( session& ) {
    if( acceptor_ ) {
        // send logon
    }
}

void application::on_disconnected( session& ) {
    // call listener
}

void application::on_message( session& sess, const message& msg ) {
    auto seq_received = std::stoi( find_field( 34, msg ) );
    auto seq_expected = sess.get_receive_sequence();
    log_debug( "expected sequence " << seq_expected << ", received " << seq_received );

    if( seq_received < seq_expected ) {
        // if the sequence is too low close the session immediately
        log_debug( "received sequence " << seq_received << " is too low. expected " << seq_expected );
        logoff( sess );
    } else {
        auto type = find_field( 35, msg );

        // login if required
        if( !logged_on_ ) {
            if( type == "A" ) {
                logon( sess );
            } else {
                log_debug( "message is not a logon" );
                logoff( sess );
            }
        }

        // process application message
        if( logged_on_ ) {
            if( seq_received == seq_expected ) {
                // sequence expected so process immediately
                process( sess, msg, type, seq_received );
            } else {
                // sequence too high therefore we've missed messages
                // queue the message so it can be processed later
                if( queue( sess, msg, seq_received ) ) {
                    // if there is no outstanding resend requests issue one
                    if( 1 == queue_.size() ) {
                        resend( sess, seq_expected, seq_received-1 );
                    }
                }
            }
        }
    }
}

void application::process( session& sess, const message& msg, const message_type& type, const sequence& seq_received ) {
    log_debug( "processing message " << seq_received );
    sess.set_receive_sequence( 1 + seq_received );
    if( type == "2" ) {
        auto low = find_field( 7, msg );
        auto high = find_field( 16, msg );
        for( int i = std::stoi( low ); i <= std::stoi( high ); i++ ) {
            auto resend_msg = sess.get_sent( i );
            sess.send( find_field( 35, resend_msg ), resend_msg );
        }
        sess.confirm_receipt( seq_received );
    } else if( type == "A" ) {
        // this should be handled already - just need to confirm we received it!
        sess.confirm_receipt( seq_received );
    } else {
        // process application message here!
    }

    // handle any queued messages that are next in sequence
    while( queue_.size() && queue_.front().first == sess.get_receive_sequence() ) {
        on_message( sess, queue_.front().second );
        queue_.pop_front();
    }
}

void application::logon( session& sess ) {
    log_debug( "logged on" );
    logged_on_ = true;
    if( acceptor_ ) {
        sess.send( "A", {} );
    }
}

void application::logoff( session& sess ) {
    log_debug( "logged off" );
    logged_on_ = false;
    queue_.clear();
    sess.disconnect();
}

void application::resend( session& sess, sequence low, sequence high ) {
    log_debug( "resend " << low << "-" << high );
    sess.send( "2", { { 7, low }, { 16, high } } );
}

bool application::queue( session& sess, const message& msg, const sequence& seq_received ) {
    log_debug( "queue message " << seq_received );

    // if there are already queued messages make sure sequences are contiguous
    if( queue_.size() ) {
        auto& last = queue_.back();
        if( seq_received - last.first != 1 ) {
            log_debug( "sequence gap detected. expected " << sess.get_receive_sequence() << " or " << last.first + 1 << ", received " << seq_received );
            logoff( sess );
            return false;
        }
    }
    // add the message to the back of the queue
    queue_.emplace_back( seq_received, msg );
    return true;
}

struct alloc_application {
    session::listener* operator()() {
        return new application( true );
    }
};

}
