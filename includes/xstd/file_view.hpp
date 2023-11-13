#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include "result.hpp"
#include "type_helpers.hpp"
#include "file.hpp"

// Required imports.
//
#if WINDOWS_TARGET
extern "C" 
{
	typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES, * PSECURITY_ATTRIBUTES, * LPSECURITY_ATTRIBUTES;
	__declspec( dllimport ) void* __stdcall CreateFileW( const wchar_t* lpFileName, unsigned long dwDesiredAccess, unsigned long dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, unsigned long dwCreationDisposition, unsigned long dwFlagsAndAttributes, void* hTemplateFile );
	__declspec( dllimport ) void* __stdcall CreateFileMappingFromApp( void* hFile, PSECURITY_ATTRIBUTES SecurityAttributes, unsigned long PageProtection, unsigned long long MaximumSize, const wchar_t* Name );
	__declspec( dllimport ) void* __stdcall MapViewOfFileFromApp( void* hFileMappingObject, unsigned long DesiredAccess, unsigned long long FileOffset, size_t NumberOfBytesToMap );
	__declspec( dllimport ) int __stdcall UnmapViewOfFile( const void* BaseAddress );
	__declspec( dllimport ) int __stdcall CloseHandle( void* hObject );
};
#else
	#include <sys/mman.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
#endif


// Declare a simple interface to map files onto memory.
//
namespace xstd::file
{
#if WINDOWS_TARGET
	namespace impl
	{
		static constexpr any_ptr invalid_handle_value = ~0ull;
	};
#endif

	template<TriviallyCopyable T = const uint8_t>
	struct view
	{
		// Region details.
		//
		std::filesystem::path origin = {};
		T* address = nullptr;
		size_t length = 0;
#if WINDOWS_TARGET
		void* file_handle = impl::invalid_handle_value;
		void* mapping_handle = impl::invalid_handle_value;
#else
		int fd = -1;
#endif

		// Default constructed.
		//
		view() = default;

		// No copy, move via swap.
		//
		view( const view& ) = delete;
		view& operator=( const view& ) = delete;
		view( view&& o ) noexcept { swap( o ); }
		view& operator=( view&& o ) noexcept { swap( o ); return *this; }
		void swap( view& o )
		{
			std::swap( origin, o.origin );
			std::swap( address, o.address );
			std::swap( length, o.length );
#if WINDOWS_TARGET
			std::swap( file_handle, o.file_handle );
			std::swap( mapping_handle, o.mapping_handle );
#else
			std::swap( fd, o.fd );
#endif
		}

		// Observers.
		//
		T* begin() const { return address; }
		T* end() const { return address + length; }
		T* data() const { return address; }
		size_t size() const { return length; }
		bool empty() const { return !size(); }
		auto rbegin() const { return std::reverse_iterator{ end() }; }
		auto rend() const { return std::reverse_iterator{ begin() }; }
		T& operator[]( size_t n ) const { return data()[ n ]; }
		const std::filesystem::path& path() const { return origin; }

		// Conversion to bool.
		//
		explicit operator bool() const { return address != nullptr; }

		// Close the handle on unmap.
		//
		~view()
		{
#if WINDOWS_TARGET
			if ( file_handle == impl::invalid_handle_value )
				return;
			if ( address )
				UnmapViewOfFile( ( void* ) address );
			if ( mapping_handle != impl::invalid_handle_value )
				CloseHandle( mapping_handle );
			CloseHandle( file_handle );
#else
			if ( fd == -1 )
				return;
			if ( address )
				munmap( ( void* ) address, length * sizeof( T ) );
			close( fd );
#endif
		}
	};

