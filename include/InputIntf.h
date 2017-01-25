#pragma once

#include "stde.h"
#include "types.h"
#include <random>

#include "date_util.h"

namespace input {
   inline std::vector<elttype> load(const std::string& fname, int mCodeSize) {
      std::vector<elttype> tweets;

      std::ifstream infile(fname, std::ios::binary);
      infile.unsetf(std::ios_base::skipws);

      // skip file header
      for (int i = 0; i < 32; ++i) {
         infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }

      tweet_t record;
      size_t record_size = 19; //file record size

      while (true) {
         try {
            if (infile.eof()) break;
            infile.read((char*)&record, record_size);

            tweets.emplace_back(record, mCodeSize);
         } catch (...) {
            break;
         }
      }
      infile.close();

      return tweets;
   }

   inline std::vector<elttype> load(const std::string& fname, int mCodeSize,
                                    uint64_t time_res, uint64_t n_elts = std::numeric_limits<uint32_t>::max()) {
      std::vector<elttype> tweets;

      std::ifstream infile(fname, std::ios::binary);
      infile.unsetf(std::ios_base::skipws);

      // skip file header
      for (int i = 0; i < 32; ++i) {
         infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      }

      tweet_t record;
      size_t record_size = 19; //file record size

      unsigned int i = 0; //time counter;

      while (n_elts--) {
         try {
            if (infile.eof()) break;
            infile.read((char*)&record, record_size);

            record.time = i / time_res;
            tweets.emplace_back(record, mCodeSize);
         } catch (...) {
            break;
         }
         i++;
      }
      infile.close();

      return tweets;
   }

   /**
    * @brief dist_random Create a synthetic input for benchmark
    * @param nb_el
    * @param rseed
    * @param time_res Tweets created will have the time incremented every time_res;
    * @return
    */
   inline std::vector<elttype> dist_random(uint64_t nb_el, long rseed, uint64_t time_res) {
      std::vector<elttype> keyVal_vec;

      std::mt19937 gen(rseed);

      std::uniform_real_distribution<> lon(-180, 180);
      std::uniform_real_distribution<> lat(-85, 85);

      for (uint64_t i = 0; i < nb_el; i++) {
         tweet_t el;
         el.longitude = (float)lon(gen);
         el.latitude = (float)lat(gen);
         el.time = i / time_res;
         keyVal_vec.emplace_back(el, 25);
      }

      return keyVal_vec;
   }

} // namespace input
