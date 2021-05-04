#pragma once
#include <tuple>
#include <iterator>
#include <vector>
#include <initializer_list>
#include "hashable.hpp"

namespace xstd
{
	namespace impl
	{
		template<typename K, typename V, typename Hs, typename Eq>
		struct flat_map_entry
		{
			std::unique_ptr<std::pair<K, V>> pair;
			size_t hash;

			// Default construction.
			//
			flat_map_entry() : flat_map_entry( K(), V() ) {};
			
			// Default move.
			//
			flat_map_entry( flat_map_entry&& ) noexcept = default;
			flat_map_entry& operator=( flat_map_entry&& ) noexcept = default;
			
			// Cloning copy.
			//
			flat_map_entry( const flat_map_entry& o ) : pair( std::make_unique<std::pair<K, V>>( *o.pair ) ), hash( o.hash ) {}
			flat_map_entry& operator=( const flat_map_entry& o )
			{
				*pair = *o.pair;
				hash = o.hash;
				return *this;
			}

			// Constructed from a key-value pair.
			//
			template<typename K2, typename V2>
			flat_map_entry( std::pair<K2, V2>&& pair ) : pair( std::make_unique<std::pair<K, V>>( std::move( pair ) ) ), hash( Hs{}( this->pair->first ) ) {}
			template<typename K2, typename V2>
			flat_map_entry( const std::pair<K2, V2>& pair ) : pair( std::make_unique<std::pair<K, V>>( pair.first, pair.second ) ), hash( Hs{}( this->pair->first ) ) {}
			flat_map_entry( K key, V value ) : pair( std::make_unique<std::pair<K, V>>( std::move( key ), std::move( value ) ) ), hash( Hs{}( this->pair->first ) ) {}

			// View from the iterator.
			//
			std::pair<const K, V>& view() & { return *( std::pair<const K, V>* ) pair.get(); }
			const std::pair<const K, V>& view() const & { return *( const std::pair<const K, V>* ) pair.get(); }

			// Comparison.
			//
			bool operator<( size_t o ) const { return hash < o; }
			bool operator==( size_t o ) const { return hash == o; }
			bool operator!=( size_t o ) const { return hash != o; }
			bool operator<( const flat_map_entry& o ) const { return hash < o.hash; }
			bool operator==( const flat_map_entry& o ) const { return hash == o.hash && Eq{}( pair->first, o.pair->first ); }
			bool operator!=( const flat_map_entry& o ) const { return hash != o.hash || !Eq{}( pair->first, o.pair->first ); }
		};
		template<typename K, typename V, typename Hs, typename Eq>
		struct inplace_flat_map_entry
		{
			std::pair<K, V> pair;
			size_t hash;

			// Default construction.
			//
			inplace_flat_map_entry() : inplace_flat_map_entry( K(), V() ) {};
			
			// Default move/copy.
			//
			inplace_flat_map_entry( inplace_flat_map_entry&& ) noexcept = default;
			inplace_flat_map_entry( const inplace_flat_map_entry& ) = default;
			inplace_flat_map_entry& operator=( inplace_flat_map_entry&& ) noexcept = default;
			inplace_flat_map_entry& operator=( const inplace_flat_map_entry& ) = default;

			// Constructed from a key-value pair.
			//
			template<typename K2, typename V2>
			inplace_flat_map_entry( std::pair<K2, V2>&& pair ) : pair( std::move( pair ) ), hash( Hs{}( this->pair.first ) ) {}
			template<typename K2, typename V2>
			inplace_flat_map_entry( const std::pair<K2, V2>& pair ) : pair( pair ), hash( Hs{}( this->pair.first ) ) {}
			inplace_flat_map_entry( K key, V value ) : pair( std::move( key ), std::move( value ) ), hash( Hs{}( this->pair.first ) ) {}

			// View from the iterator.
			//
			std::pair<const K, V>& view() & { return *( std::pair<const K, V>* ) pair.get(); }
			const std::pair<const K, V>& view() const & { return *( const std::pair<const K, V>* ) pair.get(); }

