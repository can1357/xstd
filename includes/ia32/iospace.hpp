#pragma once
#include "../ia32.hpp"
#include <xstd/type_helpers.hpp>
#include <xstd/formatting.hpp>
#include <xstd/hashable.hpp>

namespace ia32
{
	// Implements a transactional IO space.
	//
	template<typename AddressType, typename DataType>
	struct tx_io_space
	{
		using pointer_type =    xstd::convert_uint_t<AddressType>;
		using difference_type = xstd::convert_int_t<AddressType>;

		// Space range.
		//
		const uint16_t        address_register;
		const uint16_t        data_register;
		const pointer_type    address_begin;
		const pointer_type    address_limit;

		// Read/write.
		//
		FORCE_INLINE DataType read_unsafe( AddressType adr ) const 
		{
			write_io<AddressType>( address_register, adr );
			return read_io<DataType>( data_register );
		}
		FORCE_INLINE void write_unsafe( AddressType adr, DataType value ) const 
		{
			write_io<AddressType>( address_register, adr );
			write_io<DataType>( data_register, value );
		}

		// Safe wrappers that handle interruptability.
		//
		FORCE_INLINE DataType read( AddressType adr ) const 
		{
			scope_irql<NO_INTERRUPTS> _g{};
			return read_unsafe( adr );
		}
		FORCE_INLINE void write( AddressType adr, DataType value ) const 
		{
			scope_irql<NO_INTERRUPTS> _g{};
			return write_unsafe( adr, value );
		}

		// Range read-write.
		// -- Count is in units of DataType, address must be at proper alignment.
		//
		FORCE_INLINE void read_range( xstd::any_ptr dst, AddressType src, size_t count ) const 
		{
			scope_irql<NO_INTERRUPTS> _g{};
			DataType* out = dst;
			for ( size_t i = 0; i != count; i++ )
			{
				AddressType address = xstd::bit_cast<AddressType>( pointer_type( xstd::bit_cast<pointer_type>( src ) + sizeof( DataType ) * i ) );
				out[ i ] = read_unsafe( address );
			}
		}
		FORCE_INLINE void write_range( AddressType dst, xstd::any_ptr src, size_t count ) const 
		{
			scope_irql<NO_INTERRUPTS> _g{};
			const DataType* in = src;
			for ( size_t i = 0; i != count; i++ )
			{
				AddressType address = xstd::bit_cast<AddressType>( xstd::bit_cast<pointer_type>( dst ) + sizeof( DataType ) * i );
				write_unsafe( address, in[ i ] );
			}
		}

		// Implement the proxy.
		//
		struct value_proxy
		{
			const tx_io_space*   space;
			pointer_type         ptr;

			// Decay to value reads.
			//
			operator DataType() const { return space->read( xstd::bit_cast<AddressType>( ptr ) ); }

			// Assignment of value writes.
			//
			value_proxy& operator=( DataType value ) { space->write( xstd::bit_cast<AddressType>( ptr ), value ); return *this; }
		};

		// Implement the iterator.
		//
		struct iterator
		{
			// Generic iterator typedefs.
			//
			using iterator_category = std::random_access_iterator_tag;
			using difference_type =   xstd::convert_int_t<AddressType>;
			using value_type =        value_proxy;
			using reference =         const value_proxy&;
			using pointer =           const value_proxy*;

			// Current proxy.
			//
			value_proxy proxy;

			// Support random iteration.
			//
			constexpr iterator& operator++() { proxy.ptr+=sizeof(DataType); return *this; }
			constexpr iterator& operator--() { proxy.ptr-=sizeof(DataType); return *this; }
			constexpr iterator operator++( int ) { auto s = *this; operator++(); return s; }
			constexpr iterator operator--( int ) { auto s = *this; operator--(); return s; }
			constexpr iterator& operator+=( difference_type d ) { proxy.ptr += d * sizeof(DataType); return *this; }
			constexpr iterator& operator-=( difference_type d ) { proxy.ptr -= d * sizeof(DataType); return *this; }
			constexpr iterator operator+( difference_type d ) const { auto s = *this; s += d; return s; }
			constexpr iterator operator-( difference_type d ) const { auto s = *this; s -= d; return s; }

			// Comparison and difference against another iterator.
			//
			constexpr difference_type operator-( const iterator& other ) const { return proxy.ptr - other.proxy.ptr; }
			constexpr bool operator<( const iterator& other ) const { return proxy.ptr < other.proxy.ptr; }
			constexpr bool operator<=( const iterator& other ) const { return proxy.ptr <= other.proxy.ptr; }
			constexpr bool operator>( const iterator& other ) const { return proxy.ptr > other.proxy.ptr; }
			constexpr bool operator>=( const iterator& other ) const { return proxy.ptr >= other.proxy.ptr; }
			constexpr bool operator==( const iterator& other ) const { return proxy.ptr == other.proxy.ptr; }
			constexpr bool operator!=( const iterator& other ) const { return proxy.ptr != other.proxy.ptr; }

			// Redirect dereferencing to the number, decay to it as well.
			//
			constexpr pointer operator->() const { return &proxy; }
			constexpr reference operator*() const { return proxy; }
			constexpr operator value_type() const { return proxy; }
		};

		// Implement the generic container interface.
		//
		constexpr bool empty() const { return false; }
		constexpr size_t size() const { return (size_t) ( 1 + address_limit - address_begin ); }
		constexpr iterator begin() const { return { value_proxy{ this, address_begin } }; }
		constexpr iterator end() const { return { value_proxy{ this, pointer_type( address_limit + 1 ) } }; }
		constexpr value_proxy operator[]( ptrdiff_t n ) const { return value_proxy{ this, pointer_type( address_begin + n ) }; }
		constexpr value_proxy operator()( AddressType adr ) const { return value_proxy{ this, xstd::bit_cast<pointer_type>( adr ) }; }

		// Ranges.
		//
		constexpr tx_io_space slice( AddressType begin, size_t count ) const
		{
			auto bg = xstd::bit_cast<pointer_type>( begin );
			auto lim = pointer_type( bg + count - 1 );
			return tx_io_space{ address_register, data_register, bg, lim };
		}
	};

	// Common IO space entries.
	//
	struct pci_address
	{
		uint32_t offset   : 8;
		uint32_t function : 3;
		uint32_t device   : 5;
		uint32_t bus      : 8;
		uint32_t rzvd     : 7;
		uint32_t enable   : 1;

		// Construction.
		//
		inline constexpr pci_address( uint32_t bus = 0, uint32_t device = 0, uint32_t function = 0, uint32_t offset = 0 )
			: offset( offset ), function( function ), device( device ), bus( bus ), rzvd( 0 ), enable( 1 ) {}

		// Default copy/assign.
		//
		constexpr pci_address( const pci_address& ) = default;
		constexpr pci_address& operator=( const pci_address& ) = default;

		// Iteration.
		//
		inline constexpr pci_address& operator++() { offset += sizeof( uint32_t ); return *this; }
		inline constexpr pci_address& operator--() { offset -= sizeof( uint32_t ); return *this; }
		inline constexpr pci_address& operator+=( int d ) { offset += d * sizeof( uint32_t ); return *this; }
		inline constexpr pci_address& operator-=( int d ) { offset -= d * sizeof( uint32_t ); return *this; }
		inline constexpr pci_address operator++( int ) { auto s = *this; operator++(); return s; }
		inline constexpr pci_address operator--( int ) { auto s = *this; operator--(); return s; }
		inline constexpr pci_address operator+( int d ) const { auto s = *this; s += d; return s; }
		inline constexpr pci_address operator-( int d ) const { auto s = *this; s -= d; return s; }

		// Comparison.
		//
		inline bool operator==( const pci_address& o ) const { return *(uint32_t*)this == *(uint32_t*)&o; }
		inline bool operator!=( const pci_address& o ) const { return *(uint32_t*)this != *(uint32_t*)&o; }
		inline bool operator>( const pci_address& o ) const { return *(uint32_t*)this > *(uint32_t*)&o; }

		// Hashing.
		//
		inline xstd::hash_t hash() const { return xstd::make_hash( *( uint32_t* ) this ); }

		// String conversion.
		//
		inline std::string to_string() const
		{
			return xstd::fmt::str( "%02x:%02x:%02x", bus, device, function ) + ( offset ? xstd::fmt::offset( offset ) : ""s );
		}
	};
	inline constexpr tx_io_space<uint8_t, uint8_t>      cmos_io_space    = { 0x70,  0x71,  0x00,       0x5D       };
	inline constexpr tx_io_space<pci_address, uint32_t> pci_config_space = { 0xCF8, 0xCFC, 0x80000000, 0xFFFFFFFF };
};