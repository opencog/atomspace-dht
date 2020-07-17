/*
 * opencog/persist/dht/DHTPersistSCM.h
 *
 * Copyright (c) 2008 by OpenCog Foundation
 * Copyright (c) 2008, 2009, 2013, 2015, 2019 Linas Vepstas <linasvepstas@gmail.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_DHT_PERSIST_SCM_H
#define _OPENCOG_DHT_PERSIST_SCM_H

#include <string>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atoms/base/Handle.h>
#include <opencog/persist/dht/DHTAtomStorage.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

class DHTPersistSCM
{
private:
	static void* init_in_guile(void*);
	static void init_in_module(void*);
	void init(void);

	DHTAtomStorage *_backing;
	AtomSpace *_as;

public:
	DHTPersistSCM(AtomSpace*);
	~DHTPersistSCM();

	void do_open(const std::string&);
	void do_bootstrap(const std::string&);
	void do_close(void);
	void do_load(void);
	void do_store(void);
	std::string do_examine(const std::string&);
	std::string do_atomspace_hash(void);
	std::string do_immutable_hash(const Handle&);
	std::string do_atom_hash(const Handle&);
	std::string do_node_info(void);
	std::string do_storage_log(void);
	std::string do_routing_tables_log(void);
	std::string do_searches_log(void);
	void do_load_atomspace(const std::string&);

	void do_stats(void);
	void do_clear_stats(void);
}; // class

/** @}*/
}  // namespace

extern "C" {
void opencog_persist_dht_init(void);
};

#endif // _OPENCOG_DHT_PERSIST_SCM_H
