#pragma once
#include "type_helpers.hpp"
#include "hashable.hpp"
#include "text.hpp"
#include "formatting.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>

namespace xstd
{
	template<typename T, bool Directed>
	struct base_graph
	{
		// Basic traits.
		//
		static constexpr bool directed = Directed;
		using value_type = T;

		// Entry types.
		//
		struct node_entry
		{
			// Value mapped to.
			//
			value_type value;

			// DOT attribute map.
			//
			std::unordered_map<std::string_view, std::string, ihash<std::string_view>> attributes;
			auto& attribute( const std::string_view& o, const std::string& v ) { attributes.insert_or_assign( o, v ); return *this; }
		};
		struct edge_entry
		{
			// Source and destination nodes, interchangable if not directed.
			//
			const node_entry* src;
			const node_entry* dst;

			// DOT attribute map.
			//
			mutable std::unordered_map<std::string_view, std::string, ihash<std::string_view>> attributes;
			auto& attribute( const std::string_view& o, const std::string& v ) const { attributes.insert_or_assign( o, v ); return *this; }

			// Hashing and comparison, property map is discarded.
			//
			hash_t hash() const
			{
				if constexpr ( directed )
					return make_hash( src, dst );
				else
					return make_unordered_hash( src, dst );
			}
			bool operator==( const edge_entry& o ) const
			{
				if ( src == o.src && dst == o.dst )
					return true;
				return !directed && src == o.dst && dst == o.src;
			}
		};

		// Clusters and subgraphs.
		//
		std::vector<std::pair<std::unique_ptr<base_graph>, bool>> anon_subgraphs;
		std::unordered_map<std::string, std::pair<std::unique_ptr<base_graph>, bool>> subgraphs;

		// Mapping of all node entries and their edges.
		//
		std::vector<std::unique_ptr<node_entry>> nodes;
		std::unordered_set<edge_entry, hasher<>> edges;

		// DOT default attribute map.
		//
		std::unordered_map<std::string_view, std::string, ihash<std::string_view>> node_attributes;
		std::unordered_map<std::string_view, std::string, ihash<std::string_view>> edge_attributes;
		std::unordered_map<std::string_view, std::string, ihash<std::string_view>> graph_attributes;

		// Attribute lookup.
		//
		auto& attribute( const std::string_view& o, const std::string& v ) { graph_attributes.insert_or_assign( o, v ); return *this; }
		auto& node_attribute( const std::string_view& o, const std::string& v ) { node_attributes.insert_or_assign( o, v ); return *this; }
		auto& edge_attribute( const std::string_view& o, const std::string& v ) { edge_attributes.insert_or_assign( o, v ); return *this; }

		// Cluster/Subgraph insertion and lookup.
		//
		base_graph& cluster( const std::string& key = {} )
		{
			if ( key.empty() )
				return *anon_subgraphs.emplace_back( std::make_unique<base_graph>(), true ).first;
			auto& entry = subgraphs[ key ]; 
			if ( !entry.first )
				entry = { std::make_unique<base_graph>(), true };
			return *entry.first;
		}
		base_graph& subgraph( const std::string& key = {} ) 
		{
			if ( key.empty() )
				return *anon_subgraphs.emplace_back( std::make_unique<base_graph>(), false ).first;
			auto& entry = subgraphs[ key ];
			if ( !entry.first )
				entry = { std::make_unique<base_graph>(), false };
			return *entry.first;
		}

		// Node/Edge insertion and lookup.
		//
		node_entry& node( const value_type& value )
		{ 
			for ( auto& [k, gr] : subgraphs )
			{
				for ( auto& node : gr.first->nodes )
					if ( node->value == value )
						return *node;
			}

			for ( auto& node : nodes )
				if ( node->value == value )
					return *node;
			return *nodes.emplace_back( std::make_unique<node_entry>( node_entry{ value } ) );
		}
		const edge_entry& edge( const value_type& src, const value_type& dst ) 
		{ 
			return *edges.insert( edge_entry{ &node( src ), &node( dst ) } ).first; 
		}

		// Prints the graph.
		//
		std::string to_string( std::string_view name = "G", size_t depth = 0 ) const
		{
			std::string result( depth * 2, ' ' );
			if ( !depth )
			{
				result += directed ? "digraph " : "graph ";
				result += name;
			}
			else
			{
				result += "subgraph ";
				result += name;
			}
			result += " {\n";
			++depth;

			// Print the attributes.
			//
			bool attr_pad = false;
			for ( auto& [k, v] : graph_attributes )
			{
				result.insert( result.size(), depth * 2, ' ' );
				result += std::string{ k } + "=\"" + v + "\";\n";
				attr_pad = true;
			}
			for ( auto& [map, b] : { std::pair{ &node_attributes, "node[" }, std::pair{ &edge_attributes, "edge[" } } )
			{
				if ( map->empty() )
					continue;

				result.insert( result.size(), depth * 2, ' ' );
				result += b;
				for ( auto& [k, v] : node_attributes )
					result += std::string{ k } + "=\"" + v + "\",";
				result.pop_back();
				result += "];\n";
				attr_pad = true;
			}
			if ( attr_pad ) result += "\n";

			// Print subgraphs.
			//
			size_t cluster_count = 0;
			auto render_subgraph = [ & ] ( auto&& name, auto&& subgraph )
			{
				if ( subgraph.second )
				{
					result.insert( result.size(), depth * 2, ' ' );
					result += fmt::str( "subgraph cluster_%d_%llu {\n", depth, ++cluster_count );
					++depth;
				}
				result += subgraph.first->to_string( name, depth + 1 );
				if ( subgraph.second )
				{
					--depth;
					result.insert( result.size(), depth * 2, ' ' );
					result += "};\n";
				}
			};
			for ( auto& [name, subgraph] : subgraphs )
				render_subgraph( name, subgraph );
			for ( auto& subgraph : anon_subgraphs )
				render_subgraph( std::string_view{}, subgraph );
			if ( !subgraphs.empty() || !anon_subgraphs.empty() ) result += "\n";

			// Print the nodes.
			//
			for ( auto& node : nodes )
			{
				result.insert( result.size(), depth * 2, ' ' );
				result += "\"" + fmt::as_string( node->value ) + "\"";
				if ( !node->attributes.empty() )
				{
					result += " [";
					for ( auto& [k, v] : node->attributes )
						result += std::string{ k } + "=\"" + v + "\",";
					result.pop_back();
					result += "]";
				}
				result += ";\n";
			}

			// Print the edges.
			//
			for ( auto& edge : edges )
			{
				result.insert( result.size(), depth * 2, ' ' );
				result += "\"" + fmt::as_string( edge.src->value );
				result += ( directed ? "\"->\"" : "\"--\"" );
				result += fmt::as_string( edge.dst->value ) + "\"";
				if ( !edge.attributes.empty() )
				{
					result += " [";
					for ( auto& [k, v] : edge.attributes )
						result += std::string{ k } + "=\"" + v + "\",";
					result.pop_back();
					result += "]";
				}
				result += ";\n";
			}

			--depth;
			result.insert( result.size(), depth * 2, ' ' );
			return result += "}\n";
		}
	};

	template<typename T> using digraph = base_graph<T, true>;
	template<typename T> using graph =   base_graph<T, false>;
};