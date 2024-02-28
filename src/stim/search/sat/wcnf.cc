// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "stim/search/sat/wcnf.h"


using namespace stim;

typedef double Weight;
constexpr Weight HARD_CLAUSE_WEIGHT = -1.0;

constexpr size_t BOOL_LITERAL_FALSE = SIZE_MAX - 1;
constexpr size_t BOOL_LITERAL_TRUE = SIZE_MAX;
struct BoolRef {
  size_t variable = BOOL_LITERAL_FALSE;
  bool negated = false;
  BoolRef operator~() const {
    return {variable, !negated};
  }
  static BoolRef False() {
    return {BOOL_LITERAL_FALSE, false};
  }
  static BoolRef True() {
    return {BOOL_LITERAL_TRUE, false};
  }
};

struct Clause {
  std::vector<BoolRef> vars;
  Weight weight = HARD_CLAUSE_WEIGHT;
  void add_var(BoolRef x) {
    vars.push_back(x);
  }
};

struct MaxSATInstance {
  size_t num_variables = 0;
  Weight max_weight = 0;
  std::vector<Clause> clauses;
  BoolRef new_bool() {
    return {num_variables++};
  }
  void add_clause(Clause& clause) {
    if (clause.weight != HARD_CLAUSE_WEIGHT) {
      if (clause.weight <= 0) {
        throw std::invalid_argument("Clauses must have positive weight or HARD_CLAUSE_WEIGHT.");
      }
      max_weight = std::max(max_weight, clause.weight);
    }
    clauses.push_back(clause);
  }
  BoolRef Xor(BoolRef& x, BoolRef& y) {
    if (x.variable == BOOL_LITERAL_FALSE) {
      return y;
    }
    if (x.variable == BOOL_LITERAL_TRUE) {
      return ~y;
    }
    if (y.variable == BOOL_LITERAL_FALSE) {
      return x;
    }
    if (y.variable == BOOL_LITERAL_TRUE) {
      return ~x;
    }
    BoolRef z = new_bool();
    // Forbid strings (x, y, z) such that z != XOR(x, y)
    {
      Clause clause;
      // hard clause (x, y, z) != (0, 0, 1)
      clause.add_var(x);
      clause.add_var(y);
      clause.add_var(~z);
      add_clause(clause);
    }
    {
      Clause clause;
      // hard clause (x, y, z) != (0, 1, 0)
      clause.add_var(x);
      clause.add_var(~y);
      clause.add_var(z);
      add_clause(clause);
    }
    {
      Clause clause;
      // hard clause (x, y, z) != (1, 0, 0)
      clause.add_var(~x);
      clause.add_var(y);
      clause.add_var(z);
      add_clause(clause);
    }
    {
      Clause clause;
      // hard clause (x, y, z) != (1, 1, 1)
      clause.add_var(~x);
      clause.add_var(~y);
      clause.add_var(~z);
      add_clause(clause);
    }
    return z;
  }

  size_t quantized_weight(size_t num_distinct_weights, size_t top, Weight weight) {
    if (weight == HARD_CLAUSE_WEIGHT) {
      return top;
    }
    return std::max(1ul, (size_t)std::round(weight / max_weight * (double)num_distinct_weights));
  }

  std::string to_wdimacs(size_t num_distinct_weights=1) {
    if (num_distinct_weights < 1) {
      throw std::invalid_argument("There must be at least 1 distinct weight value");
    }
    // 'top' is a special weight used to indicate a hard clause.
    // Should be at least the sum of the weights of all soft clauses plus 1.
    size_t top = 1 + num_distinct_weights * clauses.size();

    // WDIMACS header format: p wcnf nbvar nbclauses top
    // see http://www.maxhs.org/docs/wdimacs.html
    std::stringstream ss;
    ss << "p wcnf "<< num_variables << " "<<clauses.size() << " " << top << "\n";

    // Add clauses, 1 on each line.
    for (const auto& clause : clauses) {
      // WDIMACS clause format: weight var1 var2 ...
      // To show negation of a variable, the index should be negated.
      ss << quantized_weight(num_distinct_weights, top, clause.weight);
      for (size_t i=0; i<clause.vars.size(); ++i) {
        BoolRef var = clause.vars[i];
        // Variables are 1-indexed
        if (var.negated) {
          ss << " -" << (var.variable + 1);
        } else {
          ss << " " << (var.variable + 1);
        }
      }
      // Each clause ends with 0
      ss << " 0\n";
    }
    return ss.str();
  }
};

std::string stim::shortest_undetectable_logical_error_wcnf(const DetectorErrorModel &model, size_t num_distinct_weights) {
  MaxSATInstance inst;

  size_t num_observables = model.count_observables();
  size_t num_detectors = model.count_detectors();
  size_t num_errors = model.count_errors();
  if (num_observables == 0 or num_detectors == 0 or num_errors == 0) {
    std::stringstream err_msg;
    err_msg << "Failed to find any logical errors.";
    if (num_observables == 0) {
        err_msg << "\n    WARNING: NO OBSERVABLES. The circuit or detector error model didn't define any observables, "
                   "making it vacuously impossible to find a logical error.";
    }
    if (num_detectors == 0) {
        err_msg << "\n    WARNING: NO DETECTORS. The circuit or detector error model didn't define any detectors.";
    }
    if (num_errors == 0) {
        err_msg << "\n    WARNING: NO ERRORS. The circuit or detector error model didn't include any errors, making it "
                   "vacuously impossible to find a logical error.";
    }
    throw std::invalid_argument(err_msg.str());
  }
  if (num_distinct_weights == 0) {
    throw std::invalid_argument("num_distinct_weights must be >= 1");
  }

  MaxSATInstance instance;
  // Create a boolean variable for each error, which indicates whether it is activated.
  std::vector<BoolRef> errors_activated;
  for (size_t i=0; i<num_errors; ++i) {
    errors_activated.push_back(instance.new_bool());
  }

  std::vector<BoolRef> detectors_activated(num_detectors, BoolRef::False());
  std::vector<BoolRef> observables_flipped(num_observables, BoolRef::False());

  size_t error_index = 0;
  model.iter_flatten_error_instructions([&](const DemInstruction &e) {
      if (e.arg_data[0] != 0) {
        BoolRef err_x = errors_activated[error_index];
        // Add parity contribution to the detectors and observables
        for (const auto &t : e.target_data) {
            if (t.is_relative_detector_id()) {
              detectors_activated[t.val()] = instance.Xor(detectors_activated[t.val()], err_x);
            } else if (t.is_observable_id()) {
              observables_flipped[t.val()] = instance.Xor(observables_flipped[t.val()], err_x);
            }
        }
        // Add a soft clause for this error to be inactive
        Clause clause;
        clause.add_var(~err_x);
        clause.weight = -std::log(e.arg_data[0] / (1 - e.arg_data[0]));
        instance.add_clause(clause);
      }
      ++error_index;
  });
  assert(error_index == num_errors);

  // Add a hard clause for each detector to be inactive
  for (size_t d=0; d<num_detectors; ++d) {
      Clause clause;
      if (detectors_activated[d].variable == BOOL_LITERAL_FALSE) continue;
      clause.add_var(~detectors_activated[d]);
      instance.add_clause(clause);
  }

  // Add a hard clause for any observable to be flipped
  Clause clause;
  for (size_t i=0; i<num_observables; ++i) {
    clause.add_var(observables_flipped[i]);
  }
  instance.add_clause(clause);

  return instance.to_wdimacs(num_distinct_weights);
}
