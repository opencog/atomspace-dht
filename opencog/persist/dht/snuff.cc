/* Sniff test.
 WTF threading!?
 */

#include <unistd.h>
#include <atomic>
#include <cstdio>
#include <iostream>
#include <thread>
#include <opendht.h>

dht::DhtRunner seeder;

#define NUSERS 16
dht::DhtRunner runner[NUSERS];
std::atomic<int> nstarted;
int n_threads = NUSERS;

void add_atoms(int thread_id)
{
	std::string astr = "add " + std::to_string(thread_id);
	dht::InfoHash space_hash = dht::InfoHash::get("foobar");

	for (int j=0; j< 5000; j++)
	{
		std::string fstr = astr + std::to_string(j);
		runner[thread_id].put(space_hash, dht::Value(0, fstr));
	}
}

void run_user_test(int thread_id)
{
	printf("begin creating thread %d\n", thread_id);
	// if (0 < thread_id) sleep(1);

	dht::DhtRunner::Config _config;
	_config.dht_config.node_config.network = 42;
	_config.dht_config.id = dht::crypto::generateIdentity();
   _config.threaded = true;
	runner[thread_id].run(4666+thread_id, _config);
	runner[thread_id].bootstrap("localhost", "5454");

	nstarted.fetch_add(1);
	printf("done creating thread %d\n", thread_id);

	// Spin, until they are all started.
	while (n_threads != nstarted.fetch_add(0)) std::this_thread::yield();

	printf("start work thread %d\n", thread_id);
	// work some
	add_atoms(thread_id);
	runner[thread_id].join();
	printf("join work thread %d\n", thread_id);
}

int main (int, const char **)
{
	dht::DhtRunner::Config _config;
	_config.dht_config.node_config.network = 42;
	_config.dht_config.id = dht::crypto::generateIdentity();
   _config.threaded = true;

	seeder.run(5454, _config);

	nstarted = 0;
	std::vector<std::thread> thread_pool;
	for (int i=0; i < n_threads; i++)
		thread_pool.push_back(std::thread(run_user_test, i));

	for (std::thread& t : thread_pool) t.join();
	printf("Done joining threads\n");

	dht::InfoHash space_hash = dht::InfoHash::get("foobar");
	auto gfut = seeder.get(space_hash);
	gfut.wait();
	auto gvals = gfut.get();
	printf("Obtained %lu vals\n", gvals.size());
	for (const auto& gv: gvals)
	{
		std::string satom = gv->unpack<std::string>();
		printf("Found %s\n", satom.c_str());
	}

	return 0;
}
