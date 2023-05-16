#include<iostream>
#include<string>
#include<pqxx/pqxx>
#include<cassert>
#include<thread>

#include "ServerDB.hpp"

int main() {
    std::thread tr(DB::Handler);

    tr.join();
 return 0;
}