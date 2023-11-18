#pragma once
#include "spinlock.hpp"
#include "coro.hpp"
#include "vec_buffer.hpp"
#include "result.hpp"
#include "event.hpp"
#include "function_view.hpp"
#include "fiber.hpp"
#include "ref_counted.hpp"
#include "wait_list.hpp"
#include "async.hpp"

namespace xstd {
	// Stream stop code.
	//
	enum stream_stop_code : uint8_t {
		stream_stop_none =     0,
		stream_stop_fin =      1,
		stream_stop_killed =   2,
		stream_stop_timeout =  3,
		stream_stop_error =    4,
	};

	// Stream state.
	//
	struct stream_state : ref_counted<stream_state>, wait_list {
		// Set if stream has ended.
		//
		std::atomic<stream_stop_code> stop_code = stream_stop_none;
		exception                     stop_reason = {};
		std::atomic<bool>             stop_written = false;

		// Stop details.
		//
		bool errored() const { return stop_code.load( std::memory_order::relaxed ) > stream_stop_fin; }
		bool finished() const { return stop_code.load( std::memory_order::relaxed ) == stream_stop_fin; }
		bool stopped() const { return stop_code.load( std::memory_order::relaxed ) != stream_stop_none; }
		exception get_stop_reason() const {
			if ( stopped() ) {
				while ( !stop_written.load( std::memory_order::relaxed ) )
					yield_cpu();
				return stop_reason;
			}
			return {};
		}
	};

	namespace detail {
		struct buffer_consumer {
			coroutine_handle<>         continuation = {};
			virtual coroutine_handle<> try_continue() = 0;
		};
		struct take_counted {
			size_t min = 1;
			size_t max = std::dynamic_extent;
			FORCE_INLINE vec_buffer operator()( vec_buffer& buf ) const {
				if ( buf.size() >= min ) {
					return buf.shift_range( std::min( buf.size(), max ) );
				} else {
					return {};
				}
			}
		};
		struct take_into_counted {
			uint8_t* out;
			size_t min;
			size_t max;
			FORCE_INLINE size_t operator()( vec_buffer& buf ) const {
				if ( buf.size() < min )
					return 0;
				size_t count = std::min( buf.size(), max );
				buf.shift_range( { out, count } );
				return count;
			}
		};
		struct wait_shutdown {
			template<typename A>
			FORCE_INLINE bool operator()( A& buf ) const {
				if ( buf.fin )
					return true;
				return false;
			}
		};
	}

	// Stream buffer state.
	//
	struct async_buffer : vec_buffer {
		struct unique_lock {
			xspinlock<>*    lck =  nullptr;
			task_priority_t prev = 0;

			unique_lock() = default;
			unique_lock( async_buffer& buf ) { reset( buf ); }
			void reset( async_buffer& buf ) {
				if ( locked() ) {
					unlock();
				} else {
					dassert( prev <= XSTD_SYNC_TPR );
					prev = get_task_priority();
				}
				this->lck = &buf.lock;
				lck->lock( prev );
			}
			
			unique_lock( const unique_lock& ) = delete;
			unique_lock& operator=( const unique_lock& ) = delete;
			unique_lock( unique_lock&& o ) noexcept { swap( o ); }
			unique_lock& operator=( unique_lock&& o ) noexcept { swap( o ); return *this; }
			void swap( unique_lock& o ) {
				std::swap( lck, o.lck );
				std::swap( prev, o.prev );
			}

			bool locked() const { return lck != nullptr; }
			explicit operator bool() const { return locked(); }

			void unlock() {
				dassert( locked() );
				std::exchange( lck, nullptr )->unlock( prev );
			}
			~unique_lock() {
				if ( locked() )
					unlock();
			}
		};

		// Lock and minimal state associated with the stream.
		//
		uint8_t             ended : 1 = false;
		uint8_t             fin   : 1 = false;
		scheduler_reference sched_enter = noop_scheduler{};
		scheduler_reference sched_leave = noop_scheduler{};
		mutable xspinlock<> lock = {};

