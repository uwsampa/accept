Hacking
=======

This page is about advanced use, corner cases, and the like.

## Make It Fast

The setup instructions by default will get you a *debug* build of LLVM, Clang, and ACCEPT, which is nice for debugging but leads to very slow compiles. If you'll be iterating with a benchmark, changing annotations and recompiling frequently, you might want a *release* build.

Just rebuild the project with `RELEASE=1`:

    make llvm accept RELEASE=1


## Using Your Makefile

For typical uses of ACCEPT, you never need to use the Makefile you've written directly. The ACCEPT driver itself invokes the Makefile during the tuning process.

But, when things go wrong, sometimes it can be useful to directly invoke
`make`. Here are some targets that are available to Makefiles that include ACCEPT's `app.mk`:

* `build_orig`: Build a version of the application with no ACCEPT optimizations enabled. Produces a configuration file template. If you want to also produce the analysis log, use `make build_orig OPTARGS=-accept-log`.
* `build_opt`: Build an ACCEPT-optimized version of the program. Uses the configuration file to determine which optimizations to enable.
* `run_orig`, `run_opt`: Execute the corresponding built version of the program with the specified command-line arguments (see `RUNARGS` above). Most notably, typing `make run_orig` is like a less-fancy version of `accept precise` that can be useful when the ACCEPT driver is acting up.
* `clean`: DWISOTT. Also cleans up the byproducts of ACCEPT like the timing file.


## Reproduce the Experiments

ACCEPT ships with several benchmarks used to evaluate it in our paper. You can reproduce the experiments using the included Makefile.

First, build ACCEPT if you haven't already. You might want to specify the `RELEASE=1` option (see above).

Next, get the experiments' dependencies (Python modules, data files) by typing `make exp_setup`.

Finally, run the experiments by typing `make exp`. The command will dump the results to a file called `results.json`.

There are several variables you can specify to customize the experiments:

* `APPS`: You can set the list of application names to evaluate. By default, we evaluate all of them.
* `MINI=1`: Turn down the replication factor. You get faster but less statistically significant results.
* `NOTIME=1`: Perform an untimed execution. This will used memoized results, if available, instead of re-executing everything from scratch.
* `CLUSTER=1`: Execute on a Slurm cluster using the [cluster-workers][cw] library.

[cw]: https://github.com/sampsyo/cluster-workers


## Troubleshooting

Here are some solutions to problems you might encounter along the way.


### Wrong "accept" Program

Do you see an error like either of these when trying to run the `accept`
program?

    $ accept -f build
    accept: Error - unknown option "f".

Or:

    $ accept run
    accept: Operation failed: client-error-not-found

This means that you're accidentally invoking a different program named `accept` installed elsewhere on your machine instead of our `accept`. To put the right program on your PATH, go to the ACCEPT directory and type:

    $ source activate.sh

Now the `accept` command should work as expected.

Of course, you can also use the full path to the `accept` program if you prefer not to muck with your `PATH`.


### Finding Build Products

You may find that ACCEPT commands don't produce files like you would expect. For example: where did the built executable go after I typed `accept build`?

ACCEPT commands hide these result files from you because they use [sandboxing](cli.md#sandboxing). You can use the ["keep sandboxes" flag][keep] to avoid automatically deleting the build products.

[keep]: cli.md#-keep-sandboxes-k
