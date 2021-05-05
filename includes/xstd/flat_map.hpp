#pragma once
#include <tuple>
#include <iterator>
#include <vector>
#include <initializer_list>
#include "hashable.hpp"
#include "small_vector.hpp"

// Implements flat map alternatives with heterogeneous lookup and O(1) storage.
//
namespace xstd
{
	namespace impl
	{
		// Adaptive search limit is the limit of item entries where we start hashing afterwards for searches.
		//
		static constexpr size_t adaptive_search_limit = 4;

		// Default hasher choice prefering string view.
		//
		template<typename T>
		struct pick_hasher { using type = hasher<>; };
		template<String T>
		struct pick_hasher<T> { using type = hasher<string_view_t<T>>; };

		// Entry types.
		//
		template<typename K, typename V, typename Hs>
		struct flat_map_entry
		{
			std::unique_ptr<std::pair<K, V>> pair;
			size_t hash;

			// Default construction.
			//
			inline flat_map_entry() : flat_map_entry( K(), V() ) {};
			
			// Default move.
			//
			flat_map_entry( flat_map_entry&& ) noexcept = default;
			flat_map_entry& operator=( flat_map_entry&& ) noexcept = default;
			
			// Cloning copy.
			//
			inline flat_map_entry( const flat_map_entry& o ) : pair( std::make_unique<std::pair<K, V>>( *o.pair ) ), hash( o.hash ) {}
			inline flat_map_entry& operator=( const flat_map_entry& o )
			{
				*pair = *o.pair;
				hash = o.hash;
				return *this;
			}

			// Constructed from a key-value pair.
			//
			template<typename K2, typename V2>
			inline flat_map_entry( std::pair<K2, V2>&& pair ) : pair( std::make_unique<std::pair<K, V>>( std::move( pair ) ) ), hash( Hs{}( this->pair->first ) ) {}
			template<typename K2, typename V2>
			inline flat_map_entry( const std::pair<K2, V2>& pair ) : pair( std::make_unique<std::pair<K, V>>( pair.first, pair.second ) ), hash( Hs{}( this->pair->first ) ) {}
			inline flat_map_entry( K key, V value ) : pair( std::make_unique<std::pair<K, V>>( std::move( key ), std::move( value ) ) ), hash( Hs{}( this->pair->first ) ) {}

			// View from the iterator.
			//
			inline std::pair<const K, V>& view() & { return *( std::pair<const K, V>* ) pair.get(); }
			inline const std::pair<const K, V>& view() const & { return *( const std::pair<const K, V>* ) pair.get(); }

			// Comparison.
			//
			inline bool operator<( size_t o ) const { return hash < o; }
			inline bool operator==( size_t o ) const { return hash == o; }
			inline bool operator!=( size_t o ) const { return hash != o; }
			inline bool operator<( const flat_map_entry& o ) const { return hash < o.hash; }
			inline bool operator==( const flat_map_entry& o ) const { return hash == o.hash && pair->first == o.pair->first; }
			inline bool operator!=( const flat_map_entry& o ) const { return hash != o.hash || pair->first != o.pair->first; }
		};
		template<typename K, typename V>
		struct flat_smap_entry
		{
			std::unique_ptr<std::pair<K, V>> pair;

			// Default construction.
			//
			inline flat_smap_entry() : flat_smap_entry( K(), V() ) {};
			
			// Default move.
			//
			flat_smap_entry( flat_smap_entry&& ) noexcept = default;
			flat_smap_entry& operator=( flat_smap_entry&& ) noexcept = default;
			
			// Cloning copy.
			//
			inline flat_smap_entry( const flat_smap_entry& o ) : pair( std::make_unique<std::pair<K, V>>( *o.pair ) ) {}
			inline flat_smap_entry& operator=( const flat_smap_entry& o )
			{
				*pair = *o.pair;
				return *this;
			}

