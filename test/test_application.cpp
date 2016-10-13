#include "application.hpp"
#include "session_factory.hpp"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

struct test_sender : fix::session::sender {
    void send( fix::session& sess, const fix::string& msg ) override {
        sent_.push_front( msg );
    }

    bool sent( const fix::string& msg ) {
        bool ret = ( msg == sent_.back() );
        sent_.pop_back();
        return ret;
    }

    int num_sent() const {
        return sent_.size();
    }

    void close( fix::session& sess ) override {
        closed_ = true;
    }

    bool is_closed() const {
        return closed_;
    }

    std::list< fix::string > sent_;
    bool closed_ = false;
};

TEST_CASE( "application", "[]" ) {
    fix::session_factory_impl< fix::alloc_application > factory;
    fix::session* sess = factory.get_session( { "P", "T", "S" } );
    auto sender = std::make_shared< test_sender >();
    REQUIRE( !sess->is_connected() );
    sess->connect( sender );
    REQUIRE( sess->is_connected() );

    fix::application* appl = (fix::application*)sess->get_receiver();
    REQUIRE( appl != nullptr );
    REQUIRE( !appl->is_logged_on() );


    SECTION( "successful logon, sequence expected" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) );
        REQUIRE( appl->is_logged_on() );
    }

    SECTION( "successful logon, sequence too high" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=666|49=S|56=T|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=665|10=??|" ) );
        REQUIRE( appl->is_logged_on() );
    }

    SECTION( "unsuccessful logon, sequence too low" ) {
        sess->set_receive_sequence( 666 );
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( !appl->is_logged_on() );
        REQUIRE( sender->is_closed() );
        REQUIRE( sender->num_sent() == 0 );
    }

    SECTION( "unsuccessful logon, first message not a logon" ) {
        sess->receive( fix::parse( "8=P|9=??|35=X|34=1|49=S|56=T|10=??|" ) );
        REQUIRE( !appl->is_logged_on() );
        REQUIRE( sender->is_closed() );
        REQUIRE( sender->num_sent() == 0 );
    }

    SECTION( "successful logon, gap fill 1-3 ok" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) );
        REQUIRE( appl->is_logged_on() );
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=2|49=S|56=T|10=??|" ) );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=3|49=S|56=T|10=??|" ) );
        REQUIRE( appl->is_logged_on() );
        REQUIRE( sess->is_connected() );
    }

    SECTION( "successful logon, gap fill 1-3, missing 1" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) );
        REQUIRE( appl->is_logged_on() );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=2|49=S|56=T|10=??|" ) );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=3|49=S|56=T|10=??|" ) );
        REQUIRE( !appl->is_logged_on() );
        REQUIRE( !sess->is_connected() );
    }

    SECTION( "successful logon, gap fill 1-3, missing 2" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) );
        REQUIRE( appl->is_logged_on() );
        sess->receive( fix::parse( "8=P|9=??|35=A|34=1|49=S|56=T|10=??|" ) );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=3|49=S|56=T|10=??|" ) );
        REQUIRE( !appl->is_logged_on() );
        REQUIRE( !sess->is_connected() );
    }

    SECTION( "successful logon, gap fill 1-3, missing 5" ) {
        sess->receive( fix::parse( "8=P|9=??|35=A|34=4|49=S|56=T|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=A|34=1|49=T|56=S|10=??|" ) );
        REQUIRE( sender->sent( "8=P|9=??|35=2|34=2|49=T|56=S|7=1|16=3|10=??|" ) );
        REQUIRE( appl->is_logged_on() );
        sess->receive( fix::parse( "8=P|9=??|35=D|34=36|49=S|56=T|10=??|" ) );
        REQUIRE( !appl->is_logged_on() );
        REQUIRE( !sess->is_connected() );
    }
}
