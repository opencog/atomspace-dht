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

#include <opendht/log.h>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atomspaceutils/TLB.h>

#include "DHTAtomStorage.h"

using namespace opencog;

std::atomic<size_t> DHTAtomStorage::_immutable_stores = 0;
std::atomic<size_t> DHTAtomStorage::_immutable_edits = 0;
std::atomic<size_t> DHTAtomStorage::_space_stores = 0;
std::atomic<size_t> DHTAtomStorage::_space_edits = 0;
std::atomic<size_t> DHTAtomStorage::_value_stores = 0;
std::atomic<size_t> DHTAtomStorage::_value_edits = 0;
std::atomic<size_t> DHTAtomStorage::_incoming_stores = 0;
std::atomic<size_t> DHTAtomStorage::_incoming_edits = 0;

/* ================================================================ */
// Constructors

void DHTAtomStorage::init(const char * uri)
{
	_observing_only = true;
	_uri = uri;

#define URIX_LEN (sizeof("dht://") - 1)  // Should be 6
	if (strncmp(uri, "dht://", URIX_LEN))
		throw IOException(TRACE_INFO, "Unknown URI '%s'\n", uri);

	// We expect the URI to be for the form
	//    dht:///atomspace-name
	//    dht://:port/atomspace-name

#define DEFAULT_ATOMSPACE_PORT 4343
	_port = DEFAULT_ATOMSPACE_PORT;
	if ('/' == uri[URIX_LEN])
	{
		_atomspace_name = &uri[URIX_LEN+1];
	}
	else
	if (':' == uri[URIX_LEN])
	{
		const char * start = &uri[URIX_LEN+1];
		_port = atoi(start);
		const char * p = strchr(start, '/');
		if (nullptr == p)
			throw IOException(TRACE_INFO, "Bad URI '%s'\n", uri);
		_atomspace_name = p+1;
	}
	else
		throw IOException(TRACE_INFO, "Bad URI '%s'\n", uri);

	// Strip out and replace trailing slash.
	size_t pos = _atomspace_name.find('/');
	if (pos != std::string::npos) _atomspace_name.resize(pos);
	_atomspace_name += '/';

	if (1 < _atomspace_name.size())
	{
		_atomspace_hash = dht::InfoHash::get(_atomspace_name);
		_observing_only = false;
	}

	clear_stats();

	// --------------------------------------------------------------
	// Network configuration
	// How long to wait for an answer
	_wait_time = std::chrono::milliseconds(4000);

	// Policies for storing atoms

	// For now, hardcode to one week. In fact, atoms should probably be
	// permanent, and the rest should be configureable on a per-atomspace
	// basis. Which means that we may need to adjust these dynamically...
#define DATA_LIFETIME 24*7
	std::chrono::hours lifetime(DATA_LIFETIME);

	_atom_policy = dht::ValueType(ATOM_ID, "atom policy",
		lifetime, cy_store_atom, cy_edit_atom);

	_space_policy = dht::ValueType(SPACE_ID, "space policy",
		lifetime, cy_store_space, cy_edit_space);

	_values_policy = dht::ValueType(VALUES_ID, "values policy",
		lifetime, cy_store_values, cy_edit_values);

	_incoming_policy = dht::ValueType(INCOMING_ID, "incoming policy",
		lifetime, cy_store_incoming, cy_edit_incoming);

	// Use filters, because the same membership hash gets used
	// for both values and for incoming sets.
	_values_filter = dht::Value::TypeFilter(_values_policy);
	_incoming_filter = dht::Value::TypeFilter(_incoming_policy);

	// Run a private NetID only for AtomSpace data!
	_config.dht_config.node_config.network = 42;
	_config.threaded = true;

	// Disable rate limiting, at least for localhost...
	// XXX FIXME, this is problematic for DDOS reasons...
	_config.dht_config.node_config.max_req_per_sec = -1;
	_config.dht_config.node_config.max_peer_req_per_sec = -1;

	// Calling dht::crypto::generateIdentity() results in an insane
	// crash in the crypto library libnettle. See
	// https://debbugs.gnu.org/cgi/bugreport.cgi?bug=38041
	// for details. It appears that gnutls is not thread-safe,
	// or something. The bug is crazy.
	//_config.dht_config.id = dht::crypto::generateIdentity();

	// Launch a dht node on a new thread, using a generated
	// RSA key pair, and listen on port 4343. If the defalut port
	// is in use, try a larger one. This can happen if more
	// than one user on the machine is accessing the DHT; also
	// the unit tests trigger this.
	if (DEFAULT_ATOMSPACE_PORT == _port)
	{
		for (int i=0; i<10; i++)
		{
			try
			{
#define DEBUG 1
#ifdef DEBUG
				// Log to stdout.
				dht::DhtRunner::Context ctxt;
				// ctxt.logger = dht::log::getStdLogger();
				ctxt.logger = dht::log::getFileLogger("atomspace-dhtnode.log");
				_runner.run(_port, _config, std::move(ctxt));
#else
				_runner.run(_port, _config);
#endif
			}
			catch (const dht::DhtException& ex)
			{
				_port++;
				continue;
			}
			break;
		}

		if (not _runner.isRunning())
			throw IOException(TRACE_INFO,
				"Unable to start DHT node, all ports are in use");
	}
	else
		_runner.run(_port, _config);

	// XXX for now, dump to a logfile. Disable this later.
	dht::log::enableFileLogging(_runner, "atomspace-dht.log");

	// Register the policies. These segfault, if done before the
	// _runner.run() call above.
	_runner.registerType(_atom_policy);
	_runner.registerType(_space_policy);
	_runner.registerType(_values_policy);
	_runner.registerType(_incoming_policy);

	// Do NOT fiddle with atomspace contents, if nothing is open!
	if (not _observing_only)
	{
		tvpred = createNode(PREDICATE_NODE, "*-TruthValueKey-*");
		store_recursive(tvpred);
	}
}

