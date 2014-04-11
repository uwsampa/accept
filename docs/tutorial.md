# Let's Get Started

This section will guide you through setting up a new application to be analyzed and optimized by the ACCEPT toolchain.

Before following this tutorial, you'll need to [build the ACCEPT toolchain](index.md#building).

## Add Source Files

First, make a new directory in `apps/` with the name of your program. We'll use `apps/foo` in this example.

Put your C/C++ sources in `apps/foo`. By default, The build system will assume
all `.c` and `.cpp` files are to be built and linked together. (You can
customize this later if need be.)

## Makefile

Next, add a `Makefile` for your experiment. Your Makefile should include at least these incantations to make everything work:

    TARGET := foo
    APP_MK := ../app.mk
    include $(APP_MK)

Of course, you'll want to change the name of your target to match your application's name.

At this point, you can also specify more complicated aspects of your build setup if necessary. You can specify `CFLAGS` and `LDFLAGS`, add targets, explicitly indicate your source files with the `SOURCES` variable, etc. See the included benchmarks' Makefiles for examples.

## Try Building

You can now use the ACCEPT toolchain to try building your application. Just type:

    $ accept -f build

(The `-f` flag avoids memoization—see the [tool documentation](cli.md).) This command shows the output of the build process, including any errors emitted by the compiler.

Like most `accept` commands, `accept build` uses the application in the working directory by default. You can specify a path as an argument to build something else.

## Annotate

Add `#include <enerc.h>` to files where you plan to add `APPROX` type
qualifiers.

Then, insert `APPROX` type qualifiers into your code as appropriate.  (The ACCEPT paper contains details on the annotation language.)

Insert a call to `accept_roi_begin()` immediately before the program's main
chunk of work, and insert a call to `accept_roi_end()` immediately afterward.
This assists the toolchain in timing the relevant portion of your computation.

You might find it helpful to repeatedly run `accept -f build` during annotation to see type errors and guide your placement of qualifiers.

## See Optimizations

Now that you have an annotated application, you can ask ACCEPT to analyze the program for optimization opportunities. Type:

    $ accept log

(Remember to add `-f` if you make any changes to your source files.) This will spit out a log of places where ACCEPT looked for—and found—possible relaxations. It will attempt to point you toward source locations that, given a bit more attention, could unlock to more opportunities. Again, the ACCEPT paper describes the purpose of this feedback log.

## Write a Quality Metric

The dynamic feedback loop component of ACCEPT relies on a function that assesses the *quality* of a relaxed program's output. You write this function in a Python script that accompanies the source code of your program.

To write your application's quality metric, create a file called `eval.py` alongside your source files. There, you'll define two Python functions: `load` and `score`.

**Load function.** The `load` function takes no arguments. It loads and parses the output of one execution of the program and returns a data structure representing it. For example, you might parse a CSV file to get a list of floating-point numbers and return that.

**Score function.** The `score` function takes two arguments, which are both outputs returned by previous invocations of `load`: the first is the output of a *precise* execution and the second is the output from some *relaxed* execution. The scoring function should compute the "difference" (defined in a domain-specific way) between the two and return a value between 0.0 and 1.0, where 0.0 is perfectly correct and 1.0 is completely wrong.

Here's an example `eval.py` to start with:

    def load():
        out = []
        with open('my_output.txt') as f:
            for line in f:
                first_num, _ = line.split(None, 1)
                out.append(float(first_num))
        return out

    def score(orig, relaxed):
        total = 0.0
        for a, b in zip(orig, relaxed):
            total += min(abs(a - b), 1.0)
        return total / len(orig)

**File caching.** Parsed program outputs—values returned by `load`—are stored serialized in an SQLite database for safekeeping and reuse. The idea is to avoid re-parsing the same output multiple times. Sometimes, however, it can be inefficient to store parsed and serialized values: when outputs are very large, it's better to just keep the file itself around and parse it on the fly each time. For example, if your program outputs an image, you probably don't want to store that in the database. In these cases, return a string starting with the prefix `file:` from your `load` function. Output files will then be cached and their *filenames* (rather than contents) passed to `score`.

Here's an example `eval.py` using this approach:

    import itertools

    def load():
        return 'file:my_output.txt'

    def score(orig, relaxed):
        total = 0.0
        count = 0
        with open(orig) as orig_f:
            with open(relaxed) as relaxed_f:
                for orig_line, relaxed_line in itertools.izip(orig_f, relaxed_f):
                    a = int(line.split(None, 1)[0])
                    b = int(line.split(None, 1)[0])
                    total += min(abs(a - b), 1.0)
                    count += 1
        return total / count


## Run the Toolchain

Once you're happy with your annotations, you can run the full toolchain to optimize your program. Run this command:

    $ accept exp foo

Unlike the other `accept` commands, the `exp` command needs the name of your application (i.e., the directory name containing your sources).
