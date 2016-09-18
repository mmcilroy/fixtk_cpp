#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "session.hpp"
#include <memory>

TEST_CASE( "sessions", "[]" ) {

    SECTION( "non-persistent, disconnected session" ) {
        fix::session_id i{ "FIX.4.4", "S", "T" };
        auto s = std::make_shared< fix::session >( i );
        REQUIRE( s->is_connected() == false );

        SECTION( "sending messages" ) {
            s->send( "D", { { 55, "VOD.L" } } );
        }
    }

    SECTION( "persistent, disconnected session" ) {
        fix::session_id i{ "FIX.4.4", "S", "T" };
        auto p = std::make_shared< fix::in_memory_persistence >();
        auto s = std::make_shared< fix::session >( i, p );
        REQUIRE( s->is_connected() == false );

        SECTION( "sending and recovering messages" ) {
            s->send( "D", { { 55, "VOD.L" } } );
            REQUIRE_THROWS( s->get_sent( 2 ) );
            REQUIRE( s->get_sent( 1 ) == fix::field_vector( {
                { 8, "FIX.4.4" }, { 9, 29 }, { 35, "D" }, { 34, 1 }, { 49, "S" }, { 56, "T" }, { 55, "VOD.L" }, { 10, "038" } } ) );

            s.reset();
            s = std::make_shared< fix::session >( i, p );
            REQUIRE( s->get_sent( 1 ) == fix::field_vector( {
                { 8, "FIX.4.4" }, { 9, 29 }, { 35, "D" }, { 34, 1 }, { 49, "S" }, { 56, "T" }, { 55, "VOD.L" }, { 10, "038" } } ) );
        }
    }
}
