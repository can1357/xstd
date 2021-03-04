#pragma once
#include <atomic>
#include <tuple>
#include <iterator>
#include <shared_mutex>
#include <initializer_list>
#include "intrinsics.hpp"
#include "hashable.hpp"
#include "spinlock.hpp"
#include "assert.hpp"

namespace xstd
{
	namespace impl
	{
		static constexpr size_t primes[] = 
		{
			53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
			196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
			50331653, 100663319, 201326611, 402653189, 805306457, 1610612741
		};

		template<typename K, typename V>
		struct atomic_hashmap_entry : std::pair<const K, V>, std::enable_shared_from_this<atomic_hashmap_entry<K, V>>
		{
			using base_type = std::pair<const K, V>;
			using std::pair<const K, V>::pair;

			// Manual inc/dec ref.
			//
			inline void inc_ref() const
			{
				std::shared_ptr<const atomic_hashmap_entry<K, V>> leaked = this->shared_from_this();
				std::uninitialized_default_construct_n( &leaked, 1 );
			}
			inline void dec_ref() const
			{
				std::shared_ptr<const atomic_hashmap_entry<K,V>> leaked = this->shared_from_this();
				std::shared_ptr<const atomic_hashmap_entry<K, V>> copy;
				bytes( copy ) = bytes( leaked );
			}
		};

		template<typename T>
		struct atomic_hashmap_bucket
		{
			// Entry type.
			//
			struct entry
			{
				uint64_t is_entry : 1;  // Bit indicating the type, 2: pair<K, v>*, 1: another level.
				uint64_t pointer  : 63;  // Pointer to the type.

				inline entry( std::nullptr_t _ = {} ) : is_entry( false ), pointer( 0 ) {}
				inline entry( T* ptr ) : is_entry( true ), pointer( ( ( uint64_t ) ptr ) >> 1 ) {}
				inline entry( atomic_hashmap_bucket* ptr ) : is_entry( false ), pointer( ( ( uint64_t ) ptr ) >> 1 ) {}

				inline atomic_hashmap_bucket* get_bucket()
				{
					return is_entry == 0 ? ( atomic_hashmap_bucket* ) ( pointer << 1 ) : nullptr;
				}
				inline T* get_entry()
				{
					return is_entry == 1 ? ( T* ) ( pointer << 1 ) : nullptr;
				}
				inline void destroy()
				{
					if ( auto e = get_entry() )
						e->dec_ref();
					else if ( auto b = get_bucket() )
						std::destroy_at( b );
					is_entry = 0; pointer = 0;
				}
			};

			// Shared spinlock used as the reference counter.
			//
			shared_spinlock        refs = {};

			// Back-link to the owner.
			//
			atomic_hashmap_bucket* upper_link = nullptr;
			uint32_t               upper_index = 0;

			// List of entries.
			//
			uint32_t               count;
			uint32_t               divisor;
			uint32_t               non_null_count;
			entry                  entries[];

			// Constructed by the backlink and the size.
			//
			inline atomic_hashmap_bucket( atomic_hashmap_bucket* upper_link, uint32_t upper_index, uint32_t count, uint32_t divisor )
				: upper_link( upper_link ), upper_index( upper_index ), count( count ), divisor( divisor ), non_null_count( 0 )
			{
				std::uninitialized_fill_n( entries, count, entry{} );
			}

			// Make it iterable.
			//
			inline auto begin() { return &entries[ 0 ]; }
			inline auto end() { return &entries[ count ]; }
			inline auto begin() const { return &entries[ 0 ]; }
			inline auto end() const { return &entries[ count ]; }

			// Destructor destroys recursively.
			//
			inline ~atomic_hashmap_bucket()
			{
				refs.lock();
				for ( auto& entry : *this )
					entry.destroy();
				refs.unlock();
			}

