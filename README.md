# atomspace-dht
[OpenDHT](https://github.com/savoirfairelinux/opendht/wiki)
backend driver to the
[AtomSpace](https://github.com/opencog/atomspace) (hyper-)graph database.

The code here is a backend driver to the AtomSpace graph database,
enabling AtomSpace contents to be shared via the OpenDHT content
distribution network.

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

## Alpha version 0.1.2
Most things mostly work. See the [examples](examples). Some unit tests
pass. There are several show-stopper or near-show-stopper issues
preventing further development; see the issues list below.

### Status
In the current implementation:
 * OpenDHT appears to be compatible with the requirements imposed by
   the AtomSpace backend API. It seems to provide most of the services
   that the AtomSpace needs. This makes future development and
   operation look promising.
 * Despite this, there are several serious issues that are roadblocks
   to further development. These are listed below.
 * The implementation is almost feature-complete.  Missing are:
    + Ability to delete Atoms.
    + Rate-limiting issues leading to missing data.
    + Inability to flush pending output to the network.
 * All seven unit tests have been ported over (from the original
   SQL backend driver tests) currently four of seven pass. The below
   (usually) pass; when they fail, its due to rate-limiting.
```
1 - BasicSaveUTest
2 - ValueSaveUTest
3 - PersistUTest
6 - MultiPersistUTest
```
 * The failing tests are:
   + `7 - MultiUserUTest` crashes with bizarre realloc bug. One
     report is
     [bug#38041 in guile](https://debbugs.gnu.org/cgi/bugreport.cgi?bug=38041).
   + `4 - FetchUTest` has errors w/ resetting truth values back to
     default.
   + `5 - DeleteUTest` does not pass due to missing delete abilities.


### Architecture
This implementation will provide a full, complete implementation of the
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
* How to delete entries? Seems that every Atom will need to be
  tagged with a version number (a timestamp) so that Atoms that
  are deleted and then restored can be placed in proper
  chronological order. CRDT seems like overkill.
* TODO: Enhancement: listen for new values on specific atoms
  or atom types.
* TODO: Enhancement: listen for atomspace updates.
* TODO: Enhancement: implement a CRDT type for `CountTruthValue`.
* TODO: Defer fetches until barrier. The futures can be created
  and then queued, until the time that they really need to be
  resolved.
* TODO: Enhancement: provide utilities that return hashes of the
  Atoms, Incoming Sets, values, so that manual exploration of the
  DHT contents is easier.

### Issues
The following are serious issues, some of which are nearly
show-stoppers:

* Rate limiting causes published data to be discarded.  This is
  currently solved with a `std::this_thread::sleep_for()` in several
  places in the code. See
  [opendht issue #460](https://github.com/savoirfairelinux/opendht/issues/460)
  for details. This is a serious issue, and makes the unit tests
  fairly unreliable, as they become timing-dependent and racy.
  (This is effectively a show-stopper.)
* There does not seem to be any way of force-pushing local data out
  onto the net, (for synchronization, e.g. for example, if it is known
  that the local node is going down. See
  [opendht issue #461](https://github.com/savoirfairelinux/opendht/issues/461)
  for details. This is effectively a show-stopper, as it makes it
  impossible to safely terminate a running node with local data in it.
* There does not seem to be any reliable way of deleting data. See
  [opendht issue #429](https://github.com/savoirfairelinux/opendht/issues/429)
  for details. A hacky work-around might be to publish timestamps
  along with the delete orders, so that the latest status can be
  known. This hack should work, although its ugly.
* There does not seem to be any way of keeping only the latest values;
  a whole cruft of them accumulate. See
  [opendht issue #462](https://github.com/savoirfairelinux/opendht/issues/462)
  for details.

## Build Prereqs

 * Clone and build the [AtomSpace](https://github.com/opencog/atomspace).
 * Install OpenDHT. On Debian/Ubuntu:
   ```
   sudo apt install dhtnode libopendht-dev
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
Then go through the [examples](examples) directory.
