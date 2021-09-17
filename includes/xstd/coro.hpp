#pragma once
#include <type_traits>
#include <functional>
#include <utility>
#include "intrinsics.hpp"
#include "result.hpp"
#include "spinlock.hpp"

#if MS_COMPILER
	#include <experimental/coroutine>
#endif

// Define the core types.
//
namespace xstd
{
#if !MS_COMPILER
	// Builtins.
	//
	namespace builtin
	{
		inline bool done( void* address ) { return __builtin_coro_done( address ); }
		inline void resume( void* address ) { __builtin_coro_resume( address ); }
		inline void destroy( void* address ) { __builtin_coro_destroy( address ); }
		template<typename P> inline void* to_handle( P& ref ) { return __builtin_coro_promise( &ref, alignof( P ), true ); }
		template<typename P> inline P& to_promise( void* hnd ) { return *( P* ) __builtin_coro_promise( hnd, alignof( P ), false ); }
	};
#endif

	template<typename T>
	concept Coroutine = requires{ typename T::promise_type; };

	// Types that are exported as std.
	//
	namespace coro_export
	{
#if !MS_COMPILER
		// Coroutine traits.
		//
		template<Coroutine Ret, typename... Args>
		struct coroutine_traits { using promise_type = typename Ret::promise_type; };

		// Coroutine handle.
		//
		template<typename Promise = void>
		struct coroutine_handle;
		template<>
		struct coroutine_handle<void>
		{
			void* handle = nullptr;

			inline coroutine_handle() : handle( nullptr ) {};
			inline coroutine_handle( std::nullptr_t ) : handle( nullptr ) {};

			inline constexpr void* address() const noexcept { return handle; }
			inline constexpr explicit operator bool() const noexcept { return handle; }

			inline void resume() { builtin::resume( handle ); }
			inline void destroy() { builtin::destroy( handle ); }
			inline bool done() const { return builtin::done( handle ); }
			inline void operator()() { resume(); }

			inline bool operator<( const coroutine_handle& o ) const noexcept { return handle < o.handle; }
			inline bool operator==( const coroutine_handle& o ) const noexcept { return handle == o.handle; }
			inline bool operator!=( const coroutine_handle& o ) const noexcept { return handle != o.handle; }

			inline static coroutine_handle<> from_address( void* addr ) noexcept { coroutine_handle<> tmp{}; tmp.handle = addr; return tmp; }
		};
		template<typename Promise>
		struct coroutine_handle : coroutine_handle<>
		{
			inline coroutine_handle() {};
			inline coroutine_handle( std::nullptr_t ) : coroutine_handle<>( nullptr ) {};

			inline Promise& promise() const { return builtin::to_promise<Promise>( handle ); }
			inline static coroutine_handle<Promise> from_promise( Promise& pr ) noexcept { return from_address( builtin::to_handle( pr ) ); }
			inline static coroutine_handle<Promise> from_address( void* addr ) noexcept { coroutine_handle<Promise> tmp{}; tmp.handle = addr; return tmp; }
		};

		// Dummy awaitables.
		//
		struct suspend_never
		{
			inline bool await_ready() const noexcept { return true; }
			inline void await_suspend( coroutine_handle<> ) const noexcept {}
			inline void await_resume() const noexcept {}
		};
		struct suspend_always
		{
			inline bool await_ready() const noexcept { return false; }
			inline void await_suspend( coroutine_handle<> ) const noexcept {}
			inline void await_resume() const noexcept {}
		};
#else
		template<Coroutine Ret, typename... Args> using coroutine_traits = std::experimental::coroutine_traits<Ret, Args...>;
		template<typename Promise>                using coroutine_handle = std::experimental::coroutine_handle<Promise>;
		using suspend_never =  std::experimental::suspend_never;
		using suspend_always = std::experimental::suspend_always;
#endif
	};
	using namespace coro_export;

	// Unique coroutine handle.
	//
	template<typename Pr>
	struct unique_coroutine
	{
		coroutine_handle<Pr> hnd{ nullptr };

		// Null handle.
		//
		inline constexpr unique_coroutine() {}
		inline constexpr unique_coroutine( std::nullptr_t ) {}

		// Constructed by handle.
		//
		inline unique_coroutine( coroutine_handle<Pr> hnd ) : hnd{ hnd } {}

		// Constructed by promise.
		//
		inline unique_coroutine( Pr& p ) : hnd{ coroutine_handle<Pr>::from_promise( p ) } {}

