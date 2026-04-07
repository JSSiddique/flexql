================ FLEXQL COMPILATION & EXECUTION =================

1. Compile Server

g++ -std=c++17 -pthread -Iinclude src/server/flexql_server.cpp src/storage/database.cpp src/storage/table.cpp src/parser/parser.cpp src/index/hash_index.cpp -o build/server

2. Compile Client

g++ -std=c++17 src/client/flexql_client.cpp src/client/flexql_api.cpp -o build/client

3. Compile Benchmark

g++ -std=c++17 -Iinclude src/client/flexql_api.cpp src/benchmark/benchmark_flexql.cpp -o build/benchmark


================ RUNNING THE SYSTEM =================

Step 1: Start the Server (MANDATORY)

./build/server

The server must always be running before executing any client or benchmark commands.


Step 2: Run Benchmark (in another terminal)

Unit Testing:
./build/benchmark --unit-test

Performance Testing:
./build/benchmark 10000000

(You can change the number to test different dataset sizes)


Step 3: Run Client (optional, in another terminal)

./build/client

This opens an interactive SQL-like interface where you can manually run queries such as:
CREATE TABLE, INSERT, SELECT, etc.


================ IMPORTANT NOTE =================

**Before running the benchmark, make sure to delete all existing data files (data/ folder).**

**If old files are not removed:**
- Tables may already exist → causing CREATE TABLE errors
- Data will be appended to existing tables → leading to incorrect results
- Unit tests may fail even if the implementation is correct

**Hence, always start with a clean data directory before benchmarking.**

================================================================