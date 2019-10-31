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
	dht::InfoHash guid = get_guid(atom);

	// Attach the value to the atom
	_runner.put(guid, dht::Value(_values_policy, atom->valuesToString()));

	_value_stores ++;

	// Put the atom into the Atomspace too
	add_atom_to_atomspace(atom);
}

/* ================================================================== */

ValuePtr DHTAtomStorage::decodeStrValue(std::string& stv, size_t& pos)
{
	size_t totlen = stv.size();
	size_t vos = stv.find("(LinkValue", pos);
	if (std::string::npos != vos)
	{
		vos += strlen("(LinkValue");
		std::vector<ValuePtr> vv;
		vos = stv.find('(', vos);
		size_t epos = vos;
		while (vos != std::string::npos)
		{
			// Find the next balanced paren, and restart there.
			// This is not very efficient, but it works.
			epos = vos;
			int pcnt = 1;
			while (0 < pcnt and epos < totlen)
			{
				char c = stv[++epos];
				if ('(' == c) pcnt ++;
				else if (')' == c) pcnt--;
			}
			if (epos >= totlen)
				throw SyntaxException(TRACE_INFO,
					"Malformed LinkValue: %s", stv.substr(pos).c_str());

			size_t junk = 0;
			vv.push_back(decodeStrValue(stv.substr(vos, epos-vos+1), junk));
			vos = stv.find('(', ++epos);
		}
		epos = stv.find(')', epos);
		if (std::string::npos == epos)
			throw SyntaxException(TRACE_INFO,
				"Malformed LinkValue: %s", stv.substr(pos).c_str());
		pos = epos + 1;
		return createLinkValue(vv);
	}

	vos = stv.find("(FloatValue ", pos);
	if (std::string::npos != vos)
	{
		vos += strlen("(FloatValue ");
		std::vector<double> fv;
		while (vos < totlen and stv[vos] != ')')
		{
			size_t epos;
			fv.push_back(stod(stv.substr(vos), &epos));
			vos += epos;
		}
		pos = vos + 1;
		return createFloatValue(fv);
	}

	vos = stv.find("(SimpleTruthValue ", pos);
	if (std::string::npos != vos)
		vos += strlen("(SimpleTruthValue ");
	else
	{
		vos = stv.find("(stv ", pos);
		if (std::string::npos != vos)
			vos += strlen("(stv ");
	}
	if (std::string::npos != vos)
	{
		size_t epos;
		double strength = stod(stv.substr(vos), &epos);
		vos += epos;
		double confidence = stod(stv.substr(vos), &epos);
		epos = stv.find(')', epos);
		if (std::string::npos == epos)
			throw SyntaxException(TRACE_INFO,
				"Malformed SimpleTruthValue: %s", stv.substr(pos).c_str());
		pos = epos + 1;
		return ValueCast(createSimpleTruthValue(strength, confidence));
	}

	vos = stv.find("(StringValue ", pos);
	if (std::string::npos != vos)
	{
		vos += strlen("(StringValue ");
		std::vector<std::string> sv;
		size_t epos;
		while (true)
		{
			vos = stv.find('\"', vos);
			if (std::string::npos == vos) break;
			epos = stv.find('\"', vos+1);
			sv.push_back(stv.substr(vos+1, epos-vos-1));
			vos = epos+1;
		}
		epos = stv.find(')', epos+1);
		if (std::string::npos == epos)
			throw SyntaxException(TRACE_INFO,
				"Malformed StringValue: %s", stv.substr(pos).c_str());
		pos = epos + 1;
		return createStringValue(sv);
	}

	throw SyntaxException(TRACE_INFO, "Unknown Value %s", stv.c_str());
}

/* ================================================================== */

/**
 * Decode a Valuations association list.
 * This list has the format
 * ((KEY . VALUE)(KEY2 . VALUE2)...)
 * Store the results as values on the atom.
 */
void DHTAtomStorage::decodeAlist(Handle& atom, const std::string& alist)
{
	// Skip over opening paren
	size_t pos = 1;
	size_t totlen = alist.size();
	while (std::string::npos != pos and pos < totlen)
	{
		Handle key(decodeStrAtom(alist, pos));
		pos = alist.find(" . ", pos);
		pos += 3;
		ValuePtr val(decodeStrValue(alist, pos));
		atom->setValue(key, val);
	}
}

/* ============================= END OF FILE ================= */
