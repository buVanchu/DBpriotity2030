#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <mutex>

#include "ServerDB.hpp"

#define t_s std::to_string

ThreadSavedQueue<std::shared_ptr<DB::Request>> g_ThSdQu;

pqxx::connection g_connection("dbname=ServerDB user=postgres password=1234");


void DB::UserListTable::AddToTable(std::string &loggin, std::string &password) {   
    pqxx::work txn(g_connection);
    pqxx::result res = txn.exec("SELECT id FROM " + table_name + " ORDER BY id DESC LIMIT 1");

    int last_user_id = res[0][0].as<int>();

    txn.exec("INSERT INTO " + table_name + " (id, loggin, password) \
    VALUES (" +
             std::to_string(++last_user_id) + ", '" + loggin + "', '" + password + "')");
    
    txn.exec("create table User" + std::to_string(last_user_id) + "_ ( \
    SolarV INT, SolarA INT, SolarStatus INT, \
    WindV INT, WindA INT, WindStatus INT, \
    genV INT, genA INT, genStatus INT, \
    batV INT, \
    V1 INT, A1 INT, Status1 INT, \
    V2 INT, A2 INT, Status2 INT, \
    V3 INT, A3 INT, Status3 INT \
    )");

    txn.commit();
}

bool DB::UserListTable::IsUserExist(std::string &loggin, std::string &password) {   
    pqxx::work txn(g_connection);
    pqxx::result res = txn.exec("SELECT * FROM " + table_name + " WHERE loggin = '" +
                   loggin + "' and password = '" + password + "'");

    txn.commit();

    if (res.size() == 0)
    {
        return false;
    }

    return true;
}

int DB::UserListTable::GetID(std::string &loggin, std::string &password)
{   
    pqxx::work txn(g_connection);
    pqxx::result res = txn.exec("SELECT id FROM " + table_name + " WHERE loggin = '" +
                   loggin + "' and password = '" + password + "'");
    txn.commit();
    return res[0][0].as<int>();
}

void DB::UserESPTable::AddToTable(ESPCondition &cond) {   
    pqxx::work txn(g_connection);

    txn.exec("INSERT INTO " + table_name + "_ \
    VALUES (  '" +
              t_s(cond.SolarV) + "', '" + t_s(cond.SolarA) + "', '" + t_s(cond.SolarStatus) + "', '" +
              t_s(cond.WindV) + "', '" + t_s(cond.WindA) + "', '" + t_s(cond.WindStatus) + "', '" +
              t_s(cond.genV) + "', '" + t_s(cond.genA) + "', '" + t_s(cond.genStatus) + "', '" +
              t_s(cond.batV) + "', '" +
              t_s(cond.V1) + "', '" + t_s(cond.A1) + "', '" + t_s(cond.Status1) + "', '" +
              t_s(cond.V2) + "', '" + t_s(cond.A2) + "', '" + t_s(cond.Status2) + "', '" +
              t_s(cond.V3) + "', '" + t_s(cond.A3) + "', '" + t_s(cond.Status3) +
              "')");
    txn.commit();
}

template <typename T>
ThreadSavedQueue<T>::ThreadSavedQueue(const ThreadSavedQueue &that) {
    std::lock_guard<std::mutex> lock(that.MyMutex);
    Data = that.Data;
}

template <typename T>
void ThreadSavedQueue<T>::push(const T &new_Data) {
    std::lock_guard<std::mutex> lock(MyMutex);
    Data.push(new_Data);
    MyCond.notify_one();
}

template <typename T>
T& ThreadSavedQueue<T>::pop() {
    std::unique_lock<std::mutex> lock(MyMutex);
    MyCond.wait(lock, [this]
                { return !Data.empty(); });

    T &ret_value = Data.front();
    Data.pop();

    return ret_value;
}

void DB::Handler() {
    DB::UserListTable user_list(std::string("user_list"));

    std::unordered_map<int, DB::UserESPTable> connected_users; // need to make a deletion of an unconnected users from map

    while (true) {
        auto req = g_ThSdQu.pop();

        switch (req->type())
        {
        case DB::InsertUser_:
            if (user_list.IsUserExist(req->user().loggin, req->user().password))
            {
                req->set_response(DB::Response(false));
            }
            else
            {   
                user_list.AddToTable(req->user().loggin, req->user().password);
                req->set_response(DB::Response(true));
            }
            break;

        case DB::Authorization_:
            if (user_list.IsUserExist(req->user().loggin, req->user().password))
            {
                req->set_response(DB::Response(true));
                DB::UserESPTable table(user_list.GetID(req->user().loggin, req->user().password));
                connected_users[user_list.GetID(req->user().loggin, req->user().password)] = table;
            }
            else
            {
                req->set_response(DB::Response(false));
            }
            break;

        case DB::InsertESPCondition_:
            connected_users[user_list.GetID(req->user().loggin, req->user().password)].AddToTable( // сделать вместо длинной вставки логина и пароль просто вставку req
                req->condition());

            req->set_response(DB::Response(true));
            break;
        }
    }
}

bool DB::InsertUser(std::string loggin, std::string password) {
    DB::RequestType req_type = DB::InsertUser_;
    std::shared_ptr<DB::Request> request = std::make_shared<DB::Request>(std::move(DB::Request(req_type, DB::User(loggin, password))));

    g_ThSdQu.push(request);

    return request->response()->status();
}

bool DB::Authorization(std::string loggin, std::string password) {
    DB::RequestType req_type = DB::Authorization_;
    std::shared_ptr<DB::Request> request = std::make_shared<DB::Request>(std::move(DB::Request(req_type, DB::User(loggin, password))));

    g_ThSdQu.push(request);

    return request->response()->status();
}

bool DB::InsertESPCondition(std::string loggin, std::string password, DB::ESPCondition &condition) {
    DB::RequestType req_type = DB::InsertESPCondition_;
    std::shared_ptr<DB::Request> request = std::make_shared<DB::Request>(std::move(DB::Request(req_type, DB::User(loggin, password), condition)));

    g_ThSdQu.push(request);

    return request->response()->status();
}