			// Allocation helper.
			//
			inline static atomic_hashmap_bucket* allocate( atomic_hashmap_bucket* upper_link, size_t upper_index, uint32_t count, uint32_t divisor = 0 )
			{
				size_t size = sizeof( atomic_hashmap_bucket ) + sizeof( entry ) * count;
				return new ( new uint64_t[ ( size + 7 ) & ~7 ] ) atomic_hashmap_bucket( upper_link, upper_index, count, divisor ? divisor : count );
			}
		};

		template<typename T>
		struct alignas(16) atomic_hashmap_iterator
		{
			// Iterator traits.
			//
			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type =   int64_t;
			using value_type =        typename T::base_type;
			using reference =         typename T::base_type&;
			using pointer =           std::shared_ptr<T>;

			using hashmap_entry_type = typename atomic_hashmap_bucket<T>::entry;
			
			// Current position, iterator holds a shared lock on bucket.
			//
			atomic_hashmap_bucket<T>* bucket;
			size_t                    hash;
			std::shared_ptr<T>        value_reference;

			// Implement the constructors.
			//
			inline atomic_hashmap_iterator() : bucket( nullptr ), hash( 0 ) {}
			inline atomic_hashmap_iterator( atomic_hashmap_bucket<T>* bucket, size_t hash ) : bucket( bucket ), hash( hash ) { if ( bucket ) { bucket->refs.lock_shared(); normalize(); } }
			inline atomic_hashmap_iterator( atomic_hashmap_bucket<T>* bucket, size_t hash, std::adopt_lock_t adopt ) : bucket( bucket ), hash( hash ) { if ( bucket ) normalize(); }
			inline atomic_hashmap_iterator( atomic_hashmap_iterator&& o ) noexcept : bucket( std::exchange( o.bucket, nullptr ) ), hash( o.hash ), value_reference( std::move( o.value_reference ) ) {}
			inline atomic_hashmap_iterator( const atomic_hashmap_iterator& o ) : atomic_hashmap_iterator( o.bucket, o.hash ) {}

			// Implement copy/move assignment.
			//
			inline atomic_hashmap_iterator& operator=( atomic_hashmap_iterator&& o ) noexcept
			{
				std::swap( bucket, o.bucket );
				std::swap( hash, o.hash );
				std::swap( value_reference, o.value_reference );
				return *this;
			}
			inline atomic_hashmap_iterator& operator=( const atomic_hashmap_iterator& o ) noexcept
			{
				reset();
				bucket = o.bucket;
				hash = o.hash;
				value_reference = o.value_reference;
				if ( bucket )
					bucket->refs.lock_shared();
				return *this;
			}

			// Implement reset/release.
			//
			inline void release()
			{
				bucket = nullptr;
				hash = 0;
			}
			inline void reset()
			{
				if ( bucket )
					bucket->refs.unlock_shared();
				bucket = nullptr;
				hash = 0;
			}

			// Helpers.
			//
			inline hashmap_entry_type* normalize()
			{
				while ( bucket )
				{
					size_t index = hash % bucket->divisor;
					auto& entry = bucket->entries[ index ];

					// If end() or item entry, return.
					//
					bool at_end = !bucket->upper_link && hash == bucket->count;
					if ( at_end || entry.is_entry )
					{
						if ( !at_end )
							value_reference = entry.get_entry()->shared_from_this();
						return &entry;
					}
					// If bucket, recurse buckets.
					//
					else if ( auto* buck = entry.get_bucket() )
					{
						bucket = buck;
						hash /= bucket->divisor;
					}
					else
					{
						return nullptr;
					}
				}
			}
			inline hashmap_entry_type* normalize() const { return xstd::make_mutable( this )->normalize(); }

