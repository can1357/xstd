#pragma once
#include <initializer_list>
#include <iterator>
#include <memory>
#include <vector>
#include "type_helpers.hpp"

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
		using size_type =              integral_shrink_t<N>;
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
		template<typename It1, typename It2> requires ( ConvertibleIterator<T, It1> && ConvertibleIterator<T, It2> )
		small_vector( It1&& first, It2&& last )
		{
			length = ( size_type ) std::distance( first, last );
			std::uninitialized_copy( first, last, begin() );
		}
		template<Iterable C = std::initializer_list<T>>
		small_vector( const C& source ) : small_vector( std::begin( source ), std::end( source ) ) {}
		
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
		inline small_vector( const small_vector& o ) requires CopyConstructable<T>
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
		inline small_vector& operator=( const small_vector& o ) requires CopyConstructable<T>
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
			size_t count = ( size_type ) std::distance( first, last );
			std::destroy_n( first, count );
			std::uninitialized_move( ( iterator ) last, end(), ( iterator ) first );
			length -= count;
			return ( iterator ) first;
		}
		inline iterator erase( const_iterator pos )
		{
			return erase( pos, pos + 1 );
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
			if ( length )
			{
				for ( auto it = end() - 1; it >= pos; --it )
					std::uninitialized_move_n( it, 1, it + 1 );
			}
			std::construct_at( ( iterator ) pos, std::forward<Tx>( args )... );
			length++;
			return *( iterator ) pos;
		}
		template<typename It1, typename It2> requires ( ConvertibleIterator<T, It1> && ConvertibleIterator<T, It2> )
		inline iterator insert( const_iterator pos, It1&& first, It2&& last )
		{
			auto count = ( size_type ) std::distance( first, last );
			if ( ( length + count ) > N )
				return nullptr;
			if ( length )
			{
				for ( auto it = end() - 1; it >= pos; --it )
					std::uninitialized_move_n( it, 1, it + count );
			}
			std::uninitialized_copy( first, last, ( iterator ) pos );
			length += count;
			return ( iterator ) pos;
		}
		inline iterator insert( const_iterator pos, const T& value )
		{
			return insert( pos, &value, std::next( &value ) );
		}
		inline iterator insert( const_iterator pos, T&& value )
		{
			return &emplace( pos, std::move( value ) );
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
			if ( n > length )
				std::uninitialized_default_construct( end(), begin() + n );
			else
				std::destroy_n( begin() + n, length - n );
			length = ( size_type ) n;
		}
		inline void resize( size_t n, const T& value )
		{
			if ( n > length )
				std::uninitialized_fill_n( begin() + length, n - length, value );
			else
				std::destroy_n( begin() + n, length - n );
			length = ( size_type ) n;
		}

		// Clear the vector at destruction.
		//
		inline ~small_vector() { clear(); }

		// Container interface.
		//
		inline constexpr bool empty() const { return length == 0; }
		inline constexpr size_t size() const { return length; }
		inline constexpr size_t max_size() const { return N; }
		inline constexpr size_t capacity() const { return N; }
		inline iterator data() { return &at( 0 ); }
		inline const_iterator data() const { return &at( 0 ); }

		inline iterator begin() { return data(); }
		inline const_iterator begin() const { return data(); }
		inline const_iterator cbegin() const { return data(); }

		inline iterator end() { return data() + length; }
		inline const_iterator end() const { return data() + length; }
		inline const_iterator cend() const { return data() + length; }

		inline reverse_iterator rbegin() { return std::reverse_iterator{ end() }; }
		inline const_reverse_iterator rbegin() const { return std::reverse_iterator{ end() }; }
		inline const_reverse_iterator crbegin() const { return std::reverse_iterator{ end() }; }

		inline reverse_iterator rend() { return std::reverse_iterator{ begin() }; }
		inline const_reverse_iterator rend() const { return std::reverse_iterator{ begin() }; }
		inline const_reverse_iterator crend() const { return std::reverse_iterator{ begin() }; }

		// No-ops.
		//
		inline constexpr void shrink_to_fit() {}
		inline constexpr void reserve( size_t ) {}

		// Decay to vector.
		//
		constexpr operator std::vector<T>() requires DefaultConstructable<T> { return { begin(), end() }; }

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

		// Comparison.
		//
		template<Iterable C>
		inline bool operator==( const C& c ) const
		{
			if ( std::size( c ) != size() )
				return false;
			return std::equal( std::begin( c ), std::end( c ), begin() );
		}
		template<Iterable C> inline bool operator!=( const C& c ) const { return !operator==( c ); }
	};

	// If trivial type, implement the constexpr version.
	//
	template<typename T, size_t N> requires ( DefaultConstructable<T> && TriviallyDestructable<T> && TriviallyCopyAssignable<T> && TriviallyMoveAssignable<T> )
	struct small_vector<T, N>
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
		using size_type =              integral_shrink_t<N>;
		using difference_type =        std::make_signed_t<size_type>;

		// Raw space.
		//
		T space[ N ] = {};
		size_type length = 0;

		// Default construction.
		//
		constexpr small_vector() {}

		// Construction by value.
		//
		template<typename It1, typename It2> requires ( ConvertibleIterator<T, It1> && ConvertibleIterator<T, It2> )
		constexpr small_vector( It1&& first, It2&& last )
		{
			length = ( size_type ) std::distance( first, last );
			std::copy( first, last, begin() );
		}
		template<Iterable C = std::initializer_list<T>>
		constexpr small_vector( const C& source ) : small_vector( std::begin( source ), std::end( source ) ) {}
		
		// Construction by length.
		//
		constexpr small_vector( size_t n )
		{
			resize( n );
		}
		constexpr small_vector( size_t n, const T& value )
		{
			resize( n, value );
		}

		// Implement copy/move construction and assignment.
		//
		inline constexpr small_vector( small_vector&& o ) noexcept
		{
			length = std::exchange( o.length, 0 );
			std::copy_n( std::make_move_iterator( o.begin() ), length, begin() );
		}
		inline constexpr small_vector( const small_vector& o )
		{
			length = o.length;
			std::copy_n( o.begin(), length, begin() );
		}
		inline constexpr small_vector& operator=( small_vector&& o ) noexcept
		{
			clear();
			length = std::exchange( o.length, 0 );
			std::copy_n( std::make_move_iterator( o.begin() ), length, begin() );
			return *this;
		}
		inline constexpr small_vector& operator=( const small_vector& o )
		{
			clear();
			length = o.length;
			std::copy_n( o.begin(), length, begin() );
			return *this;
		}
		inline constexpr void swap( small_vector& o )
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
					std::copy( std::make_move_iterator( o.begin() + n ), std::make_move_iterator( o.end() ), begin() + n );
				else if ( n < o.length )
					std::copy( std::make_move_iterator( begin() + n ), std::make_move_iterator( end() ), o.begin() + n );
			}
		}

		// Value removal.
		//
		inline constexpr iterator erase( const_iterator _first, const_iterator _last )
		{
			iterator first = &space[ _first - begin() ];
			iterator last = &space[ _last - begin() ];

			size_t count = ( size_type ) std::distance( first, last );
			std::copy( std::make_move_iterator( last ), std::make_move_iterator( end() ), first );
			length -= count;
			return first;
		}
		inline constexpr iterator erase( const_iterator pos )
		{
			return erase( pos, pos + 1 );
		}
		inline constexpr void pop_back()
		{
			--length;
		}
		inline constexpr void clear()
		{
			length = 0;
		}

		// Value insertion.
		//
		template<typename... Tx>
		inline constexpr reference emplace( const_iterator _pos, Tx&&... args )
		{
			iterator pos = &space[ _pos - cbegin() ];
			if ( length )
			{
				for ( auto it = end() - 1; it >= pos; --it )
					it[ 1 ] = it[ 0 ];
			}
			*pos = T{ std::forward<Tx>( args )... };
			length++;
			return *pos;
		}
		template<typename It1, typename It2> requires ( ConvertibleIterator<T, It1> && ConvertibleIterator<T, It2> )
		inline constexpr iterator insert( const_iterator _pos, It1&& first, It2&& last )
		{
			iterator pos = &space[ _pos - cbegin() ];

			auto count = ( size_type ) std::distance( first, last );
			if ( ( length + count ) > N )
				return nullptr;
			if ( length )
			{
				for ( auto it = end() - 1; it >= pos; --it )
					std::copy_n( it, 1, it + count );
			}
			std::copy( first, last, pos );
			length += count;
			return pos;
		}
		inline constexpr iterator insert( const_iterator pos, const T& value )
		{
			return emplace( pos, value );
		}
		inline constexpr iterator insert( const_iterator pos, T&& value )
		{
			return &emplace( pos, std::move( value ) );
		}
		inline constexpr void push_back( const T& value )
		{
			insert( end(), value );
		}
		template<typename... Tx>
		inline constexpr reference emplace_back( Tx&&... args )
		{
			return emplace( end(), std::forward<Tx>( args )... );
		}
		inline constexpr void resize( size_t n, const T& value = {} )
		{
			if ( n > length )
				std::fill_n( begin() + length, n - length, value );
			length = ( size_type ) n;
		}


		// Container interface.
		//
		inline constexpr bool empty() const { return length == 0; }
		inline constexpr size_t size() const { return length; }
		inline constexpr size_t max_size() const { return N; }
		inline constexpr size_t capacity() const { return N; }
		inline constexpr iterator data() { return &at( 0 ); }
		inline constexpr const_iterator data() const { return &at( 0 ); }

		inline constexpr iterator begin() { return data(); }
		inline constexpr const_iterator begin() const { return data(); }
		inline constexpr const_iterator cbegin() const { return data(); }

		inline constexpr iterator end() { return data() + length; }
		inline constexpr const_iterator end() const { return data() + length; }
		inline constexpr const_iterator cend() const { return data() + length; }

		inline constexpr reverse_iterator rbegin() { return std::reverse_iterator{ end() }; }
		inline constexpr const_reverse_iterator rbegin() const { return std::reverse_iterator{ end() }; }
		inline constexpr const_reverse_iterator crbegin() const { return std::reverse_iterator{ end() }; }

		inline constexpr reverse_iterator rend() { return std::reverse_iterator{ begin() }; }
		inline constexpr const_reverse_iterator rend() const { return std::reverse_iterator{ begin() }; }
		inline constexpr const_reverse_iterator crend() const { return std::reverse_iterator{ begin() }; }

		// No-ops.
		//
		inline constexpr void shrink_to_fit() {}
		inline constexpr void reserve( size_t ) {}

		// Decay to vector.
		//
		inline operator std::vector<T>() requires DefaultConstructable<T> { return { begin(), end() }; }

		// Indexing.
		//
		inline constexpr reference at( size_t n ) { return space[ n ]; }
		inline constexpr const_reference at( size_t n ) const { return space[ n ]; }
		inline constexpr reference front() { return at( 0 ); }
		inline constexpr const_reference front() const { return at( 0 ); }
		inline constexpr reference back() { return at( length - 1 ); }
		inline constexpr const_reference back() const { return at( length - 1 ); }
		inline constexpr reference operator[]( size_t n ) { return at( n ); }
		inline constexpr const_reference operator[]( size_t n ) const { return at( n ); }

		// Comparison.
		//
		template<Iterable C>
		inline constexpr bool operator==( const C& c ) const
		{
			if ( std::size( c ) != size() )
				return false;
			return std::equal( std::begin( c ), std::end( c ), begin() );
		}
		template<Iterable C> inline constexpr bool operator!=( const C& c ) const { return !operator==( c ); }
	};
};