#include "common/Helpers.h"
#include "date.h"
#include <chrono>
#include <iostream>

namespace inkfuse::helpers {

int32_t dateStrToInt(const char* str) {
   std::istringstream stream(str);
   date::sys_time<std::chrono::days> time;
   date::from_stream(stream, "%Y-%m-%d", time);
   return time.time_since_epoch().count();
}

void loadDataInto(Schema& schema, std::string path) {

}

}