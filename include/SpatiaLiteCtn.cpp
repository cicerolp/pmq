#include "stde.h"
#include "SpatiaLiteCtn.h"

SpatiaLiteCtn::SpatiaLiteCtn() {
   /*
   trying to connect the test DB:
   - this demo is intended to create a new, empty database
   */
   int ret;
   void* cache;

   std::string db = "../db/db.sqlite";
   ret = sqlite3_open_v2(db.c_str(), &_handle,
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
   if (ret != SQLITE_OK) {
      printf("cannot open '%s': %s\n", db.c_str(), sqlite3_errmsg(_handle));
      sqlite3_close(_handle);
      return;
   }
   cache = spatialite_alloc_connection();
   spatialite_init_ex(_handle, cache, 0);

   printf("SQLite version: %s\n", sqlite3_libversion());

   printf("SpatiaLite version: %s\n", spatialite_version());

   initGEOS(notice, log_and_exit);
   printf("GEOS version %s\n", GEOSversion());

   printf("\n\n");
}

SpatiaLiteCtn::~SpatiaLiteCtn() {
   finishGEOS();
}

// build container
duration_t SpatiaLiteCtn::create(uint32_t size) {
   Timer timer;
   timer.start();

   int ret;
   char sql[256];
   char *err_msg = NULL;

   /*
   we are supposing this one is an empty database,
   so we have to create the Spatial Metadata
   */
   strcpy(sql, "SELECT InitSpatialMetadata(1)");
   ret = sqlite3_exec(_handle, sql, NULL, NULL, &err_msg);
   if (ret != SQLITE_OK) {
      /* an error occurred */
      printf("InitSpatialMetadata() error: %s\n", err_msg);
      sqlite3_free(err_msg);
      
      timer.stop();
      return timer;
   }
   /*
   now we can create the test table
   for simplicity we'll define only one column, the primary key
   */
   strcpy(sql, "CREATE TABLE test (");
   strcat(sql, "PK INTEGER NOT NULL PRIMARY KEY)");
   ret = sqlite3_exec(_handle, sql, NULL, NULL, &err_msg);
   if (ret != SQLITE_OK) {
      /* an error occurred */
      printf("CREATE TABLE 'test' error: %s\n", err_msg);
      sqlite3_free(err_msg);
      
      timer.stop();
      return timer;
   }

   /*
   ... we'll add a Geometry column of POINT type to the test table
   */
   strcpy(sql, "SELECT AddGeometryColumn('test', 'geom', 3003, 'POINT', 2)");
   ret = sqlite3_exec(_handle, sql, NULL, NULL, &err_msg);
   if (ret != SQLITE_OK) {
      /* an error occurred */
      printf("AddGeometryColumn() error: %s\n", err_msg);
      sqlite3_free(err_msg);
      
      timer.stop();
      return timer;
   }

   /*
   and finally we'll enable this geo-column to have a Spatial Index based on R*Tree
   */
   strcpy(sql, "SELECT CreateSpatialIndex('test', 'geom')");
   ret = sqlite3_exec(_handle, sql, NULL, NULL, &err_msg);
   if (ret != SQLITE_OK) {
      /* an error occurred */
      printf("CreateSpatialIndex() error: %s\n", err_msg);
      sqlite3_free(err_msg);
      
      timer.stop();
      return timer;
   }

   timer.stop();
   return timer;
}

// update container
duration_t SpatiaLiteCtn::insert(std::vector<elttype> batch) {
}

// apply function for every el<valuetype>
duration_t SpatiaLiteCtn::scan_at_region(const region_t& region, scantype_function __apply) {
}

// apply function for every spatial area/region
duration_t SpatiaLiteCtn::apply_at_tile(const region_t& region, applytype_function __apply) {
}

duration_t SpatiaLiteCtn::apply_at_region(const region_t& region, applytype_function __apply) {
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
