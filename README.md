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
[key-value store](https://wiki.opencog.org/w/Value).
The Atomspace has a variety of advanced features not normally found
in ordinary graph databases, including an advanced query language
and "active" Atoms.

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
giant AtomSpace datasets.  At least, that is the general hope. How
well this might work out is unknown.

Quite unclear is how OpenDHT fetch and eviction work, when local
processing is creating hot-spots in the datasets. This will be the
interesting technical and theoretical question to explore with this
driver.  Closely tied to this are questions of RAM usage and efficiency.
These are confusing and unknown. They're counter-balanced by the dream
of some super-giant AtomSpace pie-in-the-sky with millions of users.

## Proof-of-concept version 0.2.1
All core functions are implemented. They work, on a small scale, for
small datasets.  See the [examples](examples) for a walk-through. Most
unit tests usually pass (several generic OpenDHT issues, unrelated to
this backend, prevent a full pass). Many desirable enhancements are
missing; performance is terrible, and that is a huge issue. There are
several show-stopper or near-show-stopper issues preventing further
development; see the issues list below, and also the architecture
concerns list.

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
   is unknown, but seems to involve problems of flushing out data from
   the local node, out to the network, and having the network receive
   and hold that data.
 * The consistently failing tests are:
   + `8 - LargeFlatUTest` attempts a "large" atomspace (of only 35K Atoms,
          so actually, it's small, but bigger than the other tests).
          Runs impossibly slowly, (> 10 hours) and hits hard-coded
          limits on OpenDHT.
   + `9 - LargeZipfUTest` attempts a "large" atomspace w/ Zipfian
          incoming set sizes.  Runs impossibly slowly, likely to break
          hard-coded limits on OpenDHT.

### Architecture
This implementation provides a full, complete implementation of the
standard `BackingStore` API from the Atomspace. Its a backend driver.

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
* How can we find all current members of an AtomSpace?
  Easy, the AtomSpace is just one hash, and the atoms in it are
  DHT values.
* How can we find all members of the incoming set of an Atom?
  Easy, we generate the hash for that atom, and then look at
  all the DHT entries on it.
* How to delete entries? Atoms in the AtomSpace are tagged with
  a timestamp and an add/drop verb, so that precedence is known.
  An alternate design using CRDT seems like overkill.
* TODO: gets()'s need to be queued so that they can run async,
  and then call handlers when completed. I think futures w/callbacks
  will solve this.
* TODO: Optionally use crypto signatures to verify that the data
  comes from legitimate, cooperating sources.
* TODO: Change the default one-week data expiration policy.  Not
  sure what to do, here.
* TODO: Support read-write overlays on top of read-only datasets.
  This seems like it should be easy...
* TODO: Enhancement: listen for new values on specific atoms
  or atom types.
* TODO: Enhancement: listen for atomspace updates.
* TODO: Enhancement: implement a CRDT type for `CountTruthValue`.
* TODO: Defer fetches until barrier. The futures can be created
  and then queued, until the time that they really need to be
  resolved.
* TODO: Measure total RAM usage.  This risks being quite the
  memory hog, if datasets with hundreds of millions of atoms are
  published.
* TODO: Create a "seeder", that maintains the AtomSpace in Postgres,
  listens for load requests and responds by seeding those Atoms into
  DHT if they are not already there.  Likewise, in a read-write mode,
  it listens for updates, and stores them back into the database.
  Ideally, this seeder can use the existing AtomSpace Postgres backend
  code with zero modifications, and just act as a bridge between two
  backends.

### Issues
The following are serious issues, some of which are show-stoppers:

* There are data loss issues; it appears that, for some reason, data
  from a local node is never pushed out to the bigger network, and
  thus, after the local node closes, that data cannot be retrieved later.
  The root cause of this is unknown.  This issue can be mostly avoided
  by adding `std::this_thread::sleep_for()` in several places in the code.
  Dropped data is a show-stopper; data storage MUST be reliable!
  The [opendht issue #461](https://github.com/savoirfairelinux/opendht/issues/461)
  seems to capture some of this; although it is now marked resolved,
  the intermittent data loss continues. Don't know why.
* Hard-coded limits on various OpenDHT structures. See
  [opendht issue #426](https://github.com/savoirfairelinux/opendht/issues/426)
* Its possible to hang the system. Unclear where the hang is from.
  e.g. Running
  ```
   (dht-open "dht://:4999/")
   (dht-bootstrap "dht://localhost:4555/")
  ```
  then running the unit tests (which connect to port 4555 for seeding)
  and then trying to view one of the atomspaces
  ```
  (display (dht-examine "a8cf8a3cb42aaa86eefbd1a9d3a827ac57d87385"))
  ```
  can result in a hang that never times out -- until the unit tests
  are re-run, so that the atomspace is re-published.
* It appears to be impossible to saturate the system to 100% CPU usage,
  even when running locally. This might be the reason why its slow:
  something, somewhere is blocking and taking to long; doing nothing
  at all. What this is is unknown.
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
  millions of Atoms. The OpenDHT backend might not be able to hold
  very much.
* There is no backup-to-disk; thus, a total data loss is risked if
  there are large system outages.  This is a big concern, as the
  initial networks are unlikely to have more than a few dozen nodes.
  (The data should not be mixed into the global DHT...)
* How will performance compare with traditional distributed databases
  (e.g. with Postgres?) Current performance is poor, and the reason for
  this is entirely unclear.
* There appears to be other hard-coded limits in the OpenDHT code,
  preventing large datasets from being stored. This includes limits
  on the number of values per key. It might be possible to work around
  this, but only with a fair amount of extra code and extra complexity.
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

 * Better yet, get the latest version, which contains bug fixes that
   we depend on (This includes a fix for shut-down).
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
