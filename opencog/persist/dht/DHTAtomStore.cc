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
	if (h->is_node()) return;

	// Make note of the incoming set.
	for (const Handle& ho: h->getOutgoingSet())
		store_incoming_of(ho,h);

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
