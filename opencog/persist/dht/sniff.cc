/* Sniff test.
 See if anything works.
 */

#include <iostream>
#include <opendht.h>

int main (int, const char **)
{
	dht::DhtRunner node;

	// Launch a dht node on a new thread, using a
	// generated RSA key pair, and listen on port 4222.
	node.run(4222, dht::crypto::generateIdentity(), true);

	// Join the network through any running node,
	// here using a known bootstrap node.
	node.bootstrap("bootstrap.ring.cx", "4222");

	// put some data on the dht
	node.put("atom_key", "(some test stuff)");

	// put some data on the dht, signed with our generated private key
	// node.putSigned("unique_key_42", some_data);

#if 0
	// get data from the dht
	node.get("other_unique_key", [](const std::vector<std::shared_ptr<dht::Value>>& values) {
		// Callback called when values are found
		for (const auto& value : values)
			std::cout << "Found value: " << *value << std::endl;
		return true; // return false to stop the search
	});
#endif

	// wait for dht threads to end
	node.join();
	return 0;
}
