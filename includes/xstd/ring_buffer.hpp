#pragma once
#include <memory>
#include <array>
#include <algorithm>
#include "type_helpers.hpp"

namespace xstd
{
	// Implements a ring buffer that can store a history of T upto N times and starts
	// overwriting entries after that.
	//
	template<typename T, size_t N>
	struct ring_buffer
	{
		using storage_t = std::aligned_storage_t<sizeof( T ), alignof( T )>;

		// Underlying buffer and the push counter.
		//
		std::array<storage_t, N> buffer = {};
		uint32_t                 push_count = 0;

		// Default constructed.
		//
		ring_buffer() = default;

		// Implement copy and move.
		//
		ring_buffer( const ring_buffer& o ) { assign( o ); }
		ring_buffer& operator=( const ring_buffer& o ) { assign( o ); return *this; }
		ring_buffer( ring_buffer&& o ) noexcept { swap( o ); }
		ring_buffer& operator=( ring_buffer&& o ) noexcept { swap( o ); return *this; }

		// Copies the data from another ring buffer.
		//
		void assign( ring_buffer& o )
		{
			clear();
			size_t n = o.size();
			for ( size_t i = 0; i != n; i++ )
				emplace_back( o[ n - ( i + 1 ) ] );
		}

		// Swaps two ring buffers.
		//
		void swap( ring_buffer& o )
		{
			// Sort it so that b is always smaller.
			//
			ring_buffer* a = this;
			ring_buffer* b = &o;
			if ( b->push_count > a->push_count )
				std::swap( b, a );

			// Swap entries upto b's length.
			//
			T* a_0 = ( T* ) &a->buffer[ n ];
			T* b_0 = ( T* ) &b->buffer[ n ];
			size_t a_n = a->size();
			size_t b_n = b->size();
			std::swap_ranges( a_0, a_0 + b_n, b_0 );

			// Uninitialized move the rest.
			//
			std::uninitialized_move_n( a_0 + b_n, a_n - b_n, b_0 + b_n );
			std::destroy_n( a_0 + b_n, a_n - b_n );

			// Finally swap the length.
			//
			std::swap( a->push_count, b->push_count );
		}

		// Clears all entries.
		//
		void clear()
		{
			std::destroy_n( ( T* ) &buffer[ 0 ], size() );
			push_count = 0;
		}

		// Pushes an entry.
		//
		template<typename... Tx>
		T& emplace_back( Tx&&... args )
		{
			size_t id = push_count++;
			T* slot = ( T* ) &buffer[ id % N ];

			if ( id >= N )
				std::destroy_at( slot );
			return *std::construct_at( slot, std::forward<Tx>( args )... );
		}
		void push_back( T&& value ) { emplace_back<T>( std::move( value ) ); }
		void push_back( const T& value ) { emplace_back( value ); }

		// Gets the number of entries we can read.
		//
		size_t size() const { return std::min<size_t>( push_count, N ); }

		// Usual observers.
		// - Note: the range is unordered!
		//
		bool empty() const { return push_count == 0; }
		constexpr size_t capacity() const { return N; }

		T* data() { return ( T* ) buffer.data(); }
		T* begin() { return data(); }
		T* end() { return data() + size(); }
		const T* data() const { return ( T* ) buffer.data(); }
		const T* begin() const { return data(); }
		const T* end() const { return data() + size(); }
		
		// References an entry in the buffer. 
		//  0 = last entry, 1 = previous entry and so on.
		//
		T& at( size_t n ) & { return *( T* ) &buffer[ ( push_count - ( n + 1 ) ) % N ]; }
		T&& at( size_t n ) && { return std::move( *( T* ) &buffer[ ( push_count - ( n + 1 ) ) % N ] ); }
		const T& at( size_t n ) const& { return *( const T* ) &buffer[ ( push_count - ( n + 1 ) ) % N ]; }
		T& operator[]( size_t n ) & { return at( n ); }
		T&& operator[]( size_t n ) && { return at( n ); }
		const T& operator[]( size_t n ) const& { return at( n ); }

		// Destroy all entries on destruction.
		//
		~ring_buffer() { clear(); }
	};

	// Same as above but simpler and constexpr for trivial types.
	//
	template<TriviallyCopyable T, size_t N>
	struct ring_buffer<T, N>
	{
		// Underlying buffer and the push counter.
		//
		std::array<T, N> buffer = {};
		uint32_t         push_count = 0;

		// Copies the data from another ring buffer.
		//
		constexpr void assign( ring_buffer& o )
		{
			buffer = o.buffer;
			push_count = o.push_count;
		}

		// Swaps two ring buffers.
		//
		constexpr void swap( ring_buffer& o )
		{
			std::swap( buffer, o.buffer );
			std::swap( push_count, o.push_count );
		}

		// Clears all entries.
		//
		constexpr void clear()
		{
			push_count = 0;
		}

		// Pushes an entry.
		//
		template<typename... Tx>
		constexpr T& emplace_back( Tx&&... args )
		{
			size_t id = push_count++;
			T& slot = *( T* ) &buffer[ id % N ];
			slot = T( std::forward<Tx>( args )... );
			return slot;
		}
		constexpr void push_back( T&& value ) { emplace_back<T>( std::move( value ) ); }
		constexpr void push_back( const T& value ) { emplace_back( value ); }

		// Gets the number of entries we can read.
		//
		constexpr size_t size() const { return std::min<size_t>( push_count, N ); }

		// Usual observers.
		// - Note: the range is unordered!
		//
		constexpr bool empty() const { return push_count == 0; }
		constexpr size_t capacity() const { return N; }

		constexpr T* data() { return buffer.data(); }
		constexpr T* begin() { return data(); }
		constexpr T* end() { return data() + size(); }
		constexpr const T* data() const { return buffer.data(); }
		constexpr const T* begin() const { return data(); }
		constexpr const T* end() const { return data() + size(); }
		
		// References an entry in the buffer. 
		//  0 = last entry, 1 = previous entry and so on.
		//
		constexpr T& at( size_t n ) { return buffer[ ( push_count - ( n + 1 ) ) % N ]; }
		constexpr const T& at( size_t n ) const { return buffer[ ( push_count - ( n + 1 ) ) % N ]; }
		constexpr T& operator[]( size_t n ) { return at( n ); }
		constexpr const T& operator[]( size_t n ) const { return at( n ); }
	};
}