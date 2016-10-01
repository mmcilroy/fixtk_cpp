#pragma once

#include "session.hpp"
#include "session_factory.hpp"
#include "serialization.hpp"
#include "log.hpp"

#include <memory>
#include <experimental/memory>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class tcp_session;

class tcp_sender : public fix::session::sender {
public:
    tcp_sender( tcp_session& );
    ~tcp_sender();

    void send( fix::session&, const fix::string& ) override;
    void close( fix::session& ) override;

private:
    std::observer_ptr< tcp_session > session_;
};


// ---------------------------------------------------------------------------

class tcp_session : public std::enable_shared_from_this< tcp_session > {
public:
    tcp_session( tcp::socket, std::shared_ptr< fix::session_factory >& );
    tcp_session( tcp::socket, fix::session& );
    ~tcp_session();

    void send( const fix::string& );
    void receive();
    void close();

    tcp::socket socket_;

private:
    std::shared_ptr< tcp_sender > sender_;
    std::shared_ptr< fix::session_factory > factory_;
    std::observer_ptr< fix::session > session_;

    enum { max_length = 1024 };
    char data_[ max_length ];
};


// ---------------------------------------------------------------------------

class tcp_acceptor {
public:
    tcp_acceptor( std::shared_ptr< fix::session_factory >&, short port );
    void run();

private:
    void do_accept();

    boost::asio::io_service io_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;

    std::shared_ptr< fix::session_factory > factory_;
};


// ---------------------------------------------------------------------------

class tcp_connector {
public:
    tcp_connector( std::shared_ptr< fix::session_factory >& );

    template< typename T >
    void connect( const std::string&, const fix::session_id&, T );
    void run();

private:
    boost::asio::io_service io_;

    std::shared_ptr< fix::session_factory > factory_;
};


// ---------------------------------------------------------------------------

tcp_sender::tcp_sender( tcp_session& s ) :
    session_( &s ) {
    log_debug( "new tcp_sender @ " << (void*)this );
}

tcp_sender::~tcp_sender() {
    log_debug( "del tcp_sender @ " << (void*)this );
}

void tcp_sender::send( fix::session&, const fix::string& s ) {
    session_->send( s );
}

void tcp_sender::close( fix::session& ) {
    session_->close();
}


// ---------------------------------------------------------------------------

tcp_session::tcp_session( tcp::socket sock, std::shared_ptr< fix::session_factory >& factory )  :
    socket_( std::move( sock ) ),
    sender_( std::make_shared< tcp_sender >( *this ) ),
    factory_( factory ) {
    log_debug( "new tcp_session @ " << (void*)this );
}

tcp_session::tcp_session( tcp::socket sock, fix::session& sess ) :
    socket_( std::move( sock ) ),
    sender_( std::make_shared< tcp_sender >( *this ) ),
    session_( &sess ) {
    log_debug( "new tcp_session @ " << (void*)this );
    session_->connect( sender_ );
}

tcp_session::~tcp_session() {
    log_debug( "del tcp_session @ " << (void*)this );
}

void tcp_session::send( const fix::string& s ) {
    auto self( shared_from_this() );
    socket_.get_io_service().dispatch(
        [ this, self, s ]() {
        boost::asio::async_write(
            socket_,
            boost::asio::buffer( s.c_str(), s.size() ),
            [ this, self ]( boost::system::error_code ec, std::size_t ) {
                if( !ec ) {
                    ;
                }
            } );
        } );
}

void tcp_session::receive() {
    auto self( shared_from_this() );
    log_debug( "start receive" );
    socket_.async_read_some( boost::asio::buffer( data_, max_length ),
        [ this, self ]( boost::system::error_code ec, std::size_t length ) {
            log_debug( "received " << length );
            if( !ec ) {
                fix::message m = fix::parse( data_ );
                if( session_ == nullptr ) {
                    fix::session_id i{ m, true };
                    session_ = factory_->get_session( i );
                    if( session_ ) {
                        session_->connect( sender_ );
                    }
                }
                if( session_ ) {
                    session_->receive( m );
                } else {
                    // exception
                }
                receive();
            }
        } );
}

void tcp_session::close() {
    if( socket_.is_open() ) {
        socket_.close();
    }
}


// ---------------------------------------------------------------------------

tcp_acceptor::tcp_acceptor( std::shared_ptr< fix::session_factory >& factory, short port ) :
    acceptor_( io_, tcp::endpoint( tcp::v4(), port ) ),
    socket_( io_ ),
    factory_( factory ) {
    do_accept();
}

void tcp_acceptor::do_accept() {
    acceptor_.async_accept( socket_,
        [ this ]( boost::system::error_code ec ) {
            if( !ec ) {
                std::make_shared< tcp_session >( std::move( socket_ ), factory_ )->receive();
            }
            do_accept();
        } );
}

void tcp_acceptor::run() {
    io_.run();
}


// ---------------------------------------------------------------------------

tcp_connector::tcp_connector( std::shared_ptr< fix::session_factory >& factory ) :
    factory_( factory ) {
    ;
}

template< typename T >
void tcp_connector::connect( const std::string& conn, const fix::session_id& id, T handler ) {
    log_debug( "connecting to " << conn );
    tcp::resolver resolver( io_ );
    tcp::socket sock( io_ );
    auto endpoint = resolver.resolve( { "localhost", "14002" } );
    auto fix_sess = factory_->get_session( id );
    auto tcp_sess = std::make_shared< tcp_session >( std::move( sock ), *fix_sess );
    boost::asio::async_connect( tcp_sess->socket_, endpoint,
        [ this, tcp_sess, fix_sess, handler ]( boost::system::error_code ec, tcp::resolver::iterator ) {
            log_debug( "connected!" );
            if( !ec ) {
                handler( *fix_sess );
                tcp_sess->receive();
            } else {
                log_debug( "connect error!" );
            }
        } );
}

void tcp_connector::run() {
    io_.run();
}
