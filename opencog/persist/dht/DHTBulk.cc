/*
 * DHTBulk.cc
 * Bulk save & restore of entire AtomSpaces.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/atom_types/NameServer.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atomspace/AtomSpace.h>

#include "DHTAtomStorage.h"

using namespace opencog;

/* ================================================================ */

/// load_atomspace -- load the AtomSpace with the given name.
void DHTAtomStorage::load_atomspace(AtomSpace* as,
                                    const std::string& spacename)
{
	size_t start_count = _load_count;
	printf("Loading all atoms from %s\n", spacename.c_str());
	bulk_load = true;
	bulk_start = time(0);

	dht::InfoHash space_hash = dht::InfoHash::get(spacename);
	auto sfut = _runner.get(space_hash);
	std::cout << "Start waiting for atomspace" << std::endl;
	sfut.wait();
	std::cout << "Done waiting for atomspace" << std::endl;

	for (auto ato: sfut.get())
	{
		std::string sname = ato->unpack<std::string>();
		std::cout << "Load Atom " << sname << std::endl;
		Handle h(decodeStrAtom(sname));
		as->add_atom(fetch_values(std::move(h)));
		_load_count++;
	}

	time_t secs = time(0) - bulk_start;
	double rate = ((double) _load_count) / secs;
	printf("Finished loading %zu atoms in total in %d seconds (%d per second)\n",
		(_load_count - start_count), (int) secs, (int) rate);
	bulk_load = false;

	// synchrnonize!
	as->barrier();
}

/// A copy of the above.
/// Inefficient, but it works. We could solve the inefficiency
/// by maintaining an index of atoms-by-type in the DHT. This
/// would certainly push more crap into the DHT ... is it worth
/// it, to have this index?
///
void DHTAtomStorage::loadType(AtomTable &table, Type atom_type)
{
	auto sfut = _runner.get(_atomspace_hash);
	std::cout << "Start waiting for atomspace" << std::endl;
	sfut.wait();
	std::cout << "Done waiting for atomspace" << std::endl;

	for (auto ato: sfut.get())
	{
		std::string sname = ato->unpack<std::string>();
		std::cout << "Load Atom " << sname << std::endl;
		Handle h(decodeStrAtom(sname));
		if (h->get_type() != atom_type) continue;

		as->add_atom(fetch_values(std::move(h)));
		_load_count++;
	}
}

/// Store all of the atoms in the atom table.
void DHTAtomStorage::storeAtomSpace(const AtomTable &table)
{
	logger().info("Bulk store of AtomSpace\n");

	_store_count = 0;
	bulk_start = time(0);
	bulk_store = true;

	// Try to knock out the nodes first, then the links.
	table.foreachHandleByType(
		[&](const Handle& h)->void { storeAtom(h); },
		NODE, true);

	table.foreachHandleByType(
		[&](const Handle& h)->void { storeAtom(h); },
		LINK, true);

	flushStoreQueue();
	bulk_store = false;

	time_t secs = time(0) - bulk_start;
	double rate = ((double) _store_count) / secs;
	printf("\tFinished storing %lu atoms total, in %d seconds (%d per second)\n",
		(unsigned long) _store_count, (int) secs, (int) rate);
}

void DHTAtomStorage::loadAtomSpace(AtomTable &table)
{
	load_atomspace(table.getAtomSpace(), _atomspace_name);
}

/* ============================= END OF FILE ================= */
