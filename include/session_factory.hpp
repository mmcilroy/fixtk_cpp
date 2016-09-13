#pragma once

#include "session.hpp"

#include <unordered_map>

namespace fix {

struct alloc_in_memory_persistence {
    persistence* operator()();
};

struct alloc_receiver {
    session::receiver* operator()();
};

class session_factory {
public:
    template<
        typename AllocPersistence = alloc_in_memory_persistence,
        typename AllocReceiver = alloc_receiver >
    session* get_session( const session_id& );

private:
    std::unordered_map< session_id, std::unique_ptr< session > > sessions_;
};


// ---------------------------------------------------------------------------

persistence* alloc_in_memory_persistence::operator()() {
    return nullptr;
}

session::receiver* alloc_receiver::operator()() {
    return nullptr;
}

template<
    typename AllocPersistence,
    typename AllocReceiver >
session* fix::session_factory::get_session( const session_id& id ) {
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
