#pragma once

#include "stde.h"
#include "types.h"

namespace input {
   inline std::vector<elttype> load(const std::string& fname,int mCodeSize) {
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
         }
         catch (...) {
            break;
         }
      }
      infile.close();

      return tweets;
   }


   inline std::vector<elttype> dist_random(unsigned int nb_el, long rseed){
      std::vector<elttype> keyVal_vec;

      if (rseed != 0)
      {
         srand( rseed);
      }
      else
      {
         long seed = time(NULL);
         fprintf(stdout, "Seed : %ld\n", seed);
         srand ( seed );
      }

      for(int i = 0; i< nb_el; i++)
      {
         elttype el;
         el.key = rand()%nb_el;
         el.value = tweet_t();

         keyVal_vec.push_back(el);
      }

      return keyVal_vec;
   }

} // namespace input
