# Incremental Knightship Partial Extender, version 2.0

This is an optimised native-code successor to the Python program `ikpx`
responsible for the [discovery of Sir Robin][1].

![](docs/logo.png)

The main differences between `ikpx 2.0` and the original `ikpx` are:

 - Uses a better SAT solver (Armin Biere's _kissat_);
 - Runs in a single multithreaded process, so no communication overhead;
 - Supports arbitrary isotropic rules, not just B3/S23;
 - Incorporates symmetry when searching orthogonal velocities;
 - No longer uses suboptimal lattice bases;
 - Easier to use (inputs and outputs are all RLE).

Moreover, as there is no Python interpreter involved at runtime, the
program is much more efficient in terms of speed and memory usage.

[1]: https://cp4space.wordpress.com/2018/03/11/a-rather-satisfying-winter/