			inline atomic_hashmap_iterator& advance( difference_type direction, difference_type offset = 0 )
			{
				auto index = difference_type( hash % bucket->divisor ) + direction + offset;
				do
				{
					// If iterating backwards from end():
					//
					if ( direction < 0 && index == -1 && !bucket->upper_link )
					{
						index = bucket->count - 1;
					}
					// If we've reached the end of this bucket:
					//
					else if ( direction > 0 ? index >= bucket->count : index < 0 )
					{
						std::shared_lock lock{ bucket->refs, std::adopt_lock_t{} };

						auto new_bucket = bucket->upper_link;
						index = bucket->upper_index + direction;
						if ( !new_bucket )
						{
							// next => end()
							if ( direction > 0 )
							{
								lock.release();
								if ( hash == bucket->count && !offset )
									xstd::error( XSTD_ESTR( "Iterating past end()." ) );
								index = bucket->count;
							}
							// prev <= first()
							else
							{
								xstd::error( XSTD_ESTR( "Iterating before begin()." ) );
							}
							break;
						}
						new_bucket->refs.lock_shared();
						bucket = new_bucket;
					}
					// If we've reached yet another bucket, swap and recurse.
					//
					else if ( auto* next_bucket = bucket->entries[ index ].get_bucket() )
					{
						std::shared_lock lock{ bucket->refs, std::adopt_lock_t{} };
						next_bucket->refs.lock_shared();
						index = direction > 0 ? 0 : next_bucket->count - 1;
						bucket = next_bucket;
					}
					// If we've reached an entry, break.
					//
					else if ( auto* entry = bucket->entries[ index ].get_entry() )
					{
						value_reference = entry->shared_from_this();
						break;
					}
					// If null entry, skip it.
					//
					else
					{
						index += direction;
					}
				}
				while ( bucket );
				hash = index;
				return *this;
			}

			// Support bidirectional iteration.
			//
			inline atomic_hashmap_iterator& operator++() { return advance( +1 ); }
			inline atomic_hashmap_iterator& operator--() { return advance( -1 ); }
			inline atomic_hashmap_iterator operator++( int ) { auto s = *this; operator++(); return s; }
			inline atomic_hashmap_iterator operator--( int ) { auto s = *this; operator--(); return s; }

			// Iterator comparison.
			//
			inline bool operator==( const atomic_hashmap_iterator& other ) const { return bucket == other.bucket && hash == other.hash; }
			inline bool operator!=( const atomic_hashmap_iterator& other ) const { return !operator==( other ); }

			// Decay to a shared pointer.
			//
			inline pointer operator->() const { return value_reference; }
			inline reference operator*() const { return ( reference ) value_reference->first; }
			inline operator const pointer&() const { return value_reference; }

			// Unlock on destruction.
			//
			inline ~atomic_hashmap_iterator() { reset(); }
		};
	};

	template<typename K, typename V, typename Hs = hasher<K>, typename Eq = std::equal_to<K>>
	struct atomic_hashmap
	{
		// Container traits.
		//
		using key_type =        K;
		using mapped_type =     V;
		using value_type =      std::shared_ptr<std::pair<const K, V>>;
		using hasher =          Hs;
		using key_equal =       Eq;
		using reference =       value_type&;
		using const_reference = const value_type&;
		using pointer =         value_type*;
		using const_pointer =   const value_type*;

		using iterator =        impl::atomic_hashmap_iterator<impl::atomic_hashmap_entry<K ,V>>; //
		using const_iterator =  impl::atomic_hashmap_iterator<const impl::atomic_hashmap_entry<K ,V>>; //

		// Internal traits.
		//
		using bucket_type =     impl::atomic_hashmap_bucket<impl::atomic_hashmap_entry<K, V>>; //impl::atomic_hashmap_bucket<mut_value_type>;
		using entry_type =      typename bucket_type::entry;

		// Base bucket and the number of elements.
		//
		bucket_type* base = nullptr;
		std::atomic<size_t> count = 0;

		// The bucket configuration.
		//
		const size_t* primes;
		size_t maximum_nesting_level;
		