			// Comparison.
			//
			bool operator<( size_t o ) const { return hash < o; }
			bool operator==( size_t o ) const { return hash == o; }
			bool operator!=( size_t o ) const { return hash != o; }
			bool operator<( const inplace_flat_map_entry& o ) const { return hash < o.hash; }
			bool operator==( const inplace_flat_map_entry& o ) const { return hash == o.hash && Eq{}( pair.first, o.pair.first ); }
			bool operator!=( const inplace_flat_map_entry& o ) const { return hash != o.hash || !Eq{}( pair.first, o.pair.first ); }
		};
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
			flat_map_iterator( It i ) : at( std::move( i ) ) {}

			// Default copy/move.
			//
			flat_map_iterator( flat_map_iterator&& ) noexcept = default;
			flat_map_iterator( const flat_map_iterator& ) = default;
			flat_map_iterator& operator=( flat_map_iterator&& ) noexcept = default;
			flat_map_iterator& operator=( const flat_map_iterator& ) = default;

			// Support bidirectional/random iteration.
			//
			flat_map_iterator& operator++() { ++at; return *this; }
			flat_map_iterator operator++( int ) { auto s = *this; operator++(); return s; }
			flat_map_iterator& operator--() { --at; return *this; }
			flat_map_iterator operator--( int ) { auto s = *this; operator--(); return s; }
			flat_map_iterator& operator+=( difference_type d ) { at += d; return *this; }
			flat_map_iterator& operator-=( difference_type d ) { at -= d; return *this; }
			flat_map_iterator operator+( difference_type d ) const { auto s = *this; s += d; return s; }
			flat_map_iterator operator-( difference_type d ) const { auto s = *this; s -= d; return s; }

			// Equality check against another iterator and difference.
			//
			template<typename It2> bool operator==( const flat_map_iterator<It2>& other ) const { return at == other.at; }
			template<typename It2> bool operator!=( const flat_map_iterator<It2>& other ) const { return at != other.at; }
			template<typename It2> difference_type operator-( const flat_map_iterator<It2>& other ) const { return at - other.at; }

