#pragma once
#include <tuple>
#include "intrinsics.hpp"
#include "hashable.hpp"

// TODO: Remove me.
//  Let modern compilers know that we use these operators as is,
//  implementation considering all candidates would be preferred
//  but since not all of our target compilers implement complete
//  ISO C++20, we have to go with this "patch".
//
#define REDUCABLE_EXPLICIT_INHERIT_CXX20()                      \
    using reducable::operator<;                                 \
    using reducable::operator==;                                \
    using reducable::operator!=;                                

// Reduction macro.
//
#define REDUCE_TO( ... )                                              \
    template<bool> constexpr auto cxreduce() {                        \
        return xstd::reference_as_tuple( __VA_ARGS__ );               \
    }                                                                 \
    template<bool> constexpr auto cxreduce() const {                  \
        return xstd::reference_as_tuple( __VA_ARGS__ );               \
    }                                                                 \
    auto reduce() {                                                   \
        return xstd::reference_as_tuple( __VA_ARGS__ );               \
    }                                                                 \
    auto reduce() const {                                             \
        return xstd::reference_as_tuple( __VA_ARGS__ );               \
    }                                                                 \
    REDUCABLE_EXPLICIT_INHERIT_CXX20()                          

// Reducable types essentially let us do member-type reflection
// which we use to auto generate useful but repetetive methods 
// like ::hash() or comparison operators. Base type has to define
// the following routine where [...] should be replaced by members
// that should be contributing to the comparison/hash operations.
//
// - REDUCE_TO( ... );
//
// - Note: Ideally unique elements that are faster to compare should 
//         come first to speed up the equality comparisons.
//
#pragma warning(push)
#pragma warning(disable: 4305)
namespace xstd
{
    // Reducable tag let's us check if a type is reducable without having 
    // to template for proxied type or the auto-decl flags.
    //
    struct reducable_tag_t {};

    template<typename T>
    static constexpr bool is_reducable_v = std::is_base_of_v<reducable_tag_t, T>;
    template<typename T>
    concept Reducable = is_reducable_v<T>;

    // The main definition of the helper:
    //
    template<typename T>
    struct reducable : reducable_tag_t
    {
    protected:
        // Invoking T::reduce() in a member function will create problems
        // due to the type not being defined yet, however we can proxy it.
        //
        template<typename Tx>
        __forceinline static constexpr auto reduce_proxy( Tx& p ) 
        { 
            if ( std::is_constant_evaluated() )
                return p.template cxreduce<true>();
            else
                return p.reduce(); 
        }

    public:
        // Define basic comparison operators using std::tuple.
        //
        __forceinline constexpr auto operator==( const T& other ) const { return &other == this || reduce_proxy( ( const T& ) *this ) == reduce_proxy( other ); }
        __forceinline constexpr auto operator!=( const T& other ) const { return &other != this && reduce_proxy( ( const T& ) *this ) != reduce_proxy( other ); }
        __forceinline constexpr auto operator< ( const T& other ) const { return &other != this && reduce_proxy( ( const T& ) *this ) > reduce_proxy( other ); }

        // Define XSTD hash using a simple tuple hasher.
        //
        __forceinline constexpr hash_t hash() const { return make_hash( reduce_proxy( ( const T& ) *this ) ); }
    };

    // Helper used to create reduced tuples.
    //
    template<typename... Tx>
    __forceinline static constexpr std::tuple<Tx...> reference_as_tuple( Tx&&... args ) { return std::tuple<Tx...>( std::forward<Tx>( args )... ); }
};
#pragma warning(pop)