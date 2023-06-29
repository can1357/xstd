#pragma once
#include "math.hpp"

// Implements linear algebra extensions.
//
namespace xstd::math 
{
	struct identity_t {};

	namespace impl {
		template<typename T, size_t X, size_t Y>
		struct TRIVIAL_ABI matrix_storage {
			std::array<T, X* Y> _store = {};

			// Common interface.
			//
			using key_type = size_t;
			using unit_type = T;
			static constexpr size_t Columns = X;
			static constexpr size_t Rows = Y;
			constexpr size_t cols() const { return X; }
			constexpr size_t rows() const { return Y; }
			constexpr size_t llen() const { return cols() * rows(); }
			constexpr T* __restrict data() { return &_store[ 0 ]; }
			constexpr const T* __restrict data() const { return &_store[ 0 ]; }
			constexpr T& __restrict at( size_t row, size_t col ) { return data()[ row * cols() + col ]; }
			constexpr const T& __restrict at( size_t row, size_t col ) const { return data()[ row * cols() + col ]; }
			constexpr T& __restrict operator()( size_t row, size_t col ) { return at( row, col ); }
			constexpr const T& __restrict operator()( size_t row, size_t col ) const { return at( row, col ); }

			// Default construction.
			//
			constexpr matrix_storage() = default;

			// Copy and move.
			//
			constexpr matrix_storage( const matrix_storage& o ) = default;
			constexpr matrix_storage( matrix_storage&& o ) noexcept = default;
			constexpr matrix_storage& operator=( const matrix_storage& o ) = default;
			constexpr matrix_storage& operator=( matrix_storage&& o ) noexcept = default;

			// Construction from matrix-like.
			//
			template<typename Ty> requires ( !std::is_same_v<Ty, T> && ( Ty::Columns == std::dynamic_extent || ( Ty::Columns * Ty::Rows ) == ( Columns * Rows ) ) )
				constexpr matrix_storage( const Ty& o ) : matrix_storage( o.cols(), o.rows(), o.data() ) {}

			// Fill/Copy/Identity.
			//
			constexpr matrix_storage( const T& value ) : matrix_storage() { fill( value ); }
			constexpr matrix_storage( const T* src ) : matrix_storage() { fill( src ); }
			constexpr matrix_storage( identity_t ) requires ( X == Y ) : matrix_storage() {
				for ( size_t n = 0; n != X; n++ ) {
					this->at( n, n ) = 1;
				}
			}

			// Compatibility with dynamic matrix.
			//
			constexpr matrix_storage( size_t x, size_t y ) : matrix_storage() {
				dassert( ( x * y ) == ( X * Y ) );
			}
			constexpr matrix_storage( size_t x, size_t y, const T* src ) : matrix_storage( x, y ) { fill( src ); }
			constexpr matrix_storage( size_t x, size_t y, const T& value ) : matrix_storage( x, y ) { fill( value ); }
			constexpr matrix_storage( size_t x, identity_t ) requires ( X == Y ) : matrix_storage( identity_t{} ) {
				dassert( x == X );
			}

			// Swap.
			//
			constexpr void swap( matrix_storage& o ) { std::swap( _store, o._store ); }

			// Fill.
			//
			template<typename Ty = T>
			constexpr void fill( const Ty* __restrict src ) {
				std::copy_n( src, llen(), &_store[ 0 ] );
			}
			constexpr void fill( const T& value ) {
				std::fill_n( &_store[ 0 ], llen(), value );
			}
		};

		template<typename T>
		struct TRIVIAL_ABI matrix_storage<T, std::dynamic_extent, std::dynamic_extent> {
			uint32_t nx = 0;
			uint32_t ny = 0;
			T* __restrict ptr = nullptr;