			// Override accessor to redirect to the pair.
			//
			reference operator*() const { return at->view(); }
			pointer operator->() const { return &at->view(); }
		};
	};

	// Implements a flat unordered map with heterogeneous lookup and O(1) storage.
	// - Inplace will allow you to store without another level of indirection, but pointers will not remain valid after insertion.
	//
	template<
		typename K, 
		typename V, 
		typename Hs = std::conditional_t<String<K>, hasher<string_view_t<K>>, hasher<>>,
		typename Eq = std::equal_to<K>, 
		template<typename, typename> typename Base = std::vector, 
		bool InPlace = false>
	struct flat_map
	{
		using storage_type =         std::conditional_t<InPlace, impl::inplace_flat_map_entry<K, V, Hs, Eq>, impl::flat_map_entry<K, V, Hs, Eq>>;
		using underlying_container = Base<storage_type, std::allocator<storage_type>>;

		// Container traits.
		//
		using key_type =             K;
		using mapped_type =          V;
		using value_type =           std::pair<K, V>;
		using hasher =               Hs;
		using key_equal =            Eq;
		using reference =            value_type&;
		using const_reference =      const value_type&;
		using pointer =              value_type*;
		using const_pointer =        const value_type*;
		using iterator =             impl::flat_map_iterator<typename underlying_container::iterator>;
		using const_iterator =       impl::flat_map_iterator<typename underlying_container::const_iterator>;

		// The container storing the items.
		//
		underlying_container values;

		// Default constructor/copy/move.
		//
		flat_map() {}
		flat_map( flat_map&& ) noexcept = default;
		flat_map( const flat_map& ) = default;
		flat_map& operator=( flat_map&& ) noexcept = default;
		flat_map& operator=( const flat_map& ) = default;

		// Constructed by iterator pairs / initializer list.
		//
		template<typename It1, typename It2>
		flat_map( It1&& beg, It2&& end ) 
		{ 
			values.insert( values.begin(), std::forward<It1>( beg ), std::forward<It2>( end ) );
			std::sort( values.begin(), values.end() );
		}
		flat_map( const std::initializer_list<std::pair<K, V>>& list ) : flat_map( list.begin(), list.end() ) {}

		// Implement the STL map interface.
		// - Note: Bucket details do not take multi-levelness of this container into account.
		//
		inline iterator begin() { return values.begin(); }
		inline iterator end() { return values.end(); }
		inline const_iterator begin() const { return values.begin(); }
		inline const_iterator end() const { return values.end(); }
		inline size_t size() const { return values.size(); }

		// -- Lookup.
		//
		template<typename Kx>
		inline iterator find( const Kx& key )
		{
			size_t hash = Hs{}( key );
			auto it = std::lower_bound( values.begin(), values.end(), hash );
			if ( it != values.end() && it->view().first == key )
				return it;
			else
				return values.end();
		}
		template<typename Kx>
		inline auto& at( const Kx& key )
		{
			return find( key )->second;
		}
		template<typename Kx>
		inline const_iterator find( const Kx& key ) const
		{
			size_t hash = Hs{}( key );
			auto it = std::lower_bound( values.begin(), values.end(), hash );
			if ( it != values.end() && it->view().first == key )
				return it;
			else
				return values.end();
		}
		template<typename Kx>
		inline auto& at( const Kx& key ) const
		{
			return find( key )->second;
		}
		template<typename Kx>
		inline bool contains( const Kx& key ) const { return find( key ) != end(); }

		// -- Insertation.
		//
		template<typename Kx, typename... Tx>
		inline std::pair<iterator, bool> emplace( const Kx& key, Tx&&... args )
		{
			size_t hash = Hs{}( key );
			auto it = std::lower_bound( values.begin(), values.end(), hash );
			if ( it != values.end() && it->view().first == key )
				return { it, false };
			it = values.insert( it, value_type( key, std::forward<Tx>( args )... ) );
			return { it, true };
		}
		template<typename Kx>
		inline std::pair<iterator, bool> insert_or_assign( const Kx& key, V&& value )
		{
			size_t hash = Hs{}( key );
			auto it = std::lower_bound( values.begin(), values.end(), hash );
			if ( it != values.end() && it->view().first == key )
			{
				it->view().second = std::forward<V>( value );
				return { it, false };
			}
			it = values.insert( it, value_type( key, std::forward<V>( value )... ) );
			return { it, true };
		}
		template<typename Kx>
		inline std::pair<iterator, bool> insert( const Kx& key, V&& value )
		{
			return emplace( key, std::forward<V>( value ) );
		}
		template<typename Kx>
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
		inline auto& operator[]( const Kx& key ) 
		{
			size_t hash = Hs{}( key );
			auto it = std::lower_bound( values.begin(), values.end(), hash );
			if ( it == values.end() || it->view().first != key )
				it = values.insert( it, value_type( key, V() ) );
			return it->view().second;
		}

		// -- Details.
		//
		inline constexpr key_equal key_eq() const { return key_equal{}; }
		inline constexpr hasher hash_function() const { return hasher{}; }

		// Resets the state.
		//
		inline void clear() { values.clear(); }

		// Redirect comparison to the container.
		//
		inline bool operator==( const flat_map& o ) const { return values == o.values; }
		inline bool operator!=( const flat_map& o ) const { return values != o.values; }
		inline bool operator<( const flat_map& o ) const { return values < o.values; }
	};
	
	template<typename K, typename V, typename Hs = std::conditional_t<String<K>, hasher<string_view_t<K>>, hasher<>>, typename Eq = std::equal_to<K>>
	using iflat_map = flat_map<K, V, Hs, Eq>;
};

namespace std
{
	template <typename Pr, typename K, typename V, typename Hs, typename Eq, bool Il>
	inline auto erase_if( xstd::flat_map<K, V, Hs, Eq, Il>& container, Pr&& predicate )
	{
		return std::erase_if( container.values, [ & ] ( auto&& pair ) { return predicate( pair.view() ); } );
	}
};