#pragma once
#include "Singleton.h"
#include "GeoRunner.h"

class Server: public Singleton<Server> {
   friend class Singleton<Server>;
public:
   struct server_opts {
      uint32_t port{8000};

      bool cache{true};
      bool multithreading{true};

      friend std::ostream& operator<<(std::ostream& os, const server_opts& obj) {
         return os
            << "port: " << obj.port
            << ", cache: " << obj.cache
            << ", multithreading: " << obj.multithreading << std::endl;
      }
   };

   
   static void run();

   static void run_broadcast();

   static void handler(struct mg_connection* nc, int ev, void* ev_data);

   static void printText(struct mg_connection* conn, const std::string& content);

   static void printJson(struct mg_connection* conn, const std::string& content);

   static inline int is_websocket(const struct mg_connection* nc) {
      return nc->flags & MG_F_IS_WEBSOCKET;
   }

   void renew_data();

   void push_trigger(GeoRunner::grid_coord index);

   void stop() { running = false; };

private:
   std::string renew_json;
   
   void broadcast();   
   void broadcast_info(mg_connection* conn) const;
   void broadcast_triggers();
   
   bool running{true};

   struct mg_serve_http_opts http_server_opts;
   struct mg_connection* nc;
   struct mg_mgr mgr;

   server_opts nds_opts;

   Server(server_opts opts);

   Server() = default;

   virtual ~Server() = default;

   std::unordered_map<mg_connection*, std::vector<GeoRunner::grid_coord>> triggers;
   std::unordered_map<mg_connection*, bool> up_to_date;
   std::mutex mutex;
};
