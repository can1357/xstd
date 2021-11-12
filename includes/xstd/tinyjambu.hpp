#pragma once
#include "intrinsics.hpp"
#include <array>
#include <numeric>
#include "random.hpp"

namespace xstd
{
	// TinyJAMBU implementation, one-time use state.
	// - N is key size in bits.
	//
	template<size_t N = 128>
	struct tinyjambu
	{
		using unit_type = uint32_t;
		static constexpr size_t unit_bits = sizeof( unit_type ) * 8;
		static constexpr size_t unit_bytes = sizeof( unit_type );
		static_assert( ( N % unit_bits ) == 0, "Key size must be divisible by the unit size." );

		// Types.
		//
		static constexpr size_t key_size = N / unit_bits;
		static constexpr size_t tag_size = 64 / unit_bits;
		static constexpr size_t state_size = 128 / unit_bits;
		static constexpr size_t default_iv_size = 96 / unit_bits;
		using key_type = std::array<unit_type, key_size>;
		using tag_type = std::array<unit_type, tag_size>;
		using state_type = std::array<unit_type, state_size>;
		using default_iv_type = std::array<unit_type, default_iv_size>;

		// Frame bits.
		//
		static constexpr unit_type framebits_in = 0x00;
		static constexpr unit_type framebits_iv = 0x10;
		static constexpr unit_type framebits_ad = 0x30;
		static constexpr unit_type framebits_pc = 0x50;
		static constexpr unit_type framebits_fi = 0x70;
	
		// Constants.
		//
		static constexpr size_t rounds_1 = state_size * default_iv_size;
		static constexpr size_t rounds_2 = state_size * ( tag_size + key_size );

		// Helpers.
		//
		template<size_t C = default_iv_size>
		FORCE_INLINE inline static constexpr std::array<unit_type, C> generate_iv()
		{
			if ( std::is_constant_evaluated() )
				return make_random_n<unit_type, C>();
			else
				return make_crandom_n<unit_type, C>();
		}
		FORCE_INLINE inline static constexpr key_type generate_key() { return generate_iv<key_size>(); }

		// Initialization of the state.
		//
		key_type key = {};
		state_type state = {};
		FORCE_INLINE constexpr tinyjambu() {}
		FORCE_INLINE constexpr tinyjambu( const key_type& key ) : key( key ) {}
		FORCE_INLINE constexpr tinyjambu( const unit_type* key ) : key( *( const key_type* ) key ) {}
		FORCE_INLINE constexpr tinyjambu( const key_type& key, const default_iv_type& iv ) : tinyjambu( key ) { reset( iv ); }
		FORCE_INLINE constexpr tinyjambu( const unit_type* key, const unit_type* iv, size_t len = default_iv_size ) : tinyjambu( key ) { reset( iv, len ); }

		// Default copy/move.
		//
		FORCE_INLINE constexpr tinyjambu( tinyjambu&& ) noexcept = default;
		FORCE_INLINE constexpr tinyjambu( const tinyjambu& ) = default;
		FORCE_INLINE constexpr tinyjambu& operator=( tinyjambu&& ) noexcept = default;
		FORCE_INLINE constexpr tinyjambu& operator=( const tinyjambu& ) = default;

