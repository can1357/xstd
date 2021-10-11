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
extern "C" 
{
#if WINDOWS_TARGET
	typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES, * PSECURITY_ATTRIBUTES, * LPSECURITY_ATTRIBUTES;
	__declspec( dllimport ) void* __stdcall CreateFileW( const wchar_t* lpFileName, unsigned long dwDesiredAccess, unsigned long dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, unsigned long dwCreationDisposition, unsigned long dwFlagsAndAttributes, void* hTemplateFile );
	__declspec( dllimport ) void* __stdcall CreateFileMappingFromApp( void* hFile, PSECURITY_ATTRIBUTES SecurityAttributes, unsigned long PageProtection, unsigned long long MaximumSize, const wchar_t* Name );
	__declspec( dllimport ) void* __stdcall MapViewOfFileFromApp( void* hFileMappingObject, unsigned long DesiredAccess, unsigned long long FileOffset, size_t NumberOfBytesToMap );
	__declspec( dllimport ) int __stdcall UnmapViewOfFile( const void* BaseAddress );
	__declspec( dllimport ) int __stdcall CloseHandle( void* hObject );
#else
	int open( const char* pathname, int flags );
	int close( int fd );
	void* mmap( void* addr, size_t length, int prot, int flags, int fd, long offset );
	int munmap( void* addr, size_t length );
#endif
};


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

	// Entire file mapped to memory.
	//
	template<Trivial T = uint8_t>
	struct view
	{
		// Interface.
		//
		virtual size_t size() const = 0;
		virtual const T* data() const = 0;
		virtual const std::filesystem::path& path() const { return make_default<std::filesystem::path>(); }

		// No copy or move.
		//
		view() = default;
		view( view&& ) = delete;
		view( const view& ) = delete;

		// Allow array-like use.
		//
		size_t length() const { return size(); }
		bool empty() const { return !size(); }
		auto begin() const { return data(); }
		auto end() const { return data() + size(); }
		auto rbegin() const { return std::reverse_iterator{ end() }; }
		auto rend() const { return std::reverse_iterator{ begin() }; }
		const T& operator[]( size_t n ) const { return data()[ n ]; }

		// Virtual destructor.
		//
		virtual ~view() = default;
	};
	template<Trivial T = uint8_t>
	using shared_view = std::shared_ptr<view<T>>;

	template<Trivial T = uint8_t>
	struct native_view : view<T>
	{
		// Region details.
		//
		std::filesystem::path origin = {};
		const T* address = nullptr;
		size_t length = 0;
#if WINDOWS_TARGET
		void* file_handle = impl::invalid_handle_value;
		void* mapping_handle = impl::invalid_handle_value;
#else
		int fd = -1;
#endif
		size_t size() const override { return length; }
		const T* data() const override { return address; }
		const std::filesystem::path& path() const override { return origin; }

		~native_view()
		{
#if WINDOWS_TARGET
			if ( address && !UnmapViewOfFile( ( void* ) address ) )
				error( XSTD_ESTR( "UnmapViewOfFile failed." ) );
			if ( mapping_handle != impl::invalid_handle_value )
				CloseHandle( mapping_handle );
			if ( file_handle != impl::invalid_handle_value )
				CloseHandle( file_handle );
#else
			close( fd );
			if ( address && munmap( ( void* ) address, length * sizeof( T ) ) == -1 )
				error( XSTD_ESTR( "munmap failed." ) );
#endif
		}
	};

	template<Trivial T = uint8_t>
	static inline io_result<shared_view<T>> map_view( const std::filesystem::path& path, size_t count = 0, size_t offset = 0 )
	{
		// Get file size and validate it.
		//
		std::error_code ec;
		size_t file_length = std::filesystem::file_size( path, ec );
		if ( ec )
			return { io_state::bad_file };
		if ( file_length % sizeof( T ) )
			return { io_state::invalid_alignment };
		size_t moffset = offset * sizeof( T );
		size_t mlength = count * sizeof( T );
		if ( file_length < ( moffset + mlength ) )
			return { io_state::reading_beyond_end };
		if ( !mlength )
			mlength = file_length - moffset;
		size_t tcount = mlength / sizeof( T );

#if WINDOWS_TARGET
		// Create the file view and open a handle to the file.
		//
		auto fview = std::make_shared<native_view<T>>();
		fview->origin = path;
		fview->file_handle = CreateFileW(
			path.c_str(),
			0x80000000, //GENERIC_READ,
			0x7, //FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			0x3, //OPEN_EXISTING,
			0x10000000, //FILE_FLAG_RANDOM_ACCESS,
			nullptr
		);
		fview->length = tcount;
		if ( fview->file_handle == impl::invalid_handle_value )
			return { io_state::bad_file };

		// Map the file.
		//
		fview->mapping_handle = CreateFileMappingFromApp(
			fview->file_handle,
			nullptr,
			0x2, //PAGE_READONLY,
			0,
			nullptr
		);
		if ( !fview->mapping_handle )
			return { io_state::bad_file };
		fview->address = ( T* ) MapViewOfFileFromApp(
			fview->mapping_handle,
			0x4, //SECTION_MAP_READ,
			moffset,
			mlength
		);
		if ( !fview->address )
			return { io_state::bad_file };
		else
			return { std::dynamic_pointer_cast<view<T>>( std::move( fview ) ) };
#else
		// Create the file view and open a handle to the file.
		//
		auto fview = std::make_shared<native_view<T>>();
		fview->origin = path;
		fview->fd = open( path.string().c_str(), 0 /*O_RDONLY*/ );
		fview->length = tcount;
		if ( fview->fd == -1 )
			return { io_state::bad_file };

		// Map the file.
		//
		fview->address = ( const T* ) mmap( nullptr, mlength, 1 /*PROT_READ*/, 1 /*MAP_SHARED*/, fview->fd, ( int ) moffset );
		if ( !fview->address )
			return { io_state::bad_file };
		else
			return { std::dynamic_pointer_cast< view<T> >( std::move( fview ) ) };
#endif
	}
};