void DHTAtomStorage::dht_bootstrap(const std::string& uri)
{
#define URIX_LEN (sizeof("dht://") - 1)  // Should be 6

	if (uri.compare(0, URIX_LEN, "dht://"))
		throw IOException(TRACE_INFO, "Unknown URI '%s'\n", uri);

	// We expect the URI to be for the form
	//    dht://hostname/
	//    dht://hostname:port/

	int port = DEFAULT_ATOMSPACE_PORT;
	std::string hostname = uri.substr(URIX_LEN);
	size_t pos = hostname.find('/');
	if (std::string::npos != pos)
		hostname.resize(pos);

	// Search for a port, if present.
	pos = hostname.find(':');
	if (std::string::npos != pos)
	{
		port = atoi(hostname.substr(pos+1).c_str());
		hostname.resize(pos);
	}

	_runner.bootstrap(hostname, std::to_string(port));
}

DHTAtomStorage::DHTAtomStorage(std::string uri)
{
	init(uri.c_str());
}

DHTAtomStorage::~DHTAtomStorage()
{
	// Calling the loop manually causes pending message queues to drain.
	// Once for the high-priority queue, and once for the regular queue,
	_runner.loop();
	_runner.loop();

	// The condition variable attempts to halt progress
	// until the shutdown callback is called...
	std::mutex mtx;
	std::condition_variable cv;
	bool ready = false; // Handle spurious wakeups.
	bool done = false;  // Handle spurious wakeups.

	// dhtRunner::shutdown queues the callback in a different thread.
	_runner.shutdown([&cv, &mtx, &ready, &done](void)
	{
		std::unique_lock<std::mutex> lck(mtx);
		cv.wait(lck, [&ready]{ return ready; });
		done = true;
		lck.unlock();
		cv.notify_one();
	});

	std::unique_lock<std::mutex> lck(mtx);
	ready = true;
	lck.unlock();
	cv.notify_one();

	// Use predicate to avoid spurious wakeups.
	lck.lock();
	cv.wait(lck, [&done]{ return done; });

	// Wait for dht threads to end.
	_runner.join();
}