		// Producer handle that signals "consumer wants more data" and the high watermark
		// which is the amount of bytes in the buffer after which the producer stops getting
		// repetitively called unless specifically requested.
		//
		coroutine_handle<>   producer = {};
		size_t               high_watermark = 256_kb;

		FORCE_INLINE void set_producer( coroutine_handle<> p, unique_lock& l ) {
			unique_lock _g{ std::move( l ) };
			dassert( !ended );
			dassert( !producer );
			producer = p;
		}

		// Consumer handle that signals "producer has written more data", which optionally returns
		// a coroutine handle if consumer is done consuming.
		//
		detail::buffer_consumer* consumer = nullptr;

		FORCE_INLINE void set_consumer( detail::buffer_consumer* c, unique_lock& l ) {
			unique_lock _g{ std::move( l ) };
			dassert( !ended );
			dassert( !consumer );
			consumer = c;
		}

		// Kill the coroutines on destruction.
		//
		void destroy( bool for_delete = false ) {
			if ( ended ) return;
			std::unique_lock g{ lock };
			if ( ended ) return;
			ended = 1;
			fin = 1;
			shrink_to_fit();
			high_watermark = std::dynamic_extent;
			auto producer = std::exchange( this->producer, nullptr );
			auto consumer = std::exchange( this->consumer, nullptr );
			g.unlock();
			if ( consumer ) ( sched_leave || chore_scheduler{} )( consumer->continuation )( );
			if ( producer ) ( for_delete ? noop_scheduler{} : sched_enter || chore_scheduler{} )( producer )( );
		}
		~async_buffer() { destroy( true ); }
	};

	using async_buffer_lock = typename async_buffer::unique_lock;
	struct async_buffer_locked {
		async_buffer& stream;
		mutable async_buffer_lock lock{ stream };

		async_buffer_locked( async_buffer& stream ) : stream( stream ), lock( stream ) {}
		async_buffer_locked( async_buffer& stream, async_buffer_lock&& lock ) : stream( stream ), lock( std::move( lock ) ) {}
		async_buffer_locked( async_buffer_locked&& ) noexcept = default;

		async_buffer* get() const { return &stream; }
		async_buffer* operator->() const { return get(); }
		
		vec_buffer& buffer() const { return stream; }
	};

	// Async reader/writer functions.
	//
	struct async_writer_stall : async_buffer_locked {
		async_writer_stall( async_buffer_locked&& buffer )
			: async_buffer_locked( std::move( buffer ) ) {}

		inline bool await_ready() noexcept { 
			return !!this->stream.consumer || this->stream.ended;
		}
		inline void await_suspend( coroutine_handle<> h ) noexcept { 
			this->stream.set_producer( h, this->lock ); 
		}
		inline void await_resume() const noexcept {}
	};
	struct async_writer_flush : async_buffer_locked {
		bool is_producer = false;
		async_writer_flush( async_buffer_locked&& buffer, bool is_producer = false )
			: async_buffer_locked( std::move( buffer ) ), is_producer( is_producer ) {}

		inline bool await_ready() noexcept { return this->stream.ended; }
		inline coroutine_handle<> await_suspend( coroutine_handle<> continuation ) noexcept {
			if ( this->stream.consumer ) {
				// Consume the data, if finished yield to it, else to self.
				//
				if ( auto hnd = this->stream.consumer->try_continue() ) {
					hnd = this->stream.sched_leave( hnd );
					this->stream.consumer = nullptr;
					this->stream.set_producer( continuation, this->lock );
					return hnd;
				}
			} else if ( is_producer ) {
				// If producer is over-producing, yield until sufficiently consumed.
				//
				if ( this->stream.size() >= this->stream.high_watermark ) {
					this->stream.set_producer( continuation, this->lock );
					return noop_coroutine();
				}
			}
			return continuation;
		}
		inline void await_resume() const noexcept {}
	};
	template<typename F>
	struct async_reader final : detail::buffer_consumer, async_buffer_locked {
		using result_type = decltype( std::declval<F>()( std::declval<vec_buffer&>() ) );
		
		F                     fn;
		result_type           result = {};

