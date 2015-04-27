/* Copyright 2013 Active911 Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http: *www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Log to stdout
#define _DEBUG(x) 										\
	do { 												\
		std::cout << "  (" << x << ")" << std::endl;	\
	} while(0)


#include <iostream>
#include <string>
#include <cassert>
#include <pthread.h>
#include <boost/shared_ptr.hpp>
#include "../DummyConnection.h"

using namespace active911;
using namespace std;

boost::shared_ptr<ConnectionPool<DummyConnection> >pool;

void *getConnection(void *) {
	boost::shared_ptr<DummyConnection> conn=pool->borrow();
	return NULL;
}

int main(int argc, char **argv) {

	// Create a pool of 2 dummy connections
	cout << "Creating connections..." << endl;
	boost::shared_ptr<DummyConnectionFactory>connection_factory(new DummyConnectionFactory());
	pool = boost::shared_ptr<ConnectionPool<DummyConnection> >(new ConnectionPool<DummyConnection>(2, connection_factory,2,2));
	ConnectionPoolStats stats=pool->get_stats();
	assert(stats.pool_size==2);

	{

		// Get both available connections
		boost::shared_ptr<DummyConnection> conn1=pool->borrow();
		boost::shared_ptr<DummyConnection> conn2=pool->borrow();

		// Trying to get a third should throw
		cout << "Checking for exception when pool is empty..." << endl;
		bool exception_thrown=false;
		try {

			boost::shared_ptr<DummyConnection> conn3=pool->borrow();		

		} catch (std::exception& e) {

			cout << "Exception thrown (intentional)" << endl;
			exception_thrown=true;
		}
		assert(exception_thrown);

		// Trying to get a connection in child-thread when main thread unbrrow connection
		pthread_t thread;
		pthread_create(&thread,NULL,getConnection,NULL);
		sleep(3);
		pool->unborrow(conn2);
		pthread_join(thread,NULL);


		// Release one, and make sure it was repatriated (no exception)
		cout << "Clean relase and re-borrow 1 connection" << endl;
		pool->unborrow(conn1);
		conn1=pool->borrow();		// Should not throw
	}

	// We never released pool 2, but it went out of scope.  2 connections should once again be available...
	cout << "Dirty relase and re-borrow 2 connections.  Will cause destruction of old connections" << endl;
	boost::shared_ptr<DummyConnection> conn1=pool->borrow();		
	boost::shared_ptr<DummyConnection> conn2=pool->borrow();		

	stats=pool->get_stats();
	assert(stats.pool_size==0);

	cout << "Clean relase all connections" << endl;

	return 0;
}

