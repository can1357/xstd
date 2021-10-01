#pragma once
#include <iterator>
#include <array>
#include "type_helpers.hpp"
#include "bitwise.hpp"

namespace xstd
{
	// Define a bool proxy.
	//
	template<Pointer Ptr>
	struct bool_proxy
	{
		using value_type = std::remove_cv_t<std::remove_pointer_t<Ptr>>;
		using mask_type =  std::make_unsigned_t<value_type>;
		using reference =  Ptr;

		Ptr       storage;
		mask_type mask;

		constexpr operator bool() const noexcept { return ( *storage & mask ) != 0; }
		constexpr const bool_proxy& operator=( bool value ) const noexcept requires ( !Const<std::remove_pointer_t<Ptr>> )
		{
			if ( value ) *storage |= mask;
			else         *storage &= ~mask;
			return *this;
		}
	};
	template<Integral I> bool_proxy( I*, I )->bool_proxy<I*>;
	template<Integral I> bool_proxy( const I*, I )->bool_proxy<const I*>;

	// Define a bool iterator.
	//
	template<Pointer Ptr>
	struct bool_iterator
	{
		// Declare iterator traits.
		//
		using iterator_category = std::random_access_iterator_tag;
		using difference_type =   int64_t;
		using value_type =        bool_proxy<Ptr>;
		using reference =         const bool_proxy<Ptr>&;
		using pointer =           const bool_proxy<Ptr>*;

		using mask_type =         typename value_type::mask_type;
		static constexpr size_t bit_count = sizeof( mask_type ) * 8;

		// Stores the proxy.
		//
		bool_proxy<Ptr> proxy = {};

		// Support random iteration.
		//
		constexpr bool_iterator& operator++()
		{
			size_t at_max = ( proxy.mask >> ( bit_count - 1 ) ) & 1;
			proxy.storage += at_max;
			proxy.mask = rotl( proxy.mask, 1 );
			return *this;
		}
		constexpr bool_iterator& operator--()
		{
			size_t at_max = proxy.mask & 1;
			proxy.storage -= at_max;
			proxy.mask = rotr( proxy.mask, 1 );
			return *this;
		}
		constexpr bool_iterator& operator+=( int64_t diff )
		{
			proxy.storage += ( msb( proxy.mask ) + diff ) / bit_count;
			proxy.mask = rotl( proxy.mask, diff );
			return *this;
		}
		constexpr bool_iterator& operator-=( int64_t diff )
		{
			proxy.storage -= ( diff + 63 - msb( proxy.mask ) ) / bit_count;
			proxy.mask = rotr( proxy.mask, diff );
			return *this;
		}
		constexpr bool_iterator operator+( ptrdiff_t d ) { auto s = *this; s += d; return s; }
		constexpr bool_iterator operator-( ptrdiff_t d ) { auto s = *this; s -= d; return s; }
		constexpr bool_iterator operator++( int ) { auto s = *this; operator++(); return s; }
		constexpr bool_iterator operator--( int ) { auto s = *this; operator--(); return s; }

		// Comparison against another iterator.
		//
		constexpr bool operator==( const bool_iterator& other ) const
		{ 
			return proxy.storage == other.proxy.storage && proxy.mask == other.proxy.mask; 
		}
		constexpr bool operator!=( const bool_iterator& other ) const
		{ 
			return proxy.storage != other.proxy.storage || proxy.mask != other.proxy.mask; 
		}
		constexpr bool operator<( const bool_iterator& other ) const
		{ 
			return proxy.storage < other.proxy.storage || ( proxy.storage == other.proxy.storage && proxy.mask < other.proxy.mask ); 
		}

		// Distance calculation.
		//
		constexpr ptrdiff_t operator-( const bool_iterator& other ) const
		{
			return ( ( proxy.storage - other.proxy.storage ) * bit_count ) + msb( proxy.mask ) - msb( other.proxy.mask );
		}

		// Dereferencing.
		//
		constexpr const bool_proxy<Ptr>& operator*() const { return proxy; }
		constexpr const bool_proxy<Ptr>* operator->() const { return &proxy; }
	};
	template<Pointer Ptr> bool_iterator( bool_proxy<Ptr> )->bool_iterator<Ptr>;

