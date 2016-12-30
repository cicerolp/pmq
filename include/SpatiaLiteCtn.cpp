#include "stde.h"
#ifdef __GNUC__
#include "SpatiaLiteCtn.h"

SpatiaLiteCtn::SpatiaLiteCtn() {
   int ret;

   std::string db = ":memory:";
//std::string db = "../db/db.sqlite";

// in-memory database
   ret = sqlite3_open_v2(db.c_str(), &_handle,
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
   if (ret != SQLITE_OK) {
      printf("cannot open '%s': %s\n", db.c_str(), sqlite3_errmsg(_handle));
      sqlite3_close(_handle);
      return;
   }
   _cache = spatialite_alloc_connection();
   spatialite_init_ex(_handle, _cache, 0);

   printf("SQLite version: %s\n", sqlite3_libversion());

   printf("SpatiaLite version: %s\n", spatialite_version());

   initGEOS(notice, log_and_exit);
   printf("GEOS version %s\n", GEOSversion());

   printf("\n\n");
}

SpatiaLiteCtn::~SpatiaLiteCtn() {
   finishGEOS();

   if (_handle) sqlite3_close(_handle);
   if (_cache) spatialite_cleanup_ex(_cache);

//spatialite_shutdown();
}

// build container
duration_t SpatiaLiteCtn::create(uint32_t size) {
   Timer timer;
   timer.start();

   int ret;
   char sql[256];
   char* err_msg = NULL;

// we are supposing this one is an empty database,
// so we have to create the Spatial Metadata
   strcpy(sql, "SELECT InitSpatialMetadata(1)");
   ret = sqlite3_exec(_handle, sql, NULL, NULL, &err_msg);
   if (ret != SQLITE_OK) {
// an error occurred
      printf("InitSpatialMetadata() error: %s\n", err_msg);
      sqlite3_free(err_msg);

      timer.stop();
      return {duration_info("Error", timer)};
   }

// now we can create the table
// for simplicity we'll define only one column, the primary key
   strcpy(sql, "CREATE TABLE db (");
   strcat(sql, "pk INTEGER NOT NULL PRIMARY KEY,");
   strcat(sql, "value BLOB NOT NULL)");
   ret = sqlite3_exec(_handle, sql, NULL, NULL, &err_msg);
   if (ret != SQLITE_OK) {
// an error occurred
      printf("CREATE TABLE 'db' error: %s\n", err_msg);
      sqlite3_free(err_msg);

      timer.stop();
      return {duration_info("Error", timer)};
   }

// ... we'll add a Geometry column of POINT type to the table
   strcpy(sql, "SELECT AddGeometryColumn('db', 'key', 4326, 'POINT', 2)");
   ret = sqlite3_exec(_handle, sql, NULL, NULL, &err_msg);
   if (ret != SQLITE_OK) {
// an error occurred
      printf("AddGeometryColumn() error: %s\n", err_msg);
      sqlite3_free(err_msg);

      timer.stop();
      return {duration_info("Error", timer)};
   }

// and finally we'll enable this geo-column to have a Spatial Index based on R*Tree
   strcpy(sql, "SELECT CreateSpatialIndex('db', 'key')");
   ret = sqlite3_exec(_handle, sql, NULL, NULL, &err_msg);
   if (ret != SQLITE_OK) {
// an error occurred
      printf("CreateSpatialIndex() error: %s\n", err_msg);
      sqlite3_free(err_msg);

      timer.stop();
      return {duration_info("Error", timer)};
   }

   _init = true;
   timer.stop();
   return {duration_info("create", timer)};
}

// update container
duration_t SpatiaLiteCtn::insert(std::vector<elttype> batch) {
   duration_t duration;
   Timer timer;

   if (!_init) {
      return {duration_info("Error", timer)};
   }

// insert start
   timer.start();

   int ret;
   char sql[256];
   char* err_msg = NULL;

   int blob_size;
   unsigned char* blob;

   sqlite3_stmt* stmt;
   gaiaGeomCollPtr geo = NULL;

// beginning a transaction
   strcpy(sql, "BEGIN");
   ret = sqlite3_exec(_handle, sql, NULL, NULL, &err_msg);
   if (ret != SQLITE_OK) {
// an error occurred
      printf("BEGIN error: %s\n", err_msg);
      sqlite3_free(err_msg);

      timer.stop();
      return {duration_info("total", timer)};
   }

// preparing to populate the table
   strcpy(sql, "INSERT INTO db (pk, key, value) VALUES (?, ?, ?)");
   ret = sqlite3_prepare_v2(_handle, sql, strlen(sql), &stmt, NULL);
   if (ret != SQLITE_OK) {
// an error occurred
      printf("INSERT SQL error: %s\n", sqlite3_errmsg(_handle));

      timer.stop();
      return {duration_info("total", timer)};
   }

   for (uint32_t pk = 0; pk < batch.size(); ++pk) {
      float x = batch[pk].value.longitude;
      float y = batch[pk].value.latitude;

      valuetype value = batch[pk].value;

// preparing the geometry to insert
      geo = gaiaAllocGeomColl();
      geo->Srid = 4326;
      gaiaAddPointToGeomColl(geo, value.longitude, value.latitude);

// transforming this geometry into the SpatiaLite BLOB format
      gaiaToSpatiaLiteBlobWkb(geo, &blob, &blob_size);

// we can now destroy the geometry object
      gaiaFreeGeomColl(geo);

// resetting Prepared Statement and bindings
      sqlite3_reset(stmt);
      sqlite3_clear_bindings(stmt);

// (pk, key, value)
// binding parameters to Prepared Statement
      sqlite3_bind_null(stmt, 1);
      sqlite3_bind_blob(stmt, 2, blob, blob_size, free);
      sqlite3_bind_blob(stmt, 3, &value, sizeof(valuetype), SQLITE_TRANSIENT);

// performing actual row insert
      ret = sqlite3_step(stmt);
      if (ret == SQLITE_DONE || ret == SQLITE_ROW);
      else {
// an error occurred
         printf("sqlite3_step() error: %s\n", sqlite3_errmsg(_handle));
         sqlite3_finalize(stmt);

         timer.stop();
         return {duration_info("total", timer)};
      }
   }

// we have now to finalize the query [memory cleanup]
   sqlite3_finalize(stmt);

// committing the transaction
   strcpy(sql, "COMMIT");
   ret = sqlite3_exec(_handle, sql, NULL, NULL, &err_msg);
   if (ret != SQLITE_OK) {
// an error occurred
      printf("COMMIT error: %s\n", err_msg);
      sqlite3_free(err_msg);

      timer.stop();
      return {duration_info("total", timer)};
   }

// insert end
   timer.stop();
   duration.emplace_back("Insert", timer);

// analyze start
   timer.start();

// now we'll optimize the table
   strcpy(sql, "ANALYZE db");
   ret = sqlite3_exec(_handle, sql, NULL, NULL, &err_msg);
   if (ret != SQLITE_OK) {
// an error occurred
      printf("ANALYZE error: %s\n", err_msg);
      sqlite3_free(err_msg);

      timer.stop();
      return {duration_info("Error", timer)};
   }

// analyze end
   timer.stop();
   duration.emplace_back("Analyze", timer);

   return duration;
}

// apply function for every el<valuetype>
duration_t SpatiaLiteCtn::scan_at_region(const region_t& region, scantype_function __apply) {
   Timer timer;
   timer.start();

   if (!_init) {
      timer.stop();
      return {duration_info("total", timer)};
   }

   int ret;
   char sql[256];
   char* err_msg = NULL;

   std::stringstream stream;
   stream << "SELECT value FROM db ";
   stream << "WHERE MbrWithin(key, BuildMbr(";
   stream << region.xmin() << "," << region.ymin() << ",";
   stream << region.xmax() << "," << region.ymax() << ")) AND ROWID IN (";
   stream << "SELECT pkid FROM idx_db_key WHERE ";
   stream << "xmin >= " << region.xmin() << " AND ";
   stream << "xmax <= " << region.xmax() << " AND ";
   stream << "ymin >= " << region.ymin() << " AND ";
   stream << "ymax <= " << region.ymax() << ")";

   sqlite3_stmt* stmt;

// preparing to populate the table
   ret = sqlite3_prepare_v2(_handle, stream.str().c_str(), stream.str().size(), &stmt, NULL);
   if (ret != SQLITE_OK) {
// an error occurred
      printf("INSERT SQL error: %s\n", sqlite3_errmsg(_handle));

      timer.stop();
      return {duration_info("total", timer)};
   }

   while (sqlite3_step(stmt) == SQLITE_ROW) {
      __apply((*(valuetype*)sqlite3_column_blob(stmt, 0)));
   }

   sqlite3_finalize(stmt);

   timer.stop();
   return {duration_info("total", timer)};
}

// apply function for every spatial area/region
duration_t SpatiaLiteCtn::apply_at_tile(const region_t& region, applytype_function __apply) {
   Timer timer;
   timer.start();

   if (!_init) {
      timer.stop();
      return {duration_info("total", timer)};
   }

   int ret;
   char sql[256];
   char* err_msg = NULL;

   uint32_t curr_z = std::min((uint32_t)8, 25 - region.z);
   uint32_t n = (uint64_t)1 << curr_z;

   uint32_t x_min = region.x0 * n;
   uint32_t x_max = (region.x1 + 1) * n;

   uint32_t y_min = region.y0 * n;
   uint32_t y_max = (region.y1 + 1) * n;

   curr_z += region.z;

   for (uint32_t x = x_min; x < x_max; ++x) {
      for (uint32_t y = y_min; y < y_max; ++y) {

         std::stringstream stream;

         std::string xmin = std::to_string(mercator_util::tilex2lon(x, curr_z));
         std::string xmax = std::to_string(mercator_util::tilex2lon(x + 1, curr_z));

         std::string ymin = std::to_string(mercator_util::tiley2lat(y + 1, curr_z));
         std::string ymax = std::to_string(mercator_util::tiley2lat(y, curr_z));

         stream << "SELECT count(*) FROM db ";
         stream << "WHERE ST_WITHIN(key, BuildMbr(";
         stream << xmin << "," << ymin << ",";
         stream << xmax << "," << ymax << ")) AND ROWID IN (";
         stream << "SELECT pkid FROM idx_db_key WHERE ";
         stream << "xmin >= " << xmin << " AND ";
         stream << "xmax <= " << xmax << " AND ";
         stream << "ymin >= " << ymin << " AND ";
         stream << "ymax <= " << ymax << ")";

         sqlite3_stmt* stmt;

// preparing to populate the table
         ret = sqlite3_prepare_v2(_handle, stream.str().c_str(), stream.str().size(), &stmt, NULL);
         if (ret != SQLITE_OK) {
// an error occurred
            printf("INSERT SQL error: %s\n", sqlite3_errmsg(_handle));

            timer.stop();
            return {duration_info("total", timer)};
         }

         if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            if (count > 0) __apply(spatial_t(x, y, curr_z), count);
         }

         sqlite3_finalize(stmt);
      }
   }

   timer.stop();
   return {duration_info("total", timer)};
}

