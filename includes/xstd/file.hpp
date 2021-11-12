#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "result.hpp"
#include "type_helpers.hpp"

// Declare IO state.
//
namespace xstd
{
	enum class io_state
	{
		unknown = -1,
		success = 0,
		bad_file,
		reading_beyond_end,
		invalid_alignment,
	};
	
	template<>
	struct status_traits<io_state>
	{
		// Generic success and failure values.
		//
		static constexpr io_state success_value = io_state::success;
		static constexpr io_state failure_value = io_state::unknown;

		// Declare basic traits.
		//
		constexpr static inline bool is_success( io_state v ) { return v == io_state::success; }
	};

	template<typename T = std::monostate>
	using io_result = basic_result<T, io_state>;
};

// Declare a simple interface to read/write files for convenience.
//
namespace xstd::file
{
	// Binary I/O.
	//
	template<Trivial T = uint8_t>
	static io_result<std::vector<T>> read_raw( const std::filesystem::path& path, size_t count = 0, size_t offset = 0 )
	{
		// Try to open file as binary for read.
		//
		std::ifstream file( path, std::ios::binary );
		if ( !file.good() ) 
			return { io_state::bad_file };

		// Determine file size.
		//
		file.seekg( 0, std::ios_base::end );
		size_t file_size = file.tellg();
		file.seekg( 0, std::ios_base::beg );

		// Skip to requested offset.
		//
		if ( offset )
		{
			offset *= sizeof( T );
			if ( file_size <= offset )
				return { io_state::reading_beyond_end };
			file.seekg( offset, std::ios::cur );
			file_size -= offset;
		}

		// Validate and fix read count.
		//
		if ( !count )
		{
			count = file_size / sizeof( T );
			if ( ( file_size % sizeof( T ) ) != 0 )
				return { io_state::invalid_alignment };
		}
		else
		{
			if ( file_size < ( count * sizeof( T ) ) )
				return { io_state::reading_beyond_end };
		}

		// Read the whole file and return.
		//
		std::vector<T> buffer( count );
		file.read( (char*) buffer.data(), count * sizeof( T ) );
		return buffer;
	}

	static inline io_result<> write_raw( const std::filesystem::path& path, void* data, size_t size )
	{
		// Try to open file as binary for write.
		//
		std::ofstream file( path, std::ios::binary );
		if ( !file.good() )
			return { io_state::bad_file };

		// Write the data and return.
		//
		file.write( ( char* ) data, size );
		return std::monostate{};
	}

	template<Iterable C = std::initializer_list<uint8_t>> requires ( Trivial<iterable_val_t<C>> )
	static inline io_result<> write_raw( const std::filesystem::path& path, C&& container )
	{
		// Try to open file as binary for write.
		//
		std::ofstream file( path, std::ios::binary );
		if ( !file.good() )
			return { io_state::bad_file };

		// Write every element and return.
		//
		if constexpr ( !ContiguousIterable<C> )
		{
			for ( auto& e : container )
				file.write( ( char* ) &e, sizeof( iterable_val_t<C> ) );
		}
		else
		{
			file.write( ( char* ) &*std::begin( container ), std::size( container ) * sizeof( iterable_val_t<C> ) );
		}
		return std::monostate{};
	}

	// String I/O.
	//
	template<typename C = char>
	static inline io_result<std::vector<std::basic_string<C>>> read_lines( const std::filesystem::path& path )
	{
		// Try to open file as string for read.
		//
		std::basic_ifstream<C> file( path );
		if ( !file.good() )
			return { io_state::bad_file };

		// Read every lines and return.
		//
		std::vector<std::basic_string<C>> output;
		while ( std::getline( file, output.emplace_back() ) );
		output.pop_back();
		return output;
	}

	template<typename C = char>
	static inline io_result<std::basic_string<C>> read_string( const std::filesystem::path& path )
	{
		// Try to open file as string for read.
		//
		std::basic_ifstream<C> file( path );
		if ( !file.good() )
			return { io_state::bad_file };
		return { std::basic_string<C>( std::istreambuf_iterator<C>( file ), std::istreambuf_iterator<C>() ) };
	}

	template<Iterable C = std::initializer_list<std::string_view>> requires( String<iterable_val_t<C>> )
	static inline io_result<> write_lines( const std::filesystem::path& path, C&& container )
	{
		using char_type = string_unit_t<iterable_val_t<C>>;

		// Try to open file as string for write.
		//
		std::basic_ofstream<char_type> file( path );
		if ( !file.good() )
			return { io_state::bad_file };

		// Write every line and return.
		//
		for ( std::basic_string_view<char_type> view : container )
		{
			file.write( view.data(), view.size() );
			file << std::endl;
		}
		return std::monostate{};
	}

	template<String S = std::string_view>
	static inline io_result<> write_string( const std::filesystem::path& path, S&& data )
	{
		using char_type = string_unit_t<S>;

		// Try to open file as string for write.
		//
		std::basic_ofstream<char_type> file( path );
		if ( !file.good() )
			return { io_state::bad_file };

		// Write the whole string and return.
		//
		std::basic_string_view<char_type> view = data;
		file.write( view.data(), view.size() );
		return std::monostate{};
	}
};