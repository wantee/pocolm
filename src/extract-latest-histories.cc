// extract-latest-histories.cc

// Copyright     2016  Johns Hopkins University (Author: Daniel Povey)

// See ../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABILITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <numeric>
#include <algorithm>
#include <stdlib.h>
#include "pocolm-types.h"
#include "lm-state.h"


/**
  This program extract the latest histories for every lm-states.
*/

int main (int argc, const char **argv) {
  if (argc != 1) {
    std::cerr << "extract-latest-histories: expected usage: extract-latest-histories <counts.int >latest_hist.txt\n";
    exit(1);
  }

  int64 num_lm_states = 0;
  int64 num_counts = 0;

  int32 hist;
  while (std::cin.peek(), !std::cin.eof()) {
    pocolm::IntLmState lm_state;
    lm_state.Read(std::cin);
    if (lm_state.history[0] != hist) {
        assert(lm_state.history[0] > hist);
        hist = lm_state.history[0];;
        std::cout << hist << std::endl;
    }
    num_lm_states++;
    num_counts += lm_state.counts.size();
  }

  std::cerr << "extract-latest-histories: processed "
            << num_lm_states << " LM states, with "
            << num_counts << " individual n-grams.\n";
  return 0;
}

/*
export LC_ALL=C
( echo 11 12 13; echo 11 12 13 14 ) | get-text-counts 3 | sort | uniq -c | get-int-counts /dev/null int.2 int.3

print-int-counts <int.2
# [ 1 ]: 11->2
extract-latest-histories <int.2
#1

print-int-counts <int.3
# [ 11 1 ]: 12->2
# [ 12 11 ]: 13->2
# [ 13 12 ]: 2->1 14->1
# [ 14 13 ]: 2->1
extract-latest-histories <int.3
#11
#12
#13
#14
 */