			// Constructed from a key-value pair.
			//
			template<typename K2, typename V2>
			inline flat_smap_entry( std::pair<K2, V2>&& pair ) : pair( std::make_unique<std::pair<K, V>>( std::move( pair ) ) ) {}
			template<typename K2, typename V2>
			inline flat_smap_entry( const std::pair<K2, V2>& pair ) : pair( std::make_unique<std::pair<K, V>>( pair.first, pair.second ) ) {}
			inline flat_smap_entry( K key, V value ) : pair( std::make_unique<std::pair<K, V>>( std::move( key ), std::move( value ) ) ) {}

			// View from the iterator.
			//
			inline std::pair<const K, V>& view() & { return *( std::pair<const K, V>* ) pair.get(); }
			inline const std::pair<const K, V>& view() const & { return *( const std::pair<const K, V>* ) pair.get(); }

			// Comparison.
			//
			template<typename T> inline bool operator<( const T& o ) const { return pair->first < o; }
			template<typename T> inline bool operator==( const T& o ) const { return pair->first == o; }
			template<typename T> inline bool operator!=( const T& o ) const { return pair->first != o; }
			template<> inline bool operator< <flat_smap_entry>( const flat_smap_entry& o ) const { return pair->first < o.pair->first; }
			template<> inline bool operator== <flat_smap_entry>( const flat_smap_entry& o ) const { return pair->first == o.pair->first; }
			template<> inline bool operator!= <flat_smap_entry>( const flat_smap_entry& o ) const { return pair->first != o.pair->first; }
		};
		template<typename K, typename V, typename Hs>
		struct inplace_flat_map_entry
		{
			std::pair<K, V> pair;
			size_t hash;

			// Default construction.
			//
			inline inplace_flat_map_entry() : inplace_flat_map_entry( K(), V() ) {};
			
			// Default move/copy.
			//
			inline inplace_flat_map_entry( inplace_flat_map_entry&& ) noexcept = default;
			inline inplace_flat_map_entry( const inplace_flat_map_entry& ) = default;
			inline inplace_flat_map_entry& operator=( inplace_flat_map_entry&& ) noexcept = default;
			inline inplace_flat_map_entry& operator=( const inplace_flat_map_entry& ) = default;

			// Constructed from a key-value pair.
			//
			template<typename K2, typename V2>
			inline inplace_flat_map_entry( std::pair<K2, V2>&& pair ) : pair( std::move( pair ) ), hash( Hs{}( this->pair.first ) ) {}
			template<typename K2, typename V2>
			inline inplace_flat_map_entry( const std::pair<K2, V2>& pair ) : pair( pair ), hash( Hs{}( this->pair.first ) ) {}
			inline inplace_flat_map_entry( K key, V value ) : pair( std::move( key ), std::move( value ) ), hash( Hs{}( this->pair.first ) ) {}

			// View from the iterator.
			//
			inline std::pair<const K, V>& view() & { return *( std::pair<const K, V>* ) &pair; }
			inline const std::pair<const K, V>& view() const & { return *( const std::pair<const K, V>* ) &pair; }

			// Comparison.
			//
			inline bool operator<( size_t o ) const { return hash < o; }
			inline bool operator==( size_t o ) const { return hash == o; }
			inline bool operator!=( size_t o ) const { return hash != o; }
			inline bool operator<( const inplace_flat_map_entry& o ) const { return hash < o.hash; }
			inline bool operator==( const inplace_flat_map_entry& o ) const { return hash == o.hash && pair.first == o.pair.first; }
			inline bool operator!=( const inplace_flat_map_entry& o ) const { return hash != o.hash || pair.first != o.pair.first; }
		};
		template<typename K, typename V>
		struct inplace_flat_smap_entry
		{
			std::pair<K, V> pair;

			// Default construction.
			//
			inline inplace_flat_smap_entry() : inplace_flat_smap_entry( K(), V() ) {};

			// Default move.
			//
			inplace_flat_smap_entry( inplace_flat_smap_entry&& ) noexcept = default;
			inplace_flat_smap_entry& operator=( inplace_flat_smap_entry&& ) noexcept = default;

