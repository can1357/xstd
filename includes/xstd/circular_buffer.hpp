#pragma once
#include <memory>
#include <atomic>
#include <functional>
#include "type_helpers.hpp"

namespace xstd
{
	// Implements a generic circular buffer for arbitrary length data 
	// that can be consumed and produced concurrently.
	//
	template<size_t Len = 2_mb>
	struct circular_buffer
	{
		// Enforce even length.
		//
		static constexpr size_t N = ( Len + 1 ) & ~1;

		// Define an iterator.
		//
		template<bool C>
		struct basic_iterator
		{
			// Generic iterator typedefs.
			//
			using iterator_category = std::random_access_iterator_tag;
			using difference_type =   size_t;
			using value_type =        uint8_t;
			using reference =         make_const_if_t<C, uint8_t&>;
			using pointer =           make_const_if_t<C, uint8_t*>;

			// Buffer and the position within.
			//
			pointer buffer = nullptr;
			size_t position = 0;

			// Support random iteration.
			//
			basic_iterator& operator++() { position++; return *this; }
			basic_iterator& operator--() { position--; return *this; }
			basic_iterator operator++( int ) { auto s = *this; operator++(); return s; }
			basic_iterator operator--( int ) { auto s = *this; operator--(); return s; }
			basic_iterator& operator+=( size_t d ) { position += d; return *this; }
			basic_iterator& operator-=( size_t d ) { position -= d; return *this; }
			basic_iterator operator+( size_t d ) const { auto s = *this; s += d ; return s; }
			basic_iterator operator-( size_t d ) const { auto s = *this; s -= d ; return s; }

			// Comparison and difference against another iterator.
			//
			size_t operator-( const basic_iterator& other ) const { return ( position % N ) - ( other.position % N ); }
			bool operator<( const basic_iterator& other ) const { return ( position % N ) < ( other.position % N ); }
			bool operator<=( const basic_iterator& other ) const { return ( position % N ) <= ( other.position % N ); }
			bool operator>( const basic_iterator& other ) const { return ( position % N ) > ( other.position % N ); }
			bool operator>=( const basic_iterator& other ) const { return ( position % N ) >= ( other.position % N ); }
			bool operator==( const basic_iterator& other ) const { return ( position % N ) == ( other.position % N ); }
			bool operator!=( const basic_iterator& other ) const { return ( position % N ) != ( other.position % N ); }
		
			// Redirect dereferencing to the buffer.
			//
			pointer operator->() const { return &buffer[ position % N ]; }
			reference operator*() const { return buffer[ position % N ]; }
			operator reference() const { return buffer[ position % N ]; }
		};
		using iterator = basic_iterator<false>;
		using const_iterator = basic_iterator<true>;

		// Storage of the raw data.
		//
		std::unique_ptr<uint8_t[]> raw_data = std::make_unique<uint8_t[]>( N );

		// Producer queue.
		// - Tail = Position of the next byte that will be written. 
		//
		std::atomic<size_t> producer_tail = {};

		// Consumer queue.
		// - Head = Position of the last consumed byte.
		//
		std::atomic<size_t> consumer_head = N - 1;

		// Reserves a number of bytes in the buffer.
		// - If called with spin = false, will return -1 if the buffer is full.
		//
		size_t reserve( size_t count, bool spin = true )
		{
			if ( count > ( N - 1 ) )
				return std::string::npos;

			// Load the intial number of bytes produced.
			//
			size_t lp = producer_tail.load( std::memory_order::relaxed );
			while ( true )
			{
				// Load the last consumed byte, calculate the free spaces left.
				//
				size_t lc = consumer_head.load( std::memory_order::relaxed );
				size_t free = ( lc - lp ) % N;

				// If there is enough space, return the position.
				//
				if ( free >= count )
					return lp;
				
				// If we're asked not to spin, fail.
				//
				if ( !spin )
					return std::string::npos;
				
				// Yield the cpu and try again.
				//
				yield_cpu();
			}
		}

