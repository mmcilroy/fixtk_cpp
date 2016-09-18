#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "field.hpp"

#include <sstream>

TEST_CASE( "", "[]" ) {    
    SECTION( "a field can be constructed from a int" ) {
        fix::field f{ 1, 1 };
        REQUIRE( f.tag() == 1 );
        REQUIRE( f.value() == "1" );
    }

    SECTION( "a field can be constructed from a string" ) {
        fix::field f{ 2, "two" };
        REQUIRE( f.tag() == 2 );
        REQUIRE( f.value() == "two" );
    }

    SECTION( "a field can be constructed from a char" ) {
        fix::field f{ 3, '3' };
        REQUIRE( f.tag() == 3 );
        REQUIRE( f.value() == "3" );
    }

    SECTION( "a field can be constructed from a float" ) {
        fix::field f{ 4, 1.123 };
        REQUIRE( f.tag() == 4 );
        REQUIRE( f.value() == "1.123" );
    }

    SECTION( "fields can be negative" ) {
        fix::field f{ -1, "negative" };
        REQUIRE( f.tag() == -1 );
        REQUIRE( f.value() == "negative" );
    }

    SECTION( "fields can be converted to a string" ) {
        fix::field f{ 6, "six" };
        std::stringstream ss;
        ss << f;
        REQUIRE( f.tag() == 6 );
        REQUIRE( ss.str() == "6=six" );
    }

    SECTION( "field vectors can be converted to a string" ) {
        fix::field_vector v{ { 1, "one" },{ 2, 2 },{ 3, 3.123 } };
        std::stringstream ss;
        ss << v;
        REQUIRE( ss.str() == "1=one|2=2|3=3.123|" );
        REQUIRE( v == fix::field_vector( { { 1, "one" },{ 2, 2 },{ 3, 3.123 } } ) );
    }
}
