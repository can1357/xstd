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
		small_vector( size_t n ) requires DefaultConstructible<T>
		{
			resize( n );
		}
		small_vector( size_t n, const T& value )
		{
			resize( n, value );
		}

		// Implement copy/move construction and assignment.
		//
		small_vector( small_vector&& o ) noexcept
		{
			length = std::exchange( o.length, 0 );
			std::uninitialized_move_n( o.begin(), length, begin() );
		}
		small_vector( const small_vector& o ) requires CopyConstructible<T>
		{
			length = o.length;
			std::uninitialized_copy_n( o.begin(), length, begin() );
		}
		small_vector& operator=( small_vector&& o ) noexcept
		{
			clear();
			length = std::exchange( o.length, 0 );
			std::uninitialized_move_n( o.begin(), length, begin() );
			return *this;
		}
		small_vector& operator=( const small_vector& o ) requires CopyConstructible<T>
		{
			clear();
			length = o.length;
			std::uninitialized_copy_n( o.begin(), length, begin() );
			return *this;
		}
		void swap( small_vector& o )
		{
			if constexpr ( TriviallyMoveConstructible<T> )
				return trivial_swap( *this, o );

			std::swap( length, o.length );
			size_t n = 0;
			for ( ; n != std::min<size_t>( length, o.length ); n++ )
				std::swap( at( n ), o.at( n ) );
			if ( n < length )
				std::uninitialized_move( o.begin() + n, o.end(), begin() + n );
			else if ( n < o.length )
				std::uninitialized_move( begin() + n, end(), o.begin() + n );
		}

		// Value removal.
		//
		iterator erase( const_iterator first, const_iterator last )
		{
			size_t count = ( size_type ) std::distance( first, last );
			std::destroy_n( first, count );
			std::uninitialized_move( ( iterator ) last, end(), ( iterator ) first );
			length -= count;
			return ( iterator ) first;
		}
		iterator erase( const_iterator pos )
		{
			return erase( pos, pos + 1 );
		}
		void pop_back()
		{
			std::destroy_at( &back() );
			--length;
		}
		void clear()
		{
			std::destroy_n( begin(), std::exchange( length, 0 ) );
		}

		// Value insertion.
		//
		template<typename... Tx>
		reference emplace( const_iterator pos, Tx&&... args )
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
		iterator insert( const_iterator pos, It1&& first, It2&& last )
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
		iterator insert( const_iterator pos, const T& value )
		{
			return &emplace( pos, value );
		}
		iterator insert( const_iterator pos, T&& value )
		{
			return &emplace( pos, std::move( value ) );
		}
		bool push_back( const T& value )
		{
			if ( length == N )
				return false;
			emplace( end(), value );
			return true;
		}
		bool push_back( T&& value )
		{
			if ( length == N )
				return false;
			emplace( end(), std::move( value ) );
			return true;
		}
		template<typename... Tx>
		reference emplace_back( Tx&&... args )
		{
			return emplace( end(), std::forward<Tx>( args )... );
		}
		void resize( size_t n ) requires DefaultConstructible<T>
		{
			if ( n > length )
				std::uninitialized_default_construct( end(), begin() + n );
			else
				std::destroy_n( begin() + n, length - n );
			length = ( size_type ) n;
		}
		void resize( size_t n, const T& value )
		{
			if ( n > length )
				std::uninitialized_fill_n( begin() + length, n - length, value );
			else
				std::destroy_n( begin() + n, length - n );
			length = ( size_type ) n;
		}

		// Value assignment.
		//
		template<typename It1, typename It2> requires ( ConvertibleIterator<T, It1> && ConvertibleIterator<T, It2> )
		iterator assign( It1&& first, It2&& last )
		{
			clear();
			return insert( begin(), std::forward<It1>( first ), std::forward<It2>( last ) );
		}

		// Clear the vector at destruction.
		//
		~small_vector() { clear(); }

		// Container interface.
		//
		constexpr bool empty() const { return length == 0; }
		constexpr size_t size() const { assume( length <= N ); return length; }
		constexpr size_t max_size() const { return N; }
		constexpr size_t capacity() const { return N; }
		iterator data() { return &at( 0 ); }
		const_iterator data() const { return &at( 0 ); }

		iterator begin() { return data(); }
		const_iterator begin() const { return data(); }
		const_iterator cbegin() const { return data(); }

		iterator end() { return data() + size(); }
		const_iterator end() const { return data() + size(); }
		const_iterator cend() const { return data() + size(); }

		reverse_iterator rbegin() { return std::reverse_iterator{ end() }; }
		const_reverse_iterator rbegin() const { return std::reverse_iterator{ end() }; }
		const_reverse_iterator crbegin() const { return std::reverse_iterator{ end() }; }

		reverse_iterator rend() { return std::reverse_iterator{ begin() }; }
		const_reverse_iterator rend() const { return std::reverse_iterator{ begin() }; }
		const_reverse_iterator crend() const { return std::reverse_iterator{ begin() }; }

		// No-ops.
		//
		constexpr void shrink_to_fit() {}
		constexpr void reserve( size_t ) {}

		// Decay to vector.
		//
		constexpr operator std::vector<T>() requires DefaultConstructible<T> { return { begin(), end() }; }

		// Indexing.
		//
		reference at( size_t n ) { return ( ( value_type* ) space )[ n ]; }
		const_reference at( size_t n ) const { return ( ( const value_type* ) space )[ n ]; }
		reference front() { return at( 0 ); }
		const_reference front() const { return at( 0 ); }
		reference back() { return at( length - 1 ); }
		const_reference back() const { return at( length - 1 ); }
		reference operator[]( size_t n ) { return at( n ); }
		const_reference operator[]( size_t n ) const { return at( n ); }

		// Comparison.
		//
		template<Iterable C>
		bool operator==( const C& c ) const
		{
			if ( std::size( c ) != size() )
				return false;
			return std::equal( std::begin( c ), std::end( c ), begin() );
		}
		template<Iterable C> bool operator!=( const C& c ) const { return !operator==( c ); }
	};

	// If trivial type, implement the constexpr version.
	//
	template<typename T, size_t N> requires ( DefaultConstructible<T> && TriviallyDestructable<T> )
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
		constexpr small_vector( small_vector&& o ) noexcept
		{
			length = std::exchange( o.length, 0 );
			std::copy_n( std::make_move_iterator( o.begin() ), length, begin() );
		}
		constexpr small_vector( const small_vector& o )
		{
			length = o.length;
			std::copy_n( o.begin(), length, begin() );
		}
		constexpr small_vector& operator=( small_vector&& o ) noexcept
		{
			clear();
			length = std::exchange( o.length, 0 );
			std::copy_n( std::make_move_iterator( o.begin() ), length, begin() );
			return *this;
		}
		constexpr small_vector& operator=( const small_vector& o )
		{
			clear();
			length = o.length;
			std::copy_n( o.begin(), length, begin() );
			return *this;
		}
		constexpr void swap( small_vector& o )
		{
			if constexpr ( TriviallyMoveConstructible<T> )
				if ( !std::is_constant_evaluated() )
					return trivial_swap( *this, o );

			std::swap( length, o.length );
			size_t n = 0;
			for ( ; n != std::min<size_t>( length, o.length ); n++ )
				std::swap( at( n ), o.at( n ) );
			if ( n < length )
				std::copy( std::make_move_iterator( o.begin() + n ), std::make_move_iterator( o.end() ), begin() + n );
			else if ( n < o.length )
				std::copy( std::make_move_iterator( begin() + n ), std::make_move_iterator( end() ), o.begin() + n );
		}

		// Value removal.
		//
		constexpr iterator erase( const_iterator _first, const_iterator _last )
		{
			iterator first = &space[ _first - begin() ];
			iterator last = &space[ _last - begin() ];

			size_t count = ( size_type ) std::distance( first, last );
			std::copy( std::make_move_iterator( last ), std::make_move_iterator( end() ), first );
			length -= count;
			return first;
		}
		constexpr iterator erase( const_iterator pos )
		{
			return erase( pos, pos + 1 );
		}
		constexpr void pop_back()
		{
			--length;
		}
		constexpr void clear()
		{
			length = 0;
		}

		// Value insertion.
		//
		template<typename... Tx>
		constexpr reference emplace( const_iterator _pos, Tx&&... args )
		{
			iterator pos = &space[ _pos - cbegin() ];
			if ( length )
			{
				for ( auto it = end() - 1; it >= pos; --it )
					it[ 1 ] = it[ 0 ];
			}
			*pos = T( std::forward<Tx>( args )... );
			length++;
			return *pos;
		}
		template<typename It1, typename It2> requires ( ConvertibleIterator<T, It1> && ConvertibleIterator<T, It2> )
		constexpr iterator insert( const_iterator _pos, It1&& first, It2&& last )
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
		constexpr iterator insert( const_iterator pos, const T& value )
		{
			return &emplace( pos, value );
		}
		constexpr iterator insert( const_iterator pos, T&& value )
		{
			return &emplace( pos, std::move( value ) );
		}
		constexpr bool push_back( const T& value )
		{
			if ( length == N )
				return false;
			emplace( end(), value );
			return true;
		}
		constexpr bool push_back( T&& value )
		{
			if ( length == N )
				return false;
			emplace( end(), std::move( value ) );
			return true;
		}
		template<typename... Tx>
		constexpr reference emplace_back( Tx&&... args )
		{
			return emplace( end(), std::forward<Tx>( args )... );
		}
		constexpr void resize( size_t n, const T& value = {} )
		{
			if ( n > length )
				std::fill_n( begin() + length, n - length, value );
			length = ( size_type ) n;
		}

		// Value assignment.
		//
		template<typename It1, typename It2> requires ( ConvertibleIterator<T, It1> && ConvertibleIterator<T, It2> )
		constexpr iterator assign( It1&& first, It2&& last )
		{
			clear();
			return insert( begin(), std::forward<It1>( first ), std::forward<It2>( last ) );
		}


		// Container interface.
		//
		constexpr bool empty() const { return length == 0; }
		constexpr size_t size() const { assume( length <= N ); return length; }
		constexpr size_t max_size() const { return N; }
		constexpr size_t capacity() const { return N; }
		constexpr iterator data() { return &at( 0 ); }
		constexpr const_iterator data() const { return &at( 0 ); }

		constexpr iterator begin() { return data(); }
		constexpr const_iterator begin() const { return data(); }
		constexpr const_iterator cbegin() const { return data(); }

		constexpr iterator end() { return data() + size(); }
		constexpr const_iterator end() const { return data() + size(); }
		constexpr const_iterator cend() const { return data() + size(); }

		constexpr reverse_iterator rbegin() { return std::reverse_iterator{ end() }; }
		constexpr const_reverse_iterator rbegin() const { return std::reverse_iterator{ end() }; }
		constexpr const_reverse_iterator crbegin() const { return std::reverse_iterator{ end() }; }

		constexpr reverse_iterator rend() { return std::reverse_iterator{ begin() }; }
		constexpr const_reverse_iterator rend() const { return std::reverse_iterator{ begin() }; }
		constexpr const_reverse_iterator crend() const { return std::reverse_iterator{ begin() }; }

		// No-ops.
		//
		constexpr void shrink_to_fit() {}
		constexpr void reserve( size_t ) {}

		// Decay to vector.
		//
		operator std::vector<T>() requires DefaultConstructible<T> { return { begin(), end() }; }

		// Indexing.
		//
		constexpr reference at( size_t n ) { return space[ n ]; }
		constexpr const_reference at( size_t n ) const { return space[ n ]; }
		constexpr reference front() { return at( 0 ); }
		constexpr const_reference front() const { return at( 0 ); }
		constexpr reference back() { return at( length - 1 ); }
		constexpr const_reference back() const { return at( length - 1 ); }
		constexpr reference operator[]( size_t n ) { return at( n ); }
		constexpr const_reference operator[]( size_t n ) const { return at( n ); }

		// Comparison.
		//
		template<Iterable C>
		constexpr bool operator==( const C& c ) const
		{
			if ( std::size( c ) != size() )
				return false;
			return std::equal( std::begin( c ), std::end( c ), begin() );
		}
		template<Iterable C> constexpr bool operator!=( const C& c ) const { return !operator==( c ); }
	};
};