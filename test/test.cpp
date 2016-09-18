#include "tcp.hpp"

int main() {
    std::thread a{ [](){
        std::shared_ptr< fix::session_factory > f = std::make_shared< fix::session_factory_impl >();
        tcp_acceptor t( f, 14002 );
        t.run();
    } };

    std::thread c{ [](){
        std::shared_ptr< fix::session_factory > f = std::make_shared< fix::session_factory_impl >();
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
