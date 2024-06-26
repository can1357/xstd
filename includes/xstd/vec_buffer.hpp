#pragma once
#include "intrinsics.hpp"
#include "type_helpers.hpp"
#include "assert.hpp"

namespace xstd {
	// Utilities.
	//
	namespace detail {
		template<typename T>
		FORCE_INLINE static constexpr T* copy( T* dst, const T* src, size_t count ) {
			if ( const_condition( !count ) ) return dst;
			if ( std::is_constant_evaluated() )
				std::copy_n( src, count, dst );
			else
				::memcpy( dst, src, count * sizeof( T ) );
			return dst;
		}
		FORCE_INLINE static constexpr void fill( uint8_t* dst, uint8_t value, size_t count ) {
			if ( const_condition( !count ) ) return;
			if ( std::is_constant_evaluated() )
				std::fill_n( dst, count, value );
			else
				::memset( dst, value, count );
		}
		template<typename T>
		FORCE_INLINE static constexpr T* move( T* dst, const T* src, size_t count ) {
			if ( const_condition( !count ) ) return dst;
			if ( std::is_constant_evaluated() )
				std::copy_n( src, count, dst );
			else
				::memmove( dst, src, count * sizeof( T ) );
			return dst;
		}
		template<typename T, typename S = ptrdiff_t>
		FORCE_INLINE static constexpr T* shift_fwd( T* at, size_t count, S shift ) {
			return shift ? detail::move( at + shift, at, count ) : at;
		}
		template<typename T, typename S = ptrdiff_t>
		FORCE_INLINE static constexpr T* shift_bwd( T* at, size_t count, S shift ) {
			return detail::shift_fwd( at, count, -(ptrdiff_t) shift );
		}
		FORCE_INLINE static constexpr uint8_t* allocate( size_t count ) {
			uint8_t* result = nullptr;
			if ( std::is_constant_evaluated() ) {
				if ( count ) // This check is actually asserted by caller, only to prevent errors in compilers with no cxpr heap by introducing runtime condition.
					result = new uint8_t[ count ]();
			} else {
				// This check is actually asserted by caller, only optimizing codegen if known at compile time.
				if ( !const_condition( !count ) ) {
#ifdef XSTD_VECBUF_ALLOC
					result = (uint8_t*) XSTD_VECBUF_ALLOC( nullptr, count );
#else
					result = (uint8_t*) malloc( count );
#endif
				}
			}
			return result;
		}
		FORCE_INLINE static constexpr void deallocate( uint8_t* prev ) {
			if ( std::is_constant_evaluated() ) {
				if ( prev )
					delete[] prev;
			} else {
				if ( const_condition( prev == nullptr ) )
					return;
#ifdef XSTD_VECBUF_ALLOC
				if ( prev ) XSTD_VECBUF_ALLOC( prev, 0 );
#else
				free( prev );
#endif
			}
		}
		FORCE_INLINE static constexpr uint8_t* reallocate( uint8_t* prev, size_t size, size_t prev_size ) {
			// No-op if no size change.
			//
			if ( const_condition( size == prev_size ) ) {
				return prev;
			}

			// If constexpr:
			//
			uint8_t* result;
			if ( std::is_constant_evaluated() ) {
				result = allocate( size );
				if ( prev ) {
					if ( size != 0 )
						detail::copy( result, prev, std::min( size, prev_size ) );
				}
				deallocate( prev );
			} else {
#ifdef XSTD_VECBUF_ALLOC
				result = (uint8_t*) XSTD_VECBUF_ALLOC( prev, size );
#else
				result = (uint8_t*) realloc( prev, size );
#endif
			}
			return result;
		}
	};

	// Implement vector-like buffer.
	//
	struct TRIVIAL_ABI vec_buffer {
		// Storage and allocation details.
		//
	private:
		uint8_t* __restrict m_beg  = nullptr;  // Currently used limits.
		uint8_t*            m_end  = nullptr;  //
		uint8_t*            m_limit = nullptr; // Allocation limits.
		uint8_t*            m_base  = nullptr; //
		
		// Returns the number of bytes that we're currently using.
		FORCE_INLINE constexpr size_t mm_active() const { return m_end - m_beg; }

		// Returns the allocation limits.
		FORCE_INLINE constexpr uint8_t* mm_base() const { return m_base; }
		FORCE_INLINE constexpr uint8_t* mm_limit() const { return m_limit; }

