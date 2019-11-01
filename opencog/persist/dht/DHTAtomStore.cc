/*
 * DHTAtomStore.cc
 * Save of individual atoms.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <stdlib.h>
#include <unistd.h>
#include <algorithm>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atomspace/AtomSpace.h>

#include "DHTAtomStorage.h"

using namespace opencog;

/* ================================================================ */

/**
 * Recursively store the atom, and update the incoming sets,
 * as needed. This attempts to perform these updates in a
 * specific chronological order, although it seems unlikely
 * that the actual DHT publication will preserve this chronology.
 * (The chronology is to publish leaves first, then the things
 * that hold the leaves, then the incoming sets of the leaves,
 * the idea being that many users want to see the leaves, before
 * the links holding them.).
 */
void DHTAtomStorage::store_recursive(const Handle& h)
{
	if (h->is_node())
	{
		publish_to_atomspace(h);
		return;
	}

	// Resursive store; add leaves first.
	for (const Handle& ho: h->getOutgoingSet())
		store_recursive(ho);

	// Only after adding leaves, add the atom.
	publish_to_atomspace(h);

	// Finally, update the incoming sets.
	for (const Handle& ho: h->getOutgoingSet())
		store_incoming_of(ho,h);
}

/**
 * Store the indicated atom and all of the values attached to it.
 * Also store it's incoming set.
 *
 * The actual store is done asynchronously (in a different thread)
 * by the DHT library; thus this method will typically return before
 * the store has completed.
 */
void DHTAtomStorage::storeAtom(const Handle& h, bool synchronous)
{
	store_atom_values(h);
	store_recursive(h);
}

/* ================================================================== */
/**
 * Publish Atom to the AtomSpace
 */
void DHTAtomStorage::publish_to_atomspace(const Handle& atom)
{
	std::lock_guard<std::mutex> lck(_publish_mutex);
	const auto& pa = _published.find(atom);
	if (_published.end() != pa) return;
	_runner.put(_atomspace_hash,
		dht::Value(_space_policy, encodeAtomToStr(atom)));
	_published.emplace(atom);
	_store_count ++;
}

/* ================================================================== */

/**
 * Return the globally-unique hash corresponding to the Atom.
 * XXX TODO this should use all of the hashing and alpha-equivalence
 * rules that ContentHash Atom::compute_hash() const uses.
 * !! Perhaps we should extend class Atom to provide both 20-byte
 * (160 bit) and larger (256 bit) hashes? Yes, we should!
 * That would avoid bugs & shit...
 */
dht::InfoHash DHTAtomStorage::get_guid(const Handle& h)
{
	std::lock_guard<std::mutex> lck(_guid_mutex);
	const auto& ip = _guid_map.find(h);
	if (_guid_map.end() != ip)
		return ip->second;
	std::string gstr = encodeAtomToStr(h);
	dht::InfoHash gkey = dht::InfoHash::get(gstr);
	_guid_map[h] = gkey;
	return gkey;
}

/* ================================================================== */
/**
 * Return the AtomSpace-specific (bus still globally-unique) hash
 * corresponding to the Atom, in this AtomSpace.  This hash is
 * required for looking up values and incoming sets.
 */
dht::InfoHash DHTAtomStorage::get_auid(const Handle& h)
{
	std::lock_guard<std::mutex> lck(_auid_mutex);
	const auto& ip = _auid_map.find(h);
	if (_auid_map.end() != ip)
		return ip->second;
	std::string astr = _atomspace_name + encodeAtomToStr(h);
	dht::InfoHash akey = dht::InfoHash::get(astr);
	_auid_map[h] = akey;
	return akey;
}

/* ============================= END OF FILE ================= */
