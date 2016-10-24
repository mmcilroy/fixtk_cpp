#include "persistence.hpp"
#include "serialization.hpp"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE( "", "[]" ) {
    fix::in_memory_persistence persistence;

    SECTION( "sequences are both 1 after initialization" ) {
        REQUIRE( persistence.load_send_sequence() == 1 );
        REQUIRE( persistence.load_receive_sequence() == 1 );
    }

    SECTION( "sequences can be incremented" ) {
        persistence.store_send_sequence( 2 );
        REQUIRE( persistence.load_send_sequence() == 2 );
        persistence.store_receive_sequence( 2 );
        REQUIRE( persistence.load_receive_sequence() == 2 );
    }

    SECTION( "trying to load an unknown messages throws" ) {
        REQUIRE_THROWS( persistence.load_sent_message( 1 ) );
    }

    SECTION( "sent messages can be retrieved" ) {
        fix::string m( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" );
        persistence.store_sent_message( 1, m );
        REQUIRE( persistence.load_sent_message( 1 ) == m );
    }
}
