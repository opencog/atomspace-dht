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
OpenDHT is an internet-wide globally-accesible storage system, providing
a variety of distributed hash table services.  It provides decentralized
storage of data.

## Alpha version 0.1.1
Most things mostly work. See the [examples](examples). Some unit tests
pass.

### Status
In the current implementation:
 * OpenDHT appears to be highly compatible with the requirements
   imposed by the AtomSpace backend API. It seems to provide exactly
   those services that the AtomSpace needs. This makes future
   development and operation look very promising.
 * System is almost feature-complete.  Missing are:
    + Atom deletion
    + A stable server
 * All seven unit tests have been ported over (from the original
   SQL backend driver tests) currently three of seven pass.

### Architecture
This implementation will provide a full, complete implementation of the
standard `BackingStore` API from the Atomspace. Its a backend driver.

The git repo layout is the same as that of the AtomSpace repo. Build
and install mechanisms are the same.

### Design Notes
* Every Atom gets a unique hash. This is called the GUID.
  Every GUID is published, because, givin only a GUID,
  there needs to be a way of finding out what the Atom is.
  This is needed for IncmingSet fetch, for example.
* Every (Atom, AtomSpace-name) pair gets a unique hash.
  The current values on that atom, as well as it's incoming set
  are stored under that hash.
* How can we find all current members of an AtomSpace?
  Easy, the AtomSpace is just one hash, and the atoms in it are
  DHT values.
* How can we find all members of the incoming set of an Atom?
  Easy, we generate the hash for that atom, and then look at
  all the DHT entries on it.
* TODO: listen for new values on specific atoms or atom types
* TODO: list for atomspace updates.
* TODO: implement a CRDT type for CountTruthValue
* TODO: flush writes before closing.
* TODO: defer fetches until barrier.
* TODO: hash explore utility

### Issues
* There does not seem to be any reliable way of deleting data. See
  [opendht issue #429](https://github.com/savoirfairelinux/opendht/issues/429)
  for details.
* Rate limiting causes published data to be discarded.  This is
  currently solved with a `std::this_thread::sleep_for()` in several
  places in the code. See
  [opendht issue #460](https://github.com/savoirfairelinux/opendht/issues/460)
  for details.
* There does not seem to be any way of force-pushing local data out
  onto the net, (for synchronization, e.g. for example, if it is known
  that the local node is going down. See
  [opendht issue #461](https://github.com/savoirfairelinux/opendht/issues/461)
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
