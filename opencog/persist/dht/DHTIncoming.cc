/*
 * DHTIncoming.cc
 * Save and restore of atom incoming set.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <stdlib.h>

#include <opencog/atoms/base/Atom.h>

#include "DHTAtomStorage.h"

using namespace opencog;

/* ================================================================== */
/**
 * Retreive the entire incoming set of the indicated atom.
 * This fetches the Atoms in the incoming set; the ValueSaveUTest
 * expects the associated Values to be fetched also.
 */
void DHTAtomStorage::getIncomingSet(AtomTable& table, const Handle& h)
{
	dht::InfoHash mhash = get_membership(h);

	// Get a future for the incoming set on the atom. We filter,
	// because the same membership hash gets used for both values
	// and for incoming sets. We only want the incoming set.
	auto afut = _runner.get(mhash,
		dht::Value::TypeFilter(_incoming_policy));

	// Block until we've got it.
	std::future_status status = afut.wait_for(_wait_time);
	if (std::future_status::ready != status)
		throw IOException(TRACE_INFO, "DHT is not responding!");

	auto dincs = afut.get();
	for (const auto& dinc : dincs)
	{
		// std::cout << "Got incoming guid: " <<
		//	dinc->unpack<dht::InfoHash>().toString() << std::endl;
		Handle h(fetch_values(fetch_atom(dinc->unpack<dht::InfoHash>())));
		std::cout << "Got incoming Atom: " << h->to_string() << std::endl;

		table.add(h, false);
	}

	_num_get_insets++;
	_num_get_inlinks += dincs.size();
}

/**
 * Retreive the incoming set of the indicated atom, but only those atoms
 * of type t.  This is done brute-force right now, and a faster, more
 * elegant version could be provided in the future, someday, I guess.
 */
void DHTAtomStorage::getIncomingByType(AtomTable& table, const Handle& h, Type t)
{
	dht::InfoHash mhash = get_membership(h);

	// Get a future for the incoming set on the atom. We filter,
	// because the same membership hash gets used for both values
	// and for incoming sets. We only want the incoming set.
	auto afut = _runner.get(mhash,
		dht::Value::TypeFilter(_incoming_policy));

	// Block until we've got it.
	std::future_status status = afut.wait_for(_wait_time);
	if (std::future_status::ready != status)
		throw IOException(TRACE_INFO, "DHT is not responding!");

	for (const auto& dinc : afut.get())
	{
		// std::cout << "Got incoming guid: " <<
		//	dinc->unpack<dht::InfoHash>().toString() << std::endl;
		Handle h(fetch_atom(dinc->unpack<dht::InfoHash>()));
		if (h->get_type() != t) continue;

		Handle hv(fetch_values(std::move(h)));
		std::cout << "Got typed incoming Atom: " << hv->to_string() << std::endl;

		table.add(hv, false);
		_num_get_inlinks ++;
	}

	_num_get_insets++;
}

/* ============================= END OF FILE ================= */
