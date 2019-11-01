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

#include "DHTAtomStorage.h"

using namespace opencog;


/* ================================================================ */

/// Convert a scheme expression into a C++ Atom.
/// For example: `(Concept "foobar")`  or
/// `(Evaluation (Predicate "blort") (List (Concept "foo") (Concept "bar")))`
/// will return the corresponding atoms.
///
Handle DHTAtomStorage::decodeStrAtom(std::string& scm, size_t& pos)
{
	size_t vos = scm.find('(', pos);
	if (std::string::npos == vos)
		throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n",
			scm.substr(pos).c_str());

	size_t post = scm.find(' ', vos+1);
	if (post != std::string::npos)
		scm[post] = 0;
	else
	{
		post = scm.find(')', vos+1);
		if (post == std::string::npos)
			throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n",
				scm.substr(pos).c_str());
		scm[post] = 0;
	}

	Type t = nameserver().getType(&scm[vos+1]);
	if (nameserver().isNode(t))
	{
		size_t name_start = scm.find('"', post+1);
		if (name_start == std::string::npos)
			throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n",
				scm.substr(pos).c_str());
		size_t name_end = scm.find('"', name_start+1);
		if (name_end == std::string::npos)
			throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n",
				scm.substr(pos).c_str());
		scm[name_end] = 0; // Clobber the ending quote
		size_t close = scm.find(')', name_end + 1);
		if (close == std::string::npos)
			throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n",
				scm.substr(pos).c_str());
		pos = close + 1;
		_num_got_nodes ++;
		return createNode(t, &scm[name_start+1]);
	}

	if (not nameserver().isLink(t))
		throw SyntaxException(TRACE_INFO, "Bad Atom string! %s\n",
			scm.substr(pos).c_str());

	// If we are here, its a Link.
	HandleSeq oset;
	size_t oset_start = scm.find('(', post+1);
	size_t oset_end = scm.find(')', post+1);
	if (std::string::npos == oset_end)
		throw SyntaxException(TRACE_INFO, "Bad Atom string! %s\n",
			scm.substr(pos).c_str());
	while (oset_start != std::string::npos and oset_start < oset_end)
	{
		size_t vos = oset_start;
		oset.push_back(decodeStrAtom(scm, vos));
		oset_start = scm.find('(', vos);
		oset_end = scm.find(')', vos);
	}
	if (oset_end == std::string::npos)
		throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n",
			scm.substr(pos).c_str());
	pos = oset_end + 1;

	_num_got_links ++;
	return createLink(oset, t);
}

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
	auto gfut = _runner.get(guid);
	gfut.wait();

	// Yikes! Fatal error! We're asked to process a GUID and we
	// have no clue what Atom it corresponds to!
	auto gvals = gfut.get();
	if (0 == gvals.size())
		throw RuntimeException(TRACE_INFO, "Can't find Atom!");

	// There may be more than one value, but they should all be
	// one and the same.
	std::string satom = gvals[0]->unpack<std::string>();
	Handle h(decodeStrAtom(satom));

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
