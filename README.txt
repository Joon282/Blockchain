
1. ENVIRONMENT DETAILS


This program was developed and tested in the following environment:

OS: Developed on Windows 11, Tested on Linux (Ubuntu)
Architecture: x86-64
Compiler: g++ (GNU Compiler Collection)
Compiler Version: g++ 13.2.0
C++ Standard: C++ 17

2. LIBRARIES REQUIRED

The program uses standard C++ libraries, POSIX networking APIs and external cryptographic libraries:

  // C++ Standard Library
  #include <algorithm>
  #include <atomic>
  #include <arpa/inet.h>
  #include <cstring>
  #include <fcntl.h>
  #include <fstream>
  #include <functional>
  #include <iomanip>
  #include <iostream>
  #include <mutex>
  #include <netinet/in.h>
  #include <queue>
  #include <sstream>
  #include <stdexcept>
  #include <string>
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <thread>
  #include <unordered_map>
  #include <unordered_set>
  #include <vector>

EXTERNAL DEPENDENCIES:
  - nlohmann/json - JSON serialisation
  - Boost - C++ libraries for networking and utilities
  - OpenSSL - Cryptographic operations and SSL/TLS support

3. COMPILATION INSTRUCTIONS

To compile the blockchain program, run:

  g++ -std=c++17 -pthread -lboost_system -lssl -lcrypto -o blockchain main.cpp node.cpp net.cpp consensus.cpp hasher.cpp validator.cpp

4. RUNNING THE PROGRAM

Execute the program using the provided Run.sh script:

  ./Run.sh <Port> <Peer-Config-File>

EXAMPLE:
  ./Run.sh 6000 configA.txt

CONFIGURATION FILE FORMAT:

configA.txt:
  localhost:6001
  localhost:6002
  localhost:6003


5. SIMULATING A NETWORK

To simulate a complete blockchain network, run multiple instances of the 
program in separate terminals:

Terminal 1:
  ./Run.sh 6000 configA.txt

Terminal 2:
  ./Run.sh 6001 configB.txt

Terminal 3:
  ./Run.sh 6002 configC.txt


6. PROJECT STRUCTURE

  main.cpp/hpp              Entry point 
  node.cpp/hpp              Node implementation and block management
  net.cpp/hpp               Peer-to-peer networking
  consensus.cpp/hpp         Consensus mechanism 
  hasher.cpp/hpp            Cryptographic hashing for blocks and transactions
  validator.cpp/hpp         Transaction and block validation