	namespace detail {
		template<TriviallyCopyable T>
		inline io_result<view<T>> create_mapping( const std::filesystem::path& path, size_t count, size_t offset, bool copy_on_write = true ) {
			// Get file size and validate it.
			//
			std::error_code ec;
			size_t file_length = std::filesystem::file_size( path, ec );
			if ( ec ) return { io_state::bad_file };
			if ( file_length % sizeof( T ) ) return { io_state::invalid_alignment };
		
			size_t moffset = offset * sizeof( T );
			size_t mlength = count * sizeof( T );
			if ( file_length < ( moffset + mlength ) )
				return { io_state::reading_beyond_end };
		
			if ( !mlength )
				mlength = file_length - moffset;
			size_t tcount = mlength / sizeof( T );

			// Create the view.
			//
			io_result<view<T>> result;
			auto& fview = result.emplace();

	#if WINDOWS_TARGET
			uint32_t file_access;
			uint32_t mapping_access;
			uint32_t view_access;
			if constexpr ( std::is_const_v<T> ) {
				file_access =    0x80000000; // GENERIC_READ
				mapping_access = 0x2;        // PAGE_READONLY
				view_access =    0x4;        // FILE_MAP_READ
			} else if ( copy_on_write ) {
				file_access =    0x80000000; // GENERIC_READ
				mapping_access = 0x2;        // PAGE_READONLY
				view_access =    0x1;        // FILE_MAP_COPY
			} else {
				file_access =    0xC0000000; // GENERIC_READ | GENERIC_WRITE
				mapping_access = 0x4;        // PAGE_READWRITE 
				view_access =    0x2;        // FILE_MAP_WRITE
			}

			// Open a handle to the file.
			//
			fview.origin = path;
			fview.file_handle = CreateFileW(
				path.c_str(),
				file_access,
				0x7, //FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				nullptr,
				0x3, //OPEN_EXISTING,
				0,
				nullptr
			);
			fview.length = tcount;
			if ( fview.file_handle != impl::invalid_handle_value )
			{
				// Map the file.
				//
				fview.mapping_handle = CreateFileMappingFromApp(
					fview.file_handle,
					nullptr,
					mapping_access,
					0,
					nullptr
				);
				fview.address = ( T* ) MapViewOfFileFromApp(
					fview.mapping_handle,
					view_access,
					moffset,
					mlength
				);
			}
	#else
			uint32_t file_access;
			uint32_t map_prot;
			uint32_t map_share;
			if constexpr ( std::is_const_v<T> ) {
				file_access =  0x0; // O_RDONLY
				map_prot =     0x1; // PROT_READ
				map_share =    0x1; // MAP_SHARED
			} else if ( copy_on_write ) {
				file_access =  0x0; // O_RDONLY
				map_prot =     0x3; // PROT_READ | PROT_WRITE
				map_share =    0x2; // MAP_PRIVATE
			} else {
				file_access =  0x2; // O_RDWR
				map_prot =     0x3; // PROT_READ | PROT_WRITE
				map_share =    0x1; // MAP_SHARED
			}

			// Open a handle to the file.
			//
			fview.origin = path;
			fview.fd = open( path.string().c_str(), file_access );
			fview.length = tcount;
			if ( fview.fd != -1 )
			{
				// Map the file.
				//
				fview.address = ( const T* ) mmap( nullptr, mlength, map_prot, map_share, fview.fd, ( int ) moffset );
			}
	#endif
			if ( !fview.address )
				result.raise( io_state::bad_file );
			return result;
		}
	};

	template<TriviallyCopyable T = uint8_t>
	static io_result<view<const T>> map_view( const std::filesystem::path& path, size_t count = 0, size_t offset = 0 ) {
		return detail::create_mapping<const T>( path, count, offset );
	}
	template<TriviallyCopyable T = uint8_t>
	static io_result<view<T>> map_cow( const std::filesystem::path& path, size_t count = 0, size_t offset = 0 ) {
		return detail::create_mapping<T>( path, count, offset, true );
	}
	template<TriviallyCopyable T = uint8_t>
	static io_result<view<T>> map( const std::filesystem::path& path, size_t count = 0, size_t offset = 0 ) {
		return detail::create_mapping<T>( path, count, offset );
	}
};