	// Declares a bitmap of size N with fast search helpers.
	//
	template<size_t N>
	struct bitmap
	{
		// Container traits.
		//
		using value_type =             bool;
		using iterator =               bool_iterator<uint64_t*>;
		using const_iterator =         bool_iterator<const uint64_t*>;
		using reverse_iterator =       std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		// Declare invalid iterator and block count.
		//
		static constexpr size_t npos = bit_npos;
		static constexpr size_t block_count = ( N + 63 ) / 64;

		// Store the bits, initialized to zero.
		//
		std::array<uint64_t, block_count> blocks = { 0 };

		// Default construction / copy / move.
		//
		constexpr bitmap() = default;
		constexpr bitmap( bitmap&& ) noexcept = default;
		constexpr bitmap( const bitmap& ) = default;
		constexpr bitmap& operator=( bitmap&& ) noexcept = default;
		constexpr bitmap& operator=( const bitmap& ) = default;

		// Container interface.
		//
		constexpr bool empty() const { return N == 0; }
		constexpr size_t size() const { return N; }
		constexpr iterator begin() { return { bool_proxy<uint64_t*>{ &blocks[ 0 ], 1ull } }; }
		constexpr iterator end() { return { bool_proxy<uint64_t*>{ &blocks[ N / 64 ], 1ull << ( N & 63 ) } }; }
		constexpr const_iterator begin() const { return { bool_proxy<const uint64_t*>{ &blocks[ 0 ], 1ull } }; }
		constexpr const_iterator end() const { return { bool_proxy<const uint64_t*>{ &blocks[ N / 64 ], 1ull << ( N & 63 ) } }; }
		constexpr reverse_iterator rbegin() { return std::reverse_iterator{ end() }; }
		constexpr reverse_iterator rend() { return std::reverse_iterator{ begin() }; }
		constexpr const_reverse_iterator rbegin() const { return std::reverse_iterator{ end() }; }
		constexpr const_reverse_iterator rend() const { return std::reverse_iterator{ begin() }; }

		// Gets the value of the Nth bit.
		//
		constexpr bool_proxy<uint64_t*> at( size_t n ) { return *( begin() + n ); }
		constexpr bool_proxy<const uint64_t*> at( size_t n ) const { return *( begin() + n ); }
		constexpr bool_proxy<uint64_t*> operator[]( size_t n ) { return *( begin() + n ); }
		constexpr bool_proxy<const uint64_t*> operator[]( size_t n ) const { return *( begin() + n ); }

		// Sets the value of the Nth bit.
		//
		constexpr bool set( size_t n, bool v )
		{
			dassert( n < N );
			if ( v ) return bit_set( &blocks[ n / 64 ], n & 63 );
			else     return bit_reset( &blocks[ n / 64 ], n & 63 );
		}
	};
};

// Override std::find to bit scan.
//
namespace std
{
	template<xstd::Pointer Ptr>
	xstd::bool_iterator<Ptr> find( const xstd::bool_iterator<Ptr>& first, const xstd::bool_iterator<Ptr>& last, bool value ) noexcept
	{
		if ( first == last ) return last;

		// Handle first with .mask != 1 specially.
		//
		xstd::bool_iterator<Ptr> it = first;
		if ( first.proxy.mask != 1 )
		{
			auto fval = *first.proxy.storage & ~first.proxy.mask;
			size_t fidx = xstd::lsb( fval );

			if( first.proxy.storage == last.proxy.storage && ( ( 1ull << fidx ) >= last.proxy.mask ) )
			{
				return last;
			}
			else if ( fidx == xstd::bit_npos )
			{
				it.proxy.storage++;
				it.proxy.mask = 1ull;
			}
			else
			{

				it.proxy.mask = 1ull << fidx;
				return it;
			}
		}

		// Invoke find bit.
		//
		size_t index = xstd::bit_find( it.proxy.storage, last.proxy.storage, value, false );

		// Adjust the result for overflow and return.
		//
		if ( index == xstd::bit_npos )
			return last;
		it += index;
		if ( it.proxy.storage > last.proxy.storage || ( it.proxy.storage == last.proxy.storage && it.proxy.mask >= last.proxy.mask ) )
			return last;
		return it;
	}
};