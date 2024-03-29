#include "common/Helpers.h"
#include "date.h"
#include <chrono>
#include <iostream>
#include <fstream>

namespace inkfuse::helpers {

int32_t dateStrToInt(const char* str) {
   std::istringstream stream(str);
   date::sys_time<std::chrono::days> time;
   date::from_stream(stream, "%Y-%m-%d", time);
   return time.time_since_epoch().count();
}

std::string dateIntToStr(int32_t date) {
   std::stringstream stream;
   std::chrono::sys_days dur(std::chrono::days{date});
   date::to_stream(stream, "%Y-%m-%d", dur);
   return stream.str();
}

void loadDataInto(Schema& schema, const std::string& path, bool force) {
   for (auto& [tbl_name, tbl]: schema) {
      std::ifstream input;
      if (force) {
         input.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      }
      input.open(path + "/" + tbl_name + ".tbl");
      tbl->loadRows(input);
      input.close();
   }
}

}