#ifndef DMPLOADER_HPP
#define DMPLOADER_HPP

#include <vector>

#include <fstream>
#include <utility>
#include <string>

#include <limits>

struct tweet_s {
  float latitude;
  float longitude;
  uint64_t time;
  uint8_t language;
  uint8_t device;
  uint8_t app;
};

typedef struct tweet_s tweet_t;

void loadTweetFile(std::vector<tweet_t>& tweets, std::string fname);

#endif // DMPLOADER_HPP

