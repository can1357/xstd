#pragma once
#include "type_helpers.hpp"
#include <algorithm>
#include <bit>
#include <array>
#include <span>

// [Configuration]
// XSTD_SIMD_WIDTH: Defines the default SIMD length for xstd::max_vec_t.
//
#ifndef XSTD_SIMD_WIDTH
	#if AMD64_TARGET
		#if __AVX512DQ__
			#define XSTD_SIMD_WIDTH (512/8)
		#elif __AVX__
			#define XSTD_SIMD_WIDTH (256/8)
		#else
			#define XSTD_SIMD_WIDTH (128/8)
		#endif
	#else
		#define XSTD_SIMD_WIDTH (128/8)
	#endif
#endif

// Declare helper to check vector builtins.
//
#if XSTD_VECTOR_EXT
	#define __has_vector_builtin( x )      __has_builtin( x )
	#define __has_ia32_vector_builtin( x ) AMD64_TARGET && __has_builtin( x )
#else
	#define __has_vector_builtin( x )      0
	#define __has_ia32_vector_builtin( x ) 0
#endif

// Implements wrappers around Clang vector extensions with fallbacks for non-Clang compilers.
//
namespace xstd
{
	namespace impl
	{
		// LLVM intrinsics.
		//
#if CLANG_COMPILER && !defined(__INTELLISENSE__)
		template<typename T, auto N> T __vector_reduce_or(  native_vector<T, N> ) asm( "llvm.vector.reduce.or" );
		template<typename T, auto N> T __vector_reduce_and( native_vector<T, N> ) asm( "llvm.vector.reduce.and" );
		template<typename T, auto N> T __vector_reduce_xor( native_vector<T, N> ) asm( "llvm.vector.reduce.xor" );
		template<typename T, auto N> T __vector_reduce_add( native_vector<T, N> ) asm( "llvm.vector.reduce.add" );
		template<typename T, auto N> T __vector_reduce_mul( native_vector<T, N> ) asm( "llvm.vector.reduce.mul" );
		template<typename T, auto N> T __vector_reduce_fadd( T, native_vector<T, N> ) asm( "llvm.vector.reduce.fadd" );
		template<typename T, auto N> T __vector_reduce_fmul( T, native_vector<T, N> ) asm( "llvm.vector.reduce.fmul" );
#endif

		// Comparison result helper.
		//
		template<typename T> struct comparison_unit;
		template<typename T> requires ( sizeof( T ) == 1 ) struct comparison_unit<T> { using type = char; };
		template<typename T> requires ( sizeof( T ) == 2 ) struct comparison_unit<T> { using type = int16_t; };
		template<typename T> requires ( sizeof( T ) == 4 ) struct comparison_unit<T> { using type = int32_t; };
		template<typename T> requires ( sizeof( T ) == 8 ) struct comparison_unit<T> { using type = int64_t; };
	};
	// Sequence helpers.
	//
	namespace impl
	{
		template<size_t I, size_t N1, size_t N2, int... Ix>
		FORCE_INLINE constexpr auto make_slice_sequence()
		{
			// If we reached the end, return it:
			if constexpr ( I == N2 )
				return std::integer_sequence<int, Ix...>{};
			// If we're at the current vector:
			else if constexpr ( I < N1 )
				return make_slice_sequence<I + 1, N1, N2, Ix..., int( I )>();
			// If we're at the undefined vector:
			else
				return make_slice_sequence<I + 1, N1, N2, Ix..., int( -1 )>();
		}
		template<size_t I, size_t N1, size_t N2, int... Ix>
		FORCE_INLINE constexpr auto make_combination_sequence()
		{
			// Extended max index.
			constexpr size_t NE = N1 >= N2 ? N1 : N2;

			// If we reached the end, return it:
			if constexpr ( I == ( N1 + N2 ) )
				return std::integer_sequence<int, Ix...>{};
			// If we're at the first vector:
			else if constexpr ( I < N1 )
				return make_combination_sequence<I + 1, N1, N2, Ix..., int( I )>();
			// If we're at the second vector:
			else
				return make_combination_sequence<I + 1, N1, N2, Ix..., int( NE + ( I - N1 ) )>();
		}
	};
	template<size_t N1, size_t N2> constexpr auto make_resize_sequence() { return impl::make_slice_sequence<0, N1, N2>(); }
	template<size_t N1, size_t N2> constexpr auto make_combination_sequence() { return impl::make_combination_sequence<0, N1, N2>(); }
	template<size_t N1, size_t Offset, size_t Count> constexpr auto make_slice_sequence() { return impl::make_slice_sequence<Offset, N1, (Offset+Count)>(); }

