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

/// Temporary accessor for an EquationTerm that can be modified (e.g. index-by-index). If
/// save_term=true, updates ddt_f when it goes out of scope or if save_term=false just
/// modifies ddt_f directly
class EquationTermAccessor {
public:
  EquationTermAccessor(Field3D& term_in, Field3D& ddt_f_in, bool save_term_in)
    : term(term_in), ddt_f(ddt_f_in), save_term(save_term_in) {}

  ~EquationTermAccessor() {
    if (save_term) {
      ddt_f += term;
    }
  }

  template<typename T>
  EquationTermAccessor& operator+=(const T& rhs) {
    term += rhs;
    return *this;
  }

  template<typename T>
  EquationTermAccessor& operator-=(const T& rhs) {
    term -= rhs;
    return *this;
  }

  // ElementAccessor ensures values can only be changed by addition or subtraction, so
  // that update of ddt_f in the destructor is correct and consistent between
  // save_term=true and save_term=false branches
  class ElementAccessor {
  public:
    ElementAccessor(BoutReal& value_in) : value(value_in) {}
    ElementAccessor& operator+=(BoutReal x) {
      value += x;
      return *this;
    }
    ElementAccessor& operator-=(BoutReal x) {
      value -= x;
      return *this;
    }
  private:
    BoutReal& value;
  };

  ElementAccessor operator()(int jx, int jy, int jz) {
    return ElementAccessor(term(jx, jy, jz));
  }

  ElementAccessor operator[](Ind3D i) {
    return ElementAccessor(term[i]);
  }
private:
  Field3D& term;
  Field3D& ddt_f;
  const bool save_term;
};

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

  // Modifiable access to the Field3D
  EquationTermAccessor localAccessor() {
    if (equation_counter != global_counter) {
      // First term being added to equation on this time-step
      equation_counter = global_counter;
      ddt_f = 0.0;
    }

    if (save_term and local_counter != global_counter) {
      // First time term is being modified on this time-step
      term = 0.0;
    }

    local_counter = global_counter;

    if (save_term) {
      return EquationTermAccessor(term, ddt_f, save_term);
    } else {
      return EquationTermAccessor(ddt_f, ddt_f, save_term);
    }
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
