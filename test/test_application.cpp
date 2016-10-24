#include "application.hpp"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE( "application", "[]" ) {
    fix::session_factory_impl< fix::alloc_application > factory;
    fix::session* sess = factory.get_session( { "P", "T", "S" } );
    fix::application* appl = (fix::application*)sess->get_listener();

    REQUIRE( appl != nullptr );
    REQUIRE( appl->is_logged_on() == false );

    SECTION( "successful logon, sequence expected", "[]" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( fix::parse( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) == sess->get_sent( 1 ) );
        REQUIRE( appl->is_logged_on() );
    }

    SECTION( "successful logon, sequence too high", "[]" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=666|49=S|56=T|10=??|" ) );
        REQUIRE( fix::parse( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) == sess->get_sent( 1 ) );
        REQUIRE( fix::parse( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=665|10=??|" ) == sess->get_sent( 2 ) );
        REQUIRE( appl->is_logged_on() );
    }

    SECTION( "unsuccessful logon, sequence too low", "[]" ) {
        sess->set_receive_sequence( 666 );
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( appl->is_logged_on() == false);
    }

    SECTION( "unsuccessful logon, first message not a logon", "[]" ) {
        sess->receive( fix::parse( "8=P|9=??|35=X|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( appl->is_logged_on() == false);
    }

    SECTION( "successful logon, gap fill 1-3 ok", "[]" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( fix::parse( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) == sess->get_sent( 1 ) );
        REQUIRE( fix::parse( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) == sess->get_sent( 2 ) );
        REQUIRE( appl->is_logged_on() );
        REQUIRE( sess->get_receive_sequence() == 1 );
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( sess->get_receive_sequence() == 2 );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=2|49=S|56=T|10=??|" ) );
        REQUIRE( sess->get_receive_sequence() == 3 );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=3|49=S|56=T|10=??|" ) );
        REQUIRE( sess->get_receive_sequence() == 5 );
        REQUIRE( appl->is_logged_on() );
    }

    SECTION( "successful logon, gap fill 1-3, missing 1" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( fix::parse( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) == sess->get_sent( 1 ) );
        REQUIRE( fix::parse( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) == sess->get_sent( 2 ) );
        REQUIRE( appl->is_logged_on() );
        REQUIRE( sess->get_receive_sequence() == 1 );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=2|49=S|56=T|10=??|" ) );
        REQUIRE( sess->get_receive_sequence() == 1 );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=3|49=S|56=T|10=??|" ) );
        REQUIRE( sess->get_receive_sequence() == 1 );
        REQUIRE( appl->is_logged_on() == false );
    }

    SECTION( "successful logon, gap fill 1-3, missing 2" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( fix::parse( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) == sess->get_sent( 1 ) );
        REQUIRE( fix::parse( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) == sess->get_sent( 2 ) );
        REQUIRE( appl->is_logged_on() );
        REQUIRE( sess->get_receive_sequence() == 1 );
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( sess->get_receive_sequence() == 2 );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=3|49=S|56=T|10=??|" ) );
        REQUIRE( sess->get_receive_sequence() == 2 );
        REQUIRE( appl->is_logged_on() == false );
    }

    SECTION( "successful logon, gap fill 1-3, missing 5" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( fix::parse( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) == sess->get_sent( 1 ) );
        REQUIRE( fix::parse( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) == sess->get_sent( 2 ) );
        REQUIRE( appl->is_logged_on() );
        REQUIRE( sess->get_receive_sequence() == 1 );
        sess->receive( fix::parse( "8=P|9=??|35=A|34=6|49=S|56=T|10=??|" ) );
        REQUIRE( sess->get_receive_sequence() == 1 );
        REQUIRE( appl->is_logged_on() == false );
    }
}