			// Cloning copy.
			//
			inline inplace_flat_smap_entry( const inplace_flat_smap_entry& o ) : pair( o.pair ) {}
			inline inplace_flat_smap_entry& operator=( const inplace_flat_smap_entry& o )
			{
				pair = o.pair;
				return *this;
			}

			// Constructed from a key-value pair.
			//
			template<typename K2, typename V2>
			inline inplace_flat_smap_entry( std::pair<K2, V2>&& pair ) : pair( std::move( pair ) ) {}
			template<typename K2, typename V2>
			inline inplace_flat_smap_entry( const std::pair<K2, V2>& pair ) : pair( pair.first, pair.second ) {}
			inline inplace_flat_smap_entry( K key, V value ) : pair( std::move( key ), std::move( value ) ) {}

			// View from the iterator.
			//
			inline std::pair<const K, V>& view()& { return *( std::pair<const K, V>* ) &pair; }
			inline const std::pair<const K, V>& view() const& { return *( const std::pair<const K, V>* ) &pair; }

			// Comparison.
			//
			template<typename T> inline bool operator<( const T& o ) const { return pair.first < o; }
			template<typename T> inline bool operator==( const T& o ) const { return pair.first == o; }
			template<typename T> inline bool operator!=( const T& o ) const { return pair.first != o; }
			template<> inline bool operator< <inplace_flat_smap_entry>( const inplace_flat_smap_entry& o ) const { return pair.first < o.pair.first; }
			template<> inline bool operator== <inplace_flat_smap_entry>( const inplace_flat_smap_entry& o ) const { return pair.first == o.pair.first; }
			template<> inline bool operator!= <inplace_flat_smap_entry>( const inplace_flat_smap_entry& o ) const { return pair.first != o.pair.first; }
		};

		// Common iterator type.
		//
		template<typename It>
		struct flat_map_iterator
		{
			// Define iterator traits.
			//
			using iterator_category = typename std::iterator_traits<std::remove_cvref_t<It>>::iterator_category;
			using difference_type =   typename std::iterator_traits<std::remove_cvref_t<It>>::difference_type;
			using reference =         decltype( std::declval<It>()->view() );
			using value_type =        std::remove_reference_t<reference>;
			using pointer =           value_type*;

			// Constructed by the original iterator.
			//
			It at;
			inline flat_map_iterator( It i ) : at( std::move( i ) ) {}

			// Default copy/move.
			//
			inline flat_map_iterator( flat_map_iterator&& ) noexcept = default;
			inline flat_map_iterator( const flat_map_iterator& ) = default;
			inline flat_map_iterator& operator=( flat_map_iterator&& ) noexcept = default;
			inline flat_map_iterator& operator=( const flat_map_iterator& ) = default;

			// Support bidirectional/random iteration.
			//
			inline flat_map_iterator& operator++() { ++at; return *this; }
			inline flat_map_iterator operator++( int ) { auto s = *this; operator++(); return s; }
			inline flat_map_iterator& operator--() { --at; return *this; }
			inline flat_map_iterator operator--( int ) { auto s = *this; operator--(); return s; }
			inline flat_map_iterator& operator+=( difference_type d ) { at += d; return *this; }
			inline flat_map_iterator& operator-=( difference_type d ) { at -= d; return *this; }
			inline flat_map_iterator operator+( difference_type d ) const { auto s = *this; s += d; return s; }
			inline flat_map_iterator operator-( difference_type d ) const { auto s = *this; s -= d; return s; }

			// Equality check against another iterator and difference.
			//
			template<typename It2> inline bool operator==( const flat_map_iterator<It2>& other ) const { return at == other.at; }
			template<typename It2> inline bool operator!=( const flat_map_iterator<It2>& other ) const { return at != other.at; }
			template<typename It2> inline difference_type operator-( const flat_map_iterator<It2>& other ) const { return at - other.at; }

