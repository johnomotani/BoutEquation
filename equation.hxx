/*
  Copyright J. Omotani, UKAEA, 2019
  email: john.omotani@ukaea.uk

  Equation is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Equation is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Equation.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _BOUT_EQUATION_H_
#define _BOUT_EQUATION_H_

#include <field3d.hxx>
#include <options.hxx>

// Wrapper for Field3D, allowing a restricted number of operations to be performed on it.
// Ensures that changes made to the EquationTerm are also applied consistently to ddt(f).
class EquationTerm {
public:
  EquationTerm(const bool save, Field3D& ddt_f_in, const int& gcount, int& lcount)
    : save_term(save), ddt_f(ddt_f_in), global_counter(gcount),
      equation_counter(lcount) {}

  template<typename T>
  EquationTerm& operator=(const T& rhs) {
    if (save_term) {
      term = rhs;
    }
    if (equation_counter == global_counter) {
      ddt_f += rhs;
    } else {
      // First term added to the equation on this time-step
      ddt_f = rhs;
      equation_counter = global_counter;
    }

    local_counter = global_counter;

    return *this;
  }

  template<typename T>
  EquationTerm& operator+=(const T& rhs) {
    // Check operator= has been used first
    ASSERT1(local_counter == global_counter);

    if (save_term) {
      term += rhs;
    }
    ddt_f += rhs;

    return *this;
  }

  template<typename T>
  EquationTerm& operator-=(const T& rhs) {
    // Check operator= has been used first
    ASSERT1(local_counter == global_counter);

    if (save_term) {
      term -= rhs;
    }
    ddt_f -= rhs;

    return *this;
  }

  // Read-only access to the Field3D
  const Field3D& field3D() {
    return term;
  }
private:
  friend class Equation;

  const bool save_term;
  Field3D term;
  Field3D& ddt_f;

  // References to global_counter and local_counter of the Equation object containing this
  // EquationTerm
  const int& global_counter;
  int& equation_counter;
  int local_counter = -1;
};

/// Represents an evolution equation, allowing (optionally) each term in the equation to
/// be saved individually.
class Equation {
public:
  Equation(Field3D& f_ref, const std::string& f_name, Options& opt, Datafile& out_file,
      int& counter);

  EquationTerm& first(const std::string& term_name);
  EquationTerm& operator[](const std::string& term_name);

  template<typename T>
  Equation& operator*=(const T& rhs) {
    ddt(f) *= rhs;

    if (save_equation) {
      for (auto& i : equation_terms) {
        i.second.term *= rhs;
      }
    }

    return *this;
  }

  template<typename T>
  Equation& operator/=(const T& rhs) {
    ddt(f) /= rhs;

    if (save_equation) {
      for (auto& i : equation_terms) {
        i.second.term /= rhs;
      }
    }

    return *this;
  }
private:
  Field3D& f;
  const std::string name;
  Options& options;
  Datafile& output_file;
  const int& global_counter;
  int local_counter = -1;
  bool save_equation = false;
  std::map<std::string, EquationTerm> equation_terms;
};

#endif // _BOUT_EQUATION_H_
