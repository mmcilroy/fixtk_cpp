
#include "session_factory.hpp"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

namespace fix {

class application : public session::receiver {
public:
    bool is_logged_on() const {
        return logged_on_;
    }

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
        if( type == "2" ) {
            auto low = find_field( 7, msg );
            auto high = find_field( 16, msg );
            for( int i = std::stoi( low ); i <= std::stoi( high ); i++ ) {
                auto resend_msg = sess.get_sent( i );
                sess.send( find_field( 35, resend_msg ), resend_msg );
            }
        } else if( type == "A" ) {
            // if initiator now we're logged on
            // if acceptor nothing required
        } else {
            // process application message here!        
        }

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

struct alloc_application {
    session::receiver* operator()() {
        return new application;
    }
};

}

TEST_CASE( "application", "[]" ) {
    fix::session_factory_impl< fix::alloc_application > factory;
    fix::session* sess = factory.get_session( { "P", "T", "S" } );
    fix::application* appl = (fix::application*)sess->get_receiver();

    REQUIRE( appl != nullptr );
    REQUIRE( appl->is_logged_on() == false );

    SECTION( "successful logon, sequence expected", "[]" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( fix::parse( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) == sess->get_sent( 1 ) );
        REQUIRE( appl->is_logged_on() );
    }

    SECTION( "successful logon, sequence too high", "[]" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=666|49=S|56=T|10=??|" ) );
        REQUIRE( fix::parse( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) == sess->get_sent( 1 ) );
        REQUIRE( fix::parse( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=665|10=??|" ) == sess->get_sent( 2 ) );
        REQUIRE( appl->is_logged_on() );
    }

    SECTION( "unsuccessful logon, sequence too low", "[]" ) {
        sess->set_receive_sequence( 666 );
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( appl->is_logged_on() == false);
    }

    SECTION( "unsuccessful logon, first message not a logon", "[]" ) {
        sess->receive( fix::parse( "8=P|9=??|35=X|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( appl->is_logged_on() == false);
    }

    SECTION( "successful logon, gap fill 1-3 ok", "[]" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( fix::parse( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) == sess->get_sent( 1 ) );
        REQUIRE( fix::parse( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) == sess->get_sent( 2 ) );
        REQUIRE( appl->is_logged_on() );
        REQUIRE( sess->get_receive_sequence() == 1 );
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( sess->get_receive_sequence() == 2 );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=2|49=S|56=T|10=??|" ) );
        REQUIRE( sess->get_receive_sequence() == 3 );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=3|49=S|56=T|10=??|" ) );
        REQUIRE( sess->get_receive_sequence() == 5 );
        REQUIRE( appl->is_logged_on() );
    }

    /*
    SECTION( "successful logon, gap fill 1-3, missing 1" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) );
        REQUIRE( appl->is_logged_on() );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=2|49=S|56=T|10=??|" ) );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=3|49=S|56=T|10=??|" ) );
        REQUIRE( !appl->is_logged_on() );
        REQUIRE( !sess->is_connected() );
    }

    SECTION( "successful logon, gap fill 1-3, missing 2" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) );
        REQUIRE( appl->is_logged_on() );
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=3|49=S|56=T|10=??|" ) );
        REQUIRE( !appl->is_logged_on() );
        REQUIRE( !sess->is_connected() );
    }

    SECTION( "successful logon, gap fill 1-3, missing 5" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) );
        REQUIRE( appl->is_logged_on() );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=36|49=S|56=T|10=??|" ) );
        REQUIRE( !appl->is_logged_on() );
        REQUIRE( !sess->is_connected() );
    }
    */
}
