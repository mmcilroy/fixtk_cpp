#include "message.hpp"
#include <sstream>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE( "", "[]" ) {    
    SECTION( "a field can be constructed from a int" ) {
        fix::field f{ 1, 1 };
        REQUIRE( f.get_tag() == 1 );
        REQUIRE( f.get_value() == "1" );
    }

    SECTION( "a field can be constructed from a string" ) {
        fix::field f{ 2, "two" };
        REQUIRE( f.get_tag() == 2 );
        REQUIRE( f.get_value() == "two" );
    }

    SECTION( "a field can be constructed from a char" ) {
        fix::field f{ 3, '3' };
        REQUIRE( f.get_tag() == 3 );
        REQUIRE( f.get_value() == "3" );
    }

    SECTION( "a field can be constructed from a float" ) {
        fix::field f{ 4, 1.123 };
        REQUIRE( f.get_tag() == 4 );
        REQUIRE( f.get_value() == "1.123" );
    }

    SECTION( "fields can be negative" ) {
        fix::field f{ -1, "negative" };
        REQUIRE( f.get_tag() == -1 );
        REQUIRE( f.get_value() == "negative" );
    }

    SECTION( "fields can be converted to a string" ) {
        fix::field f{ 6, "six" };
        std::stringstream ss;
        ss << f;
        REQUIRE( f.get_tag() == 6 );
        REQUIRE( ss.str() == "6=six" );
    }

    SECTION( "field vectors can be converted to a string" ) {
        fix::message m{ { 1, "one" },{ 2, 2 },{ 3, 3.123 } };
        std::stringstream ss;
        ss << m;
        REQUIRE( ss.str() == "1=one|2=2|3=3.123|" );
        REQUIRE( m == fix::message( { { 1, "one" },{ 2, 2 },{ 3, 3.123 } } ) );
    }
}