		// Returns the number of bytes after [m_end] that can be used.
		FORCE_INLINE constexpr size_t mm_reserved() const { return mm_limit() - m_end; }
		// Returns the number of bytes after [m_base] that can be used.
		FORCE_INLINE constexpr size_t mm_capacity() const { return mm_limit() - mm_base(); }
		// Returns the number of bytes before [m_beg] that can be used.
		FORCE_INLINE constexpr size_t mm_offset() const { return m_beg - mm_base(); }

		// Assigns a newly allocated memory region.
		FORCE_INLINE constexpr uint8_t* mm_reshape( size_t used, size_t total, size_t offset = 0 ) {
			auto base = ( uint8_t* ) detail::reallocate( mm_base(), total, mm_capacity() );

			m_beg =   base + offset;
			m_end =   base + offset + used;
			m_base =  base;
			m_limit = base + total;
			return base;
		}
		constexpr void mm_free() {
			if ( m_base )
				detail::deallocate( m_base );
			m_base = m_limit = m_beg = m_end = nullptr;
		}

		// Valdation.
		FORCE_INLINE constexpr void mm_validate() const {
			//dassert( m_base <= m_limit );
			//dassert( m_base <= m_beg && m_beg <= m_limit );
			//dassert( m_base <= m_end && m_end <= m_limit );
		}
		
		// Amortization calculation.
		FORCE_INLINE static constexpr size_t mm_amortize( size_t used, size_t offset = 0 ) {
			used = used + ( used >> 1 ) + offset;
			used >>= 4;
			return ++used << 4;
		}

		// Resize/reserve/shrink/unshift.
		//
		FORCE_INLINE constexpr void mm_reserve( size_t req_capacity ) {
			// If we need to allocate:
			//
			if ( ( m_beg + req_capacity ) > mm_limit() ) {
				// Try eating dropped partition, or realloc.
				//
				size_t offset = mm_offset();
				if ( ( m_beg + req_capacity - offset ) < mm_limit() )
					mm_drop_offset();
				else
					mm_reshape( mm_active(), mm_amortize( req_capacity, offset ), offset );
				mm_validate();
			}
		}
		FORCE_INLINE constexpr void mm_resize( size_t req_size ) {
			if ( ( m_beg + req_size ) > mm_limit() ) {
				mm_reserve( req_size );
			}
			m_end = m_beg + req_size;
			mm_validate();
		}
		NO_INLINE constexpr void mm_shrink() {
			if ( size_t used = mm_active() ) {
				mm_drop_offset();
				mm_reshape( used, used, 0 );
			} else {
				mm_free();
			}
			mm_validate();
		}
		FORCE_INLINE constexpr void mm_drop_offset() {
			if ( size_t offset = mm_offset() ) {
				size_t length = mm_active();
				detail::shift_bwd( m_beg, length, offset );
				m_beg  = m_base;
				m_end  = m_base + length;
				mm_validate();
			}
		}

		// Clears the memory discarding offset and data.
		//
		FORCE_INLINE constexpr void mm_clear() {
			m_beg = m_end = mm_base();
			mm_validate();
		}

		// Appends [n] bytes to the arena and returns the pointer.
		//
		FORCE_INLINE constexpr uint8_t* mm_append( size_t n ) {
			size_t pos = mm_active();
			mm_resize( n + pos );
			return m_beg + pos;
		}


		// Prepends [n] bytes to the arena and returns the pointer.
		//
		FORCE_INLINE constexpr uint8_t* mm_prepend( size_t n ) {
			// Consume all the offset.
			//
			size_t consumed_off = std::min( mm_offset(), n );
			m_beg -= consumed_off;
			auto* result = m_beg;

			// If we still need space:
			//
			n -= consumed_off;
			if ( n ) [[unlikely]] {
				size_t size =     mm_active();
				size_t copy_len = size - consumed_off;
				mm_resize( size + n );
				result = m_beg;
				detail::shift_fwd( result + consumed_off, copy_len, n );
			}
			mm_validate();
			return result;
		}

		// Removes [n] bytes from the beginning and returns the ephemeral pointer.
		//
		FORCE_INLINE constexpr uint8_t* mm_shift( size_t n, bool unchecked = false ) {
			if ( !unchecked && mm_active() < n ) [[unlikely]]
				return nullptr;
			uint8_t* pbeg = m_beg;
			m_beg  = pbeg + n;
			mm_validate();
			return pbeg;
		}

