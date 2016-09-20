#include "tcp.hpp"

template< typename It >
It find_field( fix::tag t, It b, It e ) {
    return std::find_if( b, e, [ t ]( const fix::field& f ) {
        return f.get_tag() == t;
    } );
}

class application : public fix::session::receiver {
public:
    virtual void receive( fix::session& s, const fix::message& m ) {
        auto type = find_field( 35, m.begin(), m.end() );
        if( type != m.end() ) {
            auto seq = find_field( 34, type, m.end() );
            if( seq != m.end() ) {
                log_debug( "processing message type: " << (*type).get_value() << ", " << (*seq).get_value() );
                fix::sequence expected = s.get_receive_sequence();
                if( seq >= expected ) {
                    if( seq > expected ) {
                        // do resend
                    }
                    s.send( "A", {} );
                } else {
                    s.disconnect();
                }
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
