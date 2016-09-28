#include "tcp.hpp"
#include <stdexcept>

namespace fix {

template< typename It >
It find_field( fix::tag t, It b, It e ) {
    return std::find_if( b, e, [ t ]( const fix::field& f ) {
        return f.get_tag() == t;
    } );
}

fix::value find_field( fix::tag t, const fix::message& m ) {
    auto it = find_field( t, m.begin(), m.end() );
    if( it != m.end() ) {
        return (*it).get_value();
    } else {
        throw std::runtime_error( "field not found!" );
    }
}

}

class application : public fix::session::receiver {
public:
    void receive( fix::session& sess, const fix::message& msg ) override {
        auto seq_received = std::stoi( fix::find_field( 34, msg ) );
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

private:
    void logoff( fix::session& sess ) {
        logged_on_ = false;
        sess.disconnect();
    }

    void receive_while_logged_on( fix::session& sess, const fix::message& msg, const fix::sequence& seq_received ) {
        auto type = fix::find_field( 35, msg );
        auto seq_expected = sess.get_receive_sequence();
        if( seq_received == seq_expected ) {
            process( sess, msg, type );
        } else {
            queue( sess, msg, seq_received );
            if( !pending_resend() ) {
              send_resend( sess, seq_expected, seq_received-1 );
            }
        }
    }

    void receive_while_logged_off( fix::session& sess, const fix::message& msg, const fix::sequence& seq_received ) {
        auto type = fix::find_field( 35, msg );
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

    void queue( fix::session& sess, const fix::message& msg, const fix::sequence& seq_received ) {
        if( queue_.size() ) {
            auto& last = queue_.back();
            if( seq_received - last.first != 1 ) {
                log_debug( "message sequence gap. " << last.first << " - " << seq_received );
                logoff( sess );
            }
        }
        queue_.emplace_back( seq_received, msg );
    }

    void process( fix::session& sess, const fix::message& msg, const fix::message_type& type ) {
        ;
    }

    bool pending_resend() {
        return false;
    }

    void send_logon( fix::session& sess ) {
        log_debug( "session logged on" );
        logged_on_ = true;
        sess.send( "A", {} );
    }

    void send_resend( fix::session& sess, fix::sequence low, fix::sequence high ) {
        sess.send( "2", { { 7, low }, { 16, high } } );
    }

    bool logged_on_ = false;

    std::list< std::pair< fix::sequence, fix::message > > queue_;
};



struct alloc_application {
    fix::session::receiver* operator()() {
        return new application;
    }
};

int main() {
    std::thread a{ [](){
        std::shared_ptr< fix::session_factory > f = std::make_shared< fix::session_factory_impl< alloc_application > >();
        auto s = f->get_session( { "P", "T", "S" } );
        s->set_receive_sequence( 2 );
        tcp_acceptor t( f, 14002 );
        t.run();
    } };

    std::thread c{ [](){
        std::shared_ptr< fix::session_factory > f = std::make_shared< fix::session_factory_impl<> >();

        tcp_connector t( f );
        t.connect( "localhost:14002", { "P", "S", "T" },
            []( fix::session& s ) {
                s.send( "A", {} );
            } );
        t.run();
    } };

    a.join();
    c.join();


    /*
    std::thread a{ [](){
        std::shared_ptr< fix::session_factory > f = std::make_shared< fix::session_factory_impl< alloc_application > >();
        tcp_acceptor t( f, 14002 );
        t.run();
    } };

    std::thread c{ [](){
        std::shared_ptr< fix::session_factory > f = std::make_shared< fix::session_factory_impl<> >();
        f->get_session( { "P", "S", "T" } )->send( "D", { { 55, "VOD.L" } } );

        tcp_connector t( f );
        t.connect( "localhost:14002", { "P", "S", "T" },
            []( fix::session& s ) {
                s.send( "A", {} );
            } );
        t.run();
    } };

    a.join();
    c.join();
    */
}
