/*
 * IPFSAtomDelete.cc
 * Deletion of individual atoms.
 *
 * Copyright (c) 2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <stdlib.h>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atomspace/AtomSpace.h>

#include "IPFSAtomStorage.h"

using namespace opencog;

/// Remove an atom, and all of it's associated values from the database.
/// If the atom has a non-empty incoming set, then it is NOT removed
/// unless the recursive flag is set. If the recursive flag is set, then
/// the atom, and everything in its incoming set is removed.
///
void IPFSAtomStorage::removeAtom(const Handle& h, bool recursive)
{
	// Synchronize. The atom that we are deleting might be sitting
	// in the store queue.
	flushStoreQueue();

	ipfs::Json jatom;
	{
		std::lock_guard<std::mutex> lck(_json_mutex);
		const auto& ptr = _json_map.find(h);
		// If might be not found, because it had never been
		// stored before. This is not an error.
		if (_json_map.end() == ptr) return;

		jatom = ptr->second;
	}

	auto pinc = jatom.find("incoming");
	if (jatom.end() != pinc)
	{
		auto iset = *pinc; // iset = jatom["incoming"];

		// Fail if a non-trivial incoming set.
		if (not recursive and 0 < iset.size()) return;

		// We're recursive; so recurse.
		for (auto guid: iset)
		{
			// Given only the GUID of the atom, get the handle.
			// Use the cache, if possible.
			Handle hin;
			{
				std::lock_guard<std::mutex> lck(_inv_mutex);
				auto inp = _guid_inv_map.find(guid);
				if (_guid_inv_map.end() == inp)
				{
std::cout << "Quasi-error: expected to find atom but did not!" << std::endl;
					hin = fetch_atom(guid);
				}
				else
				{
					hin = inp->second;
				}
			}
			removeAtom(hin, true);
		}
	}

	// Remove this atom from the incoming sets of those that
	// it contains.
	if (h->is_link())
	{
		std::string acid;
		{
			std::lock_guard<std::mutex> lck(_guid_mutex);
			auto pcid = _guid_map.find(h);
			if (_guid_map.end() == pcid)
				throw RuntimeException(TRACE_INFO,
					"Error: missing CID for %s", h->to_string().c_str());
			acid = pcid->second; // acid = _guid_map[h];
		}
		for (const Handle& hoth: h->getOutgoingSet())
			remove_incoming_of(hoth, acid);
	}

	// Drop the atom from out caches
	{
		std::lock_guard<std::mutex> lck(_json_mutex);
		_json_map.erase(h);
	}
	{
		std::lock_guard<std::mutex> lck(_guid_mutex);
		_guid_map.erase(h);
	}

	// Now actually remove.
	std::string new_as_id;
	ipfs::Client* conn = conn_pool.pop();
	{
		std::string name = h->to_short_string();
		try
		{
			// Update the cid under a lock, as atomspace modifications
			// can occur from multiple threads.  It's not actually the
			// cid that matters, its the patch itself.
			std::lock_guard<std::mutex> lck(_atomspace_cid_mutex);
			conn->ObjectPatchRmLink(_atomspace_cid, name, &new_as_id);
			_atomspace_cid = new_as_id;
		}
		catch (const std::exception& ex)
		{
			std::cout << "Error: Atomspace " << _atomspace_cid
			          << " does not contain " << name << std::endl;

			conn_pool.push(conn);
			throw RuntimeException(TRACE_INFO,
				"Error: Atomspace did not contain atom; how did that happen?\n");
		}
		std::cout << "Atomspace after removal of " << name
		          << " is " << _atomspace_cid << std::endl;
	}
	conn_pool.push(conn);

	// Bug with stats: should not increment on recursion.
	_num_atom_deletes++;
	_num_atom_removes++;
}

/* ============================= END OF FILE ================= */