		// Constructed by estimate capacity.
		//
		inline atomic_hashmap( size_t estimate_capacity = 0, size_t maximum_nesting_level = 3 ) : maximum_nesting_level( maximum_nesting_level )
		{
			// Determine the first-layer width.
			//
			primes = std::begin( impl::primes );
			while ( *primes < estimate_capacity )
			{
				primes++;
				if ( ( primes + maximum_nesting_level ) > std::end( impl::primes ) )
					xstd::error( XSTD_ESTR( "Invalid hashmap size." ) );
			}
		}

		// Constructed by initializer list.
		//
		inline atomic_hashmap( const std::initializer_list<std::pair<K, V>>& list ) : atomic_hashmap( list.size() * 4 )
		{
			for ( const auto& pair : list )
				insert( pair.first, pair.second );
		}

		// Copy/Move construction and assignment.
		// - Not atomic!
		//
		inline atomic_hashmap( const atomic_hashmap& o ) : primes( o.primes ), maximum_nesting_level( o.maximum_nesting_level )
		{
			for ( const auto& pair : o )
				insert( pair.first, pair.second );
		}
		inline atomic_hashmap( atomic_hashmap&& o ) noexcept : base( std::exchange( o.base, nullptr ) ), count( std::exchange( o.count, 0 ) ),  primes( o.primes ), maximum_nesting_level( o.maximum_nesting_level ) 
		{
		}
		inline atomic_hashmap& operator=( const atomic_hashmap& o )
		{
			primes = o.primes;
			maximum_nesting_level = o.maximum_nesting_level;
			clear();
			for ( const auto& pair : o )
				insert( pair.first, pair.second );
			return *this;
		}
		inline atomic_hashmap& operator=( atomic_hashmap&& o ) noexcept
		{
			std::swap( base, o.base );
			std::swap( count, o.count );
			std::swap( primes, o.primes );
			std::swap( maximum_nesting_level, o.maximum_nesting_level );
			return *this;
		}

		// Implement the STL map interface.
		// - Note: Bucket details do not take multi-levelness of this container into account.
		//
		inline iterator begin() { if ( !base ) clear(); return iterator{ base, base->divisor }.advance( +1, -1 ); }
		inline iterator end() { if ( !base ) clear(); return iterator{ base, base->count }; }
		inline const_iterator begin() const { return ( const_iterator&& ) make_mutable( this )->begin(); }
		inline const_iterator cbegin() const { return begin(); }
		inline const_iterator end() const { return ( const_iterator&& ) make_mutable( this )->end(); }
		inline const_iterator cend() const { return end(); }
		inline size_t size() const { return count; }

		template<typename... Tx>
		inline std::pair<iterator, bool> emplace( const K& key, Tx&&... args ) { return get_node_with_default( key, [ & ] ()-> V { return V{ std::forward<Tx>( args )... }; }, false, true ); }
		inline std::pair<iterator, bool> insert( const K& key, V&& value ) { return get_node_with_default( key, [ & ] ()-> V&& { return std::move( value ); }, false, true ); }
		inline std::pair<iterator, bool> insert( const K& key, const V& value ) { return get_node_with_default( key, [ & ] ()-> const V& { return value; }, false, true ); }
		inline std::pair<iterator, bool> insert_or_assign( const K& key, V&& value ) { return get_node_with_default( key, [ & ] ()-> V&& { return std::move( value ); }, true, true ); }
		inline std::pair<iterator, bool> insert_or_assign( const K& key, const V& value ) { return get_node_with_default( key, [ & ] ()-> const V& { return value; }, true, true ); }
		inline iterator erase( const iterator& it )
		{
			while ( true )
			{
				it.normalize();
				auto& entry = it.bucket->entries[ it.hash % it.bucket->count ];
				entry_type expected = entry;
				if ( cmpxchg( entry, expected, entry_type{} ) )
				{
					fassert( expected.pointer );
					count--;
					expected.destroy();
					return std::next( it );
				}
				yield_cpu();
			}
		}
		inline size_t erase( const K& key )
		{
			if ( !base ) return 0;
			auto it = find( key );
			if ( it != end() )
			{
				erase( std::move( it ) );
				return 1;
			}
			return 0;
		}

