#include "serialization.hpp"

using namespace fix;

int main() {
    buffer b = serialize( "D", 1, { "FIX.4.4", "S", "T" }, { { 55, "VOD.L" } } );
    std::cout << b << std::endl;
    std::cout << parse( b ) << std::endl;
}