		// No copy allowed.
		//
		inline unique_coroutine( const unique_coroutine& ) = delete;
		inline unique_coroutine& operator=( const unique_coroutine& ) = delete;

		// Move by swap.
		//
		inline unique_coroutine( unique_coroutine&& o ) noexcept : hnd( o.release() ) {}
		inline unique_coroutine& operator=( unique_coroutine&& o ) noexcept { std::swap( hnd, o.hnd ); return *this; }

		// Redirections.
		//
		inline void resume() { hnd.resume(); }
		inline bool done() const { return hnd.done(); }
		inline Pr& promise() const { return hnd.promise(); }
		inline operator coroutine_handle<Pr>() const { return hnd; }

		// Getter and validity check.
		//
		inline coroutine_handle<Pr> get() const { return hnd; }
		inline explicit operator bool() const { return ( bool ) hnd; }

		// Reset and release.
		//
		inline coroutine_handle<Pr> release() { return std::exchange( hnd, nullptr ); }
		inline void reset() { if ( auto h = release() ) h.destroy(); }

		// Reset on destruction.
		//
		~unique_coroutine() { reset(); }
	};
};
#if !MS_COMPILER
	namespace std 
	{ 
		using namespace xstd::coro_export; 
		template<typename Pr>
		struct hash<coroutine_handle<Pr>>
		{
			inline size_t operator()( const coroutine_handle<Pr>& hnd ) const noexcept 
			{ 
				return hash<size_t>{}( ( size_t ) hnd.address() );
			}
		};
	};
	namespace std::experimental { using namespace xstd::coro_export; };
#endif

// Implement generator, task and shared_task.
//
namespace xstd
{
	// Simple generator coroutine.
	//
	template<typename T>
	struct generator
	{
		struct promise_type
		{
			T* current = nullptr;

			inline std::suspend_always yield_value( T& value ) { current = &value; return {}; }
			inline generator get_return_object() { return *this; }
			inline std::suspend_always initial_suspend() { return {}; }
			inline std::suspend_always final_suspend() noexcept { return {}; }
			inline void return_void() {}
			inline void unhandled_exception() {}
		};

		struct iterator
		{
			coroutine_handle<promise_type> handle = {};
			
			inline iterator& operator++()
			{
				handle.resume();
				if ( handle.done() )
					handle = nullptr;
				return *this;
			}
			inline bool operator==( const iterator& rhs ) const { return handle == rhs.handle; }
			inline bool operator!=( const iterator& rhs ) const { return handle != rhs.handle; }
			inline const T& operator*() const { return *handle.promise().current; }
		};

		inline iterator begin() { return handle.done() ? end() : ++iterator{ handle }; }
		inline iterator end() { return {}; }

		unique_coroutine<promise_type> handle;
		inline generator( promise_type& pr ) : handle( pr ) {}
	};

	// Unique task coroutine.
	//
	template<typename T = void, typename S = exception>
	struct task
	{
		struct promise_type
		{
			// Continuation coroutine.
			//
			coroutine_handle<> continuation = {};

			// Result value.
			//
			basic_result<T, S> value = { std::nullopt, status_traits<S>::success_value };

			struct final_awaitable
			{
				inline bool await_ready() const noexcept { return false; }
				inline void await_suspend( coroutine_handle<promise_type> hnd ) const noexcept
				{
					if ( auto cont = hnd.promise().continuation )
						cont.resume();
				}
				void await_resume() const noexcept {}
			};

			inline task<T> get_return_object() { return *this; }
			inline std::suspend_always initial_suspend() noexcept { return {}; }
			inline final_awaitable final_suspend() noexcept { return {}; }
			inline void unhandled_exception() {}
			template<typename V> inline void return_value( V&& v ) { value.emplace( std::forward<V>( v ) ); }
			template<typename V> inline std::suspend_always yield_value( V&& v ) { value.raise( std::forward<V>( v ) ); return {}; }
		};

		// Coroutine handle and the internal constructor.
		//
		unique_coroutine<promise_type> handle;
		inline task( promise_type& pr ) : handle( pr ) {}

		// Null constructor and validity check.
		//
		inline task() : handle( nullptr ) {}
		inline task( std::nullptr_t ) : handle( nullptr ) {}
		bool valid() const { return handle.get() != nullptr; }
		explicit operator bool() const { return valid(); }

		// No copy, default move.
		//
		inline task( task&& ) noexcept = default;
		inline task( const task& ) = delete;
		inline task& operator=( task&& ) noexcept = default;
		inline task& operator=( const task& ) = delete;

