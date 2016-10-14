#include "SpatiaLiteCtn.h"

SpatiaLiteCtn::SpatiaLiteCtn() {
   // verbose	if TRUE a short start-up message is shown on stderr
   spatialite_init(1);
}

SpatiaLiteCtn ::~SpatiaLiteCtn() {
}
