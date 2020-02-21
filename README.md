# atomspace-dht
[OpenDHT](https://github.com/savoirfairelinux/opendht/wiki)
backend driver to the
[AtomSpace](https://github.com/opencog/atomspace) (hyper-)graph database.

The code here is an experimental backend driver to the AtomSpace graph
database, enabling AtomSpace contents to be shared via the OpenDHT
content distribution network.  The goal is to allow efficient
decentralized, distributed operation over the global internet,
allowing many AtomSpace processes to access and perform updates to
huge datasets.

The driver more-or-less works, but is a failed experiment. A detailed
critique of the architecture is given below. As of this writing (2019),
there isn't any obvious way of resolving the architectural issues that
are raised.

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
are presented and critiqued further below. But first, status.

# Proof-of-concept version 0.2.2
All backend functions are implemented. They work, on a small scale, for
small datasets.  See the [examples](examples) for a walk-through. Most
unit tests usually pass (several generic OpenDHT issues, unrelated to
this backend, prevent a full pass). Many desirable enhancements are
missing. There is at least one coding show-stopper issue discouraging
further development; there are also multiple serious architectural
issues suggesting that the "naive" approach is inadequate for the
stated vision (see further below).

The current implementation is a full, complete implementation of the
public (generic) `BackingStore` API from the Atomspace. It's a
standard backend driver for the AtomSpace. The actual design is a naive
mapping of AtomSpace Atoms and Values onto the DHT key-value store;
thus, Atoms are directly persisted into the DHT, as DHT content.

The git repo layout is the same as that of the AtomSpace repo. Build,
unit-test and install mechanisms are the same.

All nine unit tests have been ported over (from the original SQL
backend driver tests). Currently seven of nine (usually) pass:
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

The consistently failing tests are:
 * `8 - LargeFlatUTest` attempts a "large" atomspace (of only 35K Atoms,
        so actually, it's small, but bigger than the other tests).
 * `9 - LargeZipfUTest` attempts a "large" atomspace w/ Zipfian
        incoming set sizes.

Both of these hit hard-coded limits in OpenDHT: each DHT node can only
store 64K blocks data, and these tests generates about 3x more than
that.  When this limit is hit, OpenDHT busy-waits at 100% cpu...
for a currently-unknown reason... (I can't find the limit that is
blocking this).

## Naive Architecture
The architecture of this implementation is termed "naive", because
it uses the simplest, "most obvious" mapping of the AtomSpace and
AtomSpace concepts onto a flat key-value store. It's naive, in that
it ignores most characteristics that make a DHT different from an
ordinary key-value database. It's naive in that it ignores a number
of design challenges presented by large AtomSpaces.  These are
addressed and critiqued in a following section.

The "naive" design is characterized as follows:

* Every Atom gets a unique (160-bit) hash. This is called the GUID.
  Every GUID is published to the DHT, with the hash serving as DHT-key,
  and the Atom name/outgoing-set serving as the DHT-value.  As a
  result, given only a GUID, the actual Atom can always be recreated.
* Every (Atom, AtomSpace-name) pair gets a unique (160-bit) hash.
  This is termed the MUID or "Membership UID", as it refers to
  an Atom in a specific AtomSpace.  The MUID is used as a DHT-key;
  the corresponding DHT-values are used to hold the IncomingSet,
  and the Atom-Values. Keep in mind that although Atoms are independent
  of AtomSpaces, the IncomingSets and the Atom-Values depend on the
  actual AtomSpace an Atom might be in. That is, an AtomSpace provides
  a "context" or "frame" for an Atom.
* The AtomSpace-name gets a unique (160-bit) hash. The set of all Atoms
  in the AtomSpace are stored as DHT-values on this DHT-key.
* Given an MUID, the Atoms in the IncomingSet are stored as DHT-values
  under that MUID.  This is effectively the same mechanism as finding
  all the members of an AtomSpace.
* Atom deletion presents a challenge. This is solved by tagging each
  DHT-value under the AtomSpace key with a timestamp and an add/drop
  verb.  The timestamp indicates the most recent version, in case an
  Atom is added/dropped repeatedly.

That's it. It's pretty straight-forward. The implementation is small:
less than 2.5 KLOC grand-total, including whitespace lines, comments
and boilerplate.

### Design To-Do List
The "naive" design described above suggests some "obvious" to-do list
items.  These are  listed here.

* TODO: Optionally use crypto signatures to verify that the data
  comes from legitimate, cooperating sources.
* TODO: Change the default one-week data expiration policy to
  instead be a few hours ... or even tens of minutes?  Persistent
  data still needs to live on disk, not in RAM, and to be provided
  by seeders.
* TODO: Support read-write overlay AtomSpaces on top of read-only
  AtomSpaces.  This seems like it should be easy...
* TODO: Enhancement: listen for new Atom-Values on specific Atoms,
  or for the addition/deletion of Atom in an AtomSpace.
* TODO: use `MSGPACK_DEFINE_MAP` for more efficient serialization
  of Atom-Values (esp. of FloatValue).
* TODO: Using `DhtRunner::get()` with callbacks instead of futures
  will improve performance. This is somewhat tricky, though, because
  one callback might trigger more gets (for Atom-Values, IncomingSets,
  etc.) and it's not clear if OpenDHT might deadlock in this case...
* TODO: Enhancement: implement a CRDT type for `CountTruthValue`.
* TODO: Measure total RAM usage.  How much RAM does a DHT-Atom use?
  How does this compare to the amount of RAM that an Atom uses when
  it's in the AtomSpace?
* TODO: Create a "seeder", that maintains the AtomSpace in Postgres,
  listens for load requests and responds by seeding those Atoms into
  DHT if they are not already there.  Likewise, in a read-write mode,
  it listens for updates, and stores them back into the database.
  Ideally, this seeder can use the existing AtomSpace Postgres backend
  code with zero modifications, and just act as a bridge between two
  backends.

### Implementation Issues
The following is a list of coding issues affecting the current
code-base.  Some of these are show-stoppers.  This is distinct from
the architectural issues, given in the next section.

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
  These include a limit on the number of values per key (`MAX_VALUES`),
  a limit on the total size of a node (`MAX_HASHES`), the size of
  pending (unprocessed) received data (`RX_QUEUE_MAX_SIZE`) and
  how stale/old the unprocessed data is (`RX_QUEUE_MAX_DELAY`).

  This needs to be worked around by creating a "large set" primitive,
  and using this to hold the large sets of MUID's/GUID's. Without this,
  the driver is limited to AtomSpaces of about 15K Atoms or less.

* It has not been possible to saturate the system to 100% CPU usage,
  even when running locally. The reason for this is not known.
  Presumably, some timer somewhere is waiting.  This is particularly
  visible with `MultiUserUTest`, which starts at 100% CPU and then
  drops to single-digit percentages.

* There is some insane gnutls/libnettle bug when it interacts with
  BoehmGC.  It's provoked when running `MultiUserUTest` when the
  line that creates `dht::crypto::generateIdentity();` is enabled.
  It crashes with a bizarre realloc bug. One bug report is
  [bug#38041 in guile](https://debbugs.gnu.org/cgi/bugreport.cgi?bug=38041).
  It appears that gnutls is not thread-safe... or something weird.


# Architecture
The "naive" architecture given above has numerous very serious flaws.
These are discussed here, followed by some remarks about possible
solutions.

## Architecture Critique

* What's the AtomSpace, really? Well, it's a kind of in-RAM database.
  What's the DHT, really? Well, it's a kind of in-RAM database ...
  distributed over the net. So right here, we have a problem: two
  different systems, representing the same data in two different ways,
  both competing for RAM.  Ouch. Given that existing AtomSpaces are
  already too large to fit into RAM, this is ... a problem.

* Worse: it seems unlikely that the DHT representation of an Atom is
  smaller than an AtomSpace-Atom. Currently, AtomSpace-Atoms are about
  1.5KBytes in size; this includes "everything" (indexes, caches) for
  "typical" (Zipfian-distributed) Atoms. The size of a DHT-Atom has
  not been measured, but there is no reason to think it will be much
  smaller.

* This raises a trio of problems: Who is providing this RAM?  What
  happens if most of the providers shut down, leading to a critical
  shortage of storage?  What happens if there is a loss of network
  connectivity? All this suggests that AtomSpace data really needs
  to live on disk, where specific admins or curators can administrate
  it, publish or archive it.

* Naive storage into the DHT also presents garbage-collection issues.
  What's the lifetime of an AtomSpace, if no one is using it? By
  definition, RAM is precious; letting unused junk to accumulate in
  RAM is extremely wasteful. This again points at disk storage.

* The naive implementation ignores locality issues. The association
  between Atoms and their hash keys is strongly random. Placing an
  Atom in the DHT means that it will reside on unpredictable,
  effectively random network hosts, which, in general, will be
  unlikely be close by. This completely erases the explicit graph
  relationship between Atoms: Atoms connected with links are meant
  to be literally close-to-one-another, and access patterns are such
  that close-by Atoms are far more likely to be accessed than
  unconnected ones.  This is particularly true for pattern matching
  and graph exploration, which proceeds by walking over connected
  graph regions. What seems to be need is some kind of hashing
  scheme that preserves graph locality.  For example, it would be
  very nice, excellent, even, if two graphically-close atoms had
  XOR-close hashes. But it's not clear how to obtain this.

* The naive implementation ignores scalability issues with AtomSpace
  membership. That is, the grand-total list of *all* Atoms in a given
  AtomSpace are recorded as DHT-values on a *single* DHT-key.  For
  large AtomSpaces (hundreds of millions or even billions of Atoms)
  this means that *one* DHT-key has a vast number of DHT-values attached
  to it. This is simply not scalable, as DHT's do not distribute
  (scatter) the DHT-values, only the DHT-keys. The OpenDHT codebase
  has a hard-coded limit of 1K DHT-values per DHT-key, as well as a
  limit of 64KBytes per DHT-key. To store AtomSpaces of unbounded size,
  one needs to map some other data structure, e.g. an rb-tree, onto
  the DHT. That would allow DHT-values of unbounded size to be stored,
  but at a cost.

* The naive implementation is *NOT* decentralized. Indicating the
  AtomSpace with a single DHT-key means that there is a single point of
  truth for the contents of the AtomSpace. Multiple writers are then
  in competition to add and delete Atoms into this global singleton.
  In practical applications, there is almost never any particular need
  for a single, global AtomSpace; there is instead only the need to
  obtain all Atoms of some given particular type, or to find all of the
  (nearest) neighbors of an Atom. Both of these queries are at best only
  approximate: if Atoms are added or removed over time, so be it; one
  only needs a best-effort query in some approximate time-frame.  That
  is, there is no inherent demand that the AtomSpace be a global
  singleton. The naive implementation is treating it as such, and
  causing more problems (edit contention) than it solves (query
  uniqueness). It is enough for the AtomSpace to follow "BASE"
  consistency principles, instead of "ACID" principles; the naive
  implementation is forcing ACID when its not really needed, and worse:
  forcing a global singleton.

## Architectural Alternatives

How might some of the above problems be solved?

* Disk storage suggests that, in the absence of any active dataset
  users, any given AtomSpace should be "seeded" by a small number of
  nodes holding an on-disk copy of the AtomSpace.  This can be
  implemented in several ways, none of which seem particularly
  appealing.

  + One is to use the BitTorrent model, where the only thing the
    DHT holds are the addresses of the "peers" who are serving the
    content. The content is served via distinct, independent channels.
    At the time of this writing (2019), there isn't any code wherein
    one running AtomSpace can serve some of it's content to another
    running AtomSpace.

  + Another possibility is to keep the existing naive architecture,
    but allow DHT nodes to respond to requests by pulling in data off
    a disk. At the time of this writing (2019), OpenDHT doesn't expose
    a public API to accomplish this.  Nor is there any low-brow,
    dirt-simple way of storing (caching) Atoms into files.  Never mind
    that this doesn't solve any of the other problems: garbage-collection,
    locality, scalability.

* Locality.  A given, active running instance of an AtomSpace is likely
  to already hold most of the locally-connected Atoms that another
  user may want to get access to. Thus, the question is: how can a
  running AtomSpace publish a list of Atoms that it holds, such that
  others can find it?

  + The "BitTorrent" solution for this would be to let each Atom
    hash down to an MUID, as before, but the contents would be
    locators for the peers that are serving that Atom. This seems
    plausible, except for the fact that it's still incredibly
    RAM-hungry. An AtomSpace containing 100 million Atoms has to
    publish 100 million keys, all pointing back to itself. The
    size of the locator is not all that much smaller than the size
    of the Atom itself. Atoms are *small*.  This solution also
    has lifetime (garbage collection) issues: how long should the
    published records stay valid? If they expire quickly, then
    how can the holders of any particular Atom be found?  This
    solution is again "naive".

* Set membership.  How can one publish the contents of a large set,
  a set of unbounded size?  As hinted above, what is really needed
  is a decentralized set-membership function.

### Other Backends
All of these design issues suggest that it's entirely reasonable to let
"someone else" solve these problems: i.e. some existing distributed
database system.  For example, the AtomSpace already has a mature,
stable Postgres driver, and Postgres can be run on large, geographically
distributed clusters with petabytes of attached storage. What's wrong
with that? (What's wrong with that is that it's slow.)

Alternately, one could write AtomSpace back-ends for Apache Ignite, or
maybe even some other graph database (Redis, Riak, Grakn) that solves
some/many/most of these distributed database issues.

This is not to be sneezed at: Postgres provides a mature, proven
technology. The others have large developer bases. Displacing them
with naive grow-your-own dreams is not be easy.

In the end, the primary problem with having the AtomSpace layered on
top of another system is that Atoms have to be translated between
multiple representations: one representation for disk storage, another
for network communication, and the third, most important one: the
in-RAM AtomSpace itself, which is optimized for pattern-matching.
The hunt is for an optimal system, maximizing performance.

# Build, Test, Install, Examples
Practical matters.

### Build Prereqs

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

### Building
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

### Running and Examples
Please see the [examples](examples) directory. These show how to store
individual Atoms into the DHT, how to fetch them back out, and how to
run a DHT node so that saved values are retained even after individual
AtomSpace clients detach from the network.

In particular, these will illustrate how to run a DHT node, and how to
go about looking at DHT data as needed, for debugging and development.

### Development and Debugging
Assorted notes:

* DHT logging is going to the file `atomspace-dht.log`.
  See also `atomspace-dhtnode.log`.
* Always remember to bootstrap. You can "listen" to the unit tests
  by bootstrapping to `(dht-bootstrap "dht://localhost:4555")`,
  which is the port-number used by the unit tests.