			// Override accessor to redirect to the pair.
			//
			inline reference operator*() const { return at->view(); }
			inline pointer operator->() const { return &at->view(); }
		};
	};

	// Unordered map.
	//
	template<
		typename K, 
		typename V, 
		typename Hs = typename impl::pick_hasher<K>::type,
		bool InPlace = false,
	    bool Sorted = true,
		size_t Limit = 0>
	struct basic_flat_map
	{
		using storage_type =         std::conditional_t<InPlace, impl::inplace_flat_map_entry<K, V, Hs>, impl::flat_map_entry<K, V, Hs>>;
		using container_type =       std::conditional_t<Limit==0, std::vector<storage_type>, small_vector<storage_type, Limit>>;

		// Container traits.
		//
		using key_type =             K;
		using mapped_type =          V;
		using value_type =           std::pair<K, V>;
		using hasher =               Hs;
		using reference =            value_type&;
		using const_reference =      const value_type&;
		using pointer =              value_type*;
		using const_pointer =        const value_type*;
		using real_iterator =        typename container_type::iterator;
		using real_const_iterator =  typename container_type::const_iterator;
		using iterator =             impl::flat_map_iterator<real_iterator>;
		using const_iterator =       impl::flat_map_iterator<real_const_iterator>;

		// The container storing the items.
		//
		container_type values;

		// Default constructor/copy/move.
		//
		basic_flat_map() {}
		basic_flat_map( basic_flat_map&& ) noexcept = default;
		basic_flat_map( const basic_flat_map& ) = default;
		basic_flat_map& operator=( basic_flat_map&& ) noexcept = default;
		basic_flat_map& operator=( const basic_flat_map& ) = default;

		// Constructed by iterator pairs / initializer list.
		//
		template<typename It1, typename It2>
		basic_flat_map( It1&& beg, It2&& end )
		{ 
			values.insert( values.begin(), std::forward<It1>( beg ), std::forward<It2>( end ) );
			std::sort( values.begin(), values.end() );
		}
		inline basic_flat_map( const std::initializer_list<std::pair<K, V>>& list ) : basic_flat_map( list.begin(), list.end() ) {}

		// Implement the STL map interface.
		// - Note: Bucket details do not take multi-levelness of this container into account.
		//
		inline iterator begin() { return values.begin(); }
		inline iterator end() { return values.end(); }
		inline const_iterator begin() const { return values.begin(); }
		inline const_iterator end() const { return values.end(); }
		inline size_t size() const { return values.size(); }
		inline bool empty() const { return values.empty(); }

		// Internal searcher, returns [value pos, insertation pos].
		// 
		template<typename Kx>
		inline std::pair<real_iterator, real_iterator> searcher( const Kx& key, bool for_insert )
		{
			// If adaptive search is enabled and there's less than 8 items:
			//
			if ( values.size() <= ( for_insert ? impl::adaptive_search_limit : impl::adaptive_search_limit * 2 ) )
			{
				// See if we find a match via equality.
				//
				for ( auto it = values.begin(); it != values.end(); it++ )
					if ( it->view().first == key )
						return { it, it };

				// If not sorted or not inserting, simply return the end point.
				//
				if ( !Sorted || !for_insert )
					return { values.end(), values.end() };

				// Find the position to insert at.
				//
				size_t hash = Hs{}( key );
				return { values.end(), std::lower_bound( values.begin(), values.end(), hash ) };
			}
			else
			{
				// Hash the key.
				//
				size_t hash = Hs{}( key );
				if constexpr ( Sorted )
				{
					// Find the position, if hash and the key is matching return the iterator, else only 
					// return as the insertion pos.
					//
					auto it = std::lower_bound( values.begin(), values.end(), hash );
					if ( it != values.end() && ( it->hash != hash || it->view().first != key ) )
						return { values.end(), it };
					else
						return { it, it };
				}
				else
				{
					// Check every item, first by hash, then by key; if matching return.
					//
					for ( auto it = values.begin(); it != values.end(); ++it )
						if ( it->hash == hash && it->view().first == key )
							return { it, it };

					// Insert at the end.
					//
					return { values.end(), values.end() };
				}
			}
		}