	template<Arithmetic T, size_t N>
	union xvec
	{
		static constexpr size_t Length = N;
		static constexpr size_t ByteLength = sizeof( T ) * N;
		using native_type = native_vector<T, N>;

		// Storage.
		//
		std::array<T, N> _data = {};
		native_vector<T, N> _nat;

		// Default copy.
		//
		constexpr xvec( const xvec& ) = default;
		constexpr xvec& operator=( const xvec& ) = default;

		// Construction by native vector.
		//
		FORCE_INLINE explicit constexpr xvec( std::in_place_t, native_vector<T, N> nat ) noexcept : _nat( nat ) {}
		
		// Construction by value.
		//
#if XSTD_VECTOR_EXT
		template<typename... Tx> requires ( Convertible<Tx, T> && ... )
		FORCE_INLINE constexpr xvec( Tx... values ) noexcept
		{
			if ( !std::is_constant_evaluated() )
				_nat = native_vector<T, N>{ T( values )... };
			else
			{
				T arr[] = { T( values )... };
				for ( size_t i = 0; i != sizeof...( values ); i++ )
					_data[ i ] = arr[ i ];
			}
		}
#else
		template<typename... Tx> requires ( Convertible<Tx, T> && ... )
		FORCE_INLINE constexpr xvec( Tx... values ) noexcept : _data{ T(values)... } {}
#endif

		// Construction by load.
		//
		FORCE_INLINE constexpr static xvec load( const T* from ) noexcept
		{
			if ( !std::is_constant_evaluated() )
			{
				return xvec( std::in_place_t{}, load_misaligned<native_vector<T, N>>( from ) );
			}
			else
			{
				xvec result{};
				for ( size_t i = 0; i != N; i++ )
					result._data[ i ] = from[ i ];
				return result;
			}
		}
		FORCE_INLINE static xvec load( const void* from ) noexcept
		{
			return xvec( std::in_place_t{}, load_misaligned<native_vector<T, N>>( from ) );
		}

		// Construction by broadcast.
		//
		FORCE_INLINE constexpr static xvec broadcast( T value ) noexcept
		{
#if XSTD_VECTOR_EXT && CLANG_COMPILER
			if ( !std::is_constant_evaluated() )
				return xvec( std::in_place_t{}, native_vector<T, N>( value ) );
#endif
			return xvec{} | value;
		}

		// Indexing.
		//
		FORCE_INLINE constexpr T at( size_t n ) const noexcept
		{
			if ( !std::is_constant_evaluated() ) return _nat[ n ];
			else                                 return _data[ n ];
		}
		FORCE_INLINE constexpr void set( size_t n, T value ) noexcept
		{
			if ( !std::is_constant_evaluated() ) _nat[ n ] = value;
			else                                 _data[ n ] = value;
		}

		struct mut_proxy
		{
			xvec& ref;
			const size_t n;
			constexpr operator T() const noexcept { return ref.at( n ); }
			constexpr mut_proxy& operator=( T v ) noexcept { ref.set( n, v ); return *this; }
		};
		FORCE_INLINE constexpr T operator[]( size_t n ) const noexcept { return at( n ); }
		FORCE_INLINE constexpr mut_proxy operator[]( size_t n ) noexcept { return { *this, n }; }

		// Traits.
		//
		FORCE_INLINE _CONSTEVAL size_t size() const noexcept { return N; }
		FORCE_INLINE _CONSTEVAL size_t capacity() const noexcept { return N; }
		