		async_reader( async_buffer& stream, F&& fn ) 
			: async_buffer_locked( stream ), fn( std::forward<F>( fn ) ), result( this->fn( stream ) ) {}

		// Consumer interface.
		//
		coroutine_handle<> try_continue() override {
			result = fn( this->stream );
			return result ? this->continuation : nullptr;
		}

		// Awaitable.
		//
		inline bool await_ready() noexcept {
			return !!result || this->stream.fin;
		}
		inline coroutine_handle<> await_suspend( coroutine_handle<> continuation ) noexcept {
			this->continuation = continuation;
			auto producer = stream.producer ? this->stream.sched_enter( stream.producer ) : noop_coroutine();
			this->stream.producer = nullptr;
			this->stream.set_consumer( this, this->lock );
			return producer;
		}
		inline result_type await_resume() noexcept {
			return std::move( result );
		}
	};

	// Stream utility provider.
	//
	template<typename Self>
	struct stream_utils {
		// Immutable versions.
		//
		stream_state& state() { return ( ( Self* ) static_cast<const Self*>( this ) )->state(); }
		const stream_state& state() const { return ( ( Self* ) static_cast<const Self*>( this ) )->state(); }
		async_buffer& readable() { return ( ( Self* ) static_cast<const Self*>( this ) )->readable(); }
		const async_buffer& readable() const { return ( ( Self* ) static_cast<const Self*>( this ) )->readable(); }
		async_buffer& writable() { return ( ( Self* ) static_cast<const Self*>( this ) )->writable(); }
		const async_buffer& writable() const { return ( ( Self* ) static_cast<const Self*>( this ) )->writable(); }
		
		// Writes more data to the stream.
		//
		template<typename F>
		async_writer_flush write_using( F&& fn ) {
			async_buffer_locked buffer{ writable() };
			fn( buffer.buffer() );
			return { std::move( buffer ), true };
		}
		template<typename T>
		async_writer_flush write( T&& data ) {
			async_buffer_locked buffer{ writable() };
			buffer.buffer().append_range( std::forward<T>( data ) );
			return { std::move( buffer ), true };
		}
		async_writer_stall stall() { return { writable() }; }
		async_writer_flush flush() { return { writable() }; }
		bool shutdown() {
			coroutine_handle<> hnd;
			{
				async_buffer_locked buffer{ writable() };
				if ( buffer->fin || buffer->ended )
					return buffer->fin;
				buffer->fin = true;

				if ( buffer->consumer ) {
					hnd = buffer->consumer->try_continue();
					if ( !hnd ) hnd = buffer->consumer->continuation;
					buffer->consumer = nullptr;
					hnd = buffer->sched_leave( hnd );
				} else {
					hnd = noop_coroutine();
				}
			}
			hnd();
			return true;
		}

		// Consumes data from the stream.
		//
		template<typename F>
		decltype( auto ) peek_using( F&& fn ) {
			async_buffer_locked buffer{ readable() };
			return fn( (const vec_buffer&) buffer.buffer() );
		}
		template<typename F>
		async_reader<F> read_until( F&& fn ) {
			return { readable(), (F&&) fn };
		}
		auto read( size_t min, size_t max ) -> async_reader<detail::take_counted> {
			return { readable(), { min, max } };
		}
		auto read( size_t count ) -> async_reader<detail::take_counted> {
			return { readable(), { count, count } };
		}
		auto read() -> async_reader<detail::take_counted> {
			return { readable(), { 1, std::dynamic_extent } };
		}
		auto read_into( uint8_t* out, size_t min, size_t max ) -> async_reader<detail::take_into_counted> {
			return { readable(), { out, min, max } };
		}
		auto read_into( uint8_t* out, size_t count ) -> async_reader<detail::take_into_counted> {
			return { readable(), { out, count, count } };
		}
		auto read_into( std::span<uint8_t> out, size_t min ) -> async_reader<detail::take_into_counted> {
			return { readable(), { out.data(), min, out.size() } };
		}
		auto read_into( std::span<uint8_t> out ) -> async_reader<detail::take_into_counted> {
			return { readable(), { out.data(), out.size(), out.size() } };
		}
		auto wait_until_shutdown() -> async_reader<detail::wait_shutdown> {
			return { readable(), {} };
		}
		bool is_shutting_down() {
			return readable().fin;
		}