duration_t SpatiaLiteCtn::apply_at_region(const region_t& region, applytype_function __apply) {
   Timer timer;
   timer.start();

   if (!_init) {
      timer.stop();
      return {duration_info("total", timer)};
   }

   int ret;
   char sql[256];
   char* err_msg = NULL;

   std::stringstream stream;
   stream << "SELECT count(*) FROM db ";
   stream << "WHERE MbrWithin(key, BuildMbr(";
   stream << region.xmin() << "," << region.ymin() << ",";
   stream << region.xmax() << "," << region.ymax() << ")) AND ROWID IN (";
   stream << "SELECT pkid FROM idx_db_key WHERE ";
   stream << "xmin >= " << region.xmin() << " AND ";
   stream << "xmax <= " << region.xmax() << " AND ";
   stream << "ymin >= " << region.ymin() << " AND ";
   stream << "ymax <= " << region.ymax() << ")";

   sqlite3_stmt* stmt;

// preparing to populate the table
   ret = sqlite3_prepare_v2(_handle, stream.str().c_str(), stream.str().size(), &stmt, NULL);
   if (ret != SQLITE_OK) {
// an error occurred
      printf("INSERT SQL error: %s\n", sqlite3_errmsg(_handle));

      timer.stop();
      return {duration_info("total", timer)};
   }

   if (sqlite3_step(stmt) == SQLITE_ROW) {
      int count = sqlite3_column_int(stmt, 0);
      if (count > 0) {
         __apply(spatial_t(region.x0 + (uint32_t)((region.x1 - region.x0) / 2),
                           region.y0 + (uint32_t)((region.y1 - region.y0) / 2),
                           0), count);
      }
   }

   sqlite3_finalize(stmt);

   timer.stop();
   return {duration_info("total", timer)};
}

void SpatiaLiteCtn::notice(const char* fmt, ...) {
   va_list ap;

   fprintf(stdout, "NOTICE: ");

   va_start(ap, fmt);
   vfprintf(stdout, fmt, ap);
   va_end(ap);
   fprintf(stdout, "\n");
}

void SpatiaLiteCtn::log_and_exit(const char* fmt, ...) {
   va_list ap;

   fprintf(stdout, "ERROR: ");

   va_start(ap, fmt);
   vfprintf(stdout, fmt, ap);
   va_end(ap);
   fprintf(stdout, "\n");
   exit(1);
}

#endif
