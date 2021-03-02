#pragma once
#include <initializer_list>
#include <iterator>
#include <memory>
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
			length = std::exchange( o.length, 0 );
			std::uninitialized_move_n( o.begin(), length, begin() );
		}
		inline small_vector( const small_vector& o ) requires std::is_copy_constructible_v<T>
		{
			length = o.length;
			std::uninitialized_copy_n( o.begin(), length, begin() );
		}
		inline small_vector& operator=( small_vector&& o ) noexcept
		{
			clear();
			length = std::exchange( o.length, 0 );
			std::uninitialized_move_n( o.begin(), length, begin() );
			return *this;
		}
		inline small_vector& operator=( const small_vector& o ) requires std::is_copy_constructible_v<T>
		{
			clear();
			length = o.length;
			std::uninitialized_copy_n( o.begin(), length, begin() );
			return *this;
		}
		inline void swap( small_vector& o )
		{
			if constexpr ( TriviallySwappable<T> )
			{
				std::swap( bytes( *this ), bytes( o ) );
			}
			else
			{
				std::swap( length, o.length );

				size_t n = 0;
				for ( ; n != std::min<size_t>( length, o.length ); n++ )
					std::swap( at( n ), o.at( n ) );
				if ( n < length )
					std::uninitialized_move( o.begin() + n, o.end(), begin() + n );
				else if ( n < o.length )
					std::uninitialized_move( begin() + n, end(), o.begin() + n );
			}
		}

		// Value removal.
		//
		inline iterator erase( const_iterator first, const_iterator last )
		{
			std::destroy_n( first, last - first );
			std::uninitialized_move( ( iterator ) last, end(), ( iterator ) first );
			length -= ( last - first );
			return ( iterator ) first;
		}
		inline iterator erase( const_iterator pos )
		{
			erase( pos, pos + 1 );
		}
		inline void pop_back()
		{
			std::destroy_at( &back() );
			--length;
		}
		inline void clear()
		{
			std::destroy_n( begin(), std::exchange( length, 0 ) );
		}

		// Value insertion.
		//
		template<typename... Tx>
		inline reference emplace( const_iterator pos, Tx&&... args )
		{
			std::uninitialized_move_n( end() - 1, 1, end() );
			reference& ref = *new ( end() ) T( std::forward<Tx>( args )... );
			length++;
			return ref;
		}
		template<typename It1, typename It2>
		inline iterator insert( const_iterator pos, It1 first, const It2& last )
		{
			fassert( ( length + 1 ) <= N );
			auto count = ( size_type ) std::distance( first, last );
			if ( ( length + count ) > N )
				return nullptr;
			for ( size_t i = 0; i != count; i++ )
				std::uninitialized_move_n( end() - i, 1, end() - i + 1 );
			std::uninitialized_copy( first, last, ( iterator ) pos );
			length += count;
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
				std::uninitialized_default_construct( end(), begin() + n );
			else
				std::destroy_n( begin() + n, length - n );
			length = n;
		}
		inline void resize( size_t n, const T& value )
		{
			fassert( n <= N );
			if ( n > length )
				std::uninitialized_fill_n( begin() + length, n - length, value );
			else
				std::destroy_n( begin() + n, length - n );
			length = n;
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
		inline iterator end() { return begin() + length; }
		inline const_iterator end() const { return begin() + length; }
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