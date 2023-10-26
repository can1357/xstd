#pragma once
#include "intrinsics.hpp"
#include "type_helpers.hpp"

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
			if ( std::is_constant_evaluated() ) {
				if ( count ) // This check is actually asserted by caller, only to prevent errors in compilers with no cxpr heap by introducing runtime condition.
					return new uint8_t[ count ]();
				return nullptr;
			} else {
				if ( const_condition( !count ) ) // This check is actually asserted by caller, only optimizing codegen if known at compile time.
					return nullptr;
				return (uint8_t*) malloc( count );
			}
		}
		FORCE_INLINE static constexpr void deallocate( uint8_t* prev ) {
			if ( std::is_constant_evaluated() ) {
				if ( prev )
					delete[] prev;
			} else {
				if ( const_condition( prev == nullptr ) )
					return;
				free( prev );
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
			if ( std::is_constant_evaluated() ) {
				uint8_t* result = allocate( size );
				if ( prev ) {
					if ( size != 0 )
						detail::copy( result, prev, std::min( size, prev_size ) );
				}
				deallocate( prev );
				return result;
			} else {
				return (uint8_t*) realloc( prev, size );
			}
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
		FORCE_INLINE constexpr uint8_t* __restrict mm_base() const { return m_base; }
		FORCE_INLINE constexpr uint8_t* mm_limit() const { return m_limit; }

		// Returns the number of bytes after [m_end] that can be used.
		FORCE_INLINE constexpr size_t mm_reserved() const { return mm_limit() - m_end; }
		// Returns the number of bytes after [m_base] that can be used.
		FORCE_INLINE constexpr size_t mm_capacity() const { return mm_limit() - mm_base(); }
		// Returns the number of bytes before [m_beg] that can be used.
		FORCE_INLINE constexpr size_t mm_offset() const { return m_beg - mm_base(); }

		// Assigns a newly allocated memory region.
		FORCE_INLINE constexpr uint8_t* __restrict mm_reshape( size_t used, size_t total, size_t offset = 0 ) {
			auto base = ( uint8_t* ) detail::reallocate( mm_base(), total, mm_capacity() );
			m_beg =   base + offset;
			m_end =   base + offset + used;
			m_base =  base;
			m_limit = base + total;
			return base;
		}
		constexpr void mm_free() {
			if ( const_condition( !m_base ) )
				return;
			detail::deallocate( m_base );
			m_base = m_limit = m_beg = m_end = nullptr;
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
				if ( ( mm_base() + req_capacity ) < mm_limit() )
					mm_drop_offset();
				else
					mm_reshape( mm_active(), mm_amortize( req_capacity ), mm_offset() );
			}
		}
		FORCE_INLINE constexpr void mm_resize( size_t req_size ) {
			if ( ( m_beg + req_size ) > mm_limit() ) {
				mm_reserve( req_size );
			}
			m_end = m_beg + req_size;
		}
		NO_INLINE constexpr void mm_shrink() {
			if ( size_t used = mm_active() ) {
				mm_drop_offset();
				mm_reshape( used, used, 0 );
			} else {
				mm_free();
			}
		}
		FORCE_INLINE constexpr void mm_drop_offset() {
			if ( size_t offset = mm_offset() ) {
				size_t length = mm_active();
				detail::shift_bwd( m_beg, length, offset );
				m_beg  = m_base;
				m_end  = m_base + length;
			}
		}

		// Clears the memory discarding offset and data.
		//
		FORCE_INLINE constexpr void mm_clear() {
			m_beg = m_end = mm_base();
		}

		// Appends [n] bytes to the arena and returns the pointer.
		//
		FORCE_INLINE constexpr uint8_t* __restrict mm_append( size_t n ) {
			size_t pos = mm_active();
			mm_resize( n + pos );
			return m_beg + pos;
		}


		// Prepends [n] bytes to the arena and returns the pointer.
		//
		FORCE_INLINE constexpr uint8_t* __restrict mm_prepend( size_t n ) {
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
			return result;
		}

		// Removes [n] bytes from the beginning and returns the ephemeral pointer.
		//
		FORCE_INLINE constexpr uint8_t* __restrict mm_shift( size_t n ) {
			if ( mm_active() < n ) [[unlikely]]
				return nullptr;
			uint8_t* pbeg = m_beg;
			m_beg  = pbeg + n;
			return pbeg;
		}

		// Removes [n] bytes from the end and returns the ephemeral pointer.
		//
		FORCE_INLINE constexpr uint8_t* __restrict mm_pop( size_t n ) {
			if ( mm_active() < n ) [[unlikely]]
				return nullptr;
			uint8_t* pend = m_end - n;
			m_end = pend;
			return pend;
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
		FORCE_INLINE constexpr vec_buffer( const vec_buffer& other ) : vec_buffer( std::span{ other } ) {}
		FORCE_INLINE constexpr vec_buffer& operator=( const vec_buffer& other ) {
			if ( this == &other ) return *this;
			assign_range( other );
			return *this;
		}

		// Observers.
		//
		FORCE_INLINE constexpr uint8_t* __restrict data() { return begin(); }
		FORCE_INLINE constexpr const uint8_t* __restrict data() const { return begin(); }
		FORCE_INLINE constexpr uint8_t* __restrict begin() { return m_beg; }
		FORCE_INLINE constexpr const uint8_t* __restrict begin() const { return m_beg; }
		FORCE_INLINE constexpr uint8_t* end() { return m_end; }
		FORCE_INLINE constexpr const uint8_t* end() const { return m_end; }
		FORCE_INLINE constexpr size_t size() const { return end() - begin(); }
		FORCE_INLINE constexpr size_t capacity() const { return mm_capacity(); }
		FORCE_INLINE constexpr size_t max_size() const { return SIZE_MAX; }
		FORCE_INLINE constexpr bool empty() const { return m_beg == m_end; }
		FORCE_INLINE constexpr uint8_t& __restrict at( size_t n ) { return data()[ n ]; }
		FORCE_INLINE constexpr const uint8_t& __restrict at( size_t n ) const { return data()[ n ]; }
		FORCE_INLINE constexpr uint8_t& front() { return *begin(); }
		FORCE_INLINE constexpr const uint8_t& front() const { return *begin(); }
		FORCE_INLINE constexpr uint8_t& back() { return end()[ -1 ]; }
		FORCE_INLINE constexpr const uint8_t& back() const { return end()[ -1 ]; }
		FORCE_INLINE constexpr uint8_t& __restrict operator[]( size_t n ) { return at( n ); }
		FORCE_INLINE constexpr const uint8_t& __restrict operator[]( size_t n ) const { return at( n ); }
		FORCE_INLINE constexpr explicit operator bool() const { return !empty(); }

		// Mutators.
		//
		FORCE_INLINE constexpr void resize( size_t n ) {
			mm_resize( n );
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
		FORCE_INLINE constexpr std::tuple<uint8_t*, uint8_t*, uint8_t*, uint8_t*> release() {
			auto abeg = mm_base();
			auto dbeg=  begin();
			auto dend = end();
			auto aend = mm_limit();
			m_base = m_limit = m_beg = m_end = nullptr;
			return { abeg, dbeg, dend, aend };
		}
		FORCE_INLINE constexpr uint8_t* __restrict insert_range( const uint8_t* it, std::span<const uint8_t> data ) {
			uint8_t* dst;
			if ( it == end() ) {
				dst = mm_append( data.size() );
			} else if ( it == begin() ) {
				dst = mm_prepend( data.size() );
			} else {
				size_t copy_src = it - m_beg;
				size_t copy_len = m_end - it;
				resize( size() + data.size() );
				dst = m_beg + copy_src;
				detail::shift_fwd( dst, copy_len, data.size() );
			}
			detail::copy( dst, data.data(), data.size() );
			return dst;
		}
		FORCE_INLINE constexpr void assign_range( std::span<const uint8_t> data ) {
			mm_clear();
			append_range( data );
		}

		// Appends [count] bytes to the arena and returns the pointer.
		//
		FORCE_INLINE constexpr uint8_t* __restrict push( size_t count ) {
			return mm_append( count );
		}
		FORCE_INLINE constexpr uint8_t* __restrict append_range( std::span<const uint8_t> data ) {
			auto dst = push( data.size() );
			detail::copy( dst, data.data(), data.size() );
			return dst;
		}

		// Prepends [count] bytes to the arena and returns the pointer.
		//
		FORCE_INLINE constexpr uint8_t* __restrict unshift( size_t count ) {
			return mm_prepend( count );
		}
		FORCE_INLINE constexpr uint8_t* __restrict prepend_range( std::span<const uint8_t> data ) {
			auto dst = unshift( data.size() );
			detail::copy( dst, data.data(), data.size() );
			return dst;
		}

		// Removes [count] bytes from the beginning and returns the ephemeral pointer / copies the data.
		//
		FORCE_INLINE constexpr uint8_t* __restrict shift( size_t count ) {
			return mm_shift( count );
		}
		FORCE_INLINE constexpr bool shift_range( std::span<uint8_t> data ) {
			auto src = shift( data.size() );
			if ( !src ) [[unlikely]] 
				return data.empty();
			detail::copy( data.data(), src, data.size() );
			return true;
		}
		FORCE_INLINE constexpr vec_buffer shift_range( size_t n ) {
			auto src = shift( n );
			if ( !src ) return {};

			vec_buffer result{};
			if ( empty() ) {
				swap( result );
				result.resize( n );
				result.shrink_to_fit();
			} else {
				result.assign_range( { src, src + n } );
			}
			return result;
		}

		// Removes [count] bytes from the end and returns the ephemeral pointer / copies the data.
		//
		FORCE_INLINE constexpr uint8_t* __restrict pop( size_t count ) {
			return mm_pop( count );
		}
		FORCE_INLINE constexpr bool pop_range( std::span<uint8_t> data ) {
			auto src = pop( data.size() );
			if ( !src ) [[unlikely]] 
				return data.empty();
			detail::copy( data.data(), src, data.size() );
			return true;
		}
		FORCE_INLINE constexpr vec_buffer pop_range( size_t n ) {
			auto src = pop( n );
			if ( !src ) return {};

			vec_buffer result{};
			if ( empty() ) {
				swap( result );
				result.resize( n );
				result.shrink_to_fit();
			} else {
				result.assign_range( { src, src + n } );
			}
			return result;
		}

		// Span generation.
		//
		FORCE_INLINE constexpr std::span<const uint8_t> subspan( size_t offset = 0, size_t count = std::dynamic_extent ) const {
			return std::span{ data(), size() }.subspan(offset, count);
		}
		FORCE_INLINE constexpr std::span<uint8_t> subspan( size_t offset = 0, size_t count = std::dynamic_extent ) {
			return std::span{ data(), size() }.subspan( offset, count );
		}

		// Destructor.
		//
		FORCE_INLINE constexpr ~vec_buffer() { mm_free(); }
	};
	FORCE_INLINE inline constexpr void uninitialized_resize( vec_buffer& ref, size_t length ) {
		ref.resize( length );
	}
	FORCE_INLINE inline constexpr void shrink_resize( vec_buffer& ref, size_t length ) {
		ref.pop( ref.size() -  length );
	}
};