# The "accept" Tool

Type `accept help` to see the available commands. You can also type `accept help COMMAND` to see more documentation for a specific command.

There are a couple of important (and possibly surprising) aspects that all of the commands have in common:

**Memoization.** All of the commands save intermediate results to help save time when iterating. This means that, after executing a command successfully once, it won't respond to any changes you make out of band (e.g., updating source files). Use `accept -f COMMAND` to force re-computation.

**Sandboxing.** Builds are performed in a temporary directory to avoid cluttering your filesystem. This is why you won't see build products in your working directory after running commands.

## Commands

Here are the `accept` commands that are useful when working with the system:

### `accept build`

Build the program in the current working directory. (Like all the commands below, you can also specify a specific directory as an argument instead.) Show the output of the compiler.

The effect is much like typing `make`, except memoized and sandboxed (see above).

No program is actually executed. This is useful while writing your application to check for annotation errors and other syntax stuff.

### `accept log`

Build the program in the current working directory. Instead of showing the compiler's output, print out the ACCEPT optimization log.

This explains which opportunities where analyzed for approximation, which are ready for relaxation, and which statements are preventing relaxation opportunities.

### `accept precise`

Build and execute a baseline, precise configuration of the program in the current working directory.

Show the output of the program (parsed by the `eval.py` script's `load()` function) and the execution time in seconds.

### `accept approx`

Build and execute approximate configurations of the program in the current working directory. By default, all approximate configurations are run. An optional argument lets you select a specific single configuration by its index.


## Options

The `accept` tool provides a few common flags (which should appear before the specific subcommand):

### `--verbose`, `-v`

Enable more logging output. This is especially helpful if you have a long-running command and want to make sure it's actually making progress.

Use this option multiple times to get more verbose output (e.g., `-vv`).

### `--force`, `-f`

Don't use memoized results; force re-computation.

Every result of every substantial action in the ACCEPT tooling is stored in a database by default and re-used when possible. For example, the output of the compiler when typing `accept build` will be stored and reused the next time you run the command without actually recompiling the program. This means that subsequent invocations of `accept build` will not pick up any changes to your source files.

Forcing re-computation avoids using memoized outputs so that `accept build` actually recompiles your code.

### `--keep-sandboxes`, `-k`

Builds and executions in ACCEPT are performed in a temporary directory under `/tmp`. By default, they are deleted after the command completes. To instead keep the sandboxes around for manual inspection, supply this flag.

### `--cluster`, `-c`

By default, everything is run locally and in serial. To run data collections in
parallel on a cluster, install and set up the [cluster-workers][] library. Then
pass this option to enable cluster execution.

[cluster-workers]: https://github.com/sampsyo/cluster-workers


## Running the Included Experiments

For some of the benchmarks, you will need some large input files that are not included in this repository:

* blackscholes: "simlarge" input from PARSEC (`in_64K.txt`)
* canneal: "native" input (`2500000.nets`)
* fluidanimate: "simlarge" input (`in_300K.fluid`)
* x264: "simmedium" input (`eledream_640x360_32.y4m`)

Run the experiments by typing `accept exp`.