		// Removes [n] bytes from the end and returns the ephemeral pointer.
		//
		FORCE_INLINE constexpr uint8_t* mm_pop( size_t n, bool unchecked = false ) {
			if ( !unchecked && mm_active() < n ) [[unlikely]]
				return nullptr;
			uint8_t* pend = m_end - n;
			m_end = pend;
			mm_validate();
			return pend;
		}

		// Resize as shrink with user guarantee.
		//
		FORCE_INLINE constexpr void mm_shrink_resize( size_t n ) {
			m_end = m_beg + n;
			mm_validate();
		}
		FORCE_INLINE constexpr void mm_shrink_resize_reverse( size_t n ) {
			m_beg = m_end - n;
			mm_validate();
		}
	public:

		// Default constructor and size constructor.
		//
		constexpr vec_buffer() = default;
		FORCE_INLINE constexpr vec_buffer( size_t n ) {
			m_base =  m_beg = n ? detail::allocate( n ) : nullptr;
			m_limit = m_beg + n;
			m_end =   m_beg + n;
		}
		FORCE_INLINE constexpr vec_buffer( size_t n, uint8_t fill ) : vec_buffer( n ) {
			std::fill_n( m_beg, n, fill );
		}

		// Move by swap.
		//
		FORCE_INLINE constexpr vec_buffer( vec_buffer&& other ) noexcept {
			swap( other ); 
		}
		FORCE_INLINE constexpr vec_buffer& operator=( vec_buffer&& other ) noexcept {
			swap( other );
			return *this;
		}
		FORCE_INLINE constexpr void swap( vec_buffer& other ) {
			std::swap( m_beg,   other.m_beg );
			std::swap( m_end,   other.m_end );
			std::swap( m_base,  other.m_base );
			std::swap( m_limit, other.m_limit );
		}

		// Copy by allocation.
		//
		FORCE_INLINE constexpr vec_buffer( std::span<const uint8_t> data ) { assign_range( data ); }
		FORCE_INLINE constexpr vec_buffer( const uint8_t* beg, const uint8_t* end ) { assign_range( std::span{ beg, end } ); }
		FORCE_INLINE constexpr vec_buffer( const uint8_t* beg, size_t size ) { assign_range( std::span{ beg, beg + size } ); }
		FORCE_INLINE constexpr vec_buffer( const vec_buffer& other ) : vec_buffer( std::span{ other } ) {}
		FORCE_INLINE constexpr vec_buffer& operator=( const vec_buffer& other ) {
			if ( this == &other ) return *this;
			assign_range( other );
			return *this;
		}

		// Observers.
		//
		FORCE_INLINE constexpr uint8_t* data() { return begin(); }
		FORCE_INLINE constexpr const uint8_t* data() const { return begin(); }
		FORCE_INLINE constexpr uint8_t* begin() { return m_beg; }
		FORCE_INLINE constexpr const uint8_t* begin() const { return m_beg; }
		FORCE_INLINE constexpr uint8_t* end() { return m_end; }
		FORCE_INLINE constexpr const uint8_t* end() const { return m_end; }
		FORCE_INLINE constexpr size_t size() const { return end() - begin(); }
		FORCE_INLINE constexpr size_t length() const { return size(); }
		FORCE_INLINE constexpr size_t capacity() const { return mm_capacity(); }
		FORCE_INLINE constexpr size_t max_size() const { return SIZE_MAX; }
		FORCE_INLINE constexpr bool empty() const { return m_beg == m_end; }
		FORCE_INLINE constexpr uint8_t& at( size_t n ) { return data()[ n ]; }
		FORCE_INLINE constexpr const uint8_t& at( size_t n ) const { return data()[ n ]; }
		FORCE_INLINE constexpr uint8_t& front() { return *begin(); }
		FORCE_INLINE constexpr const uint8_t& front() const { return *begin(); }
		FORCE_INLINE constexpr uint8_t& back() { return end()[ -1 ]; }
		FORCE_INLINE constexpr const uint8_t& back() const { return end()[ -1 ]; }
		FORCE_INLINE constexpr uint8_t& operator[]( size_t n ) { return at( n ); }
		FORCE_INLINE constexpr const uint8_t& operator[]( size_t n ) const { return at( n ); }
		FORCE_INLINE constexpr explicit operator bool() const { return !empty(); }