		inline iterator find( const K& key ) { return make_mutable( this )->get_node_with_default( key, [ ] { return V{}; }, false, false ).first; }
		inline const_iterator find( const K& key ) const { return ( const_iterator&& ) make_mutable( this )->find( key ); }
		inline auto at( const K& key ) const { return find( key ).value_reference; }
		inline auto operator[]( const K& key ) { return get_node_with_default( key, [ ] { return V{}; }, false, true ).first.value_reference; }
		inline bool contains( const K& key ) const { return find( key ) != end(); }

		inline float load_factor() const
		{
			if ( !base ) return 0;
			size_t n_max = 0;
			size_t n_used = 0;
			auto rec_discover = [ & ] ( auto&& self, auto* bucket ) -> void
			{
				std::shared_lock _g{ bucket->refs };
				n_max += bucket->count;
				n_used += bucket->non_null_count;
				for ( size_t n = 0; n != bucket->count; n++ )
				{
					if ( auto* sub = bucket->entries[ n ].get_bucket() )
						self( self, sub );
				}
			};
			rec_discover( rec_discover, base );
			return float( n_used ) / float( n_max );
		}
		inline constexpr float max_load_factor() const { return 1.0f; }
		inline size_t bucket_count() const { return primes[ 0 ]; }
		inline size_t max_bucket_count() const { return primes[ maximum_nesting_level - 1 ]; }
		inline constexpr key_equal key_eq() const { return key_equal{}; }
		inline constexpr hasher hash_function() const { return hasher{}; }

