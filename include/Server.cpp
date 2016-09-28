#include "Server.h"

#include "Query.h"
#include "PMAInstance.h"

void Server::run(server_opts opts)
{
   Server::getInstance().nds_opts = opts;

   setvbuf(stdout, NULL, _IOLBF, 0);
   setvbuf(stderr, NULL, _IOLBF, 0);

   mg_mgr_init(&Server::getInstance().mgr, nullptr);
   Server::getInstance().nc =
      mg_bind(&Server::getInstance().mgr, std::to_string(Server::getInstance().nds_opts.port).c_str(), handler);

   mg_set_protocol_http_websocket(Server::getInstance().nc);

   Server::getInstance().http_server_opts.document_root = "../WebContent";

   if (Server::getInstance().nds_opts.multithreading)
   {
      mg_enable_multithreading(Server::getInstance().nc);
   }

   while (Server::getInstance().running)
   {
      mg_mgr_poll(&Server::getInstance().mgr, 1);
      Server::getInstance().broadcast();
   }
   mg_mgr_free(&Server::getInstance().mgr);
}

void Server::handler(struct mg_connection* nc, int ev, void* ev_data)
{
   switch (ev)
   {
   case MG_EV_HTTP_REQUEST:
      {
         struct http_message* hm = (struct http_message *) ev_data;
         std::string uri(hm->uri.p, hm->uri.len);

         try
         {
            std::vector<std::string> tokens = string_util::split(uri, "[/]+");

            if (tokens.size() <= 1)
            {
               mg_serve_http(nc, hm, Server::getInstance().http_server_opts);
            }
            else if (tokens[1] == "rest" && tokens.size() >= 3)
            {
               if (tokens.size() >= 5 && tokens[2] == "query")
               {
                  printJson(nc, PMAInstance::getInstance().query(Query(tokens)));
               }
               else
               {
                  printJson(nc, "[]");
               }
            }
            else
            {
               mg_serve_http(nc, hm, Server::getInstance().http_server_opts);
            }
         }
         catch (...)
         {
            std::cout << uri << std::endl;
            printJson(nc, "[]");
         }
         break;
      }

   case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
      {  
         // new websocket connection
         Server::getInstance().mutex.lock();
         Server::getInstance().up_to_date.emplace(nc, false);
         Server::getInstance().mutex.unlock();
         break;
      }
   case MG_EV_WEBSOCKET_FRAME:
      {
         struct websocket_message* wm = (struct websocket_message *) ev_data;
         std::string uri((char *)wm->data, wm->size);
         
         if (uri == "update") {
            Server::getInstance().mutex.lock();
            Server::getInstance().up_to_date[nc] = false;
            Server::getInstance().mutex.unlock();
         }
         
         break;
      }
   case MG_EV_CLOSE:
      {
         // disconnect
         if (is_websocket(nc))
         {
            Server::getInstance().mutex.lock();
            Server::getInstance().up_to_date.erase(nc);
            Server::getInstance().mutex.unlock();
         }
         break;
      }
   }
}

void Server::renew_data() {
   mutex.lock();
   for (auto& pair : up_to_date) {
      pair.second = false;
   }
   mutex.unlock();
}

void Server::broadcast() {
   static const std::string msg = "renew";
   
   mutex.lock();
   for (auto& pair : up_to_date) {      
      mg_connection* conn = pair.first;

      if (pair.second == false) {         
         mg_send_websocket_frame(conn, WEBSOCKET_OP_TEXT, msg.c_str(), msg.size());
         pair.second = true;
      }
   }
   mutex.unlock();
}

void Server::printText(mg_connection* conn, const std::string& content)
{
   if (Server::getInstance().nds_opts.cache)
   {
      mg_printf(conn,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Connection: keep-alive\r\n"
                "Cache-Control: public, max-age=86400\r\n"
                "\r\n"
                "%s",
                (int)content.size(), content.c_str());
   }
   else
   {
      mg_printf(conn,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %d\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Connection: keep-alive\r\n"
                "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                "\r\n"
                "%s",
                (int)content.size(), content.c_str());
   }
}

void Server::printJson(mg_connection* conn, const std::string& content)
{
   if (Server::getInstance().nds_opts.cache && content != "[]")
   {
      mg_printf(conn,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %d\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Connection: keep-alive\r\n"
                "Cache-Control: public, max-age=86400\r\n"
                "\r\n"
                "%s",
                (int)content.size(), content.c_str());
   }
   else
   {
      mg_printf(conn,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %d\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Connection: keep-alive\r\n"
                "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                "\r\n"
                "%s",
                (int)content.size(), content.c_str());
   }
}