		// Mutators.
		//
		FORCE_INLINE constexpr void resize( size_t n ) {
			mm_resize( n );
		}
		FORCE_INLINE constexpr void resize( size_t n, uint8_t fill ) {
			size_t pos = size();
			mm_resize( n );
			if ( pos < n )
				detail::fill( data() + pos, fill, n - pos );
		}
		FORCE_INLINE constexpr void shrink_resize( size_t n ) {
			mm_shrink_resize( n );
		}
		FORCE_INLINE constexpr void shrink_resize_reverse( size_t n ) {
			mm_shrink_resize_reverse( n );
		}
		FORCE_INLINE constexpr void reserve( size_t n ) {
			mm_reserve( n );
		}
		FORCE_INLINE constexpr void shrink_to_fit() {
			mm_shrink();
		}
		FORCE_INLINE constexpr void clear() {
			mm_clear();
		}
		FORCE_INLINE constexpr void reset() {
			mm_free();
		}
		FORCE_INLINE constexpr void drop_offset() {
			mm_drop_offset();
		}
		FORCE_INLINE constexpr std::tuple<uint8_t*, uint8_t*, uint8_t*, uint8_t*> release() {
			auto abeg = mm_base();
			auto dbeg=  begin();
			auto dend = end();
			auto aend = mm_limit();
			m_base = m_limit = m_beg = m_end = nullptr;
			return { abeg, dbeg, dend, aend };
		}
		FORCE_INLINE constexpr uint8_t* reserve_range( const uint8_t* it, size_t count ) {
			uint8_t* dst;
			if ( it == end() ) {
				dst = mm_append( count );
			} else if ( it == begin() ) {
				dst = mm_prepend( count );
			} else {
				size_t copy_src = it - m_beg;
				size_t copy_len = m_end - it;
				resize( size() + count );
				dst = m_beg + copy_src;
				detail::shift_fwd( dst, copy_len, count );
			}
			return dst;
		}
		FORCE_INLINE constexpr uint8_t* insert_range( const uint8_t* it, std::span<const uint8_t> data ) {
			auto* dst = reserve_range( it, data.size() );
			detail::copy( dst, data.data(), data.size() );
			return dst;
		}
		FORCE_INLINE constexpr uint8_t* insert( const uint8_t* it, const uint8_t* first, const uint8_t* last ) {
			return insert_range( it, std::span{ first, last } );
		}
		template<typename It1, typename It2>
		FORCE_INLINE constexpr uint8_t* insert( const uint8_t* it, It1 first, It2 last ) {
			size_t count = std::distance( first, last );
			if constexpr ( std::is_same_v<It1, const uint8_t*>   || std::is_same_v<It1, uint8_t*> ) {
				return insert_range( it, std::span<const uint8_t>{ first, count } );
			} 
			
			if constexpr ( std::is_same_v<It1, const char*>      || std::is_same_v<It1, char*> ||
						   std::is_same_v<It1, const std::byte*> || std::is_same_v<It1, std::byte*> ) {
				if ( !std::is_constant_evaluated() ) {
					return insert_range( it, std::span<const uint8_t>{ (const uint8_t*) first, count } );
				}
			}
			auto* dst = reserve_range( it, count );
			std::copy_n( first, count, dst );
			return dst;
		}
		FORCE_INLINE constexpr uint8_t* insert( const uint8_t* it, std::initializer_list<uint8_t> list ) {
			return insert( it, list.begin(), list.end() );
		}
		FORCE_INLINE constexpr uint8_t* insert( const uint8_t* it, uint8_t val ) {
			return insert( it, &val, std::next( &val ) );
		}
		FORCE_INLINE constexpr uint8_t* assign_range( std::span<const uint8_t> data ) {
			mm_clear();
			return append_range( data );
		}

		// Appends [count] bytes to the arena and returns the pointer.
		//
		FORCE_INLINE constexpr uint8_t* push( size_t count ) {
			return mm_append( count );
		}
		FORCE_INLINE constexpr uint8_t* append_range( std::span<const uint8_t> data ) {
			auto dst = push( data.size() );
			detail::copy( dst, data.data(), data.size() );
			return dst;
		}
		FORCE_INLINE constexpr uint8_t* append_range( vec_buffer&& other ) {
			uint8_t* result;
			if ( other.capacity() > capacity() ) {
				swap( other );
				result = prepend_range( other.subspan() );
				other.clear();
			} else {
				result = append_range( other.subspan() );
			}
			return result;
		}
		FORCE_INLINE constexpr void push_back( uint8_t value ) {
			*push( 1 ) = value;
		}
		FORCE_INLINE constexpr uint8_t& emplace_back( uint8_t value ) {
			return ( *push( 1 ) = value );
		}

