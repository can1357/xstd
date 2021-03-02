#pragma once
#include <initializer_list>
#include <iterator>
#include "type_helpers.hpp"
#include "assert.hpp"

// Implements a vector similar to stack_vector but with a friendlier copy implementation and more compact storage.
//
namespace xstd
{
	template<typename T, size_t N>
	struct small_vector
	{
		// Container traits.
		//
		using iterator =               T*;
		using const_iterator =         const T*;
		using pointer =                T*;
		using const_pointer =          const T*;
		using reference =              T&;
		using const_reference =        const T&;
		using value_type =             T;
		using reverse_iterator =       std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;
		using size_type =              integral_compress_t<N>;
		using difference_type =        std::make_signed_t<size_type>;

		// Raw space.
		//
		alignas( T ) uint8_t space[ sizeof( T[ N ] ) ] = {};
		size_type length = 0;

		// Default construction.
		//
		constexpr small_vector() {}

		// Construction by value.
		//
		small_vector( const std::initializer_list<T>& list )
		{
			insert( end(), list.begin(), list.end() );
		}
		template<typename It1, typename It2>
		small_vector( It1&& first, It2&& last )
		{
			insert( end(), std::forward<It1>( first ), std::forward<It1>( last ) );
		}

		// Construction by length.
		//
		small_vector( size_t n ) requires DefaultConstructable<T>
		{
			resize( n );
		}
		small_vector( size_t n, const T& value )
		{
			resize( n, value );
		}

		// Implement copy/move construction and assignment.
		//
		inline small_vector( small_vector&& o ) noexcept
		{
			swap( o );
		}
		inline small_vector( const small_vector& o )
		{
			insert( begin(), o.begin(), o.end() );
		}
		inline small_vector& operator=( small_vector&& o ) noexcept
		{
			swap( o );
			return *this;
		}
		inline small_vector& operator=( const small_vector& o )
		{
			clear();
			insert( begin(), o.begin(), o.end() );
			return *this;
		}
		inline void swap( small_vector& o )
		{
			size_t pmin = std::min( length, o.length );
			std::swap( length, o.length );
			
			size_t n = 0;
			for ( ; n != pmin; n++ )
				std::swap( at( n ), o.at( n ) );
			for ( ; n < length; n++ )
				new ( &at( n ) ) T( std::move( o.at( n ) ) );
			for ( ; n < o.length; n++ )
				new ( &o.at( n ) ) T( std::move( at( n ) ) );
		}

		// Value removal.
		//
		inline iterator erase( const_iterator first, const_iterator last )
		{
			for ( auto it = first; it != last; ++it )
				std::destroy_at( it );
			if ( last != end() )
			{
				size_t shift_count = last - first;
				for ( auto it = last; it != end(); ++it )
					new ( ( void* ) first++ ) T( std::move( *it ) );
			}
			length = first - &at( 0 );
			return ( iterator ) first;
		}
		inline iterator erase( const_iterator pos )
		{
			erase( pos, pos + 1 );
		}
		inline void pop_back()
		{
			std::destroy_at( &at( --length ) );
		}
		inline void clear()
		{
			for ( size_t n = 0; n != length; n++ )
				std::destroy_at( &at( n ) );
			length = 0;
		}

		// Value insertion.
		//
		template<typename... Tx>
		inline reference emplace( const_iterator pos, Tx&&... args )
		{
			++length;
			auto it = end();
			for ( ; it != pos; --it )
				new ( it ) T( std::move( *( it - 1 ) ) );
			return *new ( it ) T( std::forward<Tx>( args )... );
		}
		template<typename It1, typename It2>
		inline iterator insert( const_iterator pos, It1 first, const It2& last )
		{
			auto count = ( size_type ) std::distance( first, last );
			auto last_pos = pos + count;
			if ( ( length + count ) > N )
				return nullptr;
			length += count;
			for ( auto it = end(); it >= last_pos; --it )
				new ( it ) T( std::move( *( it - count ) ) );
			for ( auto it = pos; it != last_pos; ++it )
				new ( ( void* ) it ) T( *first++ );
			return ( iterator ) pos;
		}
		inline iterator insert( const_iterator pos, const T& value )
		{
			return insert( pos, &value, std::next( &value ) );
		}
		inline void push_back( const T& value )
		{
			insert( end(), value );
		}
		template<typename... Tx>
		inline reference emplace_back( Tx&&... args )
		{
			return emplace( end(), std::forward<Tx>( args )... );
		}
		inline void resize( size_t n ) requires DefaultConstructable<T>
		{
			fassert( n <= N );
			if ( n > length )
			{
				while( length != n )
					new ( &at( length++ ) ) T();
			}
			else
			{
				while ( length != n )
					new ( &at( --length ) ) T();
			}
		}
		inline void resize( size_t n, const T& value )
		{
			fassert( n <= N );
			if ( n > length )
			{
				while ( length != n )
					new ( &at( length++ ) ) T( value );
			}
			else
			{
				while ( length != n )
					new ( &at( --length ) ) T( value );
			}
		}

		// Clear the vector at destruction.
		//
		inline ~small_vector() { clear(); }

		// Container interface.
		//
		constexpr inline bool empty() const { return length == 0; }
		constexpr inline size_t size() const { return length; }
		constexpr inline size_t max_size() const { return N; }
		constexpr inline size_t capacity() const { return N; }
		inline iterator data() { return begin(); }
		inline const_iterator data() const { return begin(); }
		inline iterator begin() { return &at( 0 ); }
		inline const_iterator begin() const { return &at( 0 ); }
		inline iterator end() { return &at( length ); }
		inline const_iterator end() const { return &at( length ); }
		inline reverse_iterator rbegin() { return std::reverse_iterator{ end() }; }
		inline reverse_iterator rend() { return std::reverse_iterator{ begin() }; }
		inline const_reverse_iterator rbegin() const { return std::reverse_iterator{ end() }; }
		inline const_reverse_iterator rend() const { return std::reverse_iterator{ begin() }; }

		// Indexing.
		//
		inline reference at( size_t n ) { return ( ( value_type* ) space )[ n ]; }
		inline const_reference at( size_t n ) const { return ( ( const value_type* ) space )[ n ]; }
		inline reference front() { return at( 0 ); }
		inline const_reference front() const { return at( 0 ); }
		inline reference back() { return at( length - 1 ); }
		inline const_reference back() const { return at( length - 1 ); }
		inline reference operator[]( size_t n ) { return at( n ); }
		inline const_reference operator[]( size_t n ) const { return at( n ); }
	};
};