			// Common interface.
			//
			using key_type = size_t;
			using unit_type = T;
			static constexpr size_t Columns = std::dynamic_extent;
			static constexpr size_t Rows = std::dynamic_extent;
			constexpr size_t cols() const { return nx; }
			constexpr size_t rows() const { return ny; }
			constexpr size_t llen() const { return cols() * rows(); }
			constexpr T* __restrict data() { return ptr; }
			constexpr const T* __restrict data() const { return ptr; }
			constexpr T& __restrict at( size_t row, size_t col ) { return data()[ row * cols() + col ]; }
			constexpr const T& __restrict at( size_t row, size_t col ) const { return data()[ row * cols() + col ]; }
			constexpr T& __restrict operator()( size_t row, size_t col ) { return at( row, col ); }
			constexpr const T& __restrict operator()( size_t row, size_t col ) const { return at( row, col ); }

			// Default construction of Matrix<0,0>.
			//
			constexpr matrix_storage() = default;

			// Copy and move.
			//
			constexpr matrix_storage( const matrix_storage& o ) : matrix_storage( o.cols(), o.rows(), o.data() ) {}
			constexpr matrix_storage( matrix_storage&& o ) noexcept { swap( o ); }
			constexpr matrix_storage& operator=( const matrix_storage& o ) {
				matrix_storage clone{ o };
				swap( clone );
				return *this;
			}
			constexpr matrix_storage& operator=( matrix_storage&& o ) noexcept {
				swap( o );
				return *this;
			}

			// Construction from matrix-like.
			//
			template<typename Ty> requires ( !std::is_same_v<Ty, T> )
				constexpr matrix_storage( const Ty& o ) : matrix_storage( o.cols(), o.rows(), o.data() ) {}

			// Alloc/Fill/Copy/Identity.
			//
			constexpr matrix_storage( size_t x, size_t y ) {
				nx = x;
				ny = y;
				ptr = ( x | y ) ? new T[ x * y ]() : nullptr;
			}
			constexpr matrix_storage( size_t x, size_t y, const T& value ) : matrix_storage( x, y ) { fill( value ); }
			constexpr matrix_storage( size_t x, size_t y, const T* src ) : matrix_storage( x, y ) { fill( src ); }
			constexpr matrix_storage( size_t x, identity_t ) : matrix_storage( x, x ) {
				for ( size_t n = 0; n != x; n++ ) {
					this->at( n, n ) = 1;
				}
			}


			// Reset and swap.
			//
			constexpr void swap( matrix_storage& o ) {
				std::swap( nx, o.nx );
				std::swap( ny, o.ny );
				std::swap( ptr, o.ptr );
			}
			constexpr void reset() {
				if ( ptr ) {
					delete[] ptr;
				}
				nx = ny = 0;
				ptr = nullptr;
			}
			constexpr ~matrix_storage() { reset(); }


			// Fill.
			//
			template<typename Ty>
			constexpr void fill( const Ty* __restrict src ) {
				std::copy_n( src, llen(), ptr );
			}
			constexpr void fill( const T& value ) {
				std::fill_n( ptr, llen(), value );
			}
		};


		template<typename A, typename B>
		constexpr size_t MulX = ( A::Columns == std::dynamic_extent || B::Columns == std::dynamic_extent ) ? std::dynamic_extent : B::Columns;
		template<typename A, typename B>
		constexpr size_t MulY = ( A::Rows == std::dynamic_extent || B::Rows == std::dynamic_extent ) ? std::dynamic_extent : A::Rows;
		template<typename A, typename B>
		using MulT = decltype( std::declval<typename A::unit_type>()* std::declval<typename B::unit_type>() );

		// Class implementing utilities.
		//
		template<typename T, size_t X, size_t Y>
		struct TRIVIAL_ABI matrix_t : matrix_storage<T, X, Y> {
			// Inherit constructors and assignment.
			//
			using matrix_storage<T, X, Y>::matrix_storage;
			using matrix_storage<T, X, Y>::operator=;

			// Casting.
			//
			template<typename Ty>
			constexpr matrix_t<Ty, X, Y> cast() {
				matrix_t<Ty, X, Y> copy = { this->cols(), this->rows() };
				copy.fill( this->data() );
				return copy;
			}
			template<typename Ty>
			FORCE_INLINE inline constexpr operator matrix_t<Ty, X, Y>() const noexcept { return this->template cast<Ty>(); }

