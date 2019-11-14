/*
 * Copyright (c) 2019 Robert Falkenberg.
 *
 * This file is part of FALCON
 * (see https://github.com/falkenber9/falcon).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 */

#include "DCISearch.h"
#include "DCICollection.h"

#include "falcon/prof/Lifetime.h"

#define MIN(a, b) (a > b ? b : a)
#define MAX(a, b) (a > b ? a : b)

/**
  * MAX_RECURSION_DEPTH
  * 0: Recursion disabled (= breadth-first search)
  * 1..L: Limit depth of recursive dci search
  * 99: Unlimited (since L is not larger than 3)
  */
#ifndef MAX_RECURSION_DEPTH
#define MAX_RECURSION_DEPTH 99
#endif

/**
  * DCI_DISAMBIGUATION_DEPTH:
  * 0: Disambiguation disabled
  * 1..MAX_RECURSION_DEPTH: Recursion depth for disambiguation search
  */
#ifndef DCI_DISAMBIGUATION_DEPTH
#define DCI_DISAMBIGUATION_DEPTH MAX_RECURSION_DEPTH
#endif

#if MAX_RECURSION_DEPTH == -1
#error("FAILED")
#endif


#define CNI_HISTOGRAM

#define RNTILIFELEN 2

//#define PRINT_SCAN_TIME

//const srslte_dci_format_t falcon_ue_all_formats[] = {
//  SRSLTE_DCI_FORMAT0,
//  SRSLTE_DCI_FORMAT1,
//  SRSLTE_DCI_FORMAT1A,
//  SRSLTE_DCI_FORMAT1C,
//  SRSLTE_DCI_FORMAT1B,
//  SRSLTE_DCI_FORMAT1D,
//  SRSLTE_DCI_FORMAT2,
//  SRSLTE_DCI_FORMAT2A,
//};
//const uint32_t nof_falcon_ue_all_formats = 8;

#define IMDEA_OWL_COMPAT
#ifndef IMDEA_OWL_COMPAT

const srslte_dci_format_t falcon_ue_all_formats[] = {
  SRSLTE_DCI_FORMAT0,
  SRSLTE_DCI_FORMAT1,
  SRSLTE_DCI_FORMAT1A,
  SRSLTE_DCI_FORMAT1C,
  SRSLTE_DCI_FORMAT2A,
};
const uint32_t nof_falcon_ue_all_formats = 5;

#else

const srslte_dci_format_t falcon_ue_all_formats[] = {
  SRSLTE_DCI_FORMAT0,
  SRSLTE_DCI_FORMAT1,
  SRSLTE_DCI_FORMAT1A,
  SRSLTE_DCI_FORMAT1B,
  SRSLTE_DCI_FORMAT1C,
  SRSLTE_DCI_FORMAT2,
  SRSLTE_DCI_FORMAT2A
};
const uint32_t nof_falcon_ue_all_formats = 7;

#endif




