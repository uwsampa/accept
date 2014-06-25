# The "accept" Tool

Type `accept` with no arguments to see the available commands. You can also type `accept COMMAND --help` to see more documentation for a specific command.

Keep in mind that most actions in the `accept` tool are both *memoized* and *sandboxed*. This can be somewhat surprising at first, so read on to see what these do.


## Memoization

Commands *save intermediate results* to help save time when iterating. For example, when you type `accept build` the first time, your project is actually built and the log captured. The next time you run `accept build`, the command returns immediately with the saved log text; it doesn't actually rebuild your project.

This means that, after executing a command successfully once, it won't respond to any changes you make (e.g., modifying source files). Use the [force flag][force] (e.g., `accept -f build`) to ensure you re-compute.

[force]: #-force-f

You can also completely remove all of the memoized results and start over. Just delete the `memo.db` file from the ACCEPT directory. (This is also the solution to any SQLite errors that might occur if the database is corrupted.)


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

### `accept run`

Run the entire ACCEPT workflow for the program in the current working directory. Print out the optimal configurations discovered by ACCEPT.

By default, the command prints out the optimal configurations found by ACCEPT. The *verbose* flag, `-v`, makes this command print out all configurations---including suboptimal and broken ones---for debugging.

The *test* flag, `-t`, enables post-hoc "testing" executions. With this setting, the tool will re-execute all the *optimal* configuration using the program's test input (see [the section about TESTARGS](#execution-arguments), below). These results are shown instead of the intermediate ("training") results.

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


## eval.py

The ACCEPT tool uses a per-application Python script for collecting and evaluating the application's output quality. This means that applications need to be accompanied by an `eval.py` file. This file should define two Python functions:

* `load()`: This function takes no arguments, loads the output of a program execution (either precise or approximate), and returns this output.
* `score(orig, relaxed)`: This function takes two arguments, both of which are *outputs* returned by the `load()` function. It should return a number between 0.0 and 1.0 describing the *accuracy* of the `relaxed` output with respect to the `orig` output. For example, if `load()` just returns a number, `score` might compute the difference between the two (e.g., `return abs(orig - relaxed)`).

When running an experiment, ACCEPT does roughly the following:

* Runs the precise version of your program.
* Call your `load()` function and store that in a database under the name "orig".
* Run an approximate version of your program.
* Call `load()` again and store that in a database too under the name "relaxed".
* Finally, call your `score()` function with those two "orig" and "relaxed" values.

This means you never have to worry about *calling* the two functions; ACCEPT itself will call them during the experiment process.

The tutorial contains [an example][evalex] of eval.py.

[evalex]: tutorial.md#write-a-quality-metric

### Dealing With Large Files

If your program's output is big (e.g., an image), it might be inefficient to store the data in our database. For that reason, ACCEPT provides an alternate way to write eval.py for large outputs.

To store entire output files instead of putting parsed results in a database, write your `load()` function to just return a string containing the filename you want to save, prefixed with `file:`.
Then, your `score()` function will receive cached filenames instead of output data.
In this style, your `score()` function will need to parse both files (something that is `load()`'s job in the small-outputs style).


## Makefile

Applications tell ACCEPT how to build them using a standard Makefile. Your application's Makefile currently must contain at least these lines:

    APP_MK := ../app.mk
    include $(APP_MK)

If necessary, you can change the `APP_MK` variable to point to where ACCEPT is installed. (This is only necessary if you placed your directory outside of ACCEPT's `apps` directory.)

There are a number of other options you can specify here:

### Sources

By default, the build system will compile any files ending in `.c` and `.cpp` into your executable. You can set the `SOURCES` variable to explicitly indicate which files should be compiled (rather than using `*.c *.cpp`). For example:

    SOURCES := foo.cpp src/bar.cpp

### Build Flags

The usual `CFLAGS` and `LDFLAGS` can be used to customize the build process. For example, here's how you might enable pthreads in the compiler:

    CXXFLAGS += -pthread
    LDFLAGS := -lpthread

### Execution Arguments

The `RUNARGS` variable is used to specify command-line arguments to be used when executing your program. You might, for example, need to specify the input file. Here's an example from the fluidanimate program:

    RUNARGS := 4 5 in_300K.fluid out.fluid

It is also a good idea to provide a separate input for ACCEPT's *testing* phase, which automatically evaluates the final impact of ACCEPT's optimizations. Providing a separate input avoids overfitting to one specific input set, so we take inspiration from the [training and testing sets](http://en.wikipedia.org/wiki/Test_set) used in machine learning.

Use the `TESTARGS` variable to provide a second, potentially slower-running, invocation of your program. Again, here's an example from fluidanimate:

    TESTARGS := 4 5 in_300K.fluid out.fluid
