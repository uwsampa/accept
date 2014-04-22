# The "accept" Tool

Type `accept help` to see the available commands. You can also type `accept help COMMAND` to see more documentation for a specific command.

Keep in mind that most actions in the `accept` tool are both *memoized* and *sandboxed*. This can be somewhat surprising at first, so read on to see what these do.

## Memoization

Commands *save intermediate results* to help save time when iterating. For example, when you type `accept build` the first time, your project is actually built and the log captured. The next time you run `accept build`, the command returns immediately with the saved log text; it doesn't actually rebuild your project.

This means that, after executing a command successfully once, it won't respond to any changes you make (e.g., modifying source files). Use the [force flag][force] (e.g., `accept -f build`) to ensure you re-compute.

[force]: #-force-f

## Sandboxing

Builds are performed in a temporary directory to avoid cluttering your filesystem. This is why you won't see build products in your working directory after running commands.

If you need to inspect the results of a computation, supply the ["keep sandboxes" flag][keep]. By combining this with the [force][] and [verbose][] flags, you can see where your build products end up. For example:

    $ accept -kfv build
    building in directory: /private/var/folders/py/tyzbll7117l90mv29bnlrtl80000gn/T/tmpldzE8z 
    [...]

Now you can follow that long, garbled path to find your executable and intermediate files.

[keep]: -keep-sandboxes-k
[verbose]: -verbose-v

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
