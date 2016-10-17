#include "stde.h"
#include "PostGisCtn.h"

PostGisCtn::PostGisCtn() {
   std::string conninfo = "user=postgres password=postgres host=localhost port=5432 dbname=twittervis";

   // make a connection to the database
   _conn = PQconnectdb(conninfo.c_str());

   // check to see that the backend connection was successfully made
   if (PQstatus(_conn) != CONNECTION_OK) {
      fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(_conn));
   } else {
      _init = true;
   }
}

PostGisCtn::~PostGisCtn() {
   // close the connection to the database and cleanup
   PQfinish(_conn);
}

// build container
duration_t PostGisCtn::create(uint32_t size) {
   Timer timer;
   timer.start();

   std::string sql;

   sql += "DROP TABLE IF EXISTS db;";
   sql += "CREATE TABLE db(pk BIGSERIAL NOT NULL PRIMARY KEY, key geometry(Point, 4326), value BYTEA);";
   // spatial index using GIST
   sql += "CREATE INDEX key_gix ON  db USING GIST (key);";

   PGresult* res = PQexec(_conn, sql.c_str());
   if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(_conn));
   }
   PQclear(res);

   _init = true;

   timer.stop();
   return timer;
}

// update container
duration_t PostGisCtn::insert(std::vector<elttype> batch) {
   Timer timer;
   timer.start();

   if (!_init) {
      timer.stop();
      return timer;
   }

   PGresult* res;
   std::string sql;
   
   sql = "INSERT INTO db (key, value) VALUES (ST_GeomFromText($1, 4326), $2);";
   res = PQprepare(_conn, "stmtname", sql.c_str(), 0, nullptr);
   if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      fprintf(stderr, "PQprepare command failed: %s", PQerrorMessage(_conn));
   }
   PQclear(res);

   sql = "BEGIN;";
   res = PQexec(_conn, sql.c_str());
   if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      fprintf(stderr, "BEGIN command failed: %s", PQerrorMessage(_conn));
   }
   PQclear(res);

   int paramLengths[2];
   int paramFormats[2];
   char* paramValues[2];

   for (uint32_t pk = 0; pk < batch.size(); ++pk) {
      float x = batch[pk].value.longitude;
      float y = batch[pk].value.latitude;

      valuetype value = batch[pk].value;

      char buffer[50];
      sprintf(buffer, "POINT(%f %f)", x, y);

      paramValues[0] = (char*)buffer;
      paramValues[1] = (char*)&value;
      
      paramFormats[0] = 0;
      paramFormats[1] = 1;

      paramLengths[0] = 0;
      paramLengths[1] = sizeof(valuetype);

      res = PQexecPrepared(_conn, "stmtname", 2, paramValues, paramLengths, paramFormats, 0);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
         fprintf(stderr, "PQexecPrepared command failed: %s", PQerrorMessage(_conn));
      }
      PQclear(res);
   }

   sql = "COMMIT;";
   res = PQexec(_conn, sql.c_str());
   if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      fprintf(stderr, "COMMIT command failed: %s", PQerrorMessage(_conn));
   }
   PQclear(res);

   // reorders the table on disk based on the index 
   sql = "CLUSTER db USING key_gix;";
   res = PQexec(_conn, sql.c_str());
   if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      fprintf(stderr, "CLUSTER command failed: %s", PQerrorMessage(_conn));
   }
   PQclear(res);

   sql = "ANALYZE db;";
   res = PQexec(_conn, sql.c_str());
   if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      fprintf(stderr, "ANALYZE command failed: %s", PQerrorMessage(_conn));
   }
   PQclear(res);

   timer.stop();
   return timer;
}

// apply function for every el<valuetype>
duration_t PostGisCtn::scan_at_region(const region_t& region, scantype_function __apply) {
   Timer timer;
   timer.start();

   if (!_init) {
      timer.stop();
      return timer;
   }

   PGresult* res;
   std::string sql;

   sql = "SELECT value from db Where key && ST_MakeEnvelope(" + region.xmin() + ", " + region.ymin() + ", " + region.xmax() + ", " + region.ymax() + ");";

   res = PQexecParams(_conn, sql.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 1);
   if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      fprintf(stderr, "SELECT command failed: %s", PQerrorMessage(_conn));
   }
   
   uint32_t n_tuples = PQntuples(res);
   for (uint32_t row = 0; row < n_tuples; ++row) {      
      __apply((*(valuetype*)PQgetvalue(res, row, 0)));
   }

   PQclear(res);

   timer.stop();
   return timer;
}

// apply function for every spatial area/region
duration_t PostGisCtn::apply_at_tile(const region_t& region, applytype_function __apply) {
   Timer timer;
   timer.start();

   if (!_init) {
      timer.stop();
      return timer;
   }

   PGresult* res;
   std::string sql;

   uint32_t curr_z = std::min((uint32_t)8, 25 - region.z());
   uint32_t n = (uint64_t)1 << curr_z;

   uint32_t x_min = region.x0() * n;
   uint32_t x_max = (region.x1() + 1) * n;

   uint32_t y_min = region.y0() * n;
   uint32_t y_max = (region.y1() + 1) * n;

   curr_z += region.z();

   for (uint32_t x = x_min; x < x_max; ++x) {
      for (uint32_t y = y_min; y < y_max; ++y) {

         std::stringstream stream;

         std::string xmin = std::to_string(mercator_util::tilex2lon(x, curr_z));
         std::string xmax = std::to_string(mercator_util::tilex2lon(x + 1, curr_z));

         std::string ymin = std::to_string(mercator_util::tiley2lat(y + 1, curr_z));
         std::string ymax = std::to_string(mercator_util::tiley2lat(y, curr_z));


         sql = "SELECT count(*) from db Where key && ST_MakeEnvelope(" + xmin + ", " + xmax + ", " + ymin + ", " + ymax + ");";

         res = PQexecParams(_conn, sql.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 0);
         if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            fprintf(stderr, "SELECT command failed: %s", PQerrorMessage(_conn));
         }

         uint32_t count = std::stoi(PQgetvalue(res, 0, 0));
         if (count > 0) __apply(spatial_t(x, y, curr_z), count);

         PQclear(res);
      }
   }

   timer.stop();
   return timer;
}

duration_t PostGisCtn::apply_at_region(const region_t& region, applytype_function __apply) {
   Timer timer;
   timer.start();

   if (!_init) {
      timer.stop();
      return timer;
   }

   PGresult* res;
   std::string sql;

   sql = "SELECT count(*) from db Where key && ST_MakeEnvelope(" + region.xmin() + ", " + region.ymin() + ", " + region.xmax() + ", " + region.ymax() + ");";

   res = PQexecParams(_conn, sql.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 0);
   if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      fprintf(stderr, "SELECT command failed: %s", PQerrorMessage(_conn));
   }

   uint32_t count = std::stoi(PQgetvalue(res, 0, 0));
   if (count > 0) {
      __apply(spatial_t(region.x0() + (uint32_t)((region.x1() - region.x0()) / 2),
         region.y0() + (uint32_t)((region.y1() - region.y0()) / 2),
         0), count);
   }
   
   PQclear(res);

   timer.stop();
   return timer;
}
