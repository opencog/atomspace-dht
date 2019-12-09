# atomspace-dht
[OpenDHT](https://github.com/savoirfairelinux/opendht/wiki)
backend driver to the
[AtomSpace](https://github.com/opencog/atomspace) (hyper-)graph database.

The code here is a backend driver to the AtomSpace graph database,
enabling AtomSpace contents to be shared via the OpenDHT content
distribution network.  The goal is to allow efficient decentralized,
distributed operation over the global internet, allowing many
AtomSpace processes to access and perform updates to huge datasets.

### The AtomSpace
The [AtomSpace](https://wiki.opencog.org/w/AtomSpace) is a
(hyper-)graph database whose nodes and links are called
["Atoms"](https://wiki.opencog.org/w/Atom). Each (immutable) Atom has
an associated (mutable)
[key-value store](https://wiki.opencog.org/w/Value).  This split
between mutable and immutable data ("water" and "pipes") enables
a variety of advanced features not normally found in ordinary graph
databases, including an advanced query language and "active" Atoms.

### OpenDHT
OpenDHT is an internet-wide globally-accessible storage system, providing
a variety of distributed hash table services.  It provides decentralized
storage of data.

### Why do this? Motivation
Real-world data sets are often larger than the amount of RAM available on
a single machine; therefore, data processing algos have to be carefully
designed to fetch only that data which is needed at the moment.
Currently, this means using the PostgreSQL (PG) backend, which is the only
stable, time-tested backend currently available for the AtomSpace.  It
works great! ... but:

* Its difficult to set up; non-DBA's and ordinary users struggle to set
  it up, including the tuning it needs to be efficient for the AtomSpace.
* Its not terribly fast. It can process about 2K Atoms/sec on 2015-era
  server hardware. Which is OK, but... more is always better.

The OpenDHT backend is interesting, because the AtomSpace API is already
"distributed" or "decentralized": which one depends on the actual backend
in use.  With Postgres, its "distributed": multiple AtomSpace instances
can access a single PG dataset; it scales the same way that PG scales.
If you are a very good PG DBA, and have all the right hardware and
network gear, I suppose this means it scales very well.  So that's nice.
There's no practical experience scaling PG with the AtomSpace.

With the DHT backend, there is a vague, general hope that perhaps not
only does the install/config story becomes trivial, but also that the
scalability story will be brilliant: that, with many machines on a
local network, or globally distributed, one will be able to create
giant AtomSpace datasets.  At least, that is the general daydream.
The code here is an attempt to explore this idea.  The approach taken
is "naive", treating the DHT as a key-value store, and then mapping the
AtomSpace onto it, in the simplest, "most obvious" way possible. It
turns out that this naive approach has multiple difficulties, which
are presented and critiqued furhter below. But first, status.

## Proof-of-concept version 0.2.2
All core functions are implemented. They work, on a small scale, for
small datasets.  See the [examples](examples) for a walk-through. Most
unit tests usually pass (several generic OpenDHT issues, unrelated to
this backend, prevent a full pass). Many desirable enhancements are
missing. There is one show-stopper and one or more near-show-stopper
issues discouraging further development; see the issues list below,
and also the architecture concerns list.

### Status
In the current implementation:
 * OpenDHT appears to be compatible with the requirements imposed by
   the AtomSpace backend API. It seems to provide most of the services
   that the AtomSpace needs. This makes future development and
   operation look promising.
 * Despite this, there are several serious issues that are roadblocks
   to further development. These are listed below.
 * The implementation is feature-complete.  Missing are:
    + Assorted desirable enhancements (see list further below).
 * All nine unit tests have been ported over (from the original
   SQL backend driver tests). Currently seven of nine (usually) pass:
```
1 - BasicSaveUTest
2 - ValueSaveUTest
3 - PersistUTest
4 - FetchUTest
5 - DeleteUTest
6 - MultiPersistUTest
7 - MultiUserUTest
```
   Sometimes `ValueSaveUTest`, `FetchUTest` and/or `MultiUserUTest`
   intermittently fail because data gets lost; the root cause for this
   would seem to be that the underlying protocol for OpenDHT is UDP
   and not SCTP. Due to network congestion, the OS kernel and/or the
   network cards and routers are free to discard UDP packets. This
   translates into data loss for us.
 * The consistently failing tests are:
   + `8 - LargeFlatUTest` attempts a "large" atomspace (of only 35K Atoms,
          so actually, it's small, but bigger than the other tests).
   + `9 - LargeZipfUTest` attempts a "large" atomspace w/ Zipfian
          incoming set sizes.

   Bot of these a hard-coded limit in OpenDHT: each DHT node can only
   store 64K blocks data, and these tests generates about 3x more than
   that.  When this limit is hit, OpenDHT busy-waits at 100% cpu...
   for a currently-unkown reason... (I can't find the limit that is
   blocking this).

### Architecture
This provides a full, complete implementation of the standard
`BackingStore` API from the Atomspace. Its a backend driver.

The git repo layout is the same as that of the AtomSpace repo. Build
and install mechanisms are the same.

### Design Notes
* Every Atom gets a unique hash. This is called the GUID.
  Every GUID is published, because, given only a GUID,
  there needs to be a way of finding out what the Atom is.
  This is needed for IncomingSet fetch, for example.
* Every (Atom, AtomSpace-name) pair gets a unique hash.
  The current values on that atom, as well as it's incoming set
  are stored under that hash.
* How can we find all current members of an AtomSpace? Easy!
  The AtomSpace is just one hash, and each atom that is in it
  corresponds to one DHT value on that hash.
* How can we find all members of the incoming set of an Atom?
  Easy, we generate the DHT-hash for that Atom, and then look at
  all the DHT-values attached to it.  This is effectively the
  same mechanism as finding all the members of an AtomSpace.
* How to delete entries? Atoms in the AtomSpace are tagged with
  a timestamp and an add/drop verb, so that precedence is known.
  An alternate design using CRDT seems like overkill.
* TODO: Optionally use crypto signatures to verify that the data
  comes from legitimate, cooperating sources.
* TODO: Change the default one-week data expiration policy to
  instead be a few hours ... or even tens of minutes?  Persistant
  data still needs to live on disk, not in RAM, and to be provided
  by seeders.
* TODO: Support read-write overlay AtomSpaces on top of read-only
  AtomSpaces.  This seems like it should be easy...
* TODO: Enhancement: listen for new values on specific atoms
  or for new atom types.
* TODO: Enhancement: listen for atomspace updates (Atom insertion
  and deletion).
* TODO: use MSGPACK_DEFINE_MAP for more efficient serialization
  of values (esp. of FloatValue).
* TODO: Using `DhtRunner::get()` with callbacks instead of futures
  will improve performance. This is somewhat tricky, though, because
  one callback might trigger more gets (for values, incoming set, etc.)
  and it's not clear if OpenDHT might deadlock in this case...
* TODO: Enhancement: implement a CRDT type for `CountTruthValue`.
* TODO: Measure total RAM usage.  This risks being quite the
  memory hog, if datasets with hundreds of millions of atoms are
  published, and the default DHT node size is raised to allow
  that many hashes to be stored.
* TODO: Create a "seeder", that maintains the AtomSpace in Postgres,
  listens for load requests and responds by seeding those Atoms into
  DHT if they are not already there.  Likewise, in a read-write mode,
  it listens for updates, and stores them back into the database.
  Ideally, this seeder can use the existing AtomSpace Postgres backend
  code with zero modifications, and just act as a bridge between two
  backends.

### Issues
The following are serious issues, some of which are show-stoppers:

* There are data loss issues that are almost surely due to the use of
  UDP as the underlying OpenDHT protocol. The problem with UDP is that
  the OS kernel, the network cards, and network routers are free to
  discard UDP packets when the network is congested. For us, this means
  lost data. An SCTP shim for OpenDHT is needed.
  Dropped data is a show-stopper; data storage MUST be reliable!
  See [opendht issue #471](https://github.com/savoirfairelinux/opendht/issues/471)
  for SCTP implementation status.
* Hard-coded limits on various OpenDHT structures. See
  [opendht issue #426](https://github.com/savoirfairelinux/opendht/issues/426)
* It appears to be impossible to saturate the system to 100% CPU usage,
  even when running locally. This might be the reason why its slow:
  something, somewhere is blocking and taking too long; doing nothing
  at all. What this is is unknown. This is particularly visible with
  `MultiUserUTest`, which starts at 100% CPU and then drops to
  single-digit percentages.
* There is some insane gnutls/libnettle bug when it interacts with
  BoehmGC.  It's provoked when running `MultiUserUTest` when the
  line that creates `dht::crypto::generateIdentity();` is enabled.
  It crashes with a bizarre realloc bug. One bug report is
  [bug#38041 in guile](https://debbugs.gnu.org/cgi/bugreport.cgi?bug=38041).
  It appears that gnutls is not thread-safe... or something weird.

These all appear to be "early adopter" pains. There are likely to be many
other issues; see below.

### Architecture concerns
There are numerous concerns with using a DHT backend.
* The representation is likely to be RAM intensive, requiring KBytes
  per atom, and thus causing trouble when datasets exceed tens of
  millions of Atoms. Single DHT nodes cannot really hold all that much.
  To add insult to injury, OpenDHT is effectively an in-RAM database,
  so it is competing for RAM with the AtomSpace itself.  Perhaps the
  AtomSpace should be used only to "seed" a DHT node: if a DHT node
  does not have an atom, it should look to see if there's an attached
  AtomSpace, and if the AtomSpace is holding it.
* There is no backup-to-disk; thus, a total data loss is risked if
  there are large system outages.  This is a big concern, as the
  initial networks are unlikely to have more than a few dozen nodes.
  (The data should not be mixed into the global DHT...)
* How will performance compare with traditional distributed databases
  (e.g. with Postgres?) For the moment, it seems quite good, when
  working with a local DHT node. This makes sense: its entirely in
  RAM, and thus avoids disk I/O bottlenecks.
* There appears to be other hard-coded limits in the OpenDHT code,
  preventing large datasets from being stored. This includes limits
  on the number of values per key (`MAX_VALUES`), a limit on the
  total size of a node (`MAX_HASHES`) and others. It might be possible
  to work around these.  Other limits we've hit include:
  `RX_QUEUE_MAX_SIZE` and `RX_QUEUE_MAX_DELAY`.
* The limit on DHT-values-per-DHT-key is a critical limit on the
  maximum AtomSpace size; for a given AtomSpace, each Atom corresponds
  to a DHT-value attached to the Atomspace name hash. This is how we
  find all the members of an AtomSpace.  This same limit affects the
  incoming set; the same mechanism is used to find the incoming set of
  an atom.
* If many users use a shared network and publish hundreds or thousands
  of datasets, then how do we avoid accumulating large amounts of cruft,
  and sweep away expired/obsolete/forgotten datasets? Long lifetimes
  for Atoms threaten this.  I guess that, in the end, for each dataset,
  there always needs to be a seeder, e.g. working off of Postgres? As
  otherwise, we want to keep Atom lifetimes small-ish, so that junk on
  the net eventually expires.

## Build Prereqs

 * Clone and build the [AtomSpace](https://github.com/opencog/atomspace).
 * Install OpenDHT. On Debian/Ubuntu:
   ```
   sudo apt install dhtnode libopendht-dev
   ```
   The above might not be enough; we need recent (Nov 2019) versions of
   OpenDHT. Get these from github, as below:

 * The newest versions of OpenDHT contain bug fixes that we need.
   This includes a fix for shut-down, and a fix for rate-limiting.
   ```
   git clone https://github.com/savoirfairelinux/opendht
   cd opendht
   ./autogen.sh --no-configure
   mkdir build; cd build
	../configure
   make -j
   ```

## Building
Building is just like that for any other OpenCog component.
After installing the pre-reqs, do this:
```
   mkdir build
   cd build
   cmake ..
   make -j
   sudo make install
```
Then go through the [examples](examples) directory. Run unit tests as
usual:
```
   make -j test
```

## Running and Examples
Please see the [examples](examples) directory. These show how to store
individual Atoms into the DHT, how to fetch them back out, and how to
run a DHT node so that saved values are retained even after individual
AtomSpace clients detach from the network.

In particular, these will illustrate how to run a DHT node, and how to
go about looking at DHT data as needed, for debugging and development.

## Development and Debugging
Assorted notes:

* DHT logging is going to the file `atomspace-dht.log`.
