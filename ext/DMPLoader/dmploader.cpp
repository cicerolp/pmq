#include "dmploader.hpp"

#include <vector>

#include <fstream>
#include <utility>
#include <string>

#include <limits>

void loadTweetFile(std::vector<tweet_t> &tweets, std::__cxx11::string fname)
{

  std::ifstream infile(fname, std::ios::binary);
  infile.unsetf(std::ios_base::skipws);

  //Skip file header
  for (int i = 0; i < 32; ++i) {
    infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  tweet_t record;
  size_t record_size = sizeof(tweet_t::latitude) + sizeof(tweet_t::longitude) + sizeof(tweet_t::time) + sizeof(tweet_t::language) + sizeof(tweet_t::device) + sizeof(tweet_t::app);

#if 1
  while (true) {
    try {

      infile.read((char*)&record, record_size);

      if (infile.eof() ) break;

      tweets.push_back(record);

    } catch (...) {
      break;
    }
  }

  infile.close();
#endif
  return;
}
