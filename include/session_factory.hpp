#pragma once

#include "session.hpp"

#include <unordered_map>

namespace fix {

class session_factory {
public:
    virtual session* get_session( const session_id& ) = 0;
};


// ---------------------------------------------------------------------------

struct alloc_in_memory_persistence {
    persistence* operator()();
};

struct alloc_receiver {
    session::receiver* operator()();
};

// this is just a test implementation of session_factory
class session_factory_impl : public session_factory {
public:
    session* get_session( const session_id& ) override;

private:
    template<
        typename AllocPersistence = alloc_in_memory_persistence,
        typename AllocReceiver = alloc_receiver >
    session* get_session_impl( const session_id& );

    std::unordered_map< session_id, std::unique_ptr< session > > sessions_;
};


// ---------------------------------------------------------------------------

persistence* alloc_in_memory_persistence::operator()() {
    return new in_memory_persistence;
}

session::receiver* alloc_receiver::operator()() {
    return nullptr;
}

session* session_factory_impl::get_session( const session_id& id ) {
    return get_session_impl( id );
}

template<
    typename AllocPersistence,
    typename AllocReceiver >
session* fix::session_factory_impl::get_session_impl( const session_id& id ) {
    auto it = sessions_.find( id );
    if( it == sessions_.end() ) {
        auto p = std::unique_ptr< persistence >( AllocPersistence()() );
        auto r = std::unique_ptr< session::receiver >( AllocReceiver()() );
        sessions_[ id ] = std::unique_ptr< session >( new session{ id, std::move( p ) } );
        return sessions_[ id ].get();
    } else {
        return it->second.get();
    }
}

}