			// String conversion.
			//
			FORCE_INLINE inline std::string to_string() const noexcept
			{
				std::string parts = {};
				const T* data = this->data();
				const size_t xl = this->cols();
				const size_t yl = this->rows();
				for ( size_t y = 0; y != yl; y++ ) {
					parts += '|';
					for ( size_t x = 0; x != xl; x++ ) {
						char buffer[ 24 ];
						size_t n = ( size_t ) sprintf_s( buffer, " %-10f", *data++ );
						parts += std::string_view{buffer, n};
					}
					parts += ( y + 1 ) == yl ? "|" : "|\n";
				}
				return parts;
			}

			// Scalar operations.
			//
			FORCE_INLINE constexpr matrix_t clone() const { return *this; }

			template<typename Ty, typename F>
			FORCE_INLINE constexpr matrix_t&& stransform( Ty&& x, F&& fn )&& {
				if constexpr ( !std::is_scalar_v<std::decay_t<Ty>> ) {
					// Broadcast:

					size_t ar = this->rows(), br = x.rows();
					size_t ac = this->cols(), bc = x.cols();
					T* __restrict it = this->data();

					// Same column count, broadcast along row:
					//
					if ( br == 1 && bc == ac ) {
						for ( size_t r = 0; r != ar; r++ ) {
							auto* __restrict it2 = x.data();
							for ( size_t c = 0; c != ac; c++, it++, it2++ ) {
								*it = fn( *it, *it2 );
							}
						}
					}
					// Same row count, broadcast along column:
					//
					else if ( bc == 1 && br == ar ) {
						auto* __restrict it2 = x.data();
						for ( size_t r = 0; r != ar; r++, it2++ ) {
							for ( size_t c = 0; c != ac; c++, it++ ) {
								*it = fn( *it, *it2 );
							}
						}
					}
					// Same everything!
					//
					else if ( bc == ac && br == ar ) {
						auto* __restrict it2 = x.data();
						for ( size_t r = 0; r != ar; r++ ) {
							for ( size_t c = 0; c != ac; c++, it++, it2++ ) {
								*it = fn( *it, *it2 );
							}
						}
					} else {
						dassert( false );
					}
				} else {
					for ( auto& f : std::span{ this->data(), this->llen() } ) {
						f = fn( f, x );
					}
				}
				return std::move( *this );
			}
			template<typename Ty, typename F>
			FORCE_INLINE constexpr matrix_t stransform( Ty&& x, F&& fn )& {
				return clone().stransform( std::forward<Ty>( x ), std::forward<F>( fn ) );
			}
			template<typename Ty, typename F>
			FORCE_INLINE constexpr matrix_t stransform( Ty&& x, F&& fn ) const& {
				return clone().stransform( std::forward<Ty>( x ), std::forward<F>( fn ) );
			}

