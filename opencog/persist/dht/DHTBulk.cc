/*
 * IPFSBulk.cc
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

#include "IPFSAtomStorage.h"

using namespace opencog;

/* ================================================================ */

/// load_atomspace -- load AtomSpace from path.
/// The path could be a CID, or it could be /ipfs/CID or it could
/// be /ipns/CID. In the later case, the IPNS lookup is performed.
void IPFSAtomStorage::load_atomspace(AtomSpace* as, const std::string& path)
{
	rethrow();

	if ('/' != path[0])
	{
		load_as_from_cid(as, path);
		return;
	}

	if (std::string::npos != path.find("/ipfs/"))
	{
		load_as_from_cid(as, &path[sizeof("/ipfs/") - 1]);
		return;
	}

	if (std::string::npos != path.find("/ipns/"))
	{
		// Caution: as of this writing, name resolution takes
		// exactly 60 seconds.
		std::string ipfs_path;
		ipfs::Client* conn = conn_pool.pop();
		conn->NameResolve(path, &ipfs_path);
		conn_pool.push(conn);

		// We are expecting the name to resolve into a string
		// of the form "/ipfs/Qm..."
		load_as_from_cid(as, &ipfs_path[sizeof("/ipfs/") - 1]);
		return;
	}

	throw RuntimeException(TRACE_INFO, "Unsupported URI %s\n", path.c_str());
}

/* ================================================================ */

/// load_as_from_cid -- load all atoms listed at the indicated CID.
/// The CID is presumed to be an IPFS CID (and not an IPNS CID or
/// something else).
void IPFSAtomStorage::load_as_from_cid(AtomSpace* as, const std::string& cid)
{
	rethrow();

	size_t start_count = _load_count;
	printf("Loading all atoms from %s\n", cid.c_str());
	bulk_load = true;
	bulk_start = time(0);

	ipfs::Json dag;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagGet(cid, &dag);
	conn_pool.push(conn);
	// std::cout << "The atomspace dag is:" << dag.dump(2) << std::endl;

	auto atom_list = dag["links"];
	for (auto acid: atom_list)
	{
		// std::cout << "Atom CID is: " << acid["Cid"]["/"] << std::endl;

		// In the current design, the Cid entry is NOT an IPNS entry,
		// but is instead the IPFS CID of the Atom, with values
		// attached to it. So we have to fetch that, to get the latest
		// values on the atom.
		as->add_atom(fetch_atom(acid["Cid"]["/"]));
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
void IPFSAtomStorage::loadType(AtomTable &table, Type atom_type)
{
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
}

/// Store all of the atoms in the atom table.
void IPFSAtomStorage::storeAtomSpace(const AtomTable &table)
{
	rethrow();

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
	printf("\tAtomSpace CID: %s\n", _atomspace_cid.c_str());
}

void IPFSAtomStorage::loadAtomSpace(AtomTable &table)
{
	// Perform an IPNS lookup, if a key was given.
	if (0 < _keyname.size()) resolve_atomspace();

	load_atomspace(table.getAtomSpace(), _atomspace_cid);
}

/* ============================= END OF FILE ================= */
