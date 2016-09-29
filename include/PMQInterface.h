#pragma once
#include "ContainerInterface.h"
#include "pma_util.h"

class PMQInterface : public ContainerInterface<uint32_t> {
public:
   virtual ~PMQInterface();

   duration_t create(uint32_t size, int argc, char *argv[]) override;

   /**
   * @brief insert_batch
   * @param pma
   * @param batch
   * @param size
   *
   * Will return the windowIds of windows that were rebalanced
   */
   duration_t insert(std::vector<elttype> batch) override;

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
   duration_t diff(std::vector<elinfo_t>& keys) override;

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
   duration_t count(const uint32_t& begin, const uint32_t& end, const spatial_t& el, uint32_t& count) override;
   
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
   duration_t apply(const uint32_t& begin, const uint32_t& end, const spatial_t& el, valuetype_function _apply) override;

private:
   pma_struct* _pma {nullptr};
};