## Compilation

Compile the server:

    g++ -std=c++17 -pthread -Iinclude src/server/flexql_server.cpp src/storage/database.cpp src/storage/table.cpp src/parser/parser.cpp src/index/hash_index.cpp -o build/server

Compile the client:

    g++ -std=c++17 src/client/flexql_client.cpp src/client/flexql_api.cpp -o build/client

Compile the benchmark:

    g++ -std=c++17 -Iinclude src/client/flexql_api.cpp src/benchmark/benchmark_flexql.cpp -o build/benchmark

---

## Running the System

### Step 1: Start the Server (MANDATORY)

    ./build/server

The server must always be running before executing any benchmark or client commands.

---

### Step 2: Run Benchmark (in another terminal)

Run unit tests:

    ./build/benchmark --unit-test

Run performance benchmark (example: 10 million rows):

    ./build/benchmark 10000000

You can change the number to test different dataset sizes.

---

### Step 3: Run Client (optional, in another terminal)

    ./build/client

This opens an interactive SQL-like interface where you can run queries like:

    CREATE TABLE TEST(ID DECIMAL, NAME VARCHAR(64), BALANCE DECIMAL, EXPIRES_AT DECIMAL);
    INSERT INTO TEST VALUES (1, 'Alice', 1000, 1893456000);
    SELECT * FROM TEST;

---

## Important Note

**Before running the benchmark, make sure to delete all existing data files (data/ folder).**

If old files are not removed:
- Tables may already exist → causing CREATE TABLE errors  
- Data will be appended to existing tables → leading to incorrect results  
- Unit tests may fail even if the implementation is correct  

Always start with a clean data directory before benchmarking:

    rm -rf data/

---
