
#include "session_factory.hpp"

namespace fix {

class protocol : public session::receiver {
public:
    void receive( session& sess, const message& msg ) override {
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

protected:
    virtual void process( session& sess, const message& msg, const message_type& type, const sequence& seq_received ) {
        log_debug( "processing message " << seq_received );
        sess.set_receive_sequence( 1 + seq_received );
        // process message here!

        // handle any queued messages that are next in sequence
        while( queue_.size() && queue_.front().first == sess.get_receive_sequence() ) {
            receive( sess, queue_.front().second );
            queue_.pop_front();
        }
    }

private:
    void logon( session& sess ) {
        log_debug( "logged on" );
        logged_on_ = true;
        sess.send( "A", {} );
    }

    void logoff( session& sess ) {
        log_debug( "logged off" );
        logged_on_ = false;
        sess.disconnect();
    }

    void resend( session& sess, sequence low, sequence high ) {
        log_debug( "resend " << low << "-" << high );
        sess.send( "2", { { 7, low }, { 16, high } } );
    }

    bool queue( session& sess, const message& msg, const sequence& seq_received ) {
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

    bool logged_on_ = false;

    std::list< std::pair< sequence, message > > queue_;
};

struct alloc_protocol {
    session::receiver* operator()() {
        return new protocol;
    }
};

}

int main() {
    fix::session_factory_impl< fix::alloc_protocol > factory;
    fix::session* sess = factory.get_session( { "P", "T", "S" } );
    sess->receive( fix::parse( "8=P|9=??|35=A|34=3|49=S|56=T|10=??|" ) );
    sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
    sess->receive( fix::parse( "8=P|9=??|35=D|34=1|49=S|56=T|10=??|" ) );
    sess->receive( fix::parse( "8=P|9=??|35=D|34=2|49=S|56=T|10=??|" ) );
}
