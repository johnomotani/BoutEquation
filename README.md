Equation class
==============

Optionally output named terms in BOUT++ evolution equations.

To use, create ``Equation`` members of your ``PhysicsModel``, e.g.

    Equation density_equation{n, "n", options, dump, rhs_counter};

``rhs_counter`` should be another member of your ``PhysicsModel`` class that is
incremented every time the rhs() method is called, so that the ``Equation`` object knows
to reset the internal map of fields if necessary.

Then add named terms in the ``rhs()`` method, e.g.

    density_equation["ExB advection"] = bracket(phi, n);
    density_equation["Parallel advection"] = Vpar_Grad_par(V, n);

The terms can also be modified by index by using a local accessor, e.g.

    auto local = density_equation.localAccessor();
    for (auto i : foo) {
      local[i] = 3.45 * foo[i];
    }
    for (RangeIterator r = mesh->iterateBndryLowerY(); !r.isDone(); r++) {
      i = r.ind;
      for (int k = 0; k < mesh->LocalNz; k++) {
        local(i, mesh->ystart, k) = bar(i, mesh->ystart, k);
      }
    }

If no options are set, this is equivalent (with negligible performance penalty)
to adding directly to ddt(n) like

    ddt(n) = bracket(phi, n);
    ddt(n) += Vpar_Grad_par(V, n);

But options can be set:
- ``restart_and_save_all_terms = true`` - must set ``restart = true``, and ``nout = 0``;
  saves each named term in the ``Equation`` object to a new dump file.
- ``restart_and_append_all_terms = true`` - must set ``restart = true``, ``append =
  true``, and ``nout = 0``; appends each named term in the ``Equation`` object to existing
  dump files.
- ``save_all_terms = true`` - saves all named terms at every output timestep; likely to
  lead to very large output files.
