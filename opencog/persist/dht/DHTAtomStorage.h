/*
 * FILE:
 * opencog/persist/dht/DHTAtomStorage.h

 * FUNCTION:
 * OpenDHT-backed persistent storage.
 *
 * HISTORY:
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linasvepstas@gmail.com>
 *
 * LICENSE:
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_DHT_ATOM_STORAGE_H
#define _OPENCOG_DHT_ATOM_STORAGE_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <set>
#include <vector>

#include <opendht.h>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atoms/atom_types/types.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/atoms/value/LinkValue.h>
#include <opencog/atoms/value/StringValue.h>
#include <opencog/atoms/base/Valuation.h>

#include <opencog/atomspace/AtomTable.h>
#include <opencog/atomspace/BackingStore.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

class DHTAtomStorage : public BackingStore
{
	private:
		void init(const char *);
		std::string _uri;
		int _port;
		bool _observing_only;
		std::string _atomspace_name;

		Handle tvpred; // the key to a very special valuation.

		dht::DhtRunner::Config _config;
		dht::DhtRunner _runner;
		dht::InfoHash _atomspace_hash;

		// --------------------------
		// Storage policies
		static dht::ValueType _atom_policy;
		static dht::ValueType _space_policy;
		static dht::ValueType _values_policy;
		static dht::ValueType _incoming_policy;
		enum
		{
			ATOM_ID = 4097,
			SPACE_ID = 4098,
			VALUES_ID = 4099,
			INCOMING_ID = 4100,
		};
		static bool cy_store_atom(dht::InfoHash key,
		                std::shared_ptr<dht::Value>& value,
		                const dht::InfoHash& from,
		                const dht::SockAddr& addr);

		static bool cy_edit_atom(dht::InfoHash key,
		               const std::shared_ptr<dht::Value>& old_val,
		               std::shared_ptr<dht::Value>& new_val,
		               const dht::InfoHash& from,
		               const dht::SockAddr& addr);

		static bool cy_store_space(dht::InfoHash key,
		                std::shared_ptr<dht::Value>& value,
		                const dht::InfoHash& from,
		                const dht::SockAddr& addr);

		static bool cy_edit_space(dht::InfoHash key,
		               const std::shared_ptr<dht::Value>& old_val,
		               std::shared_ptr<dht::Value>& new_val,
		               const dht::InfoHash& from,
		               const dht::SockAddr& addr);

		static bool cy_store_values(dht::InfoHash key,
		                std::shared_ptr<dht::Value>& value,
		                const dht::InfoHash& from,
		                const dht::SockAddr& addr);

		static bool cy_edit_values(dht::InfoHash key,
		               const std::shared_ptr<dht::Value>& old_val,
		               std::shared_ptr<dht::Value>& new_val,
		               const dht::InfoHash& from,
		               const dht::SockAddr& addr);

		static bool cy_store_incoming(dht::InfoHash key,
		                std::shared_ptr<dht::Value>& value,
		                const dht::InfoHash& from,
		                const dht::SockAddr& addr);

		static bool cy_edit_incoming(dht::InfoHash key,
		               const std::shared_ptr<dht::Value>& old_val,
		               std::shared_ptr<dht::Value>& new_val,
		               const dht::InfoHash& from,
		               const dht::SockAddr& addr);

		static std::string prt_dht_value(const std::shared_ptr<dht::Value>&);
		double now(void);
		// --------------------------
		// Fetch and storing of atoms

		static std::string encodeValueToStr(const ValuePtr&);
		static std::string encodeValuesToAlist(const Handle&);
		static std::string encodeAtomToStr(const Handle& h) {
			return h->to_short_string(); }
		Handle decodeStrAtom(std::string& s) {
			size_t junk = 0;
			return decodeStrAtom(s, junk);
		}
		Handle decodeStrAtom(std::string&, size_t&);

		std::mutex _guid_mutex;
		std::unordered_map<Handle, dht::InfoHash> _guid_map;
		dht::InfoHash get_guid(const Handle&);

		Handle fetch_atom(const dht::InfoHash&);
		std::mutex _decode_mutex;
		std::map<dht::InfoHash, Handle> _decode_map;

		std::mutex _membership_mutex;
		std::unordered_map<Handle, dht::InfoHash> _membership_map;
		dht::InfoHash get_membership(const Handle&);

		std::mutex _publish_mutex;
		std::unordered_set<Handle> _published;
		void publish_to_atomspace(const Handle&);
		void store_recursive(const Handle&);

		// --------------------------
		// Bulk load and store
		bool bulk_load;
		bool bulk_store;
		time_t bulk_start;

		// --------------------------
		// Values
		void store_atom_values(const Handle &);
		Handle fetch_values(Handle&&);
		void delete_atom_values(const Handle&);

		ValuePtr decodeStrValue(std::string&, size_t&);
		void decodeAlist(Handle&, std::string&);

		// --------------------------
		// Performance statistics
		std::atomic<size_t> _num_get_atoms;
		std::atomic<size_t> _num_got_nodes;
		std::atomic<size_t> _num_got_links;
		std::atomic<size_t> _num_get_insets;
		std::atomic<size_t> _num_get_inlinks;
		std::atomic<size_t> _num_node_inserts;
		std::atomic<size_t> _num_link_inserts;
		std::atomic<size_t> _num_atom_deletes;
		std::atomic<size_t> _load_count;
		std::atomic<size_t> _store_count;
		std::atomic<size_t> _value_stores;
		time_t _stats_time;

	public:
		DHTAtomStorage(std::string uri);
		DHTAtomStorage(const DHTAtomStorage&) = delete; // disable copying
		DHTAtomStorage& operator=(const DHTAtomStorage&) = delete; // disable assignment
		virtual ~DHTAtomStorage();
		bool connected(void); // connection to DB is alive

		void dht_bootstrap(const std::string& uri);
		std::string dht_atomspace_hash(void);
		std::string dht_immutable_hash(const Handle&);
		std::string dht_atom_hash(const Handle&);
		std::string dht_examine(const std::string&);
		std::string dht_node_info(void);
		std::string dht_storage_log(void);
		std::string dht_routing_tables_log(void);
		std::string dht_searches_log(void);

		void load_atomspace(AtomSpace*, const std::string&);

		void kill_data(void); // destroy DB contents

		void registerWith(AtomSpace*);
		void unregisterWith(AtomSpace*);
		void extract_callback(const AtomPtr&);
		int _extract_sig;

		// AtomStorage interface
		Handle getNode(Type, const char *);
		Handle getLink(Type, const HandleSeq&);
		void getIncomingSet(AtomTable&, const Handle&);
		void getIncomingByType(AtomTable&, const Handle&, Type t);
		void storeAtom(const Handle&, bool synchronous = false);
		void removeAtom(const Handle&, bool recursive);
		void loadType(AtomTable&, Type);
		void loadAtomSpace(AtomTable&); // Load entire contents
		void storeAtomSpace(const AtomTable&); // Store entire contents
		void barrier();

		// Debugging and performance monitoring
		void print_stats(void);
		void clear_stats(void); // reset stats counters.
};


/** @}*/
} // namespace opencog

#endif // _OPENCOG_DHT_ATOM_STORAGE_H
