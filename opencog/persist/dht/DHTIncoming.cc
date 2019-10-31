/*
 * IPFSIncoming.cc
 * Save and restore of atom incoming set.
 *
 * Copyright (c) 2008,2009,2013,2017,2019 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <stdlib.h>

#include <opencog/atoms/base/Atom.h>

#include "IPFSAtomStorage.h"

using namespace opencog;

/* ================================================================== */

/// Store `holder` into the incoming set of atom.
void IPFSAtomStorage::store_incoming_of(const Handle& atom,
                                        const Handle& holder)
{
	// No publication of Incoming Set, if there's no AtomSpace key.
	if (0 == _keyname.size()) return;

	// Obtain the JSON representation of the Atom.
	// We cn either ask IPFS for the current json (using get_atom_json())
	// or we can work out of the cache. Seems faster to work out of the
	// cache.  Oh, and we need to do this atomically, because other
	// threads might be writing. So lock must hold for the entire
	// duration of the json edit.
	ipfs::Json jatom;
	{
		std::string holder_guid = get_atom_guid(holder);
		std::lock_guard<std::mutex> lck(_json_mutex);
		jatom = _json_map.find(atom)->second;

		ipfs::Json jinco;
		auto incli = jatom.find("incoming");
		if (jatom.end() != incli)
		{
			// Is the atom already a part of the incoming set?
			// If so, then there's nothing to do.
			auto havit = incli->find(holder_guid);
			if (incli->end() != havit) return;
			jinco = *incli;
		}

		jinco.push_back(holder_guid);
		jatom["incoming"] = jinco;
		_json_map[atom] = jatom;
	}

	// Store the thing in IPFS
	ipfs::Json result;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagPut(jatom, &result);
	conn_pool.push(conn);

	std::string atoid = result["Cid"]["/"];
	// std::cout << "Incoming Atom: " << encodeAtomToStr(atom)
	//          << " CID: " << atoid << std::endl;

	update_atom_in_atomspace(atom, atoid);
}

/* ================================================================== */

/// Remove `holder` from the incoming set of atom.
void IPFSAtomStorage::remove_incoming_of(const Handle& atom,
                                         const std::string& holder)
{
	// std::cout << "Remove from " << atom->to_short_string()
	//           << " in CID " << holder << std::endl;

	// Twiddle the incoming set of atom. As before, we can either
	// ask IPFS for the current json, or we can work out of what we
	// have in the cache. Use the cache for speed. All edits to the
	// json must be don atomically, since there may be other threads
	// racing with us.
	ipfs::Json jatom;
	{
		std::lock_guard<std::mutex> lck(_json_mutex);
		auto patom = _json_map.find(atom);
		if (_json_map.end() == patom)
			jatom = get_atom_json(atom);
		else
			jatom = patom->second; // jatom = _json_map[atom];

		// Remove the holder from the incoming set ...
		auto pinco = jatom.find("incoming");
		if (jatom.end() == pinco)
			throw RuntimeException(TRACE_INFO,
				"Error: Atom is missing incoming set! WTF!?\n");

		std::set<std::string> inco = *pinco; // inco = jatom["incoming"];
		inco.erase(holder);
		if (0 < inco.size())
			jatom["incoming"] = inco;
		else
			jatom.erase("incoming");
		// std::cout << "Atom after erasure: " << jatom.dump(2) << std::endl;
		_json_map[atom] = jatom;
	}

	// Store the edited Atom back into IPFS...
	ipfs::Json result;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagPut(jatom, &result);
	conn_pool.push(conn);

	// Finally, update the Atomspace with this revised Atom.
	std::string atoid = result["Cid"]["/"];
	update_atom_in_atomspace(atom, atoid);
}

/* ================================================================ */
/**
 * Retreive the entire incoming set of the indicated atom.
 * This fetches the Atoms in the incoming set; the ValueSaveUTest
 * expects the associated Values to be fetched also.
 */
void IPFSAtomStorage::getIncomingSet(AtomTable& table, const Handle& h)
{
	rethrow();

	// Get the incoming set of the atom.
	std::string path = _atomspace_cid + "/" + h->to_short_string();
	ipfs::Json dag;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagGet(path, &dag);
	conn_pool.push(conn);
	// std::cout << "The dag is:" << dag.dump(2) << std::endl;

	auto iset = dag["incoming"];
	for (auto acid: iset)
	{
		// std::cout << "The incoming is:" << acid.dump(2) << std::endl;
		// Fetch once, to get it's type & name/outgoing
		// Fetch a second time to get the current values.
		Handle h(fetch_atom(acid));
		table.add(do_fetch_atom(h), false);
	}

	_num_get_insets++;
	_num_get_inlinks += iset.size();
}

/**
 * Retreive the incoming set of the indicated atom, but only those atoms
 * of type t.
 */
void IPFSAtomStorage::getIncomingByType(AtomTable& table, const Handle& h, Type t)
{
	rethrow();

	// Code is almost same as above. It's not terribly efficient.
	// But it works, at least.
	std::string path = _atomspace_cid + "/" + h->to_short_string();

	ipfs::Json dag;
	ipfs::Client* conn = conn_pool.pop();
	conn->DagGet(path, &dag);
	conn_pool.push(conn);

	auto iset = dag["incoming"];
	for (auto acid: iset)
	{
		Handle h(fetch_atom(acid));
		if (t == h->get_type())
		{
			table.add(h, false);
			_num_get_inlinks ++;
		}
	}

	_num_get_insets++;
}

/* ============================= END OF FILE ================= */
