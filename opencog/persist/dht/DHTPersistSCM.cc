/*
 * opencog/persist/dht/DHTPersistSCM.cc
 *
 * Copyright (c) 2008 by OpenCog Foundation
 * Copyright (c) 2008, 2009, 2013, 2015, 2019 Linas Vepstas <linasvepstas@gmail.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <libguile.h>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atomspace/BackingStore.h>
#include <opencog/guile/SchemePrimitive.h>

#include "DHTAtomStorage.h"
#include "DHTPersistSCM.h"

using namespace opencog;


// =================================================================

DHTPersistSCM::DHTPersistSCM(AtomSpace *as)
{
    _as = as;
    _backing = nullptr;

    static bool is_init = false;
    if (is_init) return;
    is_init = true;
    scm_with_guile(init_in_guile, this);
}

void* DHTPersistSCM::init_in_guile(void* self)
{
    scm_c_define_module("opencog persist-dht", init_in_module, self);
    scm_c_use_module("opencog persist-dht");
    return NULL;
}

void DHTPersistSCM::init_in_module(void* data)
{
   DHTPersistSCM* self = (DHTPersistSCM*) data;
   self->init();
}

void DHTPersistSCM::init(void)
{
    define_scheme_primitive("dht-open", &DHTPersistSCM::do_open, this, "persist-dht");
    define_scheme_primitive("dht-close", &DHTPersistSCM::do_close, this, "persist-dht");
    define_scheme_primitive("dht-stats", &DHTPersistSCM::do_stats, this, "persist-dht");
    define_scheme_primitive("dht-clear-stats", &DHTPersistSCM::do_clear_stats, this, "persist-dht");

    define_scheme_primitive("dht-load-atomspace", &DHTPersistSCM::do_load_atomspace, this, "persist-dht");
}

DHTPersistSCM::~DHTPersistSCM()
{
    if (_backing) delete _backing;
}

void DHTPersistSCM::do_open(const std::string& uri)
{
    if (_backing)
        throw RuntimeException(TRACE_INFO,
             "dht-open: Error: Already connected to a database!");

    // Unconditionally use the current atomspace, until the next close.
    AtomSpace *as = SchemeSmob::ss_get_env_as("dht-open");
    if (nullptr != as) _as = as;

    if (nullptr == _as)
        throw RuntimeException(TRACE_INFO,
             "dht-open: Error: Can't find the atomspace!");

    // Allow only one connection at a time.
    if (_as->isAttachedToBackingStore())
        throw RuntimeException(TRACE_INFO,
             "dht-open: Error: Atomspace connected to another storage backend!");
    // Use the postgres driver.
    DHTAtomStorage *store = new DHTAtomStorage(uri);
    if (!store)
        throw RuntimeException(TRACE_INFO,
            "dht-open: Error: Unable to open the database");

    if (!store->connected())
    {
        delete store;
        throw RuntimeException(TRACE_INFO,
            "dht-open: Error: Unable to connect to the database");
    }

    _backing = store;
    _backing->registerWith(_as);
}

void DHTPersistSCM::do_close(void)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
             "dht-close: Error: Database not open");

    DHTAtomStorage *backing = _backing;
    _backing = nullptr;

    // The destructor might run for a while before its done; it will
    // be emptying the pending store queues, which might take a while.
    // So unhook the atomspace first -- this will prevent new writes
    // from accidentally being queued. (It will also drain the queues)
    // Only then actually call the dtor.
    backing->unregisterWith(_as);
    delete backing;
}

void DHTPersistSCM::do_load_atomspace(const std::string& cid)
{
    if (nullptr == _backing)
        throw RuntimeException(TRACE_INFO,
            "dht-load-atomspace: Error: Database not open");

    return _backing->load_atomspace(_as, cid);
}

void DHTPersistSCM::do_stats(void)
{
    if (nullptr == _backing) {
        printf("dht-stats: Database not open\n");
        return;
    }

    printf("dht-stats: Atomspace holds %lu atoms\n", _as->get_size());
    _backing->print_stats();
}

void DHTPersistSCM::do_clear_stats(void)
{
    if (nullptr == _backing) {
        printf("dht-stats: Database not open\n");
        return;
    }

    _backing->clear_stats();
}

void opencog_persist_dht_init(void)
{
    static DHTPersistSCM patty(NULL);
}
