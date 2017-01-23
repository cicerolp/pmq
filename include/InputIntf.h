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


   inline std::vector<elttype> load(const std::string& fname, int mCodeSize, unsigned int time_res) {
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

      while (true) {
         try {
            infile.read((char*)&record, record_size);

            if (infile.eof()) break;

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

      for (unsigned int i = 0; i < nb_el; i++) {
         tweet_t el;
         el.longitude = (float)lon(gen);
         el.latitude = (float)lat(gen);
         el.time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
         keyVal_vec.emplace_back(el, 25);
      }

      return keyVal_vec;
   }

   /**
    * @brief dist_random Create a synthetic input for benchmark
    * @param nb_el
    * @param rseed
    * @param time_res Tweets created will have the time incremented every time_res;
    * @return
    */
   inline std::vector<elttype> dist_random(unsigned int nb_el, long rseed, unsigned int time_res) {
      std::vector<elttype> keyVal_vec;

      std::mt19937 gen(rseed);

      std::uniform_real_distribution<> lon(-180, 180);
      std::uniform_real_distribution<> lat(-85, 85);

      for (unsigned int i = 0; i < nb_el; i++) {
         tweet_t el;
         el.longitude = (float)lon(gen);
         el.latitude = (float)lat(gen);
         el.time = i / time_res;
         keyVal_vec.emplace_back(el, 25);
      }

      return keyVal_vec;
   }

} // namespace input
