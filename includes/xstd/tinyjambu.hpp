#pragma once
#include "intrinsics.hpp"
#include <array>
#include <numeric>

namespace xstd::crypto
{
	// TinyJAMBU implementation, one-time use state.
	// - N is key size in bits.
	//
	template<size_t N>
	struct tinyjambu
	{
		using unit_t = uint32_t;
		static constexpr size_t unit_bits = sizeof( unit_t ) * 8;
		static_assert( ( N % unit_bits ) == 0, "Key size must be divisible by the unit size." );

		// Types.
		//
		static constexpr size_t iv_size = 96 / unit_bits;
		static constexpr size_t key_size = N / unit_bits;
		static constexpr size_t tag_size = 64 / unit_bits;
		static constexpr size_t state_size = 128 / unit_bits;
		using iv_type = std::array<unit_t, iv_size>;
		using key_type = std::array<unit_t, key_size>;
		using tag_type = std::array<unit_t, tag_size>;
		using state_type = std::array<unit_t, state_size>;

		// Frame bits.
		//
		static constexpr unit_t framebits_init = 0x00;
		static constexpr unit_t framebits_iv = 0x10;
		static constexpr unit_t framebits_ad = 0x30;
		static constexpr unit_t framebits_pc = 0x50;
		static constexpr unit_t framebits_fin = 0x70;
	
		// Constants.
		//
		static constexpr size_t rounds_1 = state_size * iv_size;
		static constexpr size_t rounds_2 = state_size * ( tag_size + key_size );

		// Initialization of the state.
		//
		state_type state = {};
		key_type key = {};
		FORCE_INLINE constexpr tinyjambu( const key_type& key ) : key( key ) {}
		FORCE_INLINE constexpr tinyjambu( const key_type& key, const iv_type& iv ) : tinyjambu( key ) { reset( iv ); }

		// Default copy/move.
		//
		FORCE_INLINE constexpr tinyjambu( tinyjambu&& ) noexcept = default;
		FORCE_INLINE constexpr tinyjambu( const tinyjambu& ) = default;
		FORCE_INLINE constexpr tinyjambu& operator=( tinyjambu&& ) noexcept = default;
		FORCE_INLINE constexpr tinyjambu& operator=( const tinyjambu& ) = default;

		// Update helpers.
		//
		FORCE_INLINE constexpr unit_t update_single( size_t rounds, unit_t framebits, unit_t data_in = 0, bool reverse = false, unit_t mask = std::numeric_limits<unit_t>::max() )
		{
			// indicate frame
			state[ 1 ] ^= framebits;

			// rotate
			#pragma unroll(4)
			for ( size_t i = 0; i != rounds; i++ )
			{
				auto t1 = ( state[ 1 ] >> 15 ) | ( state[ 2 ] << 17 );  // 47 = 1*32+15 
				auto t2 = ( state[ 2 ] >> 6 )  | ( state[ 3 ] << 26 );  // 47 + 23 = 70 = 2*32 + 6 
				auto t3 = ( state[ 2 ] >> 21 ) | ( state[ 3 ] << 11 );  // 47 + 23 + 15 = 85 = 2*32 + 21      
				auto t4 = ( state[ 2 ] >> 27 ) | ( state[ 3 ] << 5 );   // 47 + 23 + 15 + 6 = 91 = 2*32 + 27 
				auto feedback = state[ 0 ] ^ t1 ^ ( ~( t2 & t3 ) ) ^ t4 ^ key[ i % key_size ];

				// shift 32 bit positions 
				state[ 0 ] = state[ 1 ]; 
				state[ 1 ] = state[ 2 ]; 
				state[ 2 ] = state[ 3 ];
				state[ 3 ] = feedback;
			}

			// if reverse:
			if ( reverse )
			{
				// decrypt
				data_in ^= state[ 2 ] & mask;
				// feedback(dec)
				state[ 3 ] ^= data_in & mask;
				return data_in;
			}
			else
			{
				// feedback(dec)
				state[ 3 ] ^= data_in & mask;
				// encrypt
				data_in ^= state[ 2 ] & mask;
				return data_in;
			}
		}
		template<typename T> requires( sizeof( T ) == 1 || sizeof( T ) == sizeof( unit_t ) )
		FORCE_INLINE constexpr tinyjambu& update( size_t rounds, unit_t framebits, T* io, size_t count, bool reverse = false )
		{
			if constexpr ( sizeof( T ) == sizeof( unit_t ) )
			{
				while ( count-- )
				{
					unit_t res = update_single( rounds, framebits, *io, reverse );
					if constexpr ( !std::is_const_v<T> )
						*io = res;
					io++;
				}
			}
			else
			{
				// Optimize into unit-granularity updates if not constexpr:
				//
				if ( !std::is_constant_evaluated() )
				{
					while ( count >= sizeof( unit_t ) )
					{
						unit_t res = update_single( rounds, framebits, *( const unit_t* ) io, reverse );
						if constexpr ( !std::is_const_v<T> )
							*( unit_t* ) io = res;
						io += sizeof( unit_t );
						count -= sizeof( unit_t );
					}
					if ( !count ) return *this;
				}

				// Until we reach the end:
				//
				T* end = io + count;
				while ( io != end )
				{
					// Fill a unit zero-extended and update.
					//
					unit_t u = 0;
					size_t n;
					if ( std::is_constant_evaluated() )
					{
						for ( n = 0; io != end && n != sizeof( unit_t ); n++, io++ )
							u |= unit_t( ( uint8_t ) ( *io ) ) << ( n * 8 );
					}
					else
					{
						n = std::min<size_t>( end - io, 4 );
						switch ( n )
						{
							case 1: *( std::array<uint8_t, 1>* ) &u = *( const std::array<uint8_t, 1>* ) io; break;
							case 2: *( std::array<uint8_t, 2>* ) &u = *( const std::array<uint8_t, 2>* ) io; break;
							case 3: *( std::array<uint8_t, 3>* ) & u = *( const std::array<uint8_t, 3>* ) io; break;
							//case 4: *( std::array<uint8_t, 4>* ) &u = *( std::array<uint8_t, 4>* ) io;
							default: unreachable();
						}
						io += n;
					}
					u = update_single( rounds, framebits, u, reverse, ( unit_t ) ( ( 1ull << ( n * 8 ) ) - 1 ) );

					// If there is an output, write it byte by byte.
					//
					if constexpr ( !std::is_const_v<T> )
					{
						io -= n;
						if ( std::is_constant_evaluated() )
						{
							for ( size_t i = 0; i != n; i++ )
								*io++ = uint8_t( ( u >> ( 8 * i ) ) & 0xFF );
						}
						else
						{
							switch ( n )
							{
								case 1: *( std::array<uint8_t, 1>* ) io = *( std::array<uint8_t, 1>* ) &u; break;
								case 2: *( std::array<uint8_t, 2>* ) io = *( std::array<uint8_t, 2>* ) &u; break;
								case 3: *( std::array<uint8_t, 3>* ) io = *( std::array<uint8_t, 3>* ) &u; break;
								//case 4: *( std::array<uint8_t, 4>* ) io = *( std::array<uint8_t, 4>* ) &u;
								default: unreachable();
							}
							io += n;
						}
					}
				}
			}
			return *this;
		}

