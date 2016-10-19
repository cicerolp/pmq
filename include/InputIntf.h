#pragma once

#include "stde.h"
#include "types.h"
#include <random>

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
            infile.read((char*)&record, record_size);

            if (infile.eof()) break;

            tweets.emplace_back(record, mCodeSize);
         } catch (...) {
            break;
         }
      }
      infile.close();

      return tweets;
   }

   inline std::vector<elttype> dist_random(unsigned int nb_el, long rseed) {
      std::vector<elttype> keyVal_vec;

      std::mt19937 gen(rseed);
      /*
      std::random_device rd;

      if (rseed != 0) {
         gen.seed(rseed);
      } else {
         gen.seed(rd);
         fprintf(stdout, "Random seed ");
      }
*/

      std::uniform_real_distribution<> lon(-180, 180);
      std::uniform_real_distribution<> lat(-85, 85);

      for (int i = 0; i < nb_el; i++) {
         tweet_t el;
         el.longitude = lon(gen);
         el.latitude = lat(gen);
         keyVal_vec.emplace_back(el,25);
      }

      return keyVal_vec;
   }

} // namespace input