/**
 * connected -- Is the DHT node running?
 * XXX FIXME ... we can have a situation where we are
 * connected to the DHT pool, and so the runner is
 * running, but no database is open. This needs a new
 * AtomSpace API for this fix.
 */
bool DHTAtomStorage::connected(void)
{
	return _runner.isRunning();
}

/// Return the time right now, as double-precision.
double DHTAtomStorage::now(void)
{
	using flt = std::chrono::duration<double, std::ratio<1,1>>;
	auto stamp = std::chrono::system_clock::now();
	return std::chrono::duration_cast<flt>(stamp.time_since_epoch()).count();
}

/* ================================================================== */
/**
 * Get the node status.
 */
std::string DHTAtomStorage::dht_node_info(void)
{
	int net = _config.dht_config.node_config.network;
	std::stringstream ss;
	dht::NodeInfo ni = _runner.getNodeInfo();
	ss << "OpenDHT node " << ni.node_id.toString() << std::endl;
	ss << "Belongs to network " << std::to_string(net)
		<< ((0==net) ? " (public)" : " (private)")
	   << " on port " << std::to_string(_port) << std::endl;
	ss << "PKI fingerprint: " << ni.id.toString() << std::endl;
	ss << "IPv4 stats:\n" << ni.ipv4.toString() << std::endl;
	ss << "IPv6 stats:\n" << ni.ipv6.toString() << std::endl;
	return ss.str();
}

/**
 * Get the storage log.
 */
std::string DHTAtomStorage::dht_storage_log(void)
{
	return _runner.getStorageLog();
}

/**
 * Get the routine tables log.
 */
std::string DHTAtomStorage::dht_routing_tables_log(void)
{
	// XXX FIXME also support AF_INET6
	return _runner.getRoutingTablesLog(AF_INET);
}

/**
 * Get the searches log.
 */
std::string DHTAtomStorage::dht_searches_log(void)
{
	return _runner.getSearchesLog();
}

/* ================================================================== */

std::vector<std::shared_ptr<dht::Value>>
DHTAtomStorage::get_stuff(const dht::InfoHash& ihash,
                          const dht::Value::Filter& filter)
{
	auto ifut = _runner.get(ihash, filter);
	std::future_status status = ifut.wait_for(_wait_time);
	if (std::future_status::ready != status)
		throw IOException(TRACE_INFO, "DHT is not responding!");

	return ifut.get();
}

/* ================================================================== */

/**
 * Get the values attached to a key, decode and pretty-print them.
 */
std::string DHTAtomStorage::dht_examine(const std::string& hash)
{
	std::stringstream ss;

	clock_t begin = clock();
	dht::InfoHash ihash(hash);
	auto ivals = get_stuff(ihash);

	clock_t end = clock();
	double elapsed = double(end-begin)/CLOCKS_PER_SEC;
	ss << "Elapsed time: " << std::to_string(elapsed) << " seconds" << std::endl;

	ss << "Found " << std::to_string(ivals.size())
	   << " values" << std::endl;
	for (const auto& ival : ivals)
		ss << prt_dht_value(ival);

	return ss.str();
}

std::string DHTAtomStorage::prt_dht_value(
               const std::shared_ptr<dht::Value>& ival)
{
	std::stringstream ss;
	switch (ival->type)
	{
		case ATOM_ID:
			ss << "Atom seq=" << std::to_string(ival->seq) << " "
			   << ival->unpack<std::string>() << std::endl;
			break;
		case SPACE_ID:
			ss << "Member id=" << std::hex << ival->id << " "
			   << ival->unpack<std::string>() << std::endl;
			break;
		case VALUES_ID:
			ss << "Value: "
			   << ival->unpack<std::string>() << std::endl;
			break;
		case INCOMING_ID:
			ss << "Incoming: "
			   << ival->unpack<dht::InfoHash>().toString() << std::endl;
			break;
		default:
			ss << "Raw: " << ival->toString() << std::endl;
			break;
	}
	return ss.str();
}