		// Stop states.
		//
		bool stop( stream_stop_code code = stream_stop_killed, exception ex = {} ) {
			stream_stop_code expected = stream_stop_none;
			if ( !state().stop_code.compare_exchange_strong( expected, code ) ) {
				return false;
			}
			if ( !ex ) {
				switch ( code ) {
					case stream_stop_fin:     ex = XSTD_ESTR( "fin" );           break;
					case stream_stop_killed:  ex = XSTD_ESTR( "destroyed" );     break;
					case stream_stop_timeout: ex = XSTD_ESTR( "timeout" );       break;
					case stream_stop_error:   ex = XSTD_ESTR( "general error" ); break;
					default:                  ex = XSTD_ESTR( "unknown error" ); break;
				}
			}
			state().stop_reason = std::move( ex ).value_or( XSTD_ESTR( "unknown error" ) );
			state().stop_written.store( true, std::memory_order::release );
			readable().destroy();
			writable().destroy();

			// Find any async-scheduler and schedule the signal using it.
			//
			scheduler_reference ref = 
				readable().sched_enter || readable().sched_leave ||
				writable().sched_enter || writable().sched_leave ||
				chore_scheduler{};
			ref( state().signal( ref ) )( );
			return true;
		}
		bool stop( exception ex ) {
			return stop( stream_stop_error, std::move( ex ) );
		}

		// Stop details.
		//
		bool errored() const { return stop_code() > stream_stop_fin; }
		bool finished() const { return stop_code() == stream_stop_fin; }
		bool stopped() const { return stop_code() != stream_stop_none; }
		stream_stop_code stop_code() const { return state().stop_code.load( std::memory_order::relaxed ); }
		exception stop_reason() const {
			if ( stopped() ) {
				while ( !state().stop_written.load( std::memory_order::relaxed ) )
					yield_cpu();
				return state().stop_reason;
			}
			return {};
		}
	};

	// Single producer, single consumer in-memory stream.
	//
	struct stream : stream_utils<stream> {
		ref<stream_state> state_ = make_refc<stream_state>();
		async_buffer      buffer_ = {};

		stream_state& state() { return *state_; }
		async_buffer& readable() { return buffer_; }
		async_buffer& writable() { return buffer_; }

		// Creates an instance from memory.
		//
		static stream memory( vec_buffer mem = {} ) {
			return { .buffer_ = { std::move( mem ), /*.ended=*/ 0, /*.fin=*/ 1 } };
		}

		// Destroy all coroutines on destruction.
		//
		~stream() { stop( stream_stop_killed ); }
	};
	
	// Type erased async streams.
	//
	namespace detail {
		template<typename R, typename... Ax>
		using cb_asfn_t = R(*)( void* ctx, Ax... args );

		// Traits list.
		//
		struct stream_traits {
			cb_asfn_t<stream_state*, async_buffer**, async_buffer**>  unpack = nullptr;
			cb_asfn_t<void>                                           dtor = nullptr;
		};
		template<typename U>
		struct stream_traits_for : stream_traits {
			constexpr stream_traits_for() {
				this->unpack = &_unpack;
				this->dtor =   &_dtor;
			}
			static stream_state* _unpack( void* ptr, async_buffer** rb, async_buffer** wb ) { 
				auto* s = (U*) ptr;
				*rb = &s->readable();
				*wb = &s->writable();
				return &s->state();
			}
			static void _dtor( void* ptr ) { delete (U*) ptr; }
		};
		template<typename U>
		inline constexpr stream_traits stream_traits_v = stream_traits_for<U>{};
	};
	struct stream_view : stream_utils<stream_view> {
		void*                        ptr = nullptr;
		const detail::stream_traits* traits = nullptr;
		async_buffer*                readable_ = nullptr;
		async_buffer*                writable_ = nullptr;
		stream_state*                state_ = nullptr;

