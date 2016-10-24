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

struct alloc_listener {
    session::listener* operator()();
};

// this is just a test implementation of session_factory
template<
    typename AllocReceiver = alloc_listener,
    typename AllocPersistence = alloc_in_memory_persistence >
class session_factory_impl : public session_factory {
public:
    session* get_session( const session_id& ) override;

private:
    std::unordered_map< session_id, std::unique_ptr< session > > sessions_;
};


// ---------------------------------------------------------------------------

persistence* alloc_in_memory_persistence::operator()() {
    return new in_memory_persistence;
}

session::listener* alloc_listener::operator()() {
    return nullptr;
}

template<
    typename AllocReceiver,
    typename AllocPersistence >
session* session_factory_impl< AllocReceiver, AllocPersistence >::get_session( const session_id& id ) {
    auto it = sessions_.find( id );
    if( it == sessions_.end() ) {
        auto p = std::unique_ptr< persistence >( AllocPersistence()() );
        auto r = std::unique_ptr< session::listener >( AllocReceiver()() );
        sessions_[ id ] = std::unique_ptr< session >( new session{ id, std::move( r ), std::move( p ) } );
        return sessions_[ id ].get();
    } else {
        return it->second.get();
    }
}

}