		// Implement each operator.
		//
#define __DECLARE_UNARY(Op, Req)												      				\
		FORCE_INLINE constexpr xvec operator Op() const Req						         \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
				if ( !std::is_constant_evaluated() )												\
					return xvec( std::in_place_t{}, Op _nat );									\
			xvec result = {};																				\
			for ( size_t i = 0; i != N; i++ )														\
				result._data[ i ] = Op _data[ i ];													\
			return result;																					\
		}																										\
																												
#define __DECLARE_BINARY(Op, Opa, Req)												      		\
		FORCE_INLINE constexpr xvec operator Op( const xvec& other ) const Req	      \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
				if ( !std::is_constant_evaluated() )												\
					return xvec( std::in_place_t{}, _nat Op other._nat );						\
			xvec result = {};																				\
			for ( size_t i = 0; i != N; i++ )														\
				result._data[ i ] = _data[ i ] Op other[ i ];									\
			return result;																					\
		}																										\
		FORCE_INLINE constexpr xvec operator Op( T other ) const Req					   \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
				if ( !std::is_constant_evaluated() )												\
					return xvec( std::in_place_t{}, _nat Op other );							\
			xvec result = {};																				\
			for ( size_t i = 0; i != N; i++ )														\
				result._data[ i ] = _data[ i ] Op other;											\
			return result;																					\
		}																										\
		FORCE_INLINE constexpr xvec& operator Opa( const xvec& other ) Req			   \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
			{																									\
				if ( !std::is_constant_evaluated() )												\
				{																								\
					_nat Opa other._nat;																	\
					return *this;																			\
				}																								\
			}																									\
			for ( size_t i = 0; i != N; i++ )														\
				_data[ i ] Opa other[ i ];																\
			return *this;																					\
		}																										\
		FORCE_INLINE constexpr xvec& operator Opa( T other ) Req						      \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
			{																									\
				if ( !std::is_constant_evaluated() )												\
				{																								\
					_nat Opa other;     																	\
					return *this;																			\
				}																								\
			}																									\
			for ( size_t i = 0; i != N; i++ )														\
				_data[ i ] Opa other;																	\
			return *this;																					\
		}

		using cmpu_t = typename impl::comparison_unit<T>::type;
		using cmp_t =  xvec<cmpu_t, N>;
#define __DECLARE_COMPARISON(Op, Req)												      		   \
		FORCE_INLINE constexpr cmp_t operator Op( const xvec& other ) const Req	         \
		{																										   \
			if constexpr ( XSTD_VECTOR_EXT )															   \
				if ( !std::is_constant_evaluated() )												   \
					return cmp_t( std::in_place_t{}, _nat Op other._nat );					   \
			cmp_t result = {};																			   \
			for ( size_t i = 0; i != N; i++ )														   \
				result._data[ i ] = ( _data[ i ] Op other[ i ] ) ? cmpu_t(-1) : cmpu_t(0); \
			return result;																					\
		}																										\
		FORCE_INLINE constexpr cmp_t operator Op( T other ) const Req					   \
		{																										\
			if constexpr ( XSTD_VECTOR_EXT )															\
				if ( !std::is_constant_evaluated() )												\
					return cmp_t( std::in_place_t{}, _nat Op other );							\
			cmp_t result = {};																			\
			for ( size_t i = 0; i != N; i++ )														\
				result._data[ i ] = ( _data[ i ] Op other ) ? cmpu_t(-1) : cmpu_t(0);   \
			return result;																					\
		}																										

		// Arithmetic.
		__DECLARE_BINARY( +, +=, noexcept );
		__DECLARE_BINARY( -, -=, noexcept );
		__DECLARE_BINARY( *, *=, noexcept );
		__DECLARE_BINARY( /, /=, noexcept );
		__DECLARE_BINARY( %, %=, noexcept );
		__DECLARE_UNARY( -, noexcept );

		// Binary.
		__DECLARE_BINARY( &, &=, noexcept requires Integral<T> );
		__DECLARE_BINARY( |, |=, noexcept requires Integral<T> );
		__DECLARE_BINARY( ^, ^=, noexcept requires Integral<T> );
		__DECLARE_BINARY( >>, >>=, noexcept requires Integral<T> );
		__DECLARE_BINARY( <<, <<=, noexcept requires Integral<T> );
		__DECLARE_UNARY( ~, noexcept requires Integral<T> );

		// Comparison.
		__DECLARE_COMPARISON( ==, noexcept );
		__DECLARE_COMPARISON( !=, noexcept );
		__DECLARE_COMPARISON( <=, noexcept );
		__DECLARE_COMPARISON( >=, noexcept );
		__DECLARE_COMPARISON( <, noexcept );
		__DECLARE_COMPARISON( >, noexcept );
