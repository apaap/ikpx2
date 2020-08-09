# Incremental Knightship Partial Extender, version 2.0

This is an optimised native-code successor to the Python program `ikpx`
responsible for the [discovery of Sir Robin][1].

![](docs/logo.png)

The main differences between `ikpx2` and the original `ikpx` are:

 - Uses a better SAT solver (Armin Biere's [Kissat][2]);
 - Runs in a single multithreaded process, so no communication overhead;
 - Supports arbitrary isotropic rules, not just B3/S23;
 - Incorporates symmetry when searching orthogonal velocities;
 - No longer uses suboptimal lattice bases;
 - Easier to use (inputs and outputs are all RLE).

Moreover, as there is no Python interpreter involved at runtime, the
program is much more efficient in terms of speed and memory usage.

## Quick start

To build `ikpx2` for the default rule (B3/S23), simply run:

    ./recompile.sh

To specify a different rule such as B38/S23, run:

    ./recompile.sh --rule b38s23

The set of rules supported are isotropic 2-state Moore-neighbourhood
cellular automata. This includes familiar Life-like cellular automata,
in addition to non-totalistic rules expressed in [Hensel notation][3].

[1]: https://cp4space.wordpress.com/2018/03/11/a-rather-satisfying-winter/
[2]: https://github.com/arminbiere/kissat
[3]: https://www.conwaylife.com/wiki/Hensel_notation