		// Update helpers.
		//
		FORCE_INLINE constexpr unit_type update_single( size_t rounds, unit_type framebits, unit_type data_in = 0, bool reverse = false, unit_type mask = std::numeric_limits<unit_type>::max() )
		{
			// indicate frame
			state[ 1 ] ^= framebits;

			// rotate
			__hint_unroll_n( 4 )
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
		template<typename T> requires( sizeof( T ) == 1 || sizeof( T ) == sizeof( unit_type ) )
		FORCE_INLINE constexpr tinyjambu& update( size_t rounds, unit_type framebits, T* io, size_t count, bool reverse = false )
		{
			if constexpr ( sizeof( T ) == sizeof( unit_type ) )
			{
				while ( count-- )
				{
					unit_type res = update_single( rounds, framebits, *io, reverse );
					if constexpr ( !Const<T> )
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
					while ( count >= sizeof( unit_type ) )
					{
						unit_type res = update_single( rounds, framebits, *( const unit_type* ) io, reverse );
						if constexpr ( !Const<T> )
							*( unit_type* ) io = res;
						io += sizeof( unit_type );
						count -= sizeof( unit_type );
					}
				}

				// Until we reach the end:
				//
				T* end = io + count;
				while ( io != end )
				{
					// Fill a unit zero-extended and update.
					//
					unit_type u = 0;
					size_t n;
					if ( std::is_constant_evaluated() )
					{
						for ( n = 0; io != end && n != sizeof( unit_type ); n++, io++ )
							u |= unit_type( ( uint8_t ) ( *io ) ) << ( n * 8 );
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
					u = update_single( rounds, framebits, u, reverse, ( unit_type ) ( ( 1ull << ( n * 8 ) ) - 1 ) );

					// If there is an output, write it byte by byte.
					//
					if constexpr ( !Const<T> )
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
		FORCE_INLINE constexpr tinyjambu& reset( const unit_type* iv, size_t num = default_iv_size )
		{
			state.fill( 0 );

			// Update the state with the key.
			//
			update_single( rounds_2, framebits_in );

			// Introduce IV into the state.
			//
			return update( rounds_1, framebits_iv, iv, num );
		}
		FORCE_INLINE constexpr tinyjambu& reset( const default_iv_type& iv ) { return reset( iv.data(), iv.size() ); }

		// Appends associated data.
		//
		template<typename T> requires( sizeof( T ) == 1 || sizeof( T ) == sizeof( unit_type ) )
		FORCE_INLINE constexpr tinyjambu& associate( const T* data, size_t num )
		{
			return update( rounds_1, framebits_ad, data, num, false );
		}

		// Encryption and decryption.
		//
		template<typename T> requires( sizeof( T ) == 1 || sizeof( T ) == sizeof( unit_type ) )
		FORCE_INLINE constexpr tinyjambu& encrypt( T* data, size_t num )
		{
			return update( rounds_2, framebits_pc, data, num, false );
		}
		template<typename T> requires( sizeof( T ) == 1 || sizeof( T ) == sizeof( unit_type ) )
		FORCE_INLINE constexpr tinyjambu& decrypt( T* data, size_t num )
		{
			return update( rounds_2, framebits_pc, data, num, true );
		}
		
		// Primitives.
		//
		template<typename T, typename... Tx>
		FORCE_INLINE constexpr tinyjambu& associate( const T* data, const Tx*... rest )
		{
			if ( !std::is_constant_evaluated() )
			{
				using E = std::conditional_t<( sizeof( T ) % sizeof( unit_type ) ) == 0, unit_type, uint8_t>;
				associate( ( const E* ) data, sizeof( T ) / sizeof( E ) );
			}
			else
			{
				using A = std::array<uint8_t, sizeof( T )>;
				A value = bit_cast<A>( *data );
				associate( value.data(), value.size() );
			}
			if constexpr ( sizeof...( Tx ) )
				associate<Tx...>( rest... );
			return *this;
		}

		template<typename T, typename... Tx>
		FORCE_INLINE constexpr tinyjambu& decrypt( T* data, Tx*... rest )
		{
			if ( !std::is_constant_evaluated() )
			{
				using E = std::conditional_t<( sizeof( T ) % sizeof( unit_type ) ) == 0, unit_type, uint8_t>;
				decrypt( ( E* ) data, sizeof( T ) / sizeof( E ) );
			}
			else
			{
				using A = std::array<uint8_t, sizeof( T )>;
				A value = bit_cast< A >( *data );
				decrypt( value.data(), value.size() );
				*data = bit_cast< T >( value );
			}
			if constexpr ( sizeof...( Tx ) )
				decrypt<Tx...>( rest... );
			return *this;
		}
		template<typename T, typename... Tx>
		FORCE_INLINE constexpr tinyjambu& encrypt( T* data, Tx*... rest )
		{
			if ( !std::is_constant_evaluated() )
			{
				using E = std::conditional_t<( sizeof( T ) % sizeof( unit_type ) ) == 0, unit_type, uint8_t>;
				encrypt( ( E* ) data, sizeof( T ) / sizeof( E ) );
			}
			else 
			{
				using A = std::array<uint8_t, sizeof( T )>;
				A value = bit_cast< A >( *data );
				encrypt( value.data(), value.size() );
				*data = bit_cast< T >( value );
			}
			if constexpr ( sizeof...( Tx ) )
				encrypt<Tx...>( rest... );
			return *this;
		}

		// Tag calculation.
		//
		FORCE_INLINE constexpr tag_type finalize()
		{
			return { update_single( rounds_2, framebits_fi ), update_single( rounds_1, framebits_fi ) };
		}
	};
};