		// State.
		//
		inline bool done() const { return handle.done() || fail(); }
		inline bool fail() const { return promise().value.fail(); }
		inline bool success() const { return handle.done(); }
		
		inline auto& promise() const noexcept { return handle.promise(); }

		inline T& value() & { return *promise().value; }
		inline const T& value() const & { return *promise().value; }
		inline T&& value() && { return std::move( *promise().value ); }
		
		inline S& status() & { return promise().value.status; }
		inline const S& status() const & { return promise().value.status; }
		inline S&& status() && { return std::move( promise().value.status ); }

		// Resumes the coroutine and returns the status.
		//
		inline task& operator()() & { handle.resume(); return *this; }
		inline task operator()() && { handle.resume(); return std::move( *this ); }
		inline const task& operator()() const & { handle.resume(); return *this; }

		// Implement co_await.
		//
		inline bool await_ready() { return done(); }
		inline coroutine_handle<> await_suspend( coroutine_handle<> hnd )
		{
			dassert_s( std::exchange( promise().continuation, hnd ) == nullptr );
			return handle;
		}
		inline basic_result<T, S>& await_resume() & { return promise().value; }
		inline basic_result<T, S>&& await_resume() && { return std::move( promise().value ); }
		inline const basic_result<T, S>& await_resume() const & { return promise().value; }

		template<typename R>
		struct asserted_waitable
		{
			R ref;

			inline bool await_ready() { return ref.done(); }
			inline coroutine_handle<> await_suspend( coroutine_handle<> hnd )
			{
				dassert_s( std::exchange( ref.promise().continuation, hnd ) == nullptr );
				return ref.handle;
			}
			inline decltype( auto ) await_resume() { return *std::move( ref ).await_resume(); }
		};
		inline asserted_waitable<task<T, S>&> assert() & { return { *this }; }
		inline asserted_waitable<task<T, S>&&> assert() && { return { std::move( *this ) }; }
		inline asserted_waitable<const task<T, S>&> assert() const & { return { *this }; }

	};
	template<typename S>
	struct task<void, S>
	{
		struct promise_type
		{
			// Continuation coroutine.
			//
			coroutine_handle<> continuation = {};

			// Exception value.
			//
			basic_result<std::monostate, S> value = { std::nullopt, status_traits<S>::success_value };

			struct final_awaitable
			{
				inline bool await_ready() const noexcept { return false; }
				inline void await_suspend( coroutine_handle<promise_type> hnd ) const noexcept
				{
					if ( auto cont = hnd.promise().continuation )
						cont.resume();
				}
				void await_resume() const noexcept {}
			};
			inline task<void> get_return_object() { return *this; }
			inline std::suspend_always initial_suspend() noexcept { return {}; }
			inline final_awaitable final_suspend() noexcept { return {}; }
			inline void unhandled_exception() {}
			inline void return_void() {}
			template<typename V> inline std::suspend_always yield_value( V&& v ) { value.raise( std::forward<V>( v ) ); return {}; }
		};

		// Coroutine handle and the internal constructor.
		//
		unique_coroutine<promise_type> handle;
		inline task( promise_type& pr ) : handle( pr ) {}

		// Null constructor and validity check.
		//
		inline task() : handle( nullptr ) {}
		inline task( std::nullptr_t ) : handle( nullptr ) {}
		bool valid() const { return handle.get() != nullptr; }
		explicit operator bool() const { return valid(); }

		// No copy, default move.
		//
		inline task( task&& ) noexcept = default;
		inline task( const task& ) = delete;
		inline task& operator=( task&& ) noexcept = default;
		inline task& operator=( const task& ) = delete;
		
		// State.
		//
		inline bool done() const { return handle.done() || fail(); }
		inline bool fail() const { return promise().value.fail(); }
		inline bool success() const { return handle.done(); }
		
		inline auto& promise() const noexcept { return handle.promise(); }

		inline constexpr std::monostate value() const { return {}; }
		
		inline S& status() & { return promise().value.status; }
		inline const S& status() const & { return promise().value.status; }
		inline S&& status() && { return std::move( promise().value.status ); }

		// Resumes the coroutine and returns self.
		//
		inline task& operator()() & { handle.resume(); return *this; }
		inline task operator()() && { handle.resume(); return std::move( *this ); }
		inline const task& operator()() const & { handle.resume(); return *this; }

