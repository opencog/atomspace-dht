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
Handle DHTAtomStorage::decodeStrAtom(const std::string& satom)
{
	std::string scm = satom;
	if ('(' != scm[0])
		throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n", scm.c_str());
	size_t pos = scm.find(' ');
	if (pos == std::string::npos)
		throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n", scm.c_str());
	scm[pos] = 0;

	Type t = nameserver().getType(&scm[1]);
	if (nameserver().isNode(t))
	{
		size_t name_start = scm.find('"', pos+1);
		if (name_start == std::string::npos)
			throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n", scm.c_str());
		size_t name_end = scm.find('"', name_start+1);
		if (name_end == std::string::npos)
			throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n", scm.c_str());
		scm[name_end] = 0;
		_num_got_nodes ++;
		return createNode(t, &scm[name_start+1]);
	}

	if (not nameserver().isLink(t))
		throw RuntimeException(TRACE_INFO, "Bad Atom string! %s\n", scm.c_str());

	// If we are here, its a Link.
	HandleSeq oset;
	size_t oset_start = scm.find('(', pos+1);
	while (oset_start != std::string::npos)
	{
		oset.push_back(decodeStrAtom(&scm[oset_start]));

		// Find the next balanced paren, and restart there.
		// This is not very efficient, but it works.
		size_t pos = oset_start;
		int pcnt = 1;
		while (0 < pcnt and pos != std::string::npos)
		{
			char c = scm[++pos];
			if ('(' == c) pcnt ++;
			else if (')' == c) pcnt--;
		}
		if (pos == std::string::npos) break;
		oset_start = scm.find('(', pos+1);
	}

	_num_got_links ++;
	return createLink(oset, t);
}

/* ================================================================ */

Handle DHTAtomStorage::fetch_atom(Handle &h)
{
	dht::InfoHash ahash = get_guid(h);

	// Get a future for the atom
	auto afut = _runner.get(ahash);

	// Block until we've got it.
	std::cout << "Start waiting" << std::endl;
	afut.wait();
	std::cout << "Done waiting" << std::endl;

	auto dvals = afut.get();
	for (const auto& dval : dvals)
	{
		// std::cout << "Got value: " << dval->toString() << std::endl;
		std::cout << "Got svalue: " << dval->unpack<std::string>() << std::endl;
	}

	// get_atom_values(h, dag);
	return h;
}

Handle DHTAtomStorage::getNode(Type t, const char * str)
{
	Handle h(createNode(t, str));
	return fetch_atom(h);
}

Handle DHTAtomStorage::getLink(Type t, const HandleSeq& hs)
{
	Handle h(createLink(hs, t));
	return fetch_atom(h);
}

/* ============================= END OF FILE ================= */