		// Prepends [count] bytes to the arena and returns the pointer.
		//
		FORCE_INLINE constexpr uint8_t* unshift( size_t count ) {
			return mm_prepend( count );
		}
		FORCE_INLINE constexpr uint8_t* prepend_range( std::span<const uint8_t> data ) {
			auto dst = unshift( data.size() );
			detail::copy( dst, data.data(), data.size() );
			return dst;
		}
		FORCE_INLINE constexpr uint8_t* prepend_range( vec_buffer&& other ) {
			uint8_t* result;
			if ( other.capacity() > capacity() ) {
				swap( other );
				result = append_range( other.subspan() );
				other.clear();
			} else {
				result = prepend_range( other.subspan() );
			}
			return result;
		}
		FORCE_INLINE constexpr void push_front( uint8_t value ) {
			*unshift( 1 ) = value;
		}
		FORCE_INLINE constexpr uint8_t& emplace_front( uint8_t value ) {
			return ( *unshift( 1 ) = value );
		}

		// Removes [count] bytes from the beginning and returns the ephemeral pointer / copies the data.
		//
		FORCE_INLINE constexpr uint8_t* shift( size_t count, bool unchecked = true ) {
			return mm_shift( count, unchecked );
		}
		FORCE_INLINE constexpr bool shift_range( std::span<uint8_t> data, bool unchecked = true ) {
			auto src = shift( data.size(), unchecked );
			if ( !src ) [[unlikely]] 
				return data.empty();
			detail::copy( data.data(), src, data.size() );
			return true;
		}
		FORCE_INLINE constexpr vec_buffer shift_range( size_t n, bool unchecked = true ) {
			auto src = shift( n, unchecked );
			if ( !src ) return {};

			vec_buffer result{};
			if ( empty() ) {
				swap( result );
				result.unshift( n );
				result.shrink_to_fit();
			} else {
				result.assign_range( { src, src + n } );
			}
			return result;
		}
		FORCE_INLINE constexpr uint8_t* shift_if( size_t count ) { return shift( count, false ); }
		FORCE_INLINE constexpr bool shift_range_if( std::span<uint8_t> data ) { return shift_range( data, false ); }
		FORCE_INLINE constexpr vec_buffer shift_range_if( size_t count ) { return shift_range( count, false ); }

		// Removes [count] bytes from the end and returns the ephemeral pointer / copies the data.
		//
		FORCE_INLINE constexpr uint8_t* pop( size_t count, bool unchecked = true ) {
			return mm_pop( count, unchecked );
		}
		FORCE_INLINE constexpr bool pop_range( std::span<uint8_t> data, bool unchecked = true ) {
			auto src = pop( data.size(), unchecked );
			if ( !src ) [[unlikely]] 
				return data.empty();
			detail::copy( data.data(), src, data.size() );
			return true;
		}
		FORCE_INLINE constexpr vec_buffer pop_range( size_t n, bool unchecked = true ) {
			auto src = pop( n, unchecked );
			if ( !src ) return {};

			vec_buffer result{};
			if ( empty() ) {
				swap( result );
				result.push( n );
				result.shrink_to_fit();
			} else {
				result.assign_range( { src, src + n } );
			}
			return result;
		}
		FORCE_INLINE constexpr uint8_t* pop_if( size_t count ) { return pop( count, false ); }
		FORCE_INLINE constexpr bool pop_range_if( std::span<uint8_t> data ) { return pop_range( data, false ); }
		FORCE_INLINE constexpr vec_buffer pop_range_if( size_t count ) { return pop_range( count, false ); }

		// Span generation.
		//
		template<typename U = uint8_t>
		FORCE_INLINE constexpr std::span<const U> subspan( size_t offset = 0, size_t count = std::dynamic_extent ) const {
			return std::span{ (const U*) data(), size() / sizeof( U ) }.subspan(offset, count);
		}
		template<typename U = uint8_t>
		FORCE_INLINE constexpr std::span<U> subspan( size_t offset = 0, size_t count = std::dynamic_extent ) {
			return std::span{ (U*) data(), size() / sizeof( U ) }.subspan( offset, count );
		}