		// Construction by reference.
		//
		template<typename T>
		stream_view( T* p, bool swap = false ) {
			if constexpr ( std::is_base_of_v<stream_view, T> ) {
				ptr =       p->ptr;
				traits =    p->traits;
				if ( !ptr ) return;
				state_ =    traits->unpack( this->ptr, &this->readable_, &this->writable_ );
			} else {
				ptr =       p;
				traits =    &detail::stream_traits_v<T>;
				readable_ = &p->readable();
				writable_ = &p->writable();
				state_    = &p->state();
			}
			if ( swap ) std::swap( readable_, writable_ );
		}

		// Default copy/move.
		//
		stream_view() = default;
		stream_view( std::nullptr_t ) {}
		stream_view( stream_view&& ) noexcept = default;
		stream_view( const stream_view& ) = default;
		stream_view& operator=( stream_view&& ) noexcept = default;
		stream_view& operator=( const stream_view& ) = default;
		void swap( stream_view& o ) {
			std::swap( ptr,       o.ptr );
			std::swap( traits,    o.traits );
			std::swap( readable_, o.readable_ );
			std::swap( writable_, o.writable_ );
			std::swap( state_,    o.state_ );
		}

		// Observers.
		//
		void* address() const { return ptr; }
		bool has_value() const { return ptr != nullptr; }
		explicit operator bool() const { return has_value(); }
		bool operator==( const stream_view& o ) const { return ptr == o.ptr; };
		bool operator<( const stream_view& o ) const { return ptr <= o.ptr; };

		template<typename U> bool is() const { return traits == &detail::stream_traits_v<U>; }
		template<typename U> U& get() const { return *(U*) ptr; }
		template<typename U> U* get_if() const { return this->template is<U>() ? (U*) ptr : nullptr; }

		// Implementation.
		//
		stream_state& state() { return *state_; }
		async_buffer& readable() { return *readable_; }
		async_buffer& writable() { return *writable_; }
	};
	struct unique_stream : stream_view {
		unique_stream() = default;
		unique_stream( std::nullptr_t ) {}
		template<typename T>
		unique_stream( T* ptr ) : stream_view( ptr ) {}
		template<typename T>
		unique_stream( std::unique_ptr<T> ptr ) : stream_view( ptr.release() ) {}

		void reset() {
			unique_stream tmp{};
			this->swap( tmp );
		}
		void reset( unique_stream tmp ) {
			this->swap( tmp );
		}

		// Move construction, no copy.
		//
		unique_stream( unique_stream&& o ) noexcept { this->swap( o ); }
		unique_stream& operator=( unique_stream&& o ) noexcept { this->swap( o ); return *this; }
		unique_stream( const unique_stream& ) = delete;
		unique_stream& operator=( const unique_stream& ) = delete;

		// Destructor.
		//
		~unique_stream() {
			if ( ptr )
				traits->dtor( ptr );
		}
	};

	// Composition of two streams into a duplex.
	//
	struct duplex_options {
		size_t readable_high_watermark = 256_kb;
		size_t writable_high_watermark = 256_kb;
		// Used when a read request is complete.
		scheduler_reference readable_scheduler = chore_scheduler{};
		// Used when a write request is made.
		scheduler_reference writable_scheduler = chore_scheduler{};
	};
	struct duplex : stream_utils<duplex> {
		ref<stream_state> state_ = make_refc<stream_state>();
		async_buffer      input_ = {};
		async_buffer      output_ = {};

		duplex( duplex_options options = {} ) {
			writable().high_watermark = options.writable_high_watermark;
			writable().sched_enter =    options.writable_scheduler;
			readable().high_watermark = options.readable_high_watermark;
			readable().sched_leave =    options.readable_scheduler;
		}

		stream_state& state() { return *state_; }
		async_buffer& readable() { return input_; }
		async_buffer& writable() { return output_; }

		// Swapped composition for the implementation.
		//
		stream_view controller() { return { this, true }; }

		// Destroy all coroutines on destruction.
		//
		~duplex() { stop( stream_stop_killed ); }
	};
};