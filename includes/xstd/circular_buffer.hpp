#pragma once
#include <memory>
#include <atomic>
#include <functional>
#include "type_helpers.hpp"
#include "spinlock.hpp"

namespace xstd
{
	// Implements a generic circular buffer for arbitrary length data 
	// that can be consumed and produced concurrently.
	//
	template<size_t Len = 2_mb>
	struct circular_buffer
	{
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
			size_t operator-( const basic_iterator& other ) const { return ( position % N ) - ( other.at % N ); }
			bool operator<( const basic_iterator& other ) const { return ( position % N ) < ( other.at % N ); }
			bool operator<=( const basic_iterator& other ) const { return ( position % N ) <= ( other.at % N ); }
			bool operator>( const basic_iterator& other ) const { return ( position % N ) > ( other.at % N ); }
			bool operator>=( const basic_iterator& other ) const { return ( position % N ) >= ( other.at % N ); }
			bool operator==( const basic_iterator& other ) const { return ( position % N ) == ( other.at % N ); }
			bool operator!=( const basic_iterator& other ) const { return ( position % N ) != ( other.at % N ); }
		
			// Redirect dereferencing to the buffer.
			//
			pointer operator->() const { return &buffer[ position % N ]; }
			reference operator*() const { return buffer[ position % N ]; }
			operator reference() const { return buffer[ position % N ]; }
		};
		using iterator = basic_iterator<false>;
		using const_iterator = basic_iterator<true>;

		// Enforce even length.
		//
		static constexpr size_t N = ( Len + 1 ) & ~1;

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

		// Asymmetric queue locks.
		//
		xstd::spinlock        producer_lock = {};
		xstd::spinlock        consumer_lock = {};

		// Reserves a number of bytes in the buffer, returns with the producer lock held.
		// - If called with spin = false, will return -1 if the buffer is full.
		//
		size_t reserve( size_t count, bool spin = true )
		{
			if ( count > ( N - 1 ) )
				return std::string::npos;

			// Acquire the producer lock.
			//
			std::unique_lock lock{ producer_lock };
			
			// Load the intial number of bytes produced.
			//
			size_t lp = producer_tail.load();
			while ( true )
			{
				// Load the last consumed byte, calculate the free spaces left.
				//
				size_t lc = consumer_head.load();
				size_t free = ( lc - lp ) % N;

				// If there is enough space:
				//
				if ( free >= count )
				{
					// Leak the lock, caller should unlock after writing and incrementing the tail.
					//
					lock.release();

					// Return the current position.
					//
					return lp;
				}
				
				// If we're asked not to spin, fail.
				//
				if ( !spin )
					return std::string::npos;
				
				// Yield the cpu and try again.
				//
				yield_cpu();
				lp = producer_tail.load();
			}
		}

		// Commits a number of bytes to the previously reserved region, caller must hold a
		// lock which will be released when the call completes.
		//
		void commit( size_t pos, size_t count )
		{
			// Update the producer tail and release the lock.
			//
			producer_tail = ( pos + count ) % N;
			producer_lock.unlock();
		}
		void commit( size_t pos, const void* data, size_t count )
		{
			// Write the data.
			//
			auto* it = ( const uint8_t* ) data;
			auto* end = &it[ count ];
			while ( it != end )
				raw_data[ pos++ % N ] = *it++;
			return commit( pos, 0 );
		}

		// Peeks into the consumer queue and returns the position and the number of bytes,
		// returns with the consumer lock held; in the event that there is no data, will return
		// an invalid position and there is no need to call consume or unlock the lock.
		//
		std::pair<size_t, size_t> peek()
		{
			consumer_lock.lock();

			size_t lp = producer_tail.load();
			size_t lc = consumer_head.load();
			size_t length = ( lp - lc - 1 ) % N;
			if ( !length )
			{
				consumer_lock.unlock();
				return { std::string::npos, 0 };
			}
			return { lc + 1, length };
		}

		// Consumes a number of bytes at a given position previously peeked, caller must hold a 
		// lock which will be released when the call completes.
		//
		void consume( size_t pos, size_t count )
		{
			// Update the consumer head, return the number of bytes consumed.
			//
			consumer_head = ( pos - 1 + count ) % N;
			consumer_lock.unlock();
		}

		// Reads a byte from a given position.
		//
		uint8_t& at( size_t pos ) { return raw_data[ pos % N ]; }
		const uint8_t& at( size_t pos ) const { return raw_data[ pos % N ]; }
		uint8_t& operator[]( size_t pos ) { return at( pos ); }
		const uint8_t& operator[]( size_t pos ) const { return at( pos ); }

		// Returns the number of empty / filled slots.
		//
		size_t capacity() const { return ( consumer_head - producer_tail ) % N; }
		size_t length() const { return ( producer_tail - consumer_head - 1 ) % N; }

		// ::begin and ::size refer to the buffer itself ignoring the queue state.
		//
		iterator begin() { return { &raw_data[ 0 ], 0 }; }
		const_iterator begin() const { return { &raw_data[ 0 ], 0 }; }
		auto rbegin() { return std::make_reverse_iterator( begin() ); }
		auto rbegin() const { return std::make_reverse_iterator( begin() ); }

		constexpr size_t size() const { return N - 1; }

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
			
			auto* out = ( uint8_t* ) data;
			for( size_t it = 0; it != length; it++ )
				*out++ = raw_data[ ( pos + it ) % N ];
			
			consume( pos, length );
			return length;
		}
	};
}