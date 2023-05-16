#include <string>
#include <queue>
#include <mutex>
#include <memory>
#include <unordered_map>

#include <condition_variable>

#include<pqxx/pqxx>

#ifndef SERVERDB_H
#define SERVERDB_H

template<typename T>
class ThreadSavedQueue {
    public:
        ThreadSavedQueue() = default;
        ~ThreadSavedQueue() = default;
        ThreadSavedQueue& operator=(const ThreadSavedQueue&) = delete;

        ThreadSavedQueue(const ThreadSavedQueue& that);

        void push(const T &new_Data);

        T& pop();
        
    private:
    std::queue<T> Data;
    mutable std::mutex MyMutex;
    std::condition_variable MyCond;
};

namespace DB {
    class Request;
}


extern ThreadSavedQueue<std::shared_ptr<DB::Request>> g_ThSdQu;


namespace DB {

    struct ESPCondition {
        int SolarV, SolarA, SolarStatus;
        int WindV, WindA, WindStatus;
        int genV, genA, genStatus;
        int batV;
        int V1, A1, Status1;
        int V2, A2, Status2;
        int V3, A3, Status3;
    };

    struct User {
        User(std::string loggin, std::string password): loggin(loggin), password(password) {
        }
        User() = default;
        std::string loggin;
        std::string password;
    };

    enum RequestType {
        InsertUser_,
        Authorization_,
        InsertESPCondition_
    };

    struct Response {
        public:
        Response() = default;

        Response(bool user_status, ESPCondition &condition): user_status(user_status), condition(condition) {
        }

        Response(bool user_status): user_status(user_status) {
        }

        ESPCondition& data() { return condition; }

        bool status() { return user_status; }

        private:

        ESPCondition condition;
        bool user_status;

    };

    class Request{
        public:
            Request(RequestType &type, User user): type_(type), user_(user) {
                return_field = nullptr;
            }
            Request(RequestType &type, User user, ESPCondition &condition): 
            type_(type), user_(user), condition_(condition) {
                return_field = nullptr;
            }

            Request(Request&& other) {
                std::lock_guard<std::mutex> lock_other(other.Mutex);
                std::lock_guard<std::mutex> lock(Mutex);
                type_ = other.type_;
                user_ = other.user_;
                
                condition_ = other.condition_;
                return_field = other.return_field;
            }
            
            User& user() { return user_; }

            ESPCondition& condition() { return condition_; }

            RequestType& type() { return type_; }

            std::shared_ptr<Response>& response() { 
                std::unique_lock<std::mutex> lock(Mutex);
                Cond.wait(lock, [this]
                { return has_data;});
                has_data = false;

                return return_field; 
                }

            void set_response(const Response response_) {
                std::unique_lock<std::mutex> lock(Mutex);
                return_field = std::make_shared<Response>(response_);
                has_data = true;
                Cond.notify_all();
            }


        private:
            ESPCondition condition_;
            User user_;
            RequestType type_;
            std::shared_ptr<Response> return_field;
            bool has_data;

            mutable std::mutex Mutex;
            std::condition_variable Cond;
    };

    class UserListTable {

        public:
        UserListTable(std::string table_name):
        table_name(table_name) {
        }
        ~UserListTable() = default;

        void AddToTable(std::string& loggin, std::string& password);
        void DeleteFromTable() {
        }
        bool IsUserExist(std::string& loggin, std::string& password);
        int GetID(std::string& loggin, std::string& password);

        private:
        std::string table_name;
    };

    class UserESPTable {
        public:
        UserESPTable() = default;
        UserESPTable(int user_id):
        table_name("User" + std::to_string(user_id)) {
        }
        ~UserESPTable() = default;

        UserESPTable& operator=(UserESPTable& that) = default;

        void AddToTable(ESPCondition& cond);
        void DropTable() {}
        

        private:
        std::string table_name;
    };

    void Handler();

    bool InsertUser(std::string loggin, std::string password);

    bool Authorization(std::string loggin, std::string password);

    bool InsertESPCondition(std::string loggin, std::string password, ESPCondition &condition);

} //DB

#endif // SERVERDB_H