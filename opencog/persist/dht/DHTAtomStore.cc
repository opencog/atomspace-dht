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
		add_atom_to_atomspace(h);
		return;
	}

	// Resursive store; add leaves first.
	for (const Handle& ho: h->getOutgoingSet())
		store_recursive(ho);

	// Only after adding leaves, add the atom.
	add_atom_to_atomspace(h);

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

dht::InfoHash DHTAtomStorage::get_guid(const Handle& h)
{
	std::lock_guard<std::mutex> lck(_guid_mutex);
	const auto& ip = _guid_map.find(h);
	if (_guid_map.end() != ip)
		return ip->second;
	std::string astr = _atomspace_name + encodeAtomToStr(h);
	dht::InfoHash akey = dht::InfoHash::get(astr);
	_guid_map[h] = akey;
	return akey;
}

/* ============================= END OF FILE ================= */