		// Non-constexpr utils.
		//
		template<typename U, size_t E> constexpr vec_buffer( std::span<U, E> v ) : vec_buffer{ std::span{ (const uint8_t*) v.data(), v.size_bytes() } } {}
		template<typename U, size_t E> constexpr uint8_t* insert_range( const uint8_t* it, std::span<U, E> v ) { return insert_range( it, std::span{ (const uint8_t*) v.data(), v.size_bytes() } ); }
		template<typename U, size_t E> constexpr uint8_t* assign_range( std::span<U, E> v ) { return assign_range( std::span{ (const uint8_t*) v.data(), v.size_bytes() } ); }
		template<typename U, size_t E> constexpr uint8_t* append_range( std::span<U, E> v ) { return append_range( std::span{ (const uint8_t*) v.data(), v.size_bytes() } ); }
		template<typename U, size_t E> constexpr uint8_t* prepend_range( std::span<U, E> v ) { return prepend_range( std::span{ (const uint8_t*) v.data(), v.size_bytes() } ); }
		template<typename U, size_t E> constexpr bool pop_range( std::span<U, E> v ) { return pop_range( std::span{ (uint8_t*) v.data(), v.size_bytes() } ); }
		template<typename U, size_t E> constexpr bool pop_range_if( std::span<U, E> v ) { return pop_range_if( std::span{ (uint8_t*) v.data(), v.size_bytes() } ); }
		template<typename U, size_t E> constexpr bool shift_range( std::span<U, E> v ) { return shift_range( std::span{ (uint8_t*) v.data(), v.size_bytes() } ); }
		template<typename U, size_t E> constexpr bool shift_range_if( std::span<U, E> v ) { return shift_range_if( std::span{ (uint8_t*) v.data(), v.size_bytes() } ); }

		template<typename U = char> constexpr vec_buffer( std::basic_string_view<U> v ) : vec_buffer{ std::span{v} } {}
		template<typename U = char> constexpr uint8_t* insert_range( const uint8_t* it, std::basic_string_view<U> v ) { return insert_range( it, std::span{ v } ); }
		template<typename U = char> constexpr uint8_t* assign_range( std::basic_string_view<U> v ) { return assign_range( std::span{ v } ); }
		template<typename U = char> constexpr uint8_t* append_range( std::basic_string_view<U> v ) { return append_range( std::span{ v } ); }
		template<typename U = char> constexpr uint8_t* prepend_range( std::basic_string_view<U> v ) { return prepend_range( std::span{ v } ); }

		template<typename U = char> constexpr vec_buffer( const std::basic_string<U>& v ) : vec_buffer{ std::span{v} } {}
		template<typename U = char> constexpr uint8_t* insert_range( const uint8_t* it, const std::basic_string<U>& v ) { return insert_range( it, std::span{ v } ); }
		template<typename U = char> constexpr uint8_t* assign_range( const std::basic_string<U>& v ) { return assign_range( std::span{ v } ); }
		template<typename U = char> constexpr uint8_t* append_range( const std::basic_string<U>& v ) { return append_range( std::span{ v } ); }
		template<typename U = char> constexpr uint8_t* prepend_range( const std::basic_string<U>& v ) { return prepend_range( std::span{ v } ); }

		template<typename U> constexpr U& pop_as() { return *(U*) pop( sizeof( U ) ); }
		template<typename U> constexpr U* pop_as_if() { return (U*) pop( sizeof( U ), false ); }
		template<typename U> constexpr U& shift_as() { return *(U*) shift( sizeof( U ) ); }
		template<typename U> constexpr U* shift_as_if() { return (U*) shift( sizeof( U ), false ); }
		template<typename U> constexpr U& emplace_back_as( const U& value ) { return *(U*) append_range( std::span{ (const uint8_t*) &value, sizeof( U ) } ); }
		template<typename U> constexpr U& emplace_front_as( const U& value ) { return *(U*) prepend_range( std::span{ (const uint8_t*) &value, sizeof( U ) } ); }

		// Destructor.
		//
		FORCE_INLINE constexpr ~vec_buffer() { mm_free(); }
	};
};