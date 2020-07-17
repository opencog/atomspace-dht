/*
 * DHTAtomLoad.cc
 * Restore of individual atoms.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <stdlib.h>
#include <unistd.h>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/atom_types/NameServer.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/sexpr/Sexpr.h>

#include "DHTAtomStorage.h"

using namespace opencog;


/* ================================================================ */

/**
 * Given a guid, obtain and return the corresponding Atom for it.
 * This does NOT fetch the values on this Atom!
 */
Handle DHTAtomStorage::fetch_atom(const dht::InfoHash& guid)
{
	// Try to find what atom this is from our local cache.
	// XXX Investigate.  This map presumes that it is somehow
	// faster to cache locally and look up locally.  But really,
	// doesn't the DHT library maintain this map too? Wouldn't
	// it be more space-efficient to never use this map, and to
	// always got straight to the DHT library? How slow would
	// that be? How much of a diffrence does it make?
	std::unique_lock<std::mutex> lck(_decode_mutex);
	const auto& da = _decode_map.find(guid);
	if (_decode_map.end() != da) return da->second;
	lck.unlock();

	// Not found. Ask the DHT for it. Get a future for the atom,
	// and wait on it. We cannot do an async wait here; we MUST
	// get something back, before we return to the caller.
	auto gvals = get_stuff(guid);

	// Yikes! Fatal error! We're asked to process a GUID and we
	// have no clue what Atom it corresponds to!
	if (0 == gvals.size())
		throw RuntimeException(TRACE_INFO, "Can't find Atom!");

	// There may be more than one value, but they should all be
	// one and the same.
	std::string satom = gvals[0]->unpack<std::string>();
	Handle h(Sexpr::decode_atom(satom));

	lck.lock();
	_decode_map.emplace(std::make_pair(guid, h));
	return h;
}

/* ================================================================ */

Handle DHTAtomStorage::getNode(Type t, const char * str)
{
	return fetch_values(createNode(t, str));
}

Handle DHTAtomStorage::getLink(Type t, const HandleSeq& hs)
{
	return fetch_values(createLink(hs, t));
}

/* ============================= END OF FILE ================= */
