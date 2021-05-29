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

#include "equation.hxx"

Equation::Equation(Field3D& f_ref, const std::string& f_name, Options& opt,
    Datafile& out_file, int& counter)
  : f(f_ref), name(f_name), options(opt), output_file(out_file), global_counter(counter) {

  // Use only to check 'restat' and 'append' options. const to avoid printing anything
  // when reading them
  const auto global_opt = Options::root();

  if (options["restart_and_save_all_terms"].withDefault(false)
      or options["restart_and_save_all_terms_" + name].withDefault(false)) {
    if (not global_opt["restart"]) {
      throw BoutException("restart_and_save_all_terms set, but not restarting");
    }
    if (global_opt["append"]) {
      throw BoutException("restart_and_save_all_terms set, but append is true");
    }
    if (not global_opt["nout"] == 0) {
      throw BoutException("restart_and_save_all_terms set, but nout!=0");
    }
    save_equation = true;
  }

  if (options["restart_and_append_all_terms"].withDefault(false)
      or options["restart_and_append_all_terms_" + name].withDefault(false)) {
    if (not global_opt["restart"]) {
      throw BoutException("restart_and_append_all_terms set, but not restarting");
    }
    if (not global_opt["append"]) {
      throw BoutException("restart_and_append_all_terms set, but not appending");
    }
    if (not global_opt["nout"] == 0) {
      throw BoutException("restart_and_append_all_terms set, but nout!=0");
    }
    save_equation = true;
  }

  if (options["save_all_terms"].withDefault(false)
      or options["save_all_terms_" + name].withDefault(false)) {
    save_equation = true;
  }

  if (options["save_ddt"].withDefault(false)) {
    output_file.addRepeat(ddt(f), "ddt("+name+")");
  }
}

EquationTerm& Equation::operator[](const std::string& term_name) {
  auto it = equation_terms.find(term_name);

  if (it == equation_terms.end()) {
    // Term not present in equation_terms yet
    auto new_it = equation_terms.emplace(term_name,
        EquationTerm(save_equation, ddt(f), global_counter, local_counter));
    auto& term = new_it.first->second;

    if (save_equation) {
      output_file.addRepeat(term.term, name+"_equation_"+term_name);
    }

    return term;
  } else {
    return it->second;
  }
}
