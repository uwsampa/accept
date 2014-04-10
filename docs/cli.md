# The "accept" Tool

Type `accept help` to see the available commands. You can also type `accept help COMMAND` to see more documentation for a specific command.

There are a couple of important (and possibly surprising) aspects that all of the commands have in common:

**Memoization.** All of the commands save intermediate results to help save time when iterating. This means that, after executing a command successfully once, it won't respond to any changes you make out of band (e.g., updating source files). Use `accept -f COMMAND` to force re-computation.

**Sandboxing.** Builds are performed in a temporary directory to avoid cluttering your filesystem. This is why you won't see build products in your working directory after running commands.

### Running the Included Experiments

For some of the benchmarks, you will need some large input files that are not included in this repository:

* blackscholes: "simlarge" input from PARSEC (`in_64K.txt`)
* canneal: "native" input (`2500000.nets`)
* fluidanimate: "simlarge" input (`in_300K.fluid`)
* x264: "simmedium" input (`eledream_640x360_32.y4m`)

Run the experiments by typing `accept exp`.

### Running on a Cluster

The above will run all compilations and executions locally and in serial. To
run data collections on a cluster, install and set up the [cluster-workers][]
library. Then pass the `-c`/`--cluster` flag to `accept` to enable cluster
execution.

[cluster-workers]: https://github.com/sampsyo/cluster-workers

