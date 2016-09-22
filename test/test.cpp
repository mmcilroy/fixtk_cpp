#include "tcp.hpp"
#include <stdexcept>

namespace fix {

fix::value find_field( fix::tag t, const fix::message& m ) {
    auto it = std::find_if( m.begin(), m.end(), [ t ]( const fix::field& f ) {
        return f.get_tag() == t;
    } );
    if( it != m.end() ) {
        return (*it).get_value();
    } else {
        throw std::runtime_error( "field not found!" );
    }
}

template< typename It >
It find_field( fix::tag t, It b, It e ) {
    return std::find_if( b, e, [ t ]( const fix::field& f ) {
        return f.get_tag() == t;
    } );
}

}

class application : public fix::session::receiver {
public:
    application() {
        log_debug( "new application @ " << (void*)this );
    }

    virtual void receive( fix::session& s, const fix::message& m ) {
        auto type = fix::find_field( 35, m );
        auto sequence_received = std::stoi( fix::find_field( 34, m ) );
        auto sequence_expected = s.get_receive_sequence();

        // if the sequence is too low (ie the message has already been processed)
        // close the session immediately
        if( sequence_received < sequence_expected ) {
            log_debug( "received sequence " << sequence_received << " is too low. expected " << sequence_expected );
            s.disconnect();
        } else {
            if( type == "A" ) {
                s.send( "A", {} );
                log_debug( "logged on" );
            }

            // if the sequence is too high (ie we have missed messages)
            // issue a resend request
            if( sequence_received > sequence_expected ) {
                s.send( "2", { { 7, sequence_expected }, { 16, sequence_received } } );
            } else {
                ;
            }
        }
    }
};

struct alloc_application {
    fix::session::receiver* operator()() {
        return new application;
    }
};

int main() {
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
}
