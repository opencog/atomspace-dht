/*
 * DHTAtomDelete.cc
 * Deletion of individual atoms.
 *
 * Copyright (c) 2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <stdlib.h>
#include <opendht/node.h>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atomspace/AtomSpace.h>

#include "DHTAtomStorage.h"

using namespace opencog;

/// Remove an atom, and all of it's associated values from the database.
/// If the atom has a non-empty incoming set, then it is NOT removed
/// unless the recursive flag is set. If the recursive flag is set, then
/// the atom, and everything in its incoming set is removed.
///
void DHTAtomStorage::removeAtom(const Handle& atom, bool recursive)
{
	// Synchronize. The atom that we are deleting might be sitting
	// in the store queue.
	barrier();
printf("duuude gonnna remove %s\n", atom->to_string().c_str());

	// First, check to see if there's an incoming set, or not.
	dht::InfoHash mhash = get_membership(atom);
	auto afut = _runner.get(mhash,
		dht::Value::TypeFilter(_incoming_policy));
	afut.wait();
	auto dinset = afut.get();

	// Fail if a non-trivial incoming set.
	// Note that this is racey: the incoing set can change,
	// even as we are checking it. Right now, this is not
	// controlled, and might maybe lead to inconsistent state.
	if (not recursive and 0 < dinset.size()) return;

	// We're recursive; so recurse.
	for (const auto& dinc : dinset)
	{
		Handle hin(fetch_atom(dinc->unpack<dht::InfoHash>()));
		removeAtom(hin, true);
	}

	// Remove this atom from the incoming sets of those that
	// it contains.
	static dht::InfoHash zerohash;
	if (atom->is_link())
	{
		for (const Handle& held: atom->getOutgoingSet())
		{
			dht::InfoHash memuid = get_membership(held);
			_runner.put(memuid,
				dht::Value(_incoming_policy, zerohash, atom->get_hash()));
		}
	}

	// Now actually remove.
	// The space policy is using the 64-bit AtomSpace ID as the
	// DHT Value::id. Due to the birthday paradox, there is a very
	// small chance that two different Atoms will collide. In this
	// case, we want to broadcast the string, to disambiguate
	// which one is to be removed.
	std::string gstr = "drop " + std::to_string(now())
		+ " " + encodeAtomToStr(atom);
	_runner.put(_atomspace_hash,
	            dht::Value(_space_policy, gstr, atom->get_hash()));

	// Update the index, so that if the Atom is recreated later,
	// it appears to be brand-new.
	{
		std::unique_lock<std::mutex> lck(_publish_mutex);
		_published.erase(atom);
	}

	// Drop the atom from out caches
	{
		std::lock_guard<std::mutex> lck(_membership_mutex);
		_membership_map.erase(atom);
	}

	// Bug with stats: should not increment on recursion.
	_num_atom_deletes++;
	_num_atom_removes++;
}

/* ============================= END OF FILE ================= */