int DCISearch::inspect_dci_location_recursively(srslte_dci_msg_t *dci_msg,
                uint32_t cfi,
                falcon_cce_to_dci_location_map_t *cce_map,
                falcon_dci_location_t *location_list,
                uint32_t ncce,
                uint32_t L,
                uint32_t max_depth,
                falcon_dci_meta_format_t **meta_formats,
                uint32_t nof_formats,
                uint32_t enable_discovery,
                const dci_candidate_t parent_cand[])
{
  //uint16_t rnti_cand[nof_formats];
  //srslte_dci_msg_t dci_msg_cand[nof_formats];
  int hist_max_format_idx = -1;
  unsigned int hist_max_format_value = 0;
  unsigned int nof_cand_above_threshold = 0;

  dci_candidate_t* cand = falcon_alloc_candidates(nof_formats);

  //struct timeval t[3];

  if (cce_map[ncce].location[L] &&
      !cce_map[ncce].location[L]->occupied &&
      !cce_map[ncce].location[L]->checked &&
      cce_map[ncce].location[L]->sufficient_power)
  {
    // Decode candidate for each format (CRC and DCI)
    for(uint32_t format_idx=0; format_idx<nof_formats; format_idx++) {
      //gettimeofday(&t[1], nullptr);
      int result = srslte_pdcch_decode_msg_limit_avg_llr_power(&ue_dl.pdcch, &cand[format_idx].dci_msg, cce_map[ncce].location[L], meta_formats[format_idx]->format, cfi, &cand[format_idx].rnti, 0);
      stats.nof_decoded_locations++;
#ifdef PRINT_ALL_CANDIDATES
      printf("Cand. %d (sfn %d.%d, ncce %d, L %d, f_idx %d)\n", &cand[format_idx].rnti, sfn, sf_idx, ncce, L, format_idx);
#endif
      if(result != SRSLTE_SUCCESS) {
        ERROR("Error calling srslte_pdcch_decode_msg_limit_avg_llr_power\n");
      }
      if(meta_formats[format_idx]->format != cand[format_idx].dci_msg.format) {
        //format 0/1A format mismatch
        INFO("Dropped DCI cand. %d (format_idx %d) L%d ncce %d (format 0/1A mismatch)\n", cand[format_idx].rnti, format_idx, L, ncce);
        cand[format_idx].rnti = FALCON_ILLEGAL_RNTI;
        continue;
      }
      //gettimeofday(&t[2], nullptr);
      //get_time_interval(t);
      //printf("Decoding time: %d us (sfn %d, ncce %d, L %d)\n", t[0].tv_usec, sfn, ncce, L);

      // Filter 1a: Disallowed RNTI values for format 1C
      if ((meta_formats[format_idx]->format==SRSLTE_DCI_FORMAT1C) && (cand[format_idx].rnti > SRSLTE_RARNTI_END) && (cand[format_idx].rnti < SRSLTE_PRNTI)) {
        //DEBUG("Dropped DCI 1C cand. by illegal RNTI: 0x%x\n", cand[format_idx].rnti);
        INFO("Dropped DCI cand. %d (format_idx %d) L%d ncce %d (RNTI not allowed in format1C)\n", cand[format_idx].rnti, format_idx, L, ncce);
        cand[format_idx].rnti = FALCON_ILLEGAL_RNTI;
        continue;
      }
      // Filter 1a-RA: Disallowed formats for RA-RNTI
      if ((cand[format_idx].rnti > SRSLTE_RARNTI_START) &&
          (cand[format_idx].rnti < SRSLTE_RARNTI_END)) {
        if(meta_formats[format_idx]->format==SRSLTE_DCI_FORMAT1A) {
          INFO("Found RA-RNTI: 0x%x\n", cand[format_idx].rnti);
          ue_dl.current_rnti = cand[format_idx].rnti;
        }
        else {
          //DEBUG("Dropped DCI cand.: RA-RNTI only with format 1A: 0x%x\n", cand[format_idx].rnti);
          INFO("Dropped DCI cand. %d (format_idx %d) L%d ncce %d (RA-RNTI only format 1A)\n", cand[format_idx].rnti, format_idx, L, ncce);
          cand[format_idx].rnti = FALCON_ILLEGAL_RNTI;
          continue;
        }
      }
      // Shortcut: Decoding at smaller aggregation level still results in the same RNTI as parent.
      // This is a very strong indicator, that this or the parent DCI are valid.
      if(enableShortcutDiscovery &&
         enable_discovery &&
         parent_cand != nullptr &&
         parent_cand[format_idx].rnti == cand[format_idx].rnti &&
         !rntiManager.isForbidden(cand[format_idx].rnti, meta_formats[format_idx]->global_index)
         // !rnti_manager_is_forbidden(q.rnti_manager, cand[format_idx].rnti, meta_formats[format_idx]->global_index)
         )
      {
        INFO("RNTI matches to parent DCI cand. %d (format_idx %d), this: L%d ncce %d\n", cand[format_idx].rnti, meta_formats[format_idx]->global_index, L, ncce);
        falcon_free_candidates(cand);
        return -((int)format_idx+1);
      }

      // Filter 1b: Illegal DCI positions
      cand[format_idx].search_space_match_result = srslte_pdcch_validate_location(srslte_pdcch_nof_cce(&ue_dl.pdcch, cfi), ncce, L, sf_idx, cand[format_idx].rnti);
      if (cand[format_idx].search_space_match_result == 0) {
        //DEBUG("Dropped DCI cand. by illegal position: 0x%x\n", cand[format_idx].rnti);
        INFO("Dropped DCI cand. %d (format_idx %d) L%d ncce %d (illegal position)\n", cand[format_idx].rnti, format_idx, L, ncce);
        cand[format_idx].rnti = FALCON_ILLEGAL_RNTI;
        continue;
      }

#ifndef __NEW_HISTOGRAM__
      unsigned int occurence = rnti_histogram_get_occurence(&q->rnti_histogram[format_idx], cand[format_idx].rnti);
      // Online max-search
      if(occurence > RNTI_HISTOGRAM_THRESHOLD) {

#ifdef __DISABLED_FOR_TESTING__
        // Filter 2a (part2): Matches DCI-Format to recent DCI-Format for this RNTI
        if(cand[format_idx].dci_msg.format != SRSLTE_DCI_FORMAT0 &&
           cand[format_idx].dci_msg.format != q->rnti_format[cand[format_idx].rnti]) {
          INFO("Dropped DCI cand. by mismatching format: 0x%x\n", cand[format_idx].rnti);
          cand[format_idx].rnti = FALCON_ILLEGAL_RNTI;
          continue;
        }
#endif

        nof_cand_above_threshold++;
        if(occurence > hist_max_format_value) {
          hist_max_format_value = occurence;
          hist_max_format_idx = (int)format_idx;
        }
      }
#else
      //if(rnti_manager_validate_and_refresh(q.rnti_manager, cand[format_idx].rnti, meta_formats[format_idx]->global_index)) {
      if(rntiManager.validateAndRefresh(cand[format_idx].rnti, meta_formats[format_idx]->global_index)) {
        nof_cand_above_threshold++;
        hist_max_format_idx = (int)format_idx;
        //hist_max_format_value = rnti_manager_getFrequency(q.rnti_manager, cand[format_idx].rnti, meta_formats[format_idx]->global_index);
        hist_max_format_value = rntiManager.getFrequency(cand[format_idx].rnti, meta_formats[format_idx]->global_index);
      }
#endif
    }

    // if multiple active candidates found, select the most frequent
    if(nof_cand_above_threshold > 1) {
      INFO("Found %d matching candidates in different formats for same CCEs:\n", nof_cand_above_threshold);
      hist_max_format_idx = -1;
      uint32_t hist_max = 0;
      uint32_t hist = 0;
      for(uint32_t format_idx=0; format_idx<nof_formats; format_idx++) {
        if(cand[format_idx].rnti != FALCON_ILLEGAL_RNTI) {
          //hist = rnti_manager_getFrequency(q.rnti_manager, cand[format_idx].rnti, meta_formats[format_idx]->global_index);
          hist = rntiManager.getFrequency(cand[format_idx].rnti, meta_formats[format_idx]->global_index);
          INFO("\t DCI cand. %d (format_idx %d) L%d ncce %d freq %d\n", cand[format_idx].rnti, format_idx, L, ncce, hist);
          if(hist > hist_max) {
            hist_max = hist;
            hist_max_format_idx = (int)format_idx;
            //hist_max_format_value = rnti_manager_getFrequency(q.rnti_manager, cand[format_idx].rnti, meta_formats[format_idx]->global_index);
            hist_max_format_value = rntiManager.getFrequency(cand[format_idx].rnti, meta_formats[format_idx]->global_index);
          }
        }
      }
      if(hist_max_format_idx == -1) {
        INFO("\t Multiple valid DCI candidates for same CCE location, but none appear in histogram. Skipping all\n");
        nof_cand_above_threshold = 0;
      }
      else {
        INFO("\t Selected DCI cand. %d (format_idx %d)\n", cand[hist_max_format_idx].rnti, meta_formats[hist_max_format_idx]->global_index);
      }
    }

    cce_map[ncce].location[L]->checked = 1;

    int disambiguation_count = 0;
#if DCI_DISAMBIGUATION_DEPTH > 0
    // check, if the location of the preferred DCI is ambiguous with L-1
    // to prevent overshadowing of neighbouring DCI with L-1 in the right half
    if(nof_cand_above_threshold > 0 &&
       cand[hist_max_format_idx].search_space_match_result == SEARCH_SPACE_MATCH_RESULT_AMBIGUOUS ) {

      //descend only right half; pass nullptr as parent_rnti_cands; disable further shortcuts
      if(L > 0 && max_depth > 0) {
        disambiguation_count = inspect_dci_location_recursively(dci_msg, cfi, cce_map, location_list, ncce + (1 << (L-1)), L-1, max_depth-1, meta_formats, nof_formats, 0, nullptr);
      }
      if(disambiguation_count > 0) {
        INFO("Disambiguation discovered %d additional DCI\n", disambiguation_count);
      }
    }
    else
#endif
    // if no candidates were found, descend covered CCEs recursively
    if(nof_cand_above_threshold == 0) {
      // found nothing - recursion, if applicable
      int recursion_result = 0;
      if(L > 0 && max_depth > 0) {
        //descend left half; pass parent_rnti_cands for shortcuts...
        recursion_result += inspect_dci_location_recursively(dci_msg, cfi, cce_map, location_list, ncce, L-1, max_depth-1, meta_formats, nof_formats, enable_discovery, cand);
        if(recursion_result < 0) {
          //shortcut taken, activate RNTI
          INFO("Shortcut detected: RNTI: %d (format_idx %d)!\n", cand[-recursion_result - 1].rnti, -recursion_result - 1);
          hist_max_format_idx = -recursion_result - 1;
          //hist_max_format_value = rnti_manager_getFrequency(q.rnti_manager, cand[hist_max_format_idx].rnti, meta_formats[(uint32_t)hist_max_format_idx]->global_index);
          hist_max_format_value = rntiManager.getFrequency(cand[hist_max_format_idx].rnti, meta_formats[(uint32_t)hist_max_format_idx]->global_index);
          nof_cand_above_threshold = 1;
#if DCI_DISAMBIGUATION_DEPTH > 0
          //Very rare special case: The DCI just discovered may overshadow neighbouring DCI with L-1 in the right half
          if(cand[hist_max_format_idx].search_space_match_result == SEARCH_SPACE_MATCH_RESULT_AMBIGUOUS ) {
            //descend only right half; pass nullptr as parent_rnti_cands; disable further shortcuts
            disambiguation_count = inspect_dci_location_recursively(dci_msg, cfi, cce_map, location_list, ncce + (1 << (L-1)), L-1, MIN(max_depth, DCI_DISAMBIGUATION_DEPTH)-1, meta_formats, nof_formats, 0, nullptr);
            if(disambiguation_count > 0) {
              INFO("Disambiguation discovered %d additional DCI\n", disambiguation_count);
            }
          }
#endif
          //rnti_manager_activate_and_refresh(q.rnti_manager, cand[hist_max_format_idx].rnti, meta_formats[(uint32_t)hist_max_format_idx]->global_index, RM_ACT_SHORTCUT);
          rntiManager.activateAndRefresh(cand[hist_max_format_idx].rnti, meta_formats[(uint32_t)hist_max_format_idx]->global_index, RM_ACT_SHORTCUT);
        }
        else {
          //descend right half; pass NULL as parent_rnti_cands
          recursion_result += inspect_dci_location_recursively(dci_msg, cfi, cce_map, location_list, ncce + (1 << (L-1)), L-1, max_depth-1, meta_formats, nof_formats, enable_discovery, nullptr);
        }
      }

      // evaluate recursion:
      // recursion_result <  0: shortcut taken
      // recursion_result == 0: nothing found, add all cand. to histogram (if enable_discovery == true)
      // recursion_result >  0: return this value as number of findings
      if(recursion_result == 0) {
        if(enable_discovery) {
          // put all potentionally valid cand. to histogram
          for(uint32_t format_idx=0; format_idx<nof_formats; format_idx++) {
            if(cand[format_idx].rnti != FALCON_ILLEGAL_RNTI) {
  #ifndef __NEW_HISTOGRAM__
              rnti_histogram_add_rnti(&q->rnti_histogram[0], cand[format_idx].rnti);
  #else
              //rnti_manager_add_candidate(q.rnti_manager, cand[format_idx].rnti, meta_formats[format_idx]->global_index);
              rntiManager.addCandidate(cand[format_idx].rnti, meta_formats[format_idx]->global_index);
              INFO("Dropped DCI cand. %d (format_idx %d) L%d ncce %d (spurious/infrequent), add to histogram\n", cand[format_idx].rnti, format_idx, L, ncce);
  #endif

  #ifdef __DISABLED_FOR_TESTING__
              // Filter 2a (part1): Remember DCI-Format of this RNTI
              if(cand[format_idx].dci_msg.format != SRSLTE_DCI_FORMAT0) {
                q->rnti_format[cand[format_idx].rnti] = cand[format_idx].dci_msg.format;
              }
  #endif
            }
          }
        }
        falcon_free_candidates(cand);
        return 0;
      }
      else if(recursion_result > 0) {
        falcon_free_candidates(cand);
        return recursion_result;
      }
      //else if(recursion_result < 0) { do not return }
    } // recursion

    // Found matching candidate (either by histogram or by shortcut in recursion)
    if(nof_cand_above_threshold > 0) {
      // found candidate - no recursion
      cce_map[ncce].location[L]->used = 1;

      // mark all other locations which overlap this location as checked
      for(uint32_t cce_idx=ncce; cce_idx < ncce+(1 << L); cce_idx++) {
        for(uint32_t aggr=0; aggr < 4; aggr++) {
          if (cce_map[cce_idx].location[aggr]) {
            cce_map[cce_idx].location[aggr]->occupied = 1;
            cce_map[cce_idx].location[aggr]->checked = 1;
          }
        }
      }

      // consider only the most frequent dci candidate; ignore others
#ifndef __NEW_HISTOGRAM__
      rnti_histogram_add_rnti(&q->rnti_histogram[0], cand[hist_max_format_idx].rnti);
#else
      //rnti_manager_add_candidate(q.rnti_manager, cand[hist_max_format_idx].rnti, meta_formats[(uint32_t)hist_max_format_idx]->global_index);
      rntiManager.addCandidate(cand[hist_max_format_idx].rnti, meta_formats[(uint32_t)hist_max_format_idx]->global_index);
#endif

      meta_formats[(uint32_t)hist_max_format_idx]->hits++;
      // correct L if this candidate's L is ambiguous and disambiguation found another dci overshadoved by this candidate
      uint32_t L_disamb = disambiguation_count > 0 ? L-1 : L;

      // process the accepted DCI
      dciCollection.addCandidate(cand[hist_max_format_idx], srslte_dci_location_t{L_disamb, ncce}, hist_max_format_value);

      falcon_free_candidates(cand);
      return 1 + disambiguation_count;
    }
    else {
      // this should never happen
      ERROR("nof_cand_above_threshold <= 0 but this was not caught earlier.\n");
    }
  }
  falcon_free_candidates(cand);
  return 0;
}