		// Implement co_await.
		//
		inline bool await_ready() { return done(); }
		inline coroutine_handle<> await_suspend( coroutine_handle<> hnd )
		{
			dassert_s( std::exchange( promise().continuation, hnd ) == nullptr );
			return handle;
		}
		inline basic_result<std::monostate, S>& await_resume() & { return promise().value; }
		inline basic_result<std::monostate, S>&& await_resume() && { return std::move( promise().value ); }
		inline const basic_result<std::monostate, S>& await_resume() const& { return promise().value; }

		template<typename R>
		struct asserted_waitable
		{
			R ref;

			inline bool await_ready() { return ref.done(); }
			inline coroutine_handle<> await_suspend( coroutine_handle<> hnd )
			{
				dassert_s( std::exchange( ref.promise().continuation, hnd ) == nullptr );
				return ref.handle;
			}
			inline decltype( auto ) await_resume() { return *std::move( ref ).await_resume(); }
		};
		inline asserted_waitable<task<void, S>&> assert() & { return { *this }; }
		inline asserted_waitable<task<void, S>&&> assert() && { return { std::move( *this ) }; }
		inline asserted_waitable<const task<void, S>&> assert() const & { return { *this }; }
	};

	// Shared task completion task.
	//
	namespace impl
	{
		inline task<> handle_completion( xstd::spinlock& list_lock, std::vector<coroutine_handle<>>& continuation_list )
		{
			std::lock_guard _g{ list_lock };
			for ( auto&& entry : std::exchange( continuation_list, {} ) )
				entry.resume();
			co_return;
		}
	};

	// Shared task type, constructed from task<T> on demand, not a coroutine.
	//
	template<typename T>
	struct shared_task
	{
		using promise_type = typename task<T>::promise_type;
		struct storage_type
		{
			task<T> task_value;
			task<> completion_task;

			xstd::spinlock list_lock = {};
			std::vector<coroutine_handle<>> continuation_list;

			template<typename... Tx>
			inline storage_type( Tx&&... args ) : task_value( std::forward<Tx>( args )... )
			{
				completion_task = impl::handle_completion( list_lock, continuation_list );
				task_value.await_suspend( completion_task.handle );
			}
		};
		std::shared_ptr<storage_type> storage;

		// Construction from task<T> or promise_type.
		//
		inline shared_task( task<T> ts ) : storage( std::make_shared<storage_type>( std::move( ts ) ) ) {}
		inline shared_task( promise_type& pr ) : storage( std::make_shared<storage_type>( pr ) ) {}
		
		// Null constructor and validity check.
		//
		inline shared_task() : storage( nullptr ) {}
		inline shared_task( std::nullptr_t ) : storage( nullptr ) {}
		bool valid() const { return storage != nullptr; }
		explicit operator bool() const { return valid(); }
		
		// Default copy and move.
		//
		inline shared_task( shared_task&& ) noexcept = default;
		inline shared_task( const shared_task& ) = default;
		inline shared_task& operator=( shared_task&& ) noexcept = default;
		inline shared_task& operator=( const shared_task& ) = default;
		
		// State.
		//
		inline task<T>& get_task() const { return storage->task_value; }
		inline bool done() const { return get_task().done(); }
		inline decltype( auto ) value() const { return get_task().value(); }
		
		// Implement co_await.
		//
		template<bool Assert>
		struct unique_awaitable
		{
			const std::shared_ptr<storage_type>& storage = {};
			coroutine_handle<> awaiting_as = {};
		
			inline bool await_ready() { return storage->task_value.done(); }
			inline bool await_suspend( coroutine_handle<> hnd )
			{
				std::lock_guard _g{ storage->list_lock };
				if ( storage->task_value.done() )
					return false;
				awaiting_as = hnd;
				storage->continuation_list.emplace_back( hnd );
				return true;
			}
			inline decltype( auto ) await_resume() const
			{
				if constexpr( Assert )
					return ( ( const storage_type* ) storage.get() )->task_value.assert().await_resume();
				else
					return ( ( const storage_type* ) storage.get() )->task_value.await_resume();
			}
			inline ~unique_awaitable()
			{
				if ( awaiting_as )
				{
					std::lock_guard _g{ storage->list_lock };
					auto it = std::find( storage->continuation_list.begin(), storage->continuation_list.end(), awaiting_as );
					if ( it != storage->continuation_list.end() )
					{
						std::swap( *it, storage->continuation_list.back() );
						storage->continuation_list.resize( storage->continuation_list.size() - 1 );
					}
				}
			}
		};
		
		inline unique_awaitable<false> operator co_await() const { return { storage }; }
		inline unique_awaitable<true> assert() const { return { storage }; }
	};
};