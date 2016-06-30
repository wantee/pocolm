// filter-int-counts.cc

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
  This program filters all int-counts with latest history in the hist list.
*/

namespace pocolm {

class IntCountFilter {
 public:
  IntCountFilter(int argc,
                 const char **argv): total_states_(0), total_ngrams_(0),
                                     filtered_states_(0), filtered_ngrams_(0) {
    ReadArgs(argc, argv);
    ProcessInput();
    ProduceOutput();
  }

  ~IntCountFilter() {
  }
 private:
  void ReadArgs(int argc, const char **argv) {
      assert(argc == 3);

      counts_input_.open(argv[1], std::ios_base::binary|std::ios_base::in);
      if (counts_input_.fail()) {
          std::cerr << "filter-int-counts: error opening '" << argv[1]
              << "' for reading\n";
          exit(1);
      }
      hist_list_input_.open(argv[2], std::ios_base::binary|std::ios_base::in);
      if (hist_list_input_.fail()) {
          std::cerr << "filter-int-counts: error opening '" << argv[2]
              << "' for reading\n";
          exit(1);
      }
  }

  void ProcessInput() {
    IntLmState lm_state;
    int32 hist = 0;

    if (hist_list_input_.peek(), !hist_list_input_.eof()) {
        hist_list_input_ >> hist;
        assert(hist > 0);
    }
    if (counts_input_.peek(), !counts_input_.eof()) {
        lm_state.Read(counts_input_);
        ++total_states_;
        total_ngrams_ += lm_state.counts.size();
    }
    while (true) {
      if (lm_state.history[0] == hist) {
        lm_state.Write(std::cout);
        ++filtered_states_;
        filtered_ngrams_ += lm_state.counts.size();

        if (counts_input_.peek(), counts_input_.eof()) {
          break;
        }
        lm_state.Read(counts_input_);
        ++total_states_;
        total_ngrams_ += lm_state.counts.size();
      } else if (lm_state.history[0] < hist) {
        if (counts_input_.peek(), counts_input_.eof()) {
          break;
        }
        lm_state.Read(counts_input_);
        ++total_states_;
        total_ngrams_ += lm_state.counts.size();
      } else {
        if (hist_list_input_.peek(), hist_list_input_.eof()) {
          break;
        }
        hist_list_input_ >> hist;
        assert(hist > 0);
      }
    }
  }

  void ProduceOutput() {
    while (counts_input_.peek(), !counts_input_.eof()) {
        IntLmState lm_state;
        lm_state.Read(counts_input_);
        ++total_states_;
        total_ngrams_ += lm_state.counts.size();
    }

    std::cerr << "filter-int-counts: filtered "
              << filtered_states_ << " LM states(with "
              << filtered_ngrams_ << " n-grams) from total "
              << total_states_ << " LM states(with "
              << total_ngrams_ << " n-grams).\n";
  }

  std::ifstream hist_list_input_;
  std::ifstream counts_input_;

  int64 total_states_;
  int64 total_ngrams_;
  int64 filtered_states_;
  int64 filtered_ngrams_;
};

}  // namespace pocolm

int main (int argc, const char **argv) {
  if (argc <= 1) {
    std::cerr << "filter-int-counts: expected usage: <int-counts> <hist-list>\n"
              << " ( it writes the filtered int-counts to stdout).  "
              << "For example:\n"
              << " filter-int-counts counts/int.3 counts/latest_hist.3 > filter_counts/int.3\n";
    exit(1);
  }

  // everything happens in the constructor of class IntCountFilter.
  pocolm::IntCountFilter filter(argc, argv);

  return 0;
}


/*
export LC_ALL=C
( echo 11 12 13; echo 11 12 13 14; echo 11 11 13 15 ) | get-text-counts 3 | sort | uniq -c | get-int-counts /dev/null int.tr.2 int.tr.3

( echo 11 12 13 ) | get-text-counts 3 | sort | uniq -c | get-int-counts /dev/null int.dev.2 int.dev.3

print-int-counts <int.tr.2
# [ 1 ]: 11->3
print-int-counts <int.dev.2
# [ 1 ]: 11->1
extract-latest-histories <int.dev.2 > latest_hist.2
filter-int-counts int.tr.2 latest_hist.2 | print-int-counts
# [ 1 ]: 11->3

print-int-counts <int.tr.3
# [ 11 1 ]: 11->1 12->2
# [ 11 11 ]: 13->1
# [ 12 11 ]: 13->2
# [ 13 11 ]: 15->1
# [ 13 12 ]: 2->1 14->1
# [ 14 13 ]: 2->1
# [ 15 13 ]: 2->1
print-int-counts <int.dev.3
# [ 11 1 ]: 12->1
# [ 12 11 ]: 13->1
# [ 13 12 ]: 2->1
extract-latest-histories <int.dev.3 > latest_hist.3
filter-int-counts int.tr.3 latest_hist.3 | print-int-counts
# [ 11 1 ]: 11->1 12->2
# [ 11 11 ]: 13->1
# [ 12 11 ]: 13->2
# [ 13 11 ]: 15->1
# [ 13 12 ]: 2->1 14->1
 */
