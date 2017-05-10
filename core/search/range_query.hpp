//
// IResearch search engine 
// 
// Copyright (c) 2016 by EMC Corporation, All Rights Reserved
// 
// This software contains the intellectual property of EMC Corporation or is licensed to
// EMC Corporation from third parties. Use of this software and the intellectual property
// contained therein is expressly limited to the terms and conditions of the License
// Agreement under which it is provided by or on behalf of EMC.
// 

#ifndef IRESEARCH_RANGE_QUERY_H
#define IRESEARCH_RANGE_QUERY_H

#include <map>
#include <unordered_map>

#include "filter.hpp"
#include "cost.hpp"
#include "utils/string.hpp"

NS_ROOT

struct term_reader;

//////////////////////////////////////////////////////////////////////////////
/// @class range_state
/// @brief cached per reader range state
//////////////////////////////////////////////////////////////////////////////
struct range_state {
  range_state() = default;

  range_state(range_state&& rhs) NOEXCEPT
    : reader(rhs.reader),
      min_term(std::move(rhs.min_term)),
      min_cookie(std::move(rhs.min_cookie)),
      estimation(rhs.estimation),
      count(std::move(rhs.count)),
      scored_states(std::move(rhs.scored_states)) {
    rhs.reader = nullptr;
    rhs.count = 0;
    rhs.estimation = 0;
  }

  range_state& operator=(range_state&& other) NOEXCEPT {
    if (this == &other) {
      return *this;
    }

    reader = std::move(other.reader);
    min_term = std::move(other.min_term);
    min_cookie = std::move(other.min_cookie);
    estimation = std::move(other.estimation);
    count = std::move(other.count);
    scored_states = std::move(other.scored_states);
    other.reader = nullptr;
    other.count = 0;
    other.estimation = 0;

    return *this;
  }

  const term_reader* reader{}; // reader using for iterate over the terms
  bstring min_term; // minimum term to start from
  seek_term_iterator::cookie_ptr min_cookie; // cookie corresponding to the start term
  cost::cost_t estimation{}; // per-segment query estimation
  size_t count{}; // number of terms to process from start term

  // scored states/stats by their offset in range_state
  typedef std::unordered_map<size_t, attributes> scored_states_t;
  scored_states_t scored_states;
}; // reader_state

//////////////////////////////////////////////////////////////////////////////
/// @brief object to collect and track a limited number of scorers
//////////////////////////////////////////////////////////////////////////////
class limited_sample_scorer {
 public:
  limited_sample_scorer(size_t scored_terms_limit);
  void collect(
    size_t priority, // priority of this entry, lowest priority removed first
    size_t scored_state_id, // state identifier used for querying of attributes
    iresearch::range_state& scored_state, // state containing this scored term
    const iresearch::sub_reader& reader, // segment reader for the current term
    seek_term_iterator::cookie_ptr&& cookie // term-reader term offset cache
  );
  void score(order::prepared::stats& stats);

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief a representation of a term cookie with its asociated range_state
  //////////////////////////////////////////////////////////////////////////////
  struct scored_term_state_t {
    seek_term_iterator::cookie_ptr cookie; // term offset cache
    iresearch::range_state& state; // state containing this scored term
    size_t state_offset;

    const iresearch::sub_reader& sub_reader; // segment reader for the current term
    scored_term_state_t(
      const iresearch::sub_reader& sr,
      iresearch::range_state& scored_state,
      size_t scored_state_offset,
      seek_term_iterator::cookie_ptr&& scored_cookie
    ):
      cookie(std::move(scored_cookie)),
      state(scored_state),
      state_offset(scored_state_offset),
      sub_reader(sr) {
    }
  };

  typedef std::multimap<size_t, scored_term_state_t> scored_term_states_t;
  scored_term_states_t scored_states_;
  size_t scored_terms_limit_;
};

//////////////////////////////////////////////////////////////////////////////
/// @class range_query
/// @brief compiled query suitable for filters with continious range of terms
///        like "by_range" or "by_prefix". 
//////////////////////////////////////////////////////////////////////////////
class range_query : public filter::prepared {
 public:
  typedef states_cache<range_state> states_t;

  DECLARE_SPTR(range_query);

  explicit range_query(states_t&& states);

  virtual score_doc_iterator::ptr execute(
      const sub_reader& rdr,
      const order::prepared& ord) const override;

 private:
  states_t states_;
}; // range_query 

NS_END // ROOT

#endif // IRESEARCH_RANGE_QUERY_H 
