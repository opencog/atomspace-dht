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
#if PUT
	// put some data on the dht
	node.put(akey, "(some test stuff)");
	printf("done put stuff\n");
#endif

	// put some data on the dht, signed with our generated private key
	// node.putSigned("unique_key_42", some_data);

#define GET 1
#if GET
	// get data from the dht
	dht::GetCallback mycb =
		[](const std::vector<std::shared_ptr<dht::Value>>& values)->bool
		{
			std::cout << "Num values found= " << values.size() << std::endl;
			// Callback called when values are found
			for (const auto& value : values)
				std::cout << "Found value: " << *value << std::endl;
			return true; // return false to stop the search
		};

	dht::GetCallbackSimple scb =
		[](std::shared_ptr<dht::Value> value)->bool
		{
			std::cout << "Found value: " << *value << std::endl;
			return true; // return false to stop the search
		};

	dht::DoneCallback donecb =
		[](bool foo, const std::vector<std::shared_ptr<dht::Node> >& noo)
		{
			std::cout << "Done callback " << foo << std::endl;
			// std::cout << "Done callback " << noo << std::endl;
		};

	node.get(akey, mycb, donecb);
	// node.get("atom_key", mycb);
	printf("done asking for stuff\n");
#endif

	sleep(10);
	// std::cout << "Searches: " << node.getSearchesLog() << std::endl;

	// wait for dht threads to end
	node.join();
	return 0;
}
