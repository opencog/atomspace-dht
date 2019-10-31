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
#include <opencog/atoms/truthvalue/SimpleTruthValue.h>
#include <opencog/atoms/truthvalue/TruthValue.h>

#include "DHTAtomStorage.h"

using namespace opencog;

/* ================================================================== */

/// Store ALL of the values associated with the atom.
void DHTAtomStorage::store_atom_values(const Handle& atom)
{
   // Update the atomspace, so that it holds the new value.
   // update_atom_in_atomspace(atom, atoid);
}
/* ================================================================== */

#if 0
/// Get ALL of the values associated with an atom.
void DHTAtomStorage::get_atom_values(Handle& atom, const ipfs::Json& jatom)
{
	// If no values, then nothing to do.
	auto pvals = jatom.find("values");
	if (pvals == jatom.end()) return;

	ipfs::Json jvals = *pvals;
	// std::cout << "Jatom vals: " << jvals.dump(2) << std::endl;

	for (const auto& [jkey, jvalue]: jvals.items())
	{
		// std::cout << "KV Pair: " << jkey << " "<<jvalue<< std::endl;
		atom->setValue(decodeStrAtom(jkey), decodeStrValue(jvalue));
	}
}
#endif

/* ================================================================ */

ValuePtr DHTAtomStorage::decodeStrValue(const std::string& stv)
{
	size_t pos = stv.find("(LinkValue");
	if (std::string::npos != pos)
	{
		pos += strlen("(LinkValue");
		std::vector<ValuePtr> vv;
		while (pos != std::string::npos and stv[pos] != ')')
		{
			pos = stv.find('(', pos);
			if (std::string::npos == pos) break;

			// Find the next balanced paren, and restart there.
			// This is not very efficient, but it works.
			size_t epos = pos;
			int pcnt = 1;
			while (0 < pcnt and epos != std::string::npos)
			{
				char c = stv[++epos];
				if ('(' == c) pcnt ++;
				else if (')' == c) pcnt--;
			}
			if (epos == std::string::npos) break;
			vv.push_back(decodeStrValue(stv.substr(pos, epos-pos+1)));
			pos = epos+1;
		}
		return createLinkValue(vv);
	}

	pos = stv.find("(FloatValue ");
	if (std::string::npos != pos)
	{
		pos += strlen("(FloatValue ");
		std::vector<double> fv;
		while (pos != std::string::npos and stv[pos] != ')')
		{
			size_t epos;
			fv.push_back(stod(stv.substr(pos), &epos));
			pos += epos;
		}
		return createFloatValue(fv);
	}

	pos = stv.find("(SimpleTruthValue ");
	if (std::string::npos != pos)
	{
		size_t epos;
		pos += strlen("(SimpleTruthValue ");
		double strength = stod(stv.substr(pos), &epos);
		pos += epos;
		double confidence = stod(stv.substr(pos), &epos);
		return ValueCast(createSimpleTruthValue(strength, confidence));
	}

	pos = stv.find("(StringValue ");
	if (std::string::npos != pos)
	{
		pos += strlen("(StringValue ");
		std::vector<std::string> sv;
		while (true)
		{
			pos = stv.find('\"', pos);
			if (std::string::npos == pos) break;
			size_t epos = stv.find('\"', pos+1);
			sv.push_back(stv.substr(pos+1, epos-pos-1));
			pos = epos+1;
		}
		return createStringValue(sv);
	}

	throw SyntaxException(TRACE_INFO, "Unknown Value %s", stv.c_str());
}

/* ================================================================ */

/// Get all atoms having indicated key on them.
/// It appears that there are zero users of this thing, because the
/// guile API for this was never created.  Should probably get rid
/// of this.
void DHTAtomStorage::getValuations(AtomTable& table,
                                   const Handle& key, bool get_all_values)
{
	throw SyntaxException(TRACE_INFO, "Not Implemented!");
}

/* ============================= END OF FILE ================= */