int DCISearch::recursive_blind_dci_search(srslte_dci_msg_t *dci_msg,
                                          uint32_t cfi)
{
  falcon_dci_location_t locations[MAX_CANDIDATES_BLIND];
  falcon_cce_to_dci_location_map_t cce_map[MAX_NUM_OF_CCE] = {{{nullptr}, 0}};
  uint32_t nof_locations;
  int ret = 0;
  stats.nof_cce += srslte_pdcch_nof_cce(&ue_dl.pdcch, cfi);

  //struct timeval t[3];

  /* Generate PDCCH candidates and a lookup map for cce -> parent dci */
  nof_locations = srslte_pdcch_ue_locations_all_map(&ue_dl.pdcch, locations, MAX_CANDIDATES_BLIND, cce_map, MAX_NUM_OF_CCE, sf_idx, cfi);
  stats.nof_locations +=nof_locations;

  /* Calculate power levels on each cce*/
  srslte_pdcch_cce_avg_llr_power(&ue_dl.pdcch, cfi, cce_map, MAX_NUM_OF_CCE);

  ue_dl.current_rnti = 0xffff;

  // Check primary DCI formats (the most frequent)

  // inspect all locations at this aggregation level recursively
  for(unsigned int location_idx=0; location_idx < nof_locations; location_idx++) {
    ret += inspect_dci_location_recursively(dci_msg,
                                            cfi,
                                            cce_map,
                                            locations,
                                            locations[location_idx].ncce,
                                            locations[location_idx].L,
                                            MAX_RECURSION_DEPTH,
                                            metaFormats.getPrimaryMetaFormats(),
                                            metaFormats.getNofPrimaryMetaFormats(),
                                            1,
                                            nullptr);
  }

  uint32_t primary_missed = srslte_pdcch_nof_missed_cce(&ue_dl.pdcch, cfi, cce_map, MAX_NUM_OF_CCE);
  if(primary_missed > 0) {
    INFO("Primary formats: Missed CCEs in SFN %d.%d: %d\n", sfn, sf_idx, primary_missed);
  }

  // Check secondary DCI formats (remaining)
  if(!metaFormats.skipSecondaryMetaFormats()) {
    // Reset the checked flag for all locations
    srslte_pdcch_uncheck_ue_locations(locations, nof_locations);

    for(unsigned int location_idx=0; location_idx < nof_locations; location_idx++) {
      ret += inspect_dci_location_recursively(dci_msg,
                                              cfi,
                                              cce_map,
                                              locations,
                                              locations[location_idx].ncce,
                                              locations[location_idx].L,
                                              MAX_RECURSION_DEPTH,
                                              metaFormats.getSecondaryMetaFormats(),
                                              metaFormats.getNofSecondaryMetaFormats(),
                                              1,
                                              nullptr);
    }
  }

  if(dciCollection.hasCollisionDL()) {
    stats.nof_subframe_collisions_dw++;
    INFO("DL collision detected\n");
  }
  if(dciCollection.hasCollisionUL()) {
    stats.nof_subframe_collisions_up++;
    INFO("UL collision detected\n");
  }
  uint32_t missed = srslte_pdcch_nof_missed_cce(&ue_dl.pdcch, cfi, cce_map, MAX_NUM_OF_CCE);
  if(missed > 0) {
    INFO("Missed CCEs in SFN %d.%d: %d\n", sfn, sf_idx, missed);
  }
  stats.nof_missed_cce += missed;
  rntiManager.stepTime();

  return ret;
}

