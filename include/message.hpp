#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <sstream>
#include <ostream>
#include <vector>

namespace fix {

using string = std::string;
using tag = int;
using value = std::string;
using message_type = std::string;
using sequence = uint64_t;

const char delim = '|';
const string delim_str = "|";

class field {
public:
    template< typename V >
    field( tag, V );

    tag get_tag() const;
    value get_value() const;
    bool operator==( const field& ) const;

private:
    tag tag_;
    value value_;

    friend std::ostream& operator<<( std::ostream&, const field& );
};

using message = std::vector< field >;

template< typename V >
field::field( tag t, V v ) :
    tag_( t ) {
    std::stringstream ss;
    ss << v;
    value_ = ss.str();
}

tag field::get_tag() const {
    return tag_;
}

value field::get_value() const {
    return value_;
}

bool field::operator==( const field& rhs ) const {
    return tag_ == rhs.tag_ && value_ == rhs.value_;
}

std::ostream& operator<<( std::ostream& o, const field& f ) {
    o << f.tag_ << "=" << f.value_; return o;
}

std::ostream& operator<<( std::ostream& o, const message& v ) {
    for( auto f : v ) {
        o << f << delim;
    }
    return o;
}

template< typename It >
It find_field( fix::tag t, It b, It e ) {
    return std::find_if( b, e, [ t ]( const fix::field& f ) {
        return f.get_tag() == t;
    } );
}

fix::value find_field( fix::tag t, const fix::message& m ) {
    auto it = find_field( t, m.begin(), m.end() );
    if( it != m.end() ) {
        return (*it).get_value();
    } else {
        throw std::runtime_error( "field not found!" );
    }
}

}
