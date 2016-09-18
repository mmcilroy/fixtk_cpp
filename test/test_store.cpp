#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "store.hpp"
#include <iostream>

TEST_CASE( "", "[]" ) {
    fix::memory_store store;
    fix::session_id id{ "FIX.4.4", "S", "T" };

    SECTION( "sequences are both 1 after initialization" ) {
        REQUIRE( store.get_send_sequence( id ) == 1 );
        REQUIRE( store.get_receive_sequence( id ) == 1 );
    }

    SECTION( "sequences can be incremented" ) {
        store.increment_send_sequence( id );
        REQUIRE( store.get_send_sequence( id ) == 2 );
        store.increment_receive_sequence( id );
        REQUIRE( store.get_receive_sequence( id ) == 2 );
    }

    SECTION( "trying to load an unknown messages throws" ) {
        REQUIRE_THROWS( store.load_sent( id, 1 ) );
    }

    SECTION( "sent messages can be retrieved" ) {
        fix::field_vector m{ { 8, "FIX.4.4" }, { 35, "D" }, { 49, "S" }, { 56, "T" }, { 55, "VOD.L" } };
        store.store_sent( id, 1, m );
        REQUIRE( store.load_sent( id, 1 ) == m );
    }
}