		// -- Lookup.
		//
		template<typename Kx> inline iterator find( const Kx& key ) { return searcher( key, false ).first; }
		template<typename Kx> inline const_iterator find( const Kx& key ) const { return const_iterator{ make_mutable( this )->find( key ).at }; }
		template<typename Kx> inline auto& at( const Kx& key ) { return find( key )->second; }
		template<typename Kx> inline auto& at( const Kx& key ) const { return find( key )->second; }
		template<typename Kx> inline bool contains( const Kx& key ) const { return find( key ) != end(); }

		// -- Insertation.
		//
		template<typename Kx = K, typename... Tx>
		inline std::pair<iterator, bool> emplace( const Kx& key, Tx&&... args )
		{
			auto [vit, iit] = searcher( key, true );
			if ( vit != values.end() )
				return { vit, false };
			vit = values.insert( iit, value_type( key, std::forward<Tx>( args )... ) );
			return { vit, true };
		}
		template<typename Kx = K, typename Vx>
		inline std::pair<iterator, bool> insert_or_assign( const Kx& key, Vx&& value )
		{
			auto [vit, iit] = searcher( key, true );
			if ( vit != values.end() )
			{
				vit->view().second = std::forward<Vx>( value );
				return { vit, false };
			}
			vit = values.insert( iit, value_type( key, std::forward<Vx>( value ) ) );
			return { vit, true };
		}
		template<typename Kx = K>
		inline std::pair<iterator, bool> insert( const Kx& key, V&& value )
		{
			return emplace( key, std::forward<V>( value ) );
		}
		template<typename Kx = K>
		inline std::pair<iterator, bool> insert( std::pair<Kx, V> pair )
		{
			return emplace( std::move( pair.first ), std::move( pair.second ) );
		}
		template<typename It1, typename It2>
		inline void insert( It1 it, const It2& it2 )
		{
			while ( it != it2 )
				insert( *it++ );
		}
		inline iterator erase( const iterator& it )
		{
			return values.erase( it.at );
		}
		template<typename Kx>
		inline size_t erase( const Kx& key )
		{
			auto it = find( key );
			if ( it != end() )
			{
				values.erase( it.at );
				return 1;
			}
			return 0;
		}

		// -- Default lookup or insert.
		//
		template<typename Kx>
		inline V& operator[]( const Kx& key ) 
		{
			auto [vit, iit] = searcher( key, true );
			if ( vit != values.end() )
				return vit->view().second;
			return values.insert( iit, value_type( key, V{} ) )->view().second;
		}

		// -- Details.
		//
		inline constexpr hasher hash_function() const { return hasher{}; }
		inline void reserve( size_t n ) { values.reserve( n ); }

		// Resets the state.
		//
		inline void clear() { values.clear(); }

		// Redirect comparison to the container.
		//
		inline bool operator==( const basic_flat_map& o ) const { return values == o.values; }
		inline bool operator!=( const basic_flat_map& o ) const { return values != o.values; }
		inline bool operator<( const basic_flat_map& o ) const { return values < o.values; }
	};

	// Ordered map.
	//
	template<
		typename K, 
		typename V, 
		bool InPlace = false,
		size_t Limit = 0>
	struct basic_sorted_flat_map
	{
		using storage_type =         std::conditional_t<InPlace, impl::inplace_flat_smap_entry<K, V>, impl::flat_smap_entry<K, V>>;
		using container_type =       std::conditional_t<Limit==0, std::vector<storage_type>, small_vector<storage_type, Limit>>;

		// Container traits.
		//
		using key_type =             K;
		using mapped_type =          V;
		using value_type =           std::pair<K, V>;
		using reference =            value_type&;
		using const_reference =      const value_type&;
		using pointer =              value_type*;
		using const_pointer =        const value_type*;
		using real_iterator =        typename container_type::iterator;
		using real_const_iterator =  typename container_type::const_iterator;
		using iterator =             impl::flat_map_iterator<real_iterator>;
		using const_iterator =       impl::flat_map_iterator<real_const_iterator>;