#undef __DECLARE_UNARY
#undef __DECLARE_BINARY
#undef __DECLARE_COMPARISON

		// Cast to same-size vector.
		//
		template<typename Ty>
		[[nodiscard]] FORCE_INLINE constexpr xvec<Ty, ByteLength / sizeof( Ty )> reinterpret() const noexcept
		{
			if constexpr ( Same<Ty, T> )
				return *this;
			constexpr size_t N2 = ByteLength / sizeof( Ty );
			if constexpr ( XSTD_VECTOR_EXT )
				if ( !std::is_constant_evaluated() )
					return xvec<Ty, N2>( std::in_place_t{}, ( native_vector<Ty, N2> ) _nat );

			xvec<Ty, N2> result = {};
			result._data = bit_cast< std::array<Ty, N2> >( _data );
			return result;
		}
		template<typename Ty, size_t N2> requires ( N2 == ( ByteLength / sizeof( T ) ) )
		FORCE_INLINE constexpr operator xvec<Ty, N2>() const noexcept { return reinterpret<Ty>(); }
		FORCE_INLINE constexpr xvec<char, ByteLength> bytes() const noexcept { return reinterpret<char>(); }

		// Cast to same-length vector.
		//
		template<typename Ty>
		[[nodiscard]] FORCE_INLINE constexpr xvec<Ty, N> cast() const noexcept
		{
			if constexpr ( Same<Ty, T> )
				return *this;
			if constexpr ( sizeof( T ) == sizeof( Ty ) && Integral<T> && Integral<Ty> )
				return reinterpret<Ty>();

#if __has_vector_builtin(__builtin_convertvector)
			if ( !std::is_constant_evaluated() )
				return xvec<Ty, N>( std::in_place_t{}, __builtin_convertvector( _nat, native_vector<Ty, N> ) );
#endif
			xvec<Ty, N> result = {};
			for ( size_t i = 0; i != N; i++ )
				result._data[ i ] = ( Ty ) _data[ i ];
			return result;
		}

		// Array conversion.
		//
		FORCE_INLINE constexpr operator std::array<T, N>() const noexcept { return _data; }
		FORCE_INLINE constexpr std::array<T, N> to_array() const noexcept { return _data; }

		// Vector shuffle.
		//
		template<int... Ix>
		FORCE_INLINE constexpr xvec<T, sizeof...( Ix )> shuffle( const xvec& other, std::integer_sequence<int, Ix...> seq = {} ) const noexcept
		{
#if __has_vector_builtin(__builtin_shufflevector)
			if ( !std::is_constant_evaluated() )
				return xvec<T, sizeof...( Ix )>( std::in_place_t{}, __builtin_shufflevector( _nat, other._nat, Ix... ) );
#endif

			xvec<T, sizeof...( Ix )> out = {};
			auto emplace = [ & ] <int Current, int... Rest> ( auto&& self, std::integer_sequence<int, Current, Rest...> ) FORCE_INLINE
			{
				constexpr size_t I = sizeof...( Ix ) - ( sizeof...( Rest ) + 1 );
				if constexpr ( Current >= 0 )
				{
					if constexpr ( Current < N )
						out._data[ I ] = _data[ Current ];
					else
						out._data[ I ] = other._data[ Current - N ];
				}
				else
				{
					if ( std::is_constant_evaluated() )
						out._data[ I ] = 0;
				}

				if constexpr ( sizeof...( Rest ) != 0 )
					self( self, std::integer_sequence<int, Rest...>{} );
			};
			emplace( emplace, seq );
			return out;
		}

		// Vector slicing.
		//
		template<size_t Offset, size_t _Count = std::string::npos> requires ( Offset < N )
		[[nodiscard]] FORCE_INLINE constexpr auto slice() const noexcept
		{
			constexpr size_t Count = _Count == std::string::npos ? ( N - Offset ) : _Count;
			return shuffle( *this, make_slice_sequence<N, Offset, Count>() );

		}
		template<size_t N2>
		[[nodiscard]] FORCE_INLINE constexpr xvec<T, N2> resize() const noexcept
		{
			if constexpr ( N2 == N )
				return *this;
			else
				return shuffle( *this, make_resize_sequence<N, N2>() );
		}

		// Vector combine.
		//
		template<size_t N2>
		[[nodiscard]] FORCE_INLINE constexpr xvec<T, N+N2> combine( const xvec<T, N2>& other ) const noexcept
		{
			constexpr size_t NE = N >= N2 ? N : N2;
			auto a = resize<NE>();
			auto b = other.template resize<NE>();
			return a.shuffle( b, make_combination_sequence<N, N2>() );
		}
		[[nodiscard]] FORCE_INLINE constexpr xvec<T, N + 1> combine( T value ) const noexcept
		{
			auto result = resize<N+1>();
			result.set( N, value );
			return result;
		}

		// Vector zero comparison.
		//
		FORCE_INLINE constexpr bool is_zero() const noexcept
		{
			// Fix odd sizes.
			//
			auto vec_bytes = bytes();
			if constexpr ( ByteLength != std::bit_ceil( ByteLength ) )
			{
				return vec_bytes.template resize<std::bit_ceil( ByteLength )>().is_zero();
			}
			// Handle integral sizes.
			//
			else if constexpr ( ByteLength <= 8 )
			{
				using U = convert_uint_t<decltype( vec_bytes._data )>;
				return bit_cast< U >( vec_bytes._data ) == 0;
			}

			// Handle hardware accelerated sizes.
			//
			if ( !std::is_constant_evaluated() )
			{
#if AMD64_TARGET
#if MS_COMPILER
				if constexpr ( ByteLength == 16 )
					return ( bool ) _mm_testz_si128( bit_cast< __m128i >( vec_bytes._nat ), bit_cast< __m128i >( vec_bytes._nat ) );
				else if constexpr ( ByteLength == 32 )
					return ( bool ) _mm256_testz_si256( bit_cast< __m256i >( vec_bytes._nat ), bit_cast< __m256i >( vec_bytes._nat ) );
#else
#if __has_ia32_vector_builtin( __builtin_ia32_ptestz128 )
				if constexpr ( ByteLength == 16 )
					return ( bool ) __builtin_ia32_ptestz128( vec_bytes._nat, vec_bytes._nat );
#endif
#if __has_ia32_vector_builtin( __builtin_ia32_ptestz256 )
				if constexpr ( ByteLength == 32 )
					return ( bool ) __builtin_ia32_ptestz256( vec_bytes._nat, vec_bytes._nat );
#endif
#endif
#endif
				
				// Handle extended sizes.
				//
				if constexpr ( ByteLength > 32 )
				{
					if constexpr ( !( ByteLength % 2 ) )
					{
						constexpr size_t HalfLength = ByteLength / 2;
						auto vtmp = vec_bytes.template resize<HalfLength>();
						vtmp |= vec_bytes.template slice<HalfLength, HalfLength>();
						return vtmp.is_zero();
					}
					else
					{
						return
							vec_bytes.template resize<32>().is_zero() &
							vec_bytes.template slice<32>().is_zero();
					}
				}
			}

			// Handle constexpr.
			//
			uint8_t result = 0;
			for ( size_t i = 0; i != ByteLength; i++ )
				result |= vec_bytes._data[ i ];
			return result == 0;
		}

		// Creates a mask made up of the most significant bit of each byte.
		//
		FORCE_INLINE constexpr uint32_t bmask() const noexcept requires ( ByteLength <= 32 )
		{
			auto byte_vec = bytes();

			// Fix odd sizes.
			//
			if constexpr ( ByteLength != std::bit_ceil( ByteLength ) )
				return byte_vec.template resize<std::bit_ceil( ByteLength )>().bmask();

			// Handle hardware accelerated sizes.
			//
			if ( !std::is_constant_evaluated() )
			{
#if AMD64_TARGET
#if MS_COMPILER
				if constexpr ( ByteLength == 16 )
					return ( bool ) _mm_movemask_epi8( bit_cast< __m128i >( byte_vec._nat ) );
				else if constexpr ( ByteLength == 32 )
					return ( bool ) _mm256_movemask_epi8( bit_cast< __m256i >( byte_vec._nat ) );
#else
#if __has_ia32_vector_builtin( __builtin_ia32_pmovmskb128 )
				if constexpr ( ByteLength == 16 )
					return ( uint32_t ) __builtin_ia32_pmovmskb128( byte_vec._nat );
#endif
#if __has_ia32_vector_builtin( __builtin_ia32_pmovmskb256 )
				if constexpr ( ByteLength == 32 )
					return ( uint32_t ) __builtin_ia32_pmovmskb256( byte_vec._nat );
#endif
#endif
#endif
				// Handle shrinked sizes.
				//
				if constexpr ( ByteLength != 1 && ByteLength <= 8 )
					return byte_vec.template resize<16>().bmask();
			}

			// Handle constexpr.
			//
			uint32_t result = 0;
			for ( size_t i = 0; i != ByteLength; i++ )
				result |= ( ( byte_vec.at( i ) >> 7 ) & 1 ) << i;
			return result;
		}
		// - Split up calculation for > u32.
		FORCE_INLINE constexpr uint64_t bmask() const noexcept requires ( 32 < ByteLength && ByteLength <= 64 )
		{
			auto byte_vec = bytes();
			uint32_t low =  byte_vec.template slice<0, 32>().bmask();
			uint32_t high = byte_vec.template slice<32, 32>().bmask();
			return low | ( uint64_t( high ) << 32 );
		}

		// Creates a mask made up of the most significant bit of each element(!).
		//
		FORCE_INLINE constexpr auto mask() const noexcept requires( Length <= 64 && Arithmetic<T> )
		{
			if constexpr ( sizeof( T ) == 1 )
			{
				return bmask();
			}
			else
			{
				auto rotated = ( *this ) >> ( ( sizeof( T ) - 1 ) * 8 );
				return ( rotated.template cast<char>() ).bmask();
			}
		}

		// Vector equal comparison.
		//
		FORCE_INLINE constexpr bool equals( xvec<T, N> other ) const noexcept
		{
			return ( bytes() ^ other.bytes() ).is_zero();
		}

		// Tie for hashing and string conversion.
		//
		FORCE_INLINE constexpr auto tie() noexcept { return std::tie( _data ); }
	};

	// Deduction guide.
	//
	template<typename T, typename... Tx>
	xvec( T, Tx... )->xvec<T, sizeof...( Tx ) + 1>;

	// Byte vector.
	//
	template<size_t N>
	using bvec = xvec<char, N>;

	// Common SIMD sizes.
	//
	template<typename T = char> 
	using vec128_t =  xvec<T, 16 / sizeof( T )>;
	template<typename T = char>
	using vec256_t =  xvec<T, 32 / sizeof( T )>;
	template<typename T = char>
	using vec512_t =  xvec<T, 64 / sizeof( T )>;
	template<typename T = char>
	using max_vec_t = xvec<T, XSTD_SIMD_WIDTH / sizeof( T )>;

	// Vector traits.
	//
	template<typename T>
	struct vector_traits;
	template<typename T, size_t N>
	struct vector_traits<xvec<T, N>>
	{
		using value_type =             T;
		static constexpr size_t size = N;
	};
	template<typename T> struct vector_traits<T&>      : vector_traits<T> {};
	template<typename T> struct vector_traits<const T> : vector_traits<T> {};

	template<typename T> 
	using vector_element_t = typename vector_traits<T>::value_type;
	template<typename T> 
	static constexpr size_t vector_size_v = vector_traits<T>::size;
	template<typename T> 
	concept Vector = requires{ typename vector_traits<T>::element_type; };

	// Vector operations.
	//
	namespace vec
	{
		// Syntax sugars.
		//
		template<typename Tn, typename T, size_t N>
		FORCE_INLINE inline constexpr xvec<Tn, N> cast( xvec<T, N> vec ) noexcept { return vec.template cast<Tn>(); }
		template<typename T, size_t N> 
		FORCE_INLINE inline constexpr bvec<sizeof( T[ N ] )> bytes( xvec<T, N> x ) noexcept { return x.bytes(); }

		// Vector broadcast.
		//
		template<Arithmetic T, size_t N>
		FORCE_INLINE inline constexpr xvec<T, N> broadcast( T value ) { return xvec<T, N>::broadcast( value ); }
		template<Arithmetic T, size_t N> inline constexpr xvec<T, N> zero =    broadcast<T, N>( T(0) );
		template<Arithmetic T, size_t N> inline constexpr xvec<T, N> inverse = broadcast<T, N>( ~T() );

		// Vector from varargs.
		//
		template<size_t N, Arithmetic T, Arithmetic... Tx> requires ( ( sizeof...( Tx ) + 1 ) <= N )
		FORCE_INLINE inline constexpr xvec<T, N> from_n( T value, Tx... rest )
		{
			return xvec<T, N>( value, rest... );
		}
		template<Arithmetic T, Arithmetic... Tx>
		FORCE_INLINE inline constexpr xvec<T, ( sizeof...( Tx ) + 1 )> from( T value, Tx... rest )
		{
			return xvec<T, ( sizeof...( Tx ) + 1 )>( value, rest... );
		}

		// Vector from array.
		//
		template<Arithmetic T, size_t N>
		FORCE_INLINE inline constexpr xvec<T, N> from( std::array<T, N> arr )
		{
			xvec<T, N> vec = {};
			vec._data = arr;
			return vec;
		}

		// Vector from constant span.
		//
		template<Arithmetic T, size_t N> requires ( N != std::dynamic_extent )
		FORCE_INLINE inline constexpr xvec<T, N> from( std::span<T, N> span )
		{
			xvec<T, N> vec = {};
			for( size_t n = 0; n != N; n++ )
				vec._data[ n ] = span[ n ];
			return vec;
		}

		// Element-wise operations.
		//
		template<typename T, size_t N>
		FORCE_INLINE inline constexpr xvec<T, N> (max)( xvec<T, N> x, xvec<T, N> y ) noexcept
		{
#if __has_vector_builtin(__builtin_elementwise_max)
			if ( !std::is_constant_evaluated() )
				return xvec<T, N>( std::in_place_t{}, __builtin_elementwise_max( x, y ) );
#endif
			for ( size_t i = 0; i != N; i++ )
				x[ i ] = std::max<T>( x[ i ], y[ i ] );
			return x;
		}
		template<typename T, size_t N>
		FORCE_INLINE inline constexpr xvec<T, N> (min)( xvec<T, N> x, xvec<T, N> y ) noexcept
		{
#if __has_vector_builtin(__builtin_elementwise_min)
			if ( !std::is_constant_evaluated() )
				return xvec<T, N>( std::in_place_t{}, __builtin_elementwise_min( x, y ) );
#endif
			for ( size_t i = 0; i != N; i++ )
				x[ i ] = std::min<T>( x[ i ], y[ i ] );
			return x;
		}

		// Permutation.
		//
		FORCE_INLINE inline constexpr xvec<int32_t, 8> perm8x32( xvec<int32_t, 8> vec, xvec<int32_t, 8> offsets )
		{
#if __has_ia32_vector_builtin( __builtin_ia32_permvarsi256 )
			if ( !std::is_constant_evaluated() )
				return xvec<int32_t, 8>( std::in_place_t{}, __builtin_ia32_permvarsi256( vec._nat, offsets._nat ) );
#endif
			xvec<int32_t, 8> result = {};
			for ( size_t i = 0; i != 8; i++ )
				result[ i ] = ( int32_t ) vec[ offsets[ i ] % 8 ];
			return result;
		}

		// Vector reduction.
		//
		template<typename T, auto N>
		FORCE_INLINE inline constexpr T reduce_or( xvec<T, N> vec )
		{
#if CLANG_COMPILER && !defined(__INTELLISENSE__)
			if constexpr( sizeof( vec._nat ) == sizeof( vec._data ) && N >= 4 )
				if ( !std::is_constant_evaluated() )
					return impl::__vector_reduce_or<T, N>( vec._nat );
#endif
			T result = vec[ 0 ];
			for ( size_t i = 1; i != N; i++ )
				result |= vec[ i ];
			return result;
		}
		template<typename T, auto N>
		FORCE_INLINE inline constexpr T reduce_and( xvec<T, N> vec )
		{
#if CLANG_COMPILER && !defined(__INTELLISENSE__)
			if constexpr( sizeof( vec._nat ) == sizeof( vec._data ) && N >= 4 )
				if ( !std::is_constant_evaluated() )
					return impl::__vector_reduce_and<T, N>( vec._nat );
#endif
			T result = vec[ 0 ];
			for ( size_t i = 1; i != N; i++ )
				result &= vec[ i ];
			return result;
		}
		template<typename T, auto N>
		FORCE_INLINE inline constexpr T reduce_xor( xvec<T, N> vec )
		{
#if CLANG_COMPILER && !defined(__INTELLISENSE__)
			if constexpr ( sizeof( vec._nat ) == sizeof( vec._data ) && N >= 4 )
				if ( !std::is_constant_evaluated() )
					return impl::__vector_reduce_xor<T, N>( vec._nat );
#endif
			T result = vec[ 0 ];
			for ( size_t i = 1; i != N; i++ )
				result ^= vec[ i ];
			return result;
		}
		template<typename T, auto N>
		FORCE_INLINE inline constexpr T reduce_add( xvec<T, N> vec )
		{
#if CLANG_COMPILER && !defined(__INTELLISENSE__)
			if constexpr ( sizeof( vec._nat ) == sizeof( vec._data ) && N >= 4 )
			{
				if ( !std::is_constant_evaluated() )
				{
					if constexpr ( !FloatingPoint<T> )
						return impl::__vector_reduce_add<T, N>( vec._nat );
					else
						return impl::__vector_reduce_fadd<T, N>( 0, vec._nat );
				}
			}
#endif
			T result = vec[ 0 ];
			for ( size_t i = 1; i != N; i++ )
				result += vec[ i ];
			return result;
		}
		template<typename T, auto N>
		FORCE_INLINE inline constexpr T reduce_mul( xvec<T, N> vec )
		{
#if CLANG_COMPILER && !defined(__INTELLISENSE__)
			if constexpr ( sizeof( vec._nat ) == sizeof( vec._data ) && N >= 4 )
			{
				if ( !std::is_constant_evaluated() )
				{
					if constexpr ( !FloatingPoint<T> )
						return impl::__vector_reduce_mul<T, N>( vec._nat );
					else
						return impl::__vector_reduce_fmul<T, N>( 1, vec._nat );
				}
			}
#endif
			T result = vec[ 0 ];
			for ( size_t i = 1; i != N; i++ )
				result *= vec[ i ];
			return result;
		}

		// Load/Store non-temporal wrappers.
		//
		template<typename T>
		FORCE_INLINE inline T load_nontemporal( const void* p )
		{
			return T( std::in_place_t{}, xstd::load_nontemporal( ( const typename T::native_type* ) p ) );
		}
		template<typename T>
		FORCE_INLINE inline void store_nontemporal( void* p, T value )
		{
			xstd::store_nontemporal( ( typename T::native_type* ) p, value._nat );
		}
	};
};