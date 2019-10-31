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
 * Recursively store the indicated atom and all of the values attached
 * to it.  Also store it's outgoing set, and all of the values attached
 * to those atoms.  The recursive store is unconditional.
 *
 * By default, the actual store is done asynchronously (in a different
 * thread) by the DHT library; thus this method will typically return
 * before the store has completed.
 */
void DHTAtomStorage::storeAtom(const Handle& h, bool synchronous)
{
	dht::InfoHash guid = get_guid(h);
std::cout << "GUID is " << guid << std::endl;

	// do_store_atom(h);
	store_atom_values(h);
}

void DHTAtomStorage::do_store_atom(const Handle& h)
{
	if (h->is_node())
	{
		do_store_single_atom(h);
		return;
	}

	// Recurse.
	for (const Handle& ho: h->getOutgoingSet())
		do_store_atom(ho);

	do_store_single_atom(h);

	// Make note of the incoming set.
	for (const Handle& ho: h->getOutgoingSet())
		store_incoming_of(ho,h);
}

dht::InfoHash DHTAtomStorage::get_guid(const Handle& h)
{
	std::lock_guard<std::mutex> lck(_guid_mutex);
	const auto& ip = _guid_map.find(h);
	if (_guid_map.end() != ip)
		return ip->second;
	std::string astr = _atomspace_name + encodeAtomToStr(h);
std::cout << " nameo=" << astr << std::endl;
	dht::InfoHash akey = dht::InfoHash::get(astr);
	_guid_map[h] = akey;
	return akey;
}

/* ================================================================ */

/**
 * Store just this one single atom.
 * Atoms in the outgoing set are NOT stored!
 * Values attached to the Atom are not stored!
 * The store is performed synchronously (in the calling thread).
 */
void DHTAtomStorage::do_store_single_atom(const Handle& h)
{
#if 0
	{
		std::lock_guard<std::mutex> lck(_guid_mutex);
		_guid_map[h] = guid;
	}
	{
		std::lock_guard<std::mutex> lck(_inv_mutex);
		_guid_inv_map[guid] = h;
	}

	// OK, the atom itself is in DHT; add it to the atomspace, too.
	update_atom_in_atomspace(h, guid);

	// Cache the json, but only if we don't already have a version
	// of it. I guess that there is a very slight chance that some
	// other thread is racing, and maybe its twiddling the json
	// also. We don't want to clobber that with this earlier version.
	{
		std::lock_guard<std::mutex> lck(_json_mutex);
		if (_json_map.end() == _json_map.find(h))
			_json_map[h] = jatom;
	}
#endif

	// std::cout << "addAtom: " << name << " id: " << id << std::endl;

	_store_count ++;

	if (bulk_store and _store_count%100 == 0)
	{
		time_t secs = time(0) - bulk_start;
		double rate = ((double) _store_count) / secs;
		unsigned long kays = ((unsigned long) _store_count) / 1000;
		printf("\tStored %luK atoms in %d seconds (%d per second)\n",
			kays, (int) secs, (int) rate);
	}
}

/* ============================= END OF FILE ================= */
