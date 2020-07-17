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
#include <opencog/persist/sexpr/Sexpr.h>

#include "DHTAtomStorage.h"

using namespace opencog;

/* ================================================================ */

/// load_atomspace -- load the AtomSpace with the given name.
void DHTAtomStorage::load_atomspace(AtomSpace* as,
                                    const std::string& spacename)
{
	size_t start_count = _load_count;
	printf("Loading all atoms from %s\n", spacename.c_str());
	time_t bulk_start = time(0);

	dht::InfoHash space_hash = dht::InfoHash::get(spacename);

	// XXX FIXME, when working on the Atomspace, we need
	// a longer timeout!?!!!
	std::cout << "Start waiting for atomspace" << std::endl;
	auto atovs = get_stuff(space_hash);
	std::cout << "Done waiting for atomspace" << std::endl;

	for (auto ato: atovs)
	{
		std::string sname = ato->unpack<std::string>();

		// Currently, the format is the string "add" or "drop",
		// followed by a timestamp, followed by the scheme string.
		// Ignore anything that doesn't start with "add"
#define ADD_ATOM "add "
	   if (sname.compare(0, sizeof(ADD_ATOM)-1, ADD_ATOM))
			continue;

		std::cout << "Load Atom " << sname << std::endl;
		// pos is always 22 because the prefix is "add 1572978874.801600 "
		// at least it will be for the next bijillion seconds.
		// size_t pos = sname.find('(');
		size_t pos = sizeof("add 1572978874.801600");
		Handle h(Sexpr::decode_atom(sname, pos));
		as->add_atom(fetch_values(std::move(h)));
		_load_count++;
	}

	time_t secs = time(0) - bulk_start;
	double rate = ((double) _load_count) / secs;
	printf("Finished loading %zu atoms in total in %d seconds (%d per second)\n",
		(_load_count - start_count), (int) secs, (int) rate);

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
	// XXX FIXME, when working on the Atomspace, we need
	// a longer timeout!?!!!
	std::cout << "Start waiting for atomspace" << std::endl;
	auto atovs = get_stuff(_atomspace_hash);
	std::cout << "Done waiting for atomspace, got "
	          << std::to_string(atovs.size()) << std::endl;

	for (auto ato: atovs)
	{
		std::string sname = ato->unpack<std::string>();

		// See comments above.
	   if (sname.compare(0, sizeof(ADD_ATOM)-1, ADD_ATOM))
			continue;

		size_t pos = sizeof("add 1572978874.801600");
		Handle h(Sexpr::decode_atom(sname, pos));
		if (h->get_type() != atom_type) continue;

		// std::cout << "Load Atom " << sname << std::endl;
		table.add(fetch_values(std::move(h)), false);
		_load_count++;
	}
}

/// Store all of the atoms in the atom table.
void DHTAtomStorage::storeAtomSpace(const AtomTable &table)
{
	logger().info("Bulk store of AtomSpace\n");

	size_t cnt = 0;
	time_t bulk_start = time(0);

	auto storat = [&](const Handle& h)->void
	{
		storeAtom(h);
		cnt++;
		if (0 == cnt%500)
		{
			// Force DHT to flush pending queues every so often.
			// Otherwise, the user perceives a hang. Worse, failure
			// to flush often enough does TWO bad things: it wrecks
			// performance, and also causes OpenDHT to print message
			// "Dropped NNN packets with high delay".  Yow!!
			// On my system, 500 seems to be the magic number.
			barrier();
		}
		if (0 == cnt%1000)
		{
			time_t now = time(0);
			time_t elap = now - bulk_start;
			if (0 < elap)
			{
				double rate = ((double) cnt) / elap;
				printf("\tStored %zu atoms in %d seconds (%d per second)\n",
				       cnt, (int) elap, (int) rate);
			}
		}
	};

	// Try to knock out the nodes first, then the links.
	table.foreachHandleByType(storat, NODE, true);
	table.foreachHandleByType(storat, LINK, true);

	barrier();
	time_t secs = time(0) - bulk_start;
	double rate = ((double) cnt) / secs;
	printf("\tFinished storing %zu atoms total, in %d seconds (%d per second)\n",
		cnt, (int) secs, (int) rate);
}

void DHTAtomStorage::loadAtomSpace(AtomTable &table)
{
	load_atomspace(table.getAtomSpace(), _atomspace_name);
}

/* ============================= END OF FILE ================= */
