#pragma once
#include <iterator>
#include <string_view>

namespace xstd
{
   // Define a constant storage for string literals.
   //
   template<typename T, size_t N>
   struct basic_string_literal
   {
      using value_type =       T;
      using reference =        T&;
      using pointer =          T*;
      using const_reference =  const T&;
      using const_pointer =    const T*;
      using iterator =         T*;
      using const_iterator =   const T*;

      T store[ N + 1 ] = {};

      template<typename... Tx> constexpr basic_string_literal( T a, Tx... b ) : store{ a, b... } {}
      constexpr basic_string_literal( const T( &arr )[ N + 1 ] ) { std::copy_n( &arr[ 0 ], N + 1, &store[ 0 ] ); }
      constexpr basic_string_literal( basic_string_literal&& ) noexcept = default;
      constexpr basic_string_literal( const basic_string_literal& ) = default;
      constexpr basic_string_literal& operator=( basic_string_literal&& ) noexcept = default;
      constexpr basic_string_literal& operator=( const basic_string_literal& ) = default;

      constexpr T* begin() { return &store[ 0 ]; }
      constexpr T* end() { return &store[ N ]; }
      constexpr const T* cbegin() { return &store[ 0 ]; }
      constexpr const T* cend() { return &store[ N ]; }
      constexpr auto rbegin() { return std::make_reverse_iterator( end() ); }
      constexpr auto rend() { return std::make_reverse_iterator( begin() ); }
      constexpr auto rcbegin() { return std::make_reverse_iterator( cend() ); }
      constexpr auto rcend() { return std::make_reverse_iterator( cbegin() ); }
      constexpr const T* begin() const { return cbegin(); }
      constexpr const T* end() const { return cend(); }
      constexpr auto rbegin() const { return rcbegin(); }
      constexpr auto rend() const { return rcend(); }
      constexpr T& at( size_t i ) { return store[ i ]; }
      constexpr const T& at( size_t i ) const { return store[ i ]; }
      constexpr T& operator[]( size_t i ) { return store[ i ]; }
      constexpr const T& operator[]( size_t i ) const { return store[ i ]; }
      constexpr T& back() { return store[ N - 1 ]; }
      constexpr const T& back() const { return store[ N - 1 ]; }
      constexpr T& front() { return store[ 0 ]; }
      constexpr const T& front() const { return store[ 0 ]; }
      constexpr T* data() { return &store[ 0 ]; }
      constexpr const T* data() const { return &store[ 0 ]; }
      constexpr T* c_str() { return &store[ 0 ]; }
      constexpr const T* c_str() const { return &store[ 0 ]; }
      constexpr size_t size() { return N; }
      constexpr size_t length() { return N; }
      constexpr size_t max_size() { return N; }
      constexpr size_t capacity() { return N; }
      constexpr bool empty() const { return !N; }
      constexpr bool starts_with( std::basic_string_view<T> view ) const { return to_string().starts_with( view ); }
      constexpr bool ends_with( std::basic_string_view<T> view ) const { return to_string().ends_with( view ); }
      constexpr bool contains( std::basic_string_view<T> view ) const { return to_string().contains( view ); }
      constexpr size_t find( std::basic_string_view<T> view, size_t pos = 0 ) const { return to_string().find( view, pos ); }
      constexpr size_t find( T c, size_t pos = 0 ) const { return to_string().find( c, pos ); }
      constexpr size_t find_first_of( std::basic_string_view<T> view, size_t pos = 0 ) const { return to_string().find_first_of( view, pos ); }
      constexpr size_t find_first_of( T c, size_t pos = 0 ) const { return to_string().find_first_of( c, pos ); }
      constexpr size_t find_first_not_of( std::basic_string_view<T> view, size_t pos = 0 ) const { return to_string().find_first_not_of( view, pos ); }
      constexpr size_t find_first_not_of( T c, size_t pos = 0 ) const { return to_string().find_first_not_of( c, pos ); }
      constexpr size_t rfind( std::basic_string_view<T> view, size_t pos = std::string::npos ) const { return to_string().rfind( view, pos ); }
      constexpr size_t rfind( T c, size_t pos = std::string::npos ) const { return to_string().rfind( c, pos ); }
      constexpr size_t find_last_of( std::basic_string_view<T> view, size_t pos = std::string::npos ) const { return to_string().find_last_of( view, pos ); }
      constexpr size_t find_last_of( T c, size_t pos = std::string::npos ) const { return to_string().find_last_of( c, pos ); }
      constexpr size_t find_last_not_of( std::basic_string_view<T> view, size_t pos = std::string::npos ) const { return to_string().find_last_not_of( view, pos ); }
      constexpr size_t find_last_not_of( T c, size_t pos = std::string::npos ) const { return to_string().find_last_not_of( c, pos ); }

      template<size_t M>
      constexpr basic_string_literal<T, N + M> operator+( const basic_string_literal<T, M>& x ) const
      {
         basic_string_literal<T, N + M> result = {};
         std::copy_n( data(), N, result.data() );
         std::copy_n( x.data(), M, result.data() + N );
         return result;
      }

      constexpr std::basic_string_view<T> to_string() const { return { data(), size() }; }
      constexpr operator std::basic_string_view<T>() const { return to_string(); }
   };
   template<size_t N> struct string_literal :    basic_string_literal<char, N>     {};
   template<size_t N> struct wstring_literal :   basic_string_literal<wchar_t, N>  {};

   // Deduction guides.
   //
   template<typename T, size_t N> basic_string_literal( const T( & )[ N ] )->basic_string_literal<T, N - 1>;
   template<size_t N> string_literal( const char( & )[ N ] )->string_literal<N - 1>;
   template<size_t N> wstring_literal( const wchar_t( & )[ N ] )->wstring_literal<N - 1>;
};

#if ( __INTELLISENSE__ || !GNU_COMPILER )
   constexpr xstd::basic_string_literal<char, 1> operator""_cs( const char* str, size_t n );
   constexpr xstd::basic_string_literal<wchar_t, 1> operator""_cs( const wchar_t* str, size_t n );
#else
   template<typename T, T... val>
   inline constexpr xstd::basic_string_literal<T, sizeof...( val )> operator""_cs()
   {
      return xstd::basic_string_literal<T, sizeof...( val )>{ val... };
   }
#endif