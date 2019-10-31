/*
 * DHTAtomStorage.cc
 * Persistent Atom storage, OpenDHT-backed.
 *
 * Atoms and Values are saved to and restored from the OpenDHT system.
 *
 * Copyright (c) 2008,2009,2013,2015,2017 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atomspaceutils/TLB.h>

#include "DHTAtomStorage.h"

using namespace opencog;

// Number of write-back queues
#define NUM_WB_QUEUES 6

/* ================================================================ */
// Constructors

void DHTAtomStorage::init(const char * uri)
{
	tvpred = createNode(PREDICATE_NODE, "*-TruthValueKey-*");

	_uri = uri;

#define URIX_LEN (sizeof("dht://") - 1)  // Should be 6
	if (strncmp(uri, "dht://", URIX_LEN))
		throw IOException(TRACE_INFO, "Unknown URI '%s'\n", uri);

	// We expect the URI to be for the form
	//    dht:///atomspace-name
	//    dht://hostname/atomspace-name
	//    dht://hostname:port/atomspace-name

	_port = 4222;
	if ('/' == uri[URIX_LEN])
	{
		_hostname = "localhost";
		// _keyname = &uri[URIX_LEN+1];
	}
	else
	{
		const char* start = &uri[URIX_LEN];
		_hostname = start;
		char* p = strchr((char *)start, '/');
		if (nullptr == p)
			throw IOException(TRACE_INFO, "Bad URI format '%s'\n", uri);
		size_t len = p - start;
		_hostname.resize(len);

		// Search for a port, if present.
		p = strchr((char *)start, ':');
		if (p)
		{
			_port = atoi(p+1);
			size_t len = p - start;
			_hostname.resize(len);
		}

		_atomspace_name = &uri[len+URIX_LEN+1];
		// Keys are not allowed to have trailing slashes.
		size_t pos = _atomspace_name.find('/');
		if (pos != std::string::npos) _atomspace_name.resize(pos);

		_atomspace_hash = dht::InfoHash::get(_atomspace_name);
	}

	// Launch a dht node on a new thread, using a generated
	// RSA key pair, and listen on port 4224.
	// FIXME, need something better.
	_runner.run(4224, dht::crypto::generateIdentity(), true);

	_runner.bootstrap(_hostname, std::to_string(_port));

	bulk_load = false;
	bulk_store = false;
	clear_stats();
}

DHTAtomStorage::DHTAtomStorage(std::string uri)
{
	init(uri.c_str());
}

DHTAtomStorage::~DHTAtomStorage()
{
	// Wait for dht threads to end
	_runner.join();
}

/**
 * connected -- Is the DHT node running?
 */
bool DHTAtomStorage::connected(void)
{
	return _runner.isRunning();
}

/* ================================================================== */
/// Drain the pending store queue. This is a fencing operation; the
/// goal is to make sure that all writes that occurred before the
/// barrier really are performed before before all the writes after
/// the barrier.
///
void DHTAtomStorage::barrier()
{
	// Not sure... How do we do this?
}

/* ================================================================ */

void DHTAtomStorage::registerWith(AtomSpace* as)
{
	BackingStore::registerWith(as);
}

void DHTAtomStorage::unregisterWith(AtomSpace* as)
{
	BackingStore::unregisterWith(as);
}

/* ================================================================ */

/**
 * kill_data -- Publish an empty atomspace. Dangerous!
 * This will forget the DHT reference to the atomspace containing
 * all of the atoms, resulting in data loss, unless you've done
 * something to keep ahold of that CID.
 *
 * This routine is meant to be used only for running test cases.
 * It is extremely dangerous, as it can lead to total data loss.
 */
void DHTAtomStorage::kill_data(void)
{
	// Special case for TruthValues - must always have this atom.
	do_store_single_atom(tvpred);
}

/* ================================================================ */

void DHTAtomStorage::clear_stats(void)
{
	_stats_time = time(0);
	_load_count = 0;
	_store_count = 0;
	_valuation_stores = 0;
	_value_stores = 0;

	_num_get_atoms = 0;
	_num_got_nodes = 0;
	_num_got_links = 0;
	_num_get_insets = 0;
	_num_get_inlinks = 0;
	_num_node_inserts = 0;
	_num_link_inserts = 0;
	_num_atom_removes = 0;
	_num_atom_deletes = 0;
}

void DHTAtomStorage::print_stats(void)
{
	printf("dht-stats: Currently open URI: %s\n", _uri.c_str());
	time_t now = time(0);
	// ctime returns string with newline at end of it.
	printf("dht-stats: Time since stats reset=%lu secs, at %s",
		now - _stats_time, ctime(&_stats_time));


	size_t load_count = _load_count;
	size_t store_count = _store_count;
	double frac = store_count / ((double) load_count);
	printf("dht-stats: total loads = %zu total stores = %zu ratio=%f\n",
	       load_count, store_count, frac);

	size_t valuation_stores = _valuation_stores;
	size_t value_stores = _value_stores;
	printf("dht-stats: valuation updates = %zu value updates = %zu\n",
	       valuation_stores, value_stores);

	size_t num_atom_removes = _num_atom_removes;
	size_t num_atom_deletes = _num_atom_deletes;
	printf("dht-stats: atom remove requests = %zu total atom deletes = %zu\n",
	       num_atom_removes, num_atom_deletes);
	printf("\n");

	size_t num_get_atoms = _num_get_atoms;
	size_t num_got_nodes = _num_got_nodes;
	size_t num_got_links = _num_got_links;
	size_t num_get_insets = _num_get_insets;
	size_t num_get_inlinks = _num_get_inlinks;
	size_t num_node_inserts = _num_node_inserts;
	size_t num_link_inserts = _num_link_inserts;

	printf("num_get_atoms=%zu num_got_nodes=%zu num_got_links=%zu\n",
	       num_get_atoms, num_got_nodes, num_got_links);

	frac = num_get_inlinks / ((double) num_get_insets);
	printf("num_get_incoming_sets=%zu set total=%zu avg set size=%f\n",
	       num_get_insets, num_get_inlinks, frac);

	unsigned long tot_node = num_node_inserts;
	unsigned long tot_link = num_link_inserts;
	frac = tot_link / ((double) tot_node);
	printf("total stores for node=%lu link=%lu ratio=%f\n",
	       tot_node, tot_link, frac);

	printf("\n");
}

// XXX Stubs FIXME
Handle DHTAtomStorage::fetch_atom(const std::string&) { return Handle(); }
void DHTAtomStorage::load_atomspace(AtomSpace*, const std::string&) {}
Handle DHTAtomStorage::getNode(Type, const char *) { return Handle();}
Handle DHTAtomStorage::getLink(Type, const HandleSeq&) { return Handle();  }
void DHTAtomStorage::getIncomingSet(AtomTable&, const Handle&) {}
void DHTAtomStorage::getIncomingByType(AtomTable&, const Handle&, Type t) {}
void DHTAtomStorage::getValuations(AtomTable&, const Handle&, bool get_all) {}
void DHTAtomStorage::removeAtom(const Handle&, bool recursive) {}
void DHTAtomStorage::loadType(AtomTable&, Type) {}
void DHTAtomStorage::loadAtomSpace(AtomTable&) {} // Load entire contents
void DHTAtomStorage::storeAtomSpace(const AtomTable&) {} // Store entire contents
/* ============================= END OF FILE ================= */
