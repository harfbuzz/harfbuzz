// SPDX-License-Identifier: MIT
// Copyright 2011, SIL International, All rights reserved.


#pragma once

#include "inc/Code.h"

namespace graphite2 {

class json;

struct Rule {
  const vm::Machine::Code * constraint,
                          * action;
  unsigned short   sort;
  byte             preContext;
#ifndef NDEBUG
  uint16_t           rule_idx;
#endif

  Rule();
  ~Rule() {}

  CLASS_NEW_DELETE;

private:
  Rule(const Rule &);
  Rule & operator = (const Rule &);
};

inline
Rule::Rule()
: constraint(0),
  action(0),
  sort(0),
  preContext(0)
{
#ifndef NDEBUG
  rule_idx = 0;
#endif
}

class State;

class Rules
{
public:
  struct Entry  
  { 
    const Rule   * rule; 
    
    bool operator == (const Entry &r) const;
    bool operator < (const Entry &r) const;
  };

  static constexpr size_t MAX_RULES=128;
  using iterator = Entry *;
  using const_iterator = Entry const *;


  Rules();
  void            clear()       { m_end = m_begin; }
  const_iterator  begin() const { return m_begin; }
  const_iterator  end() const   { return m_end; }
  bool            empty() const { return m_begin == m_end; }
  size_t          size() const  { return m_end - m_begin; }

  void accumulate_rules(const State &state);

private:
  Entry * m_begin,
        * m_end,
          m_rules[MAX_RULES*2];
};


struct State
{
  const Rules::Entry  * rules,
                      * rules_end;

  bool   empty() const  { return rules_end == rules; }
};


inline
bool Rules::Entry::operator == (const Entry &r) const 
{ 
  return rule == r.rule; 
}

inline
bool Rules::Entry::operator < (const Entry &r) const {
  const unsigned short lsort = rule->sort,
                        rsort = r.rule->sort;
  return lsort > rsort || (lsort == rsort && rule < r.rule);
}

inline
Rules::Rules()
  : m_begin(m_rules), m_end(m_rules)
{
}

inline
void Rules::accumulate_rules(const State &state)
{
  // Only bother if there are rules in the State object.
  if (state.empty()) return;

  // Merge the new sorted rules list into the current sorted result set.
  auto const * lre = begin(), * rre = state.rules;
  auto       * out = m_rules + (m_begin == m_rules)*MAX_RULES;
  auto const * const lrend = out + MAX_RULES,
             * const rrend = state.rules_end;
  m_begin = out;
  while (lre != end() && out != lrend)
  {
    if (*lre < *rre)      *out++ = *lre++;
    else if (*rre < *lre) { *out++ = *rre++; }
    else                { *out++ = *lre++; ++rre; }

    if (rre == rrend)
    {
      while (lre != end() && out != lrend) { *out++ = *lre++; }
      m_end = out;
      return;
    }
  }
  while (rre != rrend && out != lrend) { *out++ = *rre++; }
  m_end = out;
}

} // namespace graphite2