			template<typename Ty> FORCE_INLINE constexpr matrix_t smul( Ty&& x )& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a * b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t sdiv( Ty&& x )& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a / b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t sadd( Ty&& x )& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a + b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t ssub( Ty&& x )& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a - b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t spow( Ty&& x )& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return math::fpow( a, b ); } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t smod( Ty&& x )& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return math::fmod( a, b ); } ); }
			FORCE_INLINE constexpr matrix_t neg()& { return stransform( 0.0f, [ ] ( auto a, auto ) { return -a; } ); }
			FORCE_INLINE constexpr matrix_t sqrt()& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::fsqrt( a ); } ); }
			FORCE_INLINE constexpr matrix_t sin()& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::fsin( a ); } ); }
			FORCE_INLINE constexpr matrix_t cos()& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::fcos( a ); } ); }
			FORCE_INLINE constexpr matrix_t round()& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::fround( a ); } ); }
			FORCE_INLINE constexpr matrix_t trunc()& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::ftrunc( a ); } ); }
			FORCE_INLINE constexpr matrix_t ceil()& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::fceil( a ); } ); }
			FORCE_INLINE constexpr matrix_t floor()& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::ffloor( a ); } ); }

			template<typename Ty> FORCE_INLINE constexpr matrix_t smul( Ty&& x ) const& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a * b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t sdiv( Ty&& x ) const& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a / b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t sadd( Ty&& x ) const& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a + b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t ssub( Ty&& x ) const& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a - b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t spow( Ty&& x ) const& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return math::fpow( a, b ); } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t smod( Ty&& x ) const& { return stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return math::fmod( a, b ); } ); }
			FORCE_INLINE constexpr matrix_t neg()const& { return stransform( 0.0f, [ ] ( auto a, auto ) { return -a; } ); }
			FORCE_INLINE constexpr matrix_t sqrt()const& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::fsqrt( a ); } ); }
			FORCE_INLINE constexpr matrix_t sin()const& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::fsin( a ); } ); }
			FORCE_INLINE constexpr matrix_t cos()const& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::fcos( a ); } ); }
			FORCE_INLINE constexpr matrix_t round()const& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::fround( a ); } ); }
			FORCE_INLINE constexpr matrix_t trunc()const& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::ftrunc( a ); } ); }
			FORCE_INLINE constexpr matrix_t ceil()const& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::fceil( a ); } ); }
			FORCE_INLINE constexpr matrix_t floor()const& { return stransform( 0.0f, [ ] ( auto a, auto ) { return math::ffloor( a ); } ); }

			template<typename Ty> FORCE_INLINE constexpr matrix_t&& smul( Ty&& x )&& { return std::move( *this ).stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a * b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t&& sdiv( Ty&& x )&& { return std::move( *this ).stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a / b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t&& sadd( Ty&& x )&& { return std::move( *this ).stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a + b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t&& ssub( Ty&& x )&& { return std::move( *this ).stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return a - b; } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t&& spow( Ty&& x )&& { return std::move( *this ).stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return math::fpow( a, b ); } ); }
			template<typename Ty> FORCE_INLINE constexpr matrix_t&& smod( Ty&& x )&& { return std::move( *this ).stransform( std::forward<Ty>( x ), [ ] ( auto a, auto b ) { return math::fmod( a, b ); } ); }
			FORCE_INLINE constexpr matrix_t&& neg()&& { return std::move( *this ).stransform( 0.0f, [ ] ( auto a, auto ) { return -a; } ); }
			FORCE_INLINE constexpr matrix_t&& sqrt()&& { return std::move( *this ).stransform( 0.0f, [ ] ( auto a, auto ) { return math::fsqrt( a ); } ); }
			FORCE_INLINE constexpr matrix_t&& sin()&& { return std::move( *this ).stransform( 0.0f, [ ] ( auto a, auto ) { return math::fsin( a ); } ); }
			FORCE_INLINE constexpr matrix_t&& cos()&& { return std::move( *this ).stransform( 0.0f, [ ] ( auto a, auto ) { return math::fcos( a ); } ); }
			FORCE_INLINE constexpr matrix_t&& round()&& { return std::move( *this ).stransform( 0.0f, [ ] ( auto a, auto ) { return math::fround( a ); } ); }
			FORCE_INLINE constexpr matrix_t&& trunc()&& { return std::move( *this ).stransform( 0.0f, [ ] ( auto a, auto ) { return math::ftrunc( a ); } ); }
			FORCE_INLINE constexpr matrix_t&& ceil()&& { return std::move( *this ).stransform( 0.0f, [ ] ( auto a, auto ) { return math::fceil( a ); } ); }
			FORCE_INLINE constexpr matrix_t&& floor()&& { return std::move( *this ).stransform( 0.0f, [ ] ( auto a, auto ) { return math::ffloor( a ); } ); }

			FORCE_INLINE constexpr matrix_t operator+()& { return clone(); }
			FORCE_INLINE constexpr matrix_t operator+()const& { return clone(); }
			FORCE_INLINE constexpr matrix_t operator+()&& { return std::move( *this ); }
			FORCE_INLINE constexpr matrix_t operator-()& { return neg(); }
			FORCE_INLINE constexpr matrix_t operator-()const& { return neg(); }
			FORCE_INLINE constexpr matrix_t operator-()&& { return std::move( *this ).neg(); }
			FORCE_INLINE constexpr matrix_t operator+( T x )& { return sadd( x ); }
			FORCE_INLINE constexpr matrix_t operator+( T x ) const& { return sadd( x ); }
			FORCE_INLINE constexpr matrix_t&& operator+( T x )&& { return std::move( *this ).sadd( x ); }
			FORCE_INLINE constexpr matrix_t& operator+=( T x ) { return std::move( *this ).sadd( x ); }
			FORCE_INLINE constexpr matrix_t operator-( T x )& { return ssub( x ); }
			FORCE_INLINE constexpr matrix_t operator-( T x ) const& { return ssub( x ); }
			FORCE_INLINE constexpr matrix_t&& operator-( T x )&& { return std::move( *this ).ssub( x ); }
			FORCE_INLINE constexpr matrix_t& operator-=( T x ) { return std::move( *this ).ssub( x ); }
			FORCE_INLINE constexpr matrix_t operator*( T x )& { return smul( x ); }
			FORCE_INLINE constexpr matrix_t operator*( T x ) const& { return smul( x ); }
			FORCE_INLINE constexpr matrix_t&& operator*( T x )&& { return std::move( *this ).smul( x ); }
			FORCE_INLINE constexpr matrix_t& operator*=( T x ) { return std::move( *this ).smul( x ); }
			FORCE_INLINE constexpr matrix_t operator/( T x )& { return sdiv( x ); }
			FORCE_INLINE constexpr matrix_t operator/( T x ) const& { return sdiv( x ); }
			FORCE_INLINE constexpr matrix_t&& operator/( T x )&& { return std::move( *this ).sdiv( x ); }
			FORCE_INLINE constexpr matrix_t& operator/=( T x ) { return std::move( *this ).sdiv( x ); }
			FORCE_INLINE constexpr matrix_t operator%( T x )& { return smod( x ); }
			FORCE_INLINE constexpr matrix_t operator%( T x ) const& { return smod( x ); }
			FORCE_INLINE constexpr matrix_t&& operator%( T x )&& { return std::move( *this ).smod( x ); }
			FORCE_INLINE constexpr matrix_t& operator%=( T x ) { return std::move( *this ).smod( x ); }

			// Matrix multiplication.
			//
			template<typename Ty>
			FORCE_INLINE constexpr auto dot( const Ty& rhs ) const ->
				matrix_t<MulT<matrix_t<T, X, Y>, Ty>, MulX<matrix_t<T, X, Y>, Ty>, MulY<matrix_t<T, X, Y>, Ty>>
			{
				auto& lhs = *this;
				dassert( lhs.cols() == rhs.rows() );

				using C = MulT<matrix_t<T, X, Y>, Ty>;
				matrix_t<C, MulX<matrix_t<T, X, Y>, Ty>, MulY<matrix_t<T, X, Y>, Ty>> result{ rhs.cols(), lhs.rows() };
				for ( size_t i = 0; i < lhs.rows(); ++i ) {
					for ( size_t j = 0; j < rhs.cols(); ++j ) {
						C sum = 0;
						for ( size_t k = 0; k < lhs.cols(); ++k ) {
							sum += lhs( i, k ) * rhs( k, j );
						}
						result( i, j ) = sum;
					}
				}
				return result;
			}

			// Matrix transposition.
			//
			FORCE_INLINE constexpr matrix_t<T, Y, X> transpose() const {
				matrix_t<T, Y, X> result = { this->rows(), this->cols() };

				// Acceleration.
				//
				if ( result.rows() == 4 && result.cols() == 4 && !std::is_constant_evaluated() ) {
					*( ( mat4x4_t<T>* ) result.data() ) = ( ( const mat4x4_t<T>* ) this->data() )->transpose();
					return result;
				}

				if ( result.cols() == 1 || result.rows() == 1 ) {
					result.fill( this->data() );
				} else {
					for ( size_t i = 0; i != this->rows(); i++ ) {
						for ( size_t j = 0; j != this->cols(); j++ ) {
							result( j, i ) = this->at( i, j );
						}
					}
				}
				return result;
			}

			// Minor matrix, cofactors and determinant.
			//
			FORCE_INLINE constexpr auto minor( size_t row, size_t col ) const ->
				matrix_t<T, X == std::dynamic_extent ? std::dynamic_extent : X - 1, X == std::dynamic_extent ? std::dynamic_extent : X - 1>
				requires ( X == Y )
			{
				size_t n = this->rows();
				matrix_t<T, X == std::dynamic_extent ? std::dynamic_extent : X - 1, X == std::dynamic_extent ? std::dynamic_extent : X - 1> r = { n - 1, n - 1 };

				for ( size_t i = 0; i != n; i++ ) {
					size_t minor_row = i;
					if ( i > row )
						minor_row--;
					for ( size_t j = 0; j != n; j++ ) {
						size_t minor_col = j;
						if ( j > col )
							minor_col--;
						if ( i != row && j != col )
							r( minor_row, minor_col ) = this->at( i, j );
					}
				}
				return r;
			}
			FORCE_INLINE constexpr T cofactor( size_t row, size_t col ) const requires ( X == Y ) {
				return ( ( ( row + col ) & 1 ) ? -1 : +1 ) * this->minor( row, col ).determinant();
			}
			FORCE_INLINE constexpr T determinant() const requires ( X == Y ) {
				dassert( this->rows() == this->cols() );

				// Acceleration.
				//
				size_t n = this->cols();
				if ( n == 2 ) {
					return ( this->at( 0, 0 ) * this->at( 1, 1 ) ) - ( this->at( 1, 0 ) * this->at( 0, 1 ) );
				} else if ( n == 4 && !std::is_constant_evaluated() ) {
					return ( ( mat4x4_t<T>* ) this->data() )->determinant();
				}

				T det = 0;
				constexpr size_t N = X == std::dynamic_extent ? std::dynamic_extent : X - 1;
				matrix_t<T, N, N> sub = { n - 1, n - 1 };
				for ( size_t x = 0; x != n; x++ ) {
					det += this->at( 0, x ) * this->cofactor( 0, x );
				}
				return det;
			}

			// Matrix inversion.
			//
			FORCE_INLINE constexpr matrix_t inverse( T& _det ) const requires ( X == Y ) {
				// Acceleration.
				//
				size_t n = this->cols();
				matrix_t result{ n, n };
				if ( n == 4 && !std::is_constant_evaluated() ) {
					*( ( mat4x4_t<T>* ) result.data() ) = math::inverse( *( const mat4x4_t<T>* ) this->data(), _det );
					return result;
				}

				const T det = ( _det = this->determinant() );
				T rdet = 1 / det;
				if ( n == 2 ) {
					T mat[] = {
						this->at( 1,1 ) * rdet, -1 * this->at( 0,1 ) * rdet,
						-1 * this->at( 1,0 ) * rdet, this->at( 0,0 ) * rdet
					};
					return matrix_t{ n, n, mat };
				}
				for ( size_t i = 0; i != n; i++ ) {
					for ( size_t j = 0; j != n; j++ ) {
						result( j, i ) = this->cofactor( i, j ) * rdet;
					}
				}
				return result;
			}
			FORCE_INLINE constexpr matrix_t inverse() const requires ( X == Y ) {
				T det;
				return inverse( det );
			}

			// Vertical and horizontal sums.
			//
			FORCE_INLINE constexpr auto vsum() const
				-> matrix_t<T, X, Y == std::dynamic_extent ? Y : 1>
			{
				size_t x = this->cols(), y = this->rows();
				matrix_t<T, X, Y == std::dynamic_extent ? Y : 1> result = { x, 1 };
				for ( size_t i = 0; i != x; i++ ) {
					T sum = 0;
					for ( size_t j = 0; j != y; j++ ) {
						sum += this->at( j, i );
					}
					result( 0, i ) = sum;
				}
				return result;
			}
			FORCE_INLINE constexpr auto hsum() const
				-> matrix_t<T, X == std::dynamic_extent ? X : 1, Y>
			{
				size_t x = this->cols(), y = this->rows();
				matrix_t<T, X == std::dynamic_extent ? X : 1, Y> result = { 1, y };
				for ( size_t i = 0; i != y; i++ ) {
					T sum = 0;
					for ( size_t j = 0; j != x; j++ ) {
						sum += this->at( i, j );
					}
					result( i, 0 ) = sum;
				}
				return result;
			}
		};
	};

	template<typename T, size_t X = std::dynamic_extent, size_t Y = std::dynamic_extent>
	using matrix_t = impl::matrix_t<T, X, Y>;
	template<size_t X = std::dynamic_extent, size_t Y = std::dynamic_extent>
	using matrix = impl::matrix_t<float, X, Y>;

	// Constant matrices.
	//
	template<typename F, size_t N>
	static constexpr matrix_t<std::remove_const_t<F>, N == std::dynamic_extent ? N : 1, N> to_vmatrix( std::span<F, N> x ) {
		return { 1, x.size(), x.data() };
	}
	template<typename F, size_t N>
	static constexpr matrix_t<std::remove_const_t<F>, N, N == std::dynamic_extent ? N : 1> to_hmatrix( std::span<F, N> x ) {
		return { x.size(), 1, x.data() };
	}

	// Vandermonde matrix.
	//
	template<size_t D, typename F, size_t N>
	static constexpr auto vandermonde( std::span<F, N> x, size_t d = D ) {
		constexpr bool is_dynamic = D == std::dynamic_extent || N == std::dynamic_extent;
		using Ty = matrix_t<std::remove_const_t<F>, is_dynamic ? std::dynamic_extent : D, is_dynamic ? std::dynamic_extent : N>;

		using Fy = std::remove_const_t<F>;
		size_t n = x.size();
		Ty result( d, n, Fy( 1.0 ) );
		for ( size_t row = 0; row != n; row++ ) {
			Fy* data = &result( row, d - 1 );
			Fy acc = 1, val = x[ row ];
			for ( size_t col = 0; col != ( d - 1 ); col++ ) {
				*--data = ( acc *= val );
			}
		}
		return result;
	}

	// Least squares solver.
	//
	template<typename A, typename Y>
	static auto lstsq( A&& lhs, Y&& rhs ) {
		auto lhst = lhs.transpose();
		return lhst.dot( lhs ).inverse().dot( lhst ).dot( rhs );
	}

	// Polynomial line fitting.
	//
	template<size_t D, size_t X = std::dynamic_extent, size_t K = 0> requires ( K == std::dynamic_extent || K == X || K == 0 )
	static auto polyfit( std::span<const float, X> x, std::span<const float, X> y, std::span<const float, K> w = {}, size_t d = D ) {
		constexpr bool is_dynamic = D == std::dynamic_extent;
		auto lhs = vandermonde<is_dynamic ? D : D + 1>( x, d + 1 );
		auto rhs = to_vmatrix( y );

		if ( K && !w.empty() ) {
			dassert( w.size() == y.size() );
			auto weights = to_vmatrix( w );
			lhs = std::move( lhs ).smul( weights );
			rhs = std::move( rhs ).smul( weights );
		}

		auto scale = lhs.smul( lhs ).vsum().sqrt().transpose();
		lhs = std::move( lhs ).sdiv( lhs.smul( lhs ).vsum().sqrt() );
		return lstsq( lhs, rhs ).sdiv( scale );
	}
};