std::string DHTAtomStorage::dht_atomspace_hash(void)
{
	if (_atomspace_name.size() <= 1)
		throw IOException(TRACE_INFO, "AtomSpace DHT has not been opened.\n");
	return _atomspace_hash.toString();
}

std::string DHTAtomStorage::dht_immutable_hash(const Handle& atom)
{
	return get_guid(atom).toString();
}

std::string DHTAtomStorage::dht_atom_hash(const Handle& atom)
{
	if (_atomspace_name.size() <= 1)
		throw IOException(TRACE_INFO, "AtomSpace DHT has not been opened.\n");
	return get_membership(atom).toString();
}

/* ================================================================== */
/// Drain the pending store queue. This is a fencing operation; the
/// goal is to make sure that all writes that occurred before the
/// barrier really are performed before before all the writes after
/// the barrier.
///
void DHTAtomStorage::barrier()
{
	// Calling this twice seems to cause all queues to be drained:
	// The first time, its the high-priority queue, and the second
	// time, the regular queue.
	_runner.loop();
	_runner.loop();
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
 * This will delete the contents of the atomspace in the DHT.
 *
 * This routine is meant to be used only for running test cases.
 * It is extremely dangerous, as it can lead to total data loss.
 */
void DHTAtomStorage::kill_data(void)
{
	// Special case for TruthValues - must always have this atom.
	storeAtom(tvpred);

	// XXX not done!
}

/* ================================================================ */

void DHTAtomStorage::clear_stats(void)
{
	_stats_time = time(0);
	_load_count = 0;
	_store_count = 0;
	_value_stores = 0;

	_num_get_atoms = 0;
	_num_got_nodes = 0;
	_num_got_links = 0;
	_num_get_insets = 0;
	_num_get_inlinks = 0;
	_num_node_inserts = 0;
	_num_link_inserts = 0;
	_num_atom_deletes = 0;
	_value_updates = 0;
	_value_deletes = 0;
	_value_fetches = 0;

	_immutable_stores = 0;
	_immutable_edits = 0;
	_space_stores = 0;
	_space_edits = 0;
	_value_stores = 0;
	_value_edits = 0;
	_incoming_stores = 0;
	_incoming_edits = 0;
}

void DHTAtomStorage::print_stats(void)
{
	printf("dht-stats: Currently open URI: %s\n", _uri.c_str());
	printf("dht-stats: AtomSpace hash: %s\n", _atomspace_hash.to_c_str());
	time_t now = time(0);
	// ctime returns string with newline at end of it.
	printf("dht-stats: Time since stats reset=%lu secs, at %s",
		now - _stats_time, ctime(&_stats_time));

	size_t load_count = _load_count;
	size_t store_count = _store_count;
	double frac = store_count / ((double) load_count);
	printf("\n");
	printf("dht-stats: total loads = %zu total stores = %zu ratio=%f\n",
	       load_count, store_count, frac);

	size_t value_updates = _value_updates;
	size_t value_deletes = _value_deletes;
	size_t value_fetches = _value_fetches;
	printf("dht-stats: value updates = %zu deletes = %zu fetches = %zu\n",
	       value_updates, value_deletes, value_fetches);

	size_t num_atom_deletes = _num_atom_deletes;
	printf("dht-stats: total atom deletes = %zu\n",
	       num_atom_deletes);
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

	size_t immutable_stores = _immutable_stores;
	size_t immutable_edits = _immutable_edits;
	size_t space_stores = _space_stores;
	size_t space_edits = _space_edits;
	size_t value_stores = _value_stores;
	size_t value_edits = _value_edits;
	size_t incoming_stores = _incoming_stores;
	size_t incoming_edits = _incoming_edits;

	printf("\n");
	printf("dht immutable stores = %zu edits = %zu\n", immutable_stores, immutable_edits);
	printf("dht space stores     = %zu edits = %zu\n", space_stores, space_edits);
	printf("dht value stores     = %zu edits = %zu\n", value_stores, value_edits);
	printf("dht incoming stores  = %zu edits = %zu\n", incoming_stores, incoming_edits);

	printf("\n");
}

/* ============================= END OF FILE ================= */
