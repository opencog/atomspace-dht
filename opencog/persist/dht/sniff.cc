/* Sniff test.
 See if anything works.
 */

#include <unistd.h>
#include <iostream>
#include <opendht.h>

int main (int, const char **)
{
	dht::DhtRunner node;

	// Launch a dht node on a new thread, using a
	// generated RSA key pair, and listen on port 4222.
	node.run(4223, dht::crypto::generateIdentity(), true);

	// Join the network through any running node,
	// here using a known bootstrap node.
	// node.bootstrap("bootstrap.ring.cx", "4222");
	node.bootstrap("localhost", "4222");

	std::cout << "My node id: " << node.getId() << std::endl;
	std::cout << "My DHT node id: " << node.getNodeId() << std::endl;
	// std::cout << "Node info: " << node.getNodeInfo() << std::endl;

	dht::InfoHash akey = dht::InfoHash::get("atom_key");
	std::cout << "Key's this: " << akey << std::endl;
#if 0
	// put some data on the dht
	node.put(akey, "(some test stuff)");
printf("duude put stuff\n");
#endif

	// put some data on the dht, signed with our generated private key
	// node.putSigned("unique_key_42", some_data);

	// get data from the dht
	dht::GetCallback mycb =
		[](const std::vector<std::shared_ptr<dht::Value>>& values)
		{
			std::cout << "Num values found= " << values.size() << std::endl;
			// Callback called when values are found
			for (const auto& value : values)
				std::cout << "Found value: " << *value << std::endl;
			return true; // return false to stop the search
		};

	printf("sleep\n");
	sleep(10);
	printf("wake\n");
	node.get(akey, mycb);
	printf("and again\n");
	node.get("atom_key", mycb);
printf("duude got stuff\n");

	std::cout << "Searches: " << node.getSearchesLog() << std::endl;

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
