/*
 * DHTBulk.cc
 * Bulk save & restore of Atoms.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <opencog/util/oc_assert.h>
#include <opencog/util/oc_omp.h>

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
/// Stunningly inefficient, but it works.
///
void DHTAtomStorage::loadType(AtomTable &table, Type atom_type)
{
#if 0
	rethrow();

	ipfs::Json dag;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagGet(_atomspace_cid, &dag);
	conn_pool.push(conn);
	// std::cout << "The atomspace dag is:" << dag.dump(2) << std::endl;

	auto atom_list = dag["links"];
	for (auto acid: atom_list)
	{
		// std::cout << "Atom CID is: " << acid["Cid"]["/"] << std::endl;

		Handle h(fetch_atom(acid["Cid"]["/"]));
		if (h->get_type() == atom_type)
		{
			table.add(h, false);
			_load_count++;
		}
	}
#endif
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