		// The container storing the items.
		//
		container_type values;

		// Default constructor/copy/move.
		//
		basic_sorted_flat_map() {}
		basic_sorted_flat_map( basic_sorted_flat_map&& ) noexcept = default;
		basic_sorted_flat_map( const basic_sorted_flat_map& ) = default;
		basic_sorted_flat_map& operator=( basic_sorted_flat_map&& ) noexcept = default;
		basic_sorted_flat_map& operator=( const basic_sorted_flat_map& ) = default;

		// Constructed by iterator pairs / initializer list.
		//
		template<typename It1, typename It2>
		basic_sorted_flat_map( It1&& beg, It2&& end )
		{ 
			values.insert( values.begin(), std::forward<It1>( beg ), std::forward<It2>( end ) );
			std::sort( values.begin(), values.end() );
		}
		inline basic_sorted_flat_map( const std::initializer_list<std::pair<K, V>>& list ) : basic_sorted_flat_map( list.begin(), list.end() ) {}

		// Implement the STL map interface.
		// - Note: Bucket details do not take multi-levelness of this container into account.
		//
		inline iterator begin() { return values.begin(); }
		inline iterator end() { return values.end(); }
		inline const_iterator begin() const { return values.begin(); }
		inline const_iterator end() const { return values.end(); }
		inline size_t size() const { return values.size(); }
		inline bool empty() const { return values.empty(); }

		// Internal searcher, returns [value pos, insertation pos].
		// 
		template<typename Kx>
		inline std::pair<real_iterator, real_iterator> searcher( const Kx& key, bool for_insert )
		{
			// Find the position, if the key is matching return the iterator, else only 
			// return as the insertion pos.
			//
			auto it = std::lower_bound( values.begin(), values.end(), key );
			if ( it != values.end() && it->view().first != key )
				return { values.end(), it };
			else
				return { it, it };
		}

		// -- Lookup.
		//
		template<typename Kx> inline iterator find( const Kx& key ) { return searcher( key, false ).first; }
		template<typename Kx> inline const_iterator find( const Kx& key ) const { return const_iterator{ make_mutable( this )->find( key ).at }; }
		template<typename Kx> inline auto& at( const Kx& key ) { return find( key )->second; }
		template<typename Kx> inline auto& at( const Kx& key ) const { return find( key )->second; }
		template<typename Kx> inline bool contains( const Kx& key ) const { return find( key ) != end(); }
		template<typename Kx> inline iterator lower_bound( const Kx& key ) { return std::lower_bound( values.begin(), values.end(), key ); }
		template<typename Kx> inline const_iterator lower_bound( const Kx& key ) { return std::lower_bound( values.begin(), values.end(), key ); }
		template<typename Kx> inline iterator upper_bound( const Kx& key ) { return std::upper_bound( values.begin(), values.end(), key ); }
		template<typename Kx> inline const_iterator upper_bound( const Kx& key ) { return std::upper_bound( values.begin(), values.end(), key ); }

