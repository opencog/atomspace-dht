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

/// Store `holder` into the incoming set of atom.
void DHTAtomStorage::store_incoming_of(const Handle& atom,
                                       const Handle& holder)
{
}

/* ================================================================== */

/// Remove `holder` from the incoming set of atom.
void DHTAtomStorage::remove_incoming_of(const Handle& atom,
                                        const std::string& holder)
{
	// std::cout << "Remove from " << atom->to_short_string()
	//           << " in CID " << holder << std::endl;
}

/* ================================================================ */
/**
 * Retreive the entire incoming set of the indicated atom.
 * This fetches the Atoms in the incoming set; the ValueSaveUTest
 * expects the associated Values to be fetched also.
 */
void DHTAtomStorage::getIncomingSet(AtomTable& table, const Handle& h)
{

#if 0
	auto iset = dag["incoming"];
	for (auto acid: iset)
	{
		// std::cout << "The incoming is:" << acid.dump(2) << std::endl;
		// Fetch once, to get it's type & name/outgoing
		// Fetch a second time to get the current values.
		Handle h(fetch_atom(acid));
		table.add(do_fetch_atom(h), false);
	}
#endif

	_num_get_insets++;
	// _num_get_inlinks += iset.size();
}

/**
 * Retreive the incoming set of the indicated atom, but only those atoms
 * of type t.
 */
void DHTAtomStorage::getIncomingByType(AtomTable& table, const Handle& h, Type t)
{
#if 0
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
#endif

	_num_get_insets++;
}

/* ============================= END OF FILE ================= */