DCISearch::DCISearch(srslte_ue_dl_t& ue_dl,
                     const DCIMetaFormats& metaFormats,
                     RNTIManager& rntiManager,
                     SubframeInfo& subframeInfo,
                     uint32_t sf_idx,
                     uint32_t sfn) :
  ue_dl(ue_dl),
  metaFormats(metaFormats),
  rntiManager(rntiManager),
  dciCollection(subframeInfo.getDCICollection()),
  subframePower(subframeInfo.getSubframePower()),
  sf_idx(sf_idx),
  sfn(sfn),
  stats(),
  enableShortcutDiscovery(true)
{

}

int DCISearch::search() {
  srslte_dci_msg_t dci_msg;
  int ret = SRSLTE_ERROR;
  uint32_t cfi;

  struct timeval timestamp;
  gettimeofday(&timestamp, nullptr);
  dciCollection.setTimestamp(timestamp);

  { //PrintLifetime lt("###>> FFT: ");
    if ((ret = srslte_ue_dl_decode_fft_estimate_mbsfn(&ue_dl, sf_idx, &cfi, SRSLTE_SF_NORM)) < 0) {
      return ret;
    }
    subframePower.computePower(ue_dl.sf_symbols_m[0]);  //first antenna only (for now)
    dciCollection.setSubframe(sfn, sf_idx, cfi);
  }
  { //PrintLifetime lt("###>> Chest + LLR extract: ");
    float noise_estimate = srslte_chest_dl_get_noise_estimate(&ue_dl.chest);

    if (srslte_pdcch_extract_llr_multi(&ue_dl.pdcch, ue_dl.sf_symbols_m, ue_dl.ce_m, noise_estimate, sf_idx, cfi)) {
      fprintf(stderr, "Error extracting LLRs\n");
      return SRSLTE_ERROR;
    }
  }

  ret = recursive_blind_dci_search(&dci_msg, cfi);
  stats.nof_subframes++;

  return ret;
}

DCIBlindSearchStats& DCISearch::getStats() {
  return stats;
}

void DCISearch::setShortcutDiscovery(bool enable) {
  enableShortcutDiscovery = enable;
}

bool DCISearch::getShortcutDiscovery() const {
  return enableShortcutDiscovery;
}
