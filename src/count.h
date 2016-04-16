// count.h

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

#ifndef POCOLM_COUNT_H_
#define POCOLM_COUNT_H_

#include <iostream>

namespace pocolm {


/**
   This class is used to store a special type of count that we use in estimating
   these language models.  You can think of it as a type of 'extended' float that
   stores the sum of a bunch of individual small counts or parts of counts.
   In addition to storing the total count, it also stores the top-1 "part"
   (i.e. the largest of the component parts), and also the runners up, which
   we call top-2 and top-3.


 */
class Count {
 public:
  float total;
  float top1;
  float top2;
  float top3;

  Count() { }

  Count(float f): total(f), top1(f),
                  top2(0.0f), top3(0.0f) {
    assert(f >= 0.0f);
  }

  Count(const Count &other):
      total(other.total), top1(other.top1),
      top2(other.top2), top3(other.top3) { }

  // use default assignment operator.

  inline Count &operator = (const Count &other) {
    total = other.total;
    top1 = other.top1;
    top2 = other.top2;
    return *this;
  }

  inline Count &operator = (float f) {
    total = f;
    top1 = f;
    top2 = 0.0f;
    top3 = 0.0f;
    return *this;
  }

  // add another count.
  inline void Add(const Count &other);

  // Add a single float... c.Add(f) gives identical results to c.Add(Count(f)),
  // it's just more efficient.
  inline void Add(float f);


  /**
     This function is used in reverse-mode automatic differentiation
     of summations in counts.  Suppose in the 'forward' computation
     you had done as follows:
        Count c(0.0f);
        c.Add(c1);  c.Add(c2);  c.Add(c3); ...
     And suppose you have obtained a derivative
        Count c_deriv(...);
     which interpreted as the derivative of a scalar objective function w.r.t.
     the total count c.  Then you can obtain the derivatives w.r.t. c1, c2 and
     c3 as follows:
        Count c1_deriv(0.0f), c2_deriv(0.0f), c3_deriv(0.0f);
        c.AddBackward(c1, &c1_deriv, &c_deriv);
        c.AddBackward(c2, &c2_deriv, &c_deriv);
        c.AddBackward(c3, &c3_deriv, &c_deriv);

     Actually this will add to 'other_deriv', values which can be interpreted as
     a subgradient of the function w.r.t. those inputs (the function is
     non-smooth, which is why we need to mention this).  The reason why
     'this_deriv' needs to be non-const is that we need to avoid counting things
     twice in case of ties, so we need to zero out the top1, top2 or top3 of
     'this_deriv' in case we already propagated the derivative w.r.t that top1,
     top2 or top3.  At the end of processing all the summands, the top1, top2
     and top3 of 'this_deriv' should all be zero.
  */
  inline void AddBackward(const Count &other,
                          Count *this_deriv,
                          Count *other_deriv);

  inline void AddBackward(float f,
                          Count *this_deriv,
                          Count *other_deriv);

  void Write(std::ostream &os, bool binary);

  void Read(std::istream &os, bool binary);

  // returns total * other.total + top1 * other.top1, etc.
  // only really useful when dealing with derivatives.
  inline float DotProduct(const Count &other);

  // this does some assertions to make sure this is a well-formed
  // count (do not apply this to counts that represent derivatives).
  inline void Check() const;

};

}

#include "count-inl.h"


#endif  // POCOLM_COUNT_H_