		// Commits a number of bytes to the previously reserved region.
		//
		void commit( size_t pos, size_t count )
		{
			producer_tail.store( ( pos + count ) % N, std::memory_order::release );
		}
		void commit( size_t pos, const void* data, size_t count )
		{
			write_raw( pos, data, count );
			return commit( pos, count );
		}

		// Peeks into the consumer queue and returns the position and the number of bytes.
		//
		std::pair<size_t, size_t> peek()
		{
			size_t lp = producer_tail.load( std::memory_order::relaxed );
			size_t lc = consumer_head.load( std::memory_order::relaxed );
			size_t length = ( lp - lc - 1 ) % N;
			return { lc + 1, length };
		}

		// Consumes a number of bytes at a given position previously peeked.
		//
		void consume( size_t pos, size_t count )
		{
			// Update the consumer head.
			//
			consumer_head.store( ( pos - 1 + count ) % N, std::memory_order::release );
		}

		// Reads a byte from a given position.
		//
		uint8_t& at( size_t pos ) { return raw_data[ pos % N ]; }
		const uint8_t& at( size_t pos ) const { return raw_data[ pos % N ]; }
		uint8_t& operator[]( size_t pos ) { return at( pos ); }
		const uint8_t& operator[]( size_t pos ) const { return at( pos ); }

		// Returns the number of empty / filled slots.
		//
		size_t current_capacity() const { return ( consumer_head - producer_tail ) % N; }
		size_t size() const { return ( producer_tail - consumer_head - 1 ) % N; }

		// ::begin and ::size refer to the buffer itself ignoring the queue state.
		//
		iterator begin() { return { &raw_data[ 0 ], 0 }; }
		const_iterator begin() const { return { &raw_data[ 0 ], 0 }; }
		auto rbegin() { return std::make_reverse_iterator( begin() ); }
		auto rbegin() const { return std::make_reverse_iterator( begin() ); }

		static constexpr size_t capacity() { return N - 1; }

		// Fast read and write primitives.
		//
		size_t read_raw( void* dst, size_t pos, size_t count ) const
		{
			dassert( count < N );

			// Normalize the iterator and calculate the ranges.
			//
			size_t it = pos % N;
			size_t copy_end = it + count;

			// Copy the data, split into two if it goes around the buffer.
			//
			size_t copy_1 = std::min( N, copy_end );
			memcpy( dst, &raw_data[ it ], copy_1 - it );
			if ( copy_1 != copy_end ) [[unlikely]]
				memcpy( xstd::ptr_at( dst, copy_1 - it ), &raw_data[ 0 ], copy_end -= N );
			return copy_end;
		}
		size_t write_raw( size_t pos, const void* src, size_t count )
		{
			dassert( count < N );

			// Normalize the iterator and calculate the ranges.
			//
			size_t it = pos % N;
			size_t copy_end = it + count;

			// Copy the data, split into two if it goes around the buffer.
			//
			size_t copy_1 = std::min( N, copy_end );
			memcpy( &raw_data[ it ], src, copy_1 - it );
			if ( copy_1 != copy_end ) [[unlikely]]
				memcpy( &raw_data[ 0 ], xstd::ptr_at( src, copy_1 - it ), copy_end -= N );
			return copy_end;
		}

		// -- Simplified interface.
		//

		// Places a number of bytes on the queue.
		// - If called with spin = false, will return false if the buffer is full.
		//
		bool write( const void* data, size_t count, bool spin = true )
		{
			if ( !count ) return true;
			size_t pos = reserve( count, true );
			if ( pos == std::string::npos )
				return false;
			commit( pos, data, count );
			return true;
		}

		// Consumes the queue with a limit on the number of bytes consumed.
		//
		size_t read( void* data, size_t max_length )
		{
			if ( !max_length ) return 0;

			auto [pos, length] = peek();
			if ( !length ) return 0;
			length = std::min( length, max_length );
			read_raw( data, pos, length );
			consume( pos, length );
			return length;
		}
	};
}