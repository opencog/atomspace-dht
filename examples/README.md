
Examples
========
Example use of the DHT backend.

* [basic-store.scm](basic-store.scm) -- Storing Atoms.
* [basic-fetch.scm](basic-fetch.scm) -- Fetching Atoms.
* [dht-node.scm](dht-node.scm) -- Running a stand-alone node.

Note that the second demo will fail to fetch the Atoms stored in the
first demo, if you do not run a bootstrap node. The boostrap node is
needed to hold (seed) the AtomSpace data, when no one else is connected.
We cannot use the main OpenDHT network for this, as it defaults to
holding on to data for only ten minutes!  Please read the demos
carefully for details.
