#include "stde.h"
#include "dmploader.hpp"

void loadTweetFile(std::vector<elttype> &tweets, std::string fname) {

  std::ifstream infile(fname, std::ios::binary);
  infile.unsetf(std::ios_base::skipws);

  //Skip file header
  for (int i = 0; i < 32; ++i) {
    infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

   tweet_t record;
   size_t record_size = 19;
   
   const uint32_t depth = 25;
   
   while (!infile.eof()) {
      try {         
         infile.read((char*)&record, record_size);         
         tweets.emplace_back(record, depth);        
      } catch (...) {
         break;
      }
   }
  infile.close();
  return;
}
