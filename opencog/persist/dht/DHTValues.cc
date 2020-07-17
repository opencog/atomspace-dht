/*
 * DHTValues.cc
 * Save and restore of atom values.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/atoms/value/LinkValue.h>
#include <opencog/atoms/value/StringValue.h>
#include <opencog/atoms/base/Valuation.h>
#include <opencog/atoms/truthvalue/CountTruthValue.h>
#include <opencog/atoms/truthvalue/SimpleTruthValue.h>
#include <opencog/atoms/truthvalue/TruthValue.h>
#include <opencog/persist/sexpr/Sexpr.h>

#include "DHTAtomStorage.h"

using namespace opencog;

/* ================================================================== */

/// Store ALL of the values associated with the atom.
void DHTAtomStorage::store_atom_values(const Handle& atom)
{
	if (_observing_only)
		throw IOException(TRACE_INFO, "DHT Node is only observing!");

	dht::InfoHash muid = get_membership(atom);

	// If there are no keys currently on the atom, but there are values
	// in the DHT, then we need to clobber the values in the DHT.  Try
	// to avoid having to to a put, by doing a get first.  Maybe we can
	// get more efficient by caching?
	if (0 == atom->getKeys().size())
	{
		auto dvals = get_stuff(muid, _values_filter);
		if (0 < dvals.size())
			delete_atom_values(atom);
		return;
	}

	// Make sure all of the keys appear in the AtomSpace
	for (const Handle& key : atom->getKeys())
		store_recursive(key);

	// Attach the value to the atom
	_runner.put(muid,
		dht::Value(_values_policy, Sexpr::encode_atom_values(atom), 1));

	_value_updates ++;
}

/* ================================================================== */

/// Delete ALL of the values associated with the atom.
void DHTAtomStorage::delete_atom_values(const Handle& atom)
{
	if (_observing_only)
		throw IOException(TRACE_INFO, "DHT Node is only observing!");

	// Attach the value to the atom
	dht::InfoHash muid = get_membership(atom);
	_runner.put(muid, dht::Value(_values_policy, "", 1));

	_value_deletes ++;
}

/* ================================================================ */

Handle DHTAtomStorage::fetch_values(Handle&& h)
{
	dht::InfoHash muid = get_membership(h);

	auto dvals = get_stuff(muid, _values_filter);

	// There may be multiple values attached to this Atom.
	// We only want one; the one with the latest timestamp.
	unsigned long timestamp = 0;
	std::string alist;
	for (const auto& dval : dvals)
	{
		// std::cout << "Got value: " << dval->toString() << std::endl;
		if (timestamp < dval->id)
		{
			timestamp = dval->id;
			alist = dval->unpack<std::string>();
		}
	}
	// std::cout << "Latest svalue: " << alist << std::endl;
	Sexpr::decode_alist(h, alist);
	_value_fetches++;

	return h;
}

/* ============================= END OF FILE ================= */