		// Common node search/insert algorithm.
		//
		template<typename F>
		std::pair<iterator, bool> get_node_with_default( const K& key, F&& fetch_value, bool assign, bool set_default )
		{
			if ( !base ) clear();

			entry_type new_entry = {};
			std::shared_ptr<impl::atomic_hashmap_entry<K, V>> sptr;
			auto make_entry = [ & ] () -> auto&
			{
				if ( !new_entry.pointer )
				{
					sptr = std::make_shared<impl::atomic_hashmap_entry<K, V>>( key, fetch_value() );
					new_entry = entry_type{ sptr.get() };
				}
				return new_entry;
			};

			// Get the base iterator.
			//
			bucket_type* bucket = base;
			bucket->refs.lock_shared();

			// Search for the bucket:
			//
			size_t depth = 1;
			size_t hash = hasher{}( key );
			while ( bucket )
			{
				// If we're still iterating a map.
				//
				if ( depth <= maximum_nesting_level )
				{
					std::shared_lock local_lock{ bucket->refs, std::adopt_lock_t{} };

					size_t index = hash % bucket->divisor;
					entry_type entry = bucket->entries[ index ];

					// If storing a pair:
					//
					if ( auto* item = entry.get_entry() )
					{
						if ( Eq{}( item->first, key ) )
						{
							if ( assign )
								item->second = fetch_value();
							local_lock.release();
							return { iterator{ bucket, hash, std::adopt_lock_t{} }, false };
						}
						else
						{
							// Rehash the item. 
							//
							size_t rehash = hasher{}( item->first );
							size_t rehash_index = rehash;
							for ( auto it = bucket; it; it = it->upper_link )
								rehash_index /= it->divisor;

							// Relocate the item to a new block and try again.
							//
							auto* new_bucket = bucket_type::allocate(
								bucket,
								index,
								depth == maximum_nesting_level ? 8 :          primes[ depth ],
								depth == maximum_nesting_level ? UINT32_MAX : primes[ depth ]
							);
							auto& new_pentry = new_bucket->entries[ rehash_index % new_bucket->divisor ] = entry;
							new_bucket->non_null_count = 1;
							if ( !cmpxchg( bucket->entries[ index ], new_pentry, entry_type{ new_bucket } ) )
							{
								new_bucket->entries[ rehash_index % new_bucket->divisor ] = {};
								delete new_bucket;
								bucket->refs.lock_shared();
							}
							else
							{
								hash /= bucket->divisor;
								new_bucket->refs.lock_shared();
								bucket = new_bucket;
								++depth;
							}
							continue;
						}
					}
					// If nested level:
					//
					else if ( auto* next_bucket = entry.get_bucket() )
					{
						hash /= bucket->divisor;
						next_bucket->refs.lock_shared();
						bucket = next_bucket;
						++depth;
					}
					// If storing null, but we have no default value:
					//
					else if ( !set_default && !assign )
					{
						return { end(), false };
					}
					// If storing null and we have a default value:
					//
					else if ( cmpxchg( bucket->entries[ index ], entry, make_entry() ) )
					{
						make_entry().get_entry()->inc_ref();

						local_lock.release();
						++count;
						bucket->non_null_count++;
						return { iterator{ bucket, hash, std::adopt_lock_t{} }, true };
					}
				}
				// If we've switched to a collision list.
				//
				else
				{
					// Upgrade the lock to a unique one if we have a default value given.
					//
					if ( set_default || assign )
						bucket->refs.upgrade();

					// Try to find the entry within the linear list.
					//
					size_t count_left = bucket->non_null_count;
					entry_type* first_null = nullptr;
					for ( size_t n = 0; n != bucket->count; n++ )
					{
						entry_type& entry = bucket->entries[ n ];
						if ( auto* e = entry.get_entry() )
						{
							if ( Eq{}( e->first, key ) )
							{
								if ( assign )
									e->second = fetch_value();
								bucket->refs.downgrade();
								return { iterator{ bucket, n, std::adopt_lock_t{} }, false };
							}

							// If no other entries are left, break.
							//
							if ( !--count_left )
								break;
						}
						else if ( !first_null )
						{
							first_null = &entry;
						}
					}

					// If we don't have a default value, return.
					//
					if ( set_default || assign )
					{
						bucket->refs.unlock_shared();
						return { end(), false };
					}

					// If we did find a null entry, insert and return after downgrading the lock.
					//
					if ( first_null )
					{
						*first_null = make_entry();
						make_entry().get_entry()->inc_ref();
						bucket->refs.downgrade();
						++count;
						bucket->non_null_count++;
						return { iterator{ bucket, ( size_t ) ( first_null - &bucket->entries[ 0 ] ), std::adopt_lock_t{} }, true };
					}

					// Acquire a unique lock on the parent.
					//
					std::unique_lock _g{ bucket->upper_link->refs };

					// Allocate a new bucket and copy all entries.
					//
					auto* new_bucket = bucket_type::allocate( bucket->upper_link, bucket->upper_index, bucket->count * 2, UINT32_MAX );
					new_bucket->refs.lock_shared();
					for ( size_t n = 0; n != bucket->count; n++ )
						new_bucket->entries[ n ] = bucket->entries[ n ];

					// Write our entry.
					//
					size_t idx = bucket->count;
					new_bucket->entries[ idx ] = make_entry();
					make_entry().get_entry()->inc_ref();

					// Swap with the previous bucket.
					//
					bucket->upper_link->entries[ bucket->upper_link->upper_index ] = new_bucket;
					
					// Delete the leftover and return after downgrading the lock.
					//
					bucket->refs.downgrade();
					delete bucket;
					++count;
					new_bucket->non_null_count++;
					return { iterator{ new_bucket, idx, std::adopt_lock_t{} }, true };
				}
			}
			unreachable();
		}

		// Resets the state.
		//
		inline void clear()
		{
			auto old = std::exchange( base, bucket_type::allocate( nullptr, 0, primes[ 0 ] ) );
			count = 0;
			if ( old )
				delete old;
		}

		// Delete base on destruction.
		//
		inline ~atomic_hashmap() { if ( base ) delete base; }
	};
};