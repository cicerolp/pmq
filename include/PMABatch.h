#pragma once
#include "ContainerInterface.h"

extern uint32_t g_Quadtree_Depth;

class PMABatch : public ContainerInterface {
public:
   virtual ~PMABatch();

   duration_t create(uint32_t size, int argc, char *argv[]) override final;

   /**
   * @brief insert_batch
   * @param pma
   * @param batch
   * @param size
   *
   * Will return the windowIds of windows that were rebalanced
   */
   duration_t insert(std::vector<elttype> batch) override final;

   /**
   * @brief pma_diff Scans the segments modified on last rebalance operation and returns the segmentID of the elements in these windows;
   * @param pma
   * @param modified [OUT] a vector containing (KEY, START_SEG, END_SEG) where [ START_SED, END_SED [ is the interval os segments that contains key \a KEY
   *
   * for each wId modified on in pma:
   * - get wId.beg and wId.end
   * - modified <- (mcode, wId.beg, wId.end)
   *
   * @return the number of KEYS in vector.
   */
   duration_t diff(std::vector<elinfo_t>& keys) override final;

   /**
   * @brief count_elts_pma find in [sbeg, send[ all the elements starting with mcode at level z.
   * @param pma
   * @param sbeg
   * @param send
   * @param mcode the morton code of a quadtree at level z
   * @param z
   *
   * Counts the number elements in the PMA that are contained in the subtree (quad-tree) of depth z and morton code \a mCode;
   *
   * @return
   */
   duration_t count(const uint32_t& begin, const uint32_t& end, const spatial_t& el, uint32_t& count) const override final;
   
   /**
   * @brief elts_pma Gets the elements in the \a pma between segments [seg_beg , seg_end[ with prefix equal to \a mCode.
   * @param pma
   * @param seg_beg
   * @param seg_end
   * @param mCode The MortonCode prefix the function will be looking for
   * @param z The depth in the quadtree used to compute the prefix.
   * @param writer Will be filled with the elements retreived
   * @param max_cnt limits to max_cnt elements.
   * @return the amount elements written.
   */
   duration_t apply(const uint32_t& begin, const uint32_t& end, const spatial_t& el, uint32_t& count, valuetype_function _apply) const override final;

private:
   /**
   * @brief get_mcode_range : Computes the min and max values for a geo_hash with prefix \a mCode
   * @param mCode : The prefix of the morton code representing a quadtree node.
   * @param z : The depth of the quadtree node (corresponding to this mCode).
   * @param min [OUT]
   * @param max [OUT]
   */
   static inline void get_mcode_range(uint64_t code, uint32_t zoom, uint64_t& min, uint64_t& max);

   pma_struct* _pma {nullptr};
};

void PMABatch::get_mcode_range(uint64_t code, uint32_t zoom, uint64_t& min, uint64_t& max) {
   uint32_t diffDepth = g_Quadtree_Depth - zoom;
   min = code << 2 * (diffDepth);
   max = min | ((uint64_t)~0 >> (64 - 2 * diffDepth));
}