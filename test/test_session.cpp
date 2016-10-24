#include "session_factory.hpp"
#include "application.hpp"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE( "sessions", "[]" ) {
    fix::session_factory_impl< fix::alloc_application > factory;
    fix::session* sess = factory.get_session( { "P", "S", "T" } );
    REQUIRE( sess->is_connected() == false );

    SECTION( "non-persistent, disconnected session" ) {
        SECTION( "sending messages" ) {
            sess->send( "D", { { 55, "VOD.L" } } );
        }
    }

    SECTION( "persistent, disconnected session" ) {
        SECTION( "sending and recovering messages" ) {
            sess->send( "D", { { 55, "VOD.L" } } );
            REQUIRE_THROWS( sess->get_sent( 2 ) );
            REQUIRE( sess->get_sent( 1 ) == fix::message( {
                { 8, "P" }, { 9, "??" }, { 35, "D" }, { 34, 1 }, { 49, "S" }, { 56, "T" }, { 55, "VOD.L" }, { 10, "??" } } ) );
        }
    }
}