		// -- Insertation.
		//
		template<typename Kx = K, typename... Tx>
		inline std::pair<iterator, bool> emplace( const Kx& key, Tx&&... args )
		{
			auto [vit, iit] = searcher( key, true );
			if ( vit != values.end() )
				return { vit, false };
			vit = values.insert( iit, value_type( key, std::forward<Tx>( args )... ) );
			return { vit, true };
		}
		template<typename Kx = K, typename Vx>
		inline std::pair<iterator, bool> insert_or_assign( const Kx& key, Vx&& value )
		{
			auto [vit, iit] = searcher( key, true );
			if ( vit != values.end() )
			{
				vit->view().second = std::forward<Vx>( value );
				return { vit, false };
			}
			vit = values.insert( iit, value_type( key, std::forward<Vx>( value ) ) );
			return { vit, true };
		}
		template<typename Kx = K>
		inline std::pair<iterator, bool> insert( const Kx& key, V&& value )
		{
			return emplace( key, std::forward<V>( value ) );
		}
		template<typename Kx = K>
		inline std::pair<iterator, bool> insert( std::pair<Kx, V> pair )
		{
			return emplace( std::move( pair.first ), std::move( pair.second ) );
		}
		template<typename It1, typename It2>
		inline void insert( It1 it, const It2& it2 )
		{
			while ( it != it2 )
				insert( *it++ );
		}
		inline iterator erase( const iterator& it )
		{
			return values.erase( it.at );
		}
		template<typename Kx>
		inline size_t erase( const Kx& key )
		{
			auto it = find( key );
			if ( it != end() )
			{
				values.erase( it.at );
				return 1;
			}
			return 0;
		}

		// -- Default lookup or insert.
		//
		template<typename Kx>
		inline V& operator[]( const Kx& key ) 
		{
			auto [vit, iit] = searcher( key, true );
			if ( vit != values.end() )
				return vit->view().second;
			return values.insert( iit, value_type( key, V{} ) )->view().second;
		}

		// -- Details.
		//
		inline void reserve( size_t n ) { values.reserve( n ); }

		// Resets the state.
		//
		inline void clear() { values.clear(); }

		// Redirect comparison to the container.
		//
		inline bool operator==( const basic_sorted_flat_map& o ) const { return values == o.values; }
		inline bool operator!=( const basic_sorted_flat_map& o ) const { return values != o.values; }
		inline bool operator<( const basic_sorted_flat_map& o ) const { return values < o.values; }
	};

	// Variants.
	//
	// -> std::unordered_map, O(log n) insert, O(log n) lookup
	template<typename K, typename V, typename Hs = void>
	using flat_map =           basic_flat_map<K, V, std::conditional_t<std::is_void_v<Hs>, typename impl::pick_hasher<K>::type, Hs>, false, true>;
	// -> std::map,           O(log n) insert, O(log n) lookup
	template<typename K, typename V>
	using sorted_flat_map =    basic_sorted_flat_map<K, V, false>;
	// -> std::unordered_map, O(1) insert, O(N) lookup.
	template<typename K, typename V, typename Hs = void>
	using random_flat_map =    basic_flat_map<K, V, std::conditional_t<std::is_void_v<Hs>, typename impl::pick_hasher<K>::type, Hs>, false, false>;
	
	// Inplace versions.
	// - Does not allocate per entry, but no guarantee on pointer remaining the same upon insertion.
	//
	template<typename K, typename V, typename Hs = void, size_t L = 0>
	using inplace_map =        basic_flat_map<K, V, std::conditional_t<std::is_void_v<Hs>, typename impl::pick_hasher<K>::type, Hs>, true, true, L>;
	template<typename K, typename V, size_t L = 0>
	using sorted_inplace_map = basic_sorted_flat_map<K, V, true, L>;
	template<typename K, typename V, typename Hs = void, size_t L = 0>
	using random_inplace_map = basic_flat_map<K, V, std::conditional_t<std::is_void_v<Hs>, typename impl::pick_hasher<K>::type, Hs>, true, false, L>;
};

namespace std
{
	template <typename Pr, typename K, typename V, typename Hs, bool Ip, bool So, size_t L>
	inline size_t erase_if( xstd::basic_flat_map<K, V, Hs, Ip, So, L>& container, Pr&& predicate )
	{
		return std::erase_if( container.values, [ & ] ( auto&& pair ) { return predicate( pair.view() ); } );
	}
	template <typename Pr, typename K, typename V, bool Ip, size_t L>
	inline size_t erase_if( xstd::basic_sorted_flat_map<K, V, Ip, L>& container, Pr&& predicate )
	{
		return std::erase_if( container.values, [ & ] ( auto&& pair ) { return predicate( pair.view() ); } );
	}
};