		// Initializes the state.
		//
		FORCE_INLINE constexpr tinyjambu& reset( const iv_type& iv )
		{
			state.fill( 0 );

			// Update the state with the key.
			//
			update_single( rounds_2, framebits_init );

			// Introduce IV into the state.
			//
			return update( rounds_1, framebits_iv, iv.data(), iv.size() );
		}

		// Appends associated data.
		//
		template<typename T> requires( sizeof( T ) == 1 || sizeof( T ) == sizeof( unit_t ) )
		FORCE_INLINE constexpr tinyjambu& associate( const T* data, size_t num )
		{
			return update( rounds_1, framebits_ad, data, num, false );
		}

		// Encryption and decryption.
		//
		template<typename T> requires( sizeof( T ) == 1 || sizeof( T ) == sizeof( unit_t ) )
		FORCE_INLINE constexpr tinyjambu& encrypt( T* data, size_t num )
		{
			return update( rounds_2, framebits_pc, data, num, false );
		}
		template<typename T> requires( sizeof( T ) == 1 || sizeof( T ) == sizeof( unit_t ) )
		FORCE_INLINE constexpr tinyjambu& decrypt( T* data, size_t num )
		{
			return update( rounds_2, framebits_pc, data, num, true );
		}
		
		// Primitives.
		//
		template<typename T>
		FORCE_INLINE constexpr tinyjambu& associate( const T& data )
		{
			if ( !std::is_constant_evaluated() )
			{
				using E = std::conditional_t<( sizeof( T ) % sizeof( unit_t ) ) == 0, unit_t, uint8_t>;
				associate( ( const E* ) &data, sizeof( T ) / sizeof( E ) );
			}
			else
			{
				using A = std::array<uint8_t, sizeof( T )>;
				A value = bit_cast<A>( data );
				associate( value.data(), value.size() );
			}
			return *this;
		}
		template<typename T>
		FORCE_INLINE constexpr tinyjambu& decrypt( T& data )
		{
			if ( !std::is_constant_evaluated() )
			{
				using E = std::conditional_t<( sizeof( T ) % sizeof( unit_t ) ) == 0, unit_t, uint8_t>;
				return decrypt( ( E* ) &data, sizeof( T ) / sizeof( E ) );
			}
			else
			{
				using A = std::array<uint8_t, sizeof( T )>;
				A value = bit_cast< A >( data );
				decrypt( value.data(), value.size() );
				data = bit_cast< T >( value );
				return *this;
			}
		}
		template<typename T>
		FORCE_INLINE constexpr tinyjambu& encrypt( T& data )
		{
			if ( !std::is_constant_evaluated() )
			{
				using E = std::conditional_t<( sizeof( T ) % sizeof( unit_t ) ) == 0, unit_t, uint8_t>;
				return encrypt( ( E* ) &data, sizeof( T ) / sizeof( E ) );
			}
			else 
			{
				using A = std::array<uint8_t, sizeof( T )>;
				A value = bit_cast< A >( data );
				encrypt( value.data(), value.size() );
				data = bit_cast< T >( value );
				return *this;
			}
		}

		// Tag calculation.
		//
		FORCE_INLINE constexpr tag_type finalize()
		{
			return { update_single( rounds_2, framebits_fin ), update_single( rounds_1, framebits_fin ) };
		}
	};
};