#include<iostream>
#include<string>
#include<pqxx/pqxx>
#include<cassert>
#include<thread>

#include "ServerDB.hpp"

int main() {
    std::thread tr(DB::Handler);

    std::cout << DB::Authorization("vo", "vo") << std::endl;

    DB::ConsumersData data;

    data.priority = 3;
    data.id  = 3;
    data.consumer_name = "ye";
    data.status  = 3;
    std::cout << DB::InsertClientGroup("vo", "vo", data) << std::endl;



    tr.join();
 return 0;
}