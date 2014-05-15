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

For simple programs, this is all you need; by default, the build system will compile any files ending in `.c` and `.cpp` into your executable.

In more complex build scenarios, you can specify more variables and targets here. For example, you can specify `CFLAGS` and `LDFLAGS` to add to the compiler invocation command-lines. You can set the `SOURCES` variable to explicitly indicate which files should be compiled (rather than using `*.c *.cpp`). See the included benchmarks' Makefiles for examples.

## Try Building

You can now use the ACCEPT toolchain to try building your application. Just type:

    $ accept -f build

(The `-f` flag avoids memoization—see the [tool documentation](cli.md).) This command shows the output of the build process, including any errors emitted by the compiler.

Like most `accept` commands, `accept build` uses the application in the working directory by default. You can specify a path as an argument to build something else.

## Source Setup

You will need to make a few small changes to your application's source code to make it fit the ACCEPT workflow:

* The output will need to be written to a file (or multiple files). This is because the quality measurement infrastructure needs to pick up the output after the execution finishes. If the program's output is sent to the console, change it to instead send writes to a text file.
* Add `#include <enerc.h>` to relevant files. This header file includes definitions for both annotations and ROI markers (next!).
* Mark the "interesting" part of the workload for timing using *region of interest* (ROI) markers. ACCEPT needs to time your program's execution, but you don't want it to time mundane tasks like reading input files. Insert a call to `accept_roi_begin()` immediately before the program's main chunk of work, and insert a call to `accept_roi_end()` immediately afterward.

## Annotate

Your next task is to actually annotate the application to enable approximation.
Insert `APPROX` type qualifiers and `ENDORSE()` casts into your code as appropriate.

The ACCEPT paper contains details on the annotation language. This guide should eventually contain a summary of the annotation features, but for now, take a look at the paper.

You might find it helpful to repeatedly run `accept -f build` during annotation to see type errors and guide your placement of qualifiers.

Remember that you will need to use `#include <enerc.h>` in files where use annotations.

## See Optimizations

Now that you have an annotated application, you can ask ACCEPT to analyze the program for optimization opportunities. Type:

    $ accept log

(Remember to add `-f` if you make any changes to your source files.) This will spit out a log of places where ACCEPT looked for—and found—possible relaxations. It will attempt to point you toward source locations that, given a bit more attention, could unlock to more opportunities. Again, the ACCEPT paper describes the purpose of this feedback log.

## Write a Quality Metric

The dynamic feedback loop component of ACCEPT relies on a function that assesses the *quality* of a relaxed program's output. You write this function in a Python script that accompanies the source code of your program.

To write your application's quality metric, create a file called `eval.py` alongside your source files. There, you'll define two Python functions: `load` and `score`. You never have to worry about calling these functions—the ACCEPT driver itself invokes them during the auto-tuning process.

**Load function.** The `load` function takes no arguments. It loads and parses the output of one execution of the program and returns a data structure representing it. For example, you might parse a CSV file to get a list of floating-point numbers and return that.

**Score function.** The `score` function takes two arguments, which are both outputs returned by previous invocations of `load`: the first is the output of a *precise* execution and the second is the output from some *relaxed* execution. The scoring function should compute the "difference" (defined in a domain-specific way) between the two and return a value between 0.0 and 1.0, where 0.0 is perfectly correct and 1.0 is completely wrong.

It bears repeating that both of these functions are *application-specific*: there is no "standard" implementation of either `load` or `score`. Both functions convey unique information about your program to the ACCEPT system. This means that your program can have *any* output format as long as the output is written to a file (or even multiple files); you write `load` to explain your chosen format. Similarly, you get to decide what "quality" means for your program; you write `score` to mechanise your chosen notion of quality.

Here's an example `eval.py` written for a notional program whose output consists of a text file, `my_output.txt`, containing a list of numbers. The `load` function here strips off some additional (irrelevant) text on each line and returns the parsed data as a list of floats. The `score` function takes the mean absolute difference, capped at 1, between the two lists of numbers:

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

## Run the Toolchain

Once you're happy with your annotations, you can run the full toolchain to optimize your program. Run this command:

    $ accept exp foo

Unlike the other `accept` commands, the `exp` command needs the name of your application (i.e., the directory name containing your sources).

## Experiment Workflow

The following is a high-level description of the part of the workflow of an ACCEPT experiment involving eval.py.

The program is first run once without approximation. This run without approximation is known as the precise run.

* During the precise run, the program writes precise data values to an output file.
* After the precise run, the `load()` function of the `eval.py` script stores those precise data values in a data structure that is in the database.

The program is then run several times with approximation. These runs with approximation are known as the approximate runs. The approximate runs differ slightly in their methods of relaxation, but each approximate run is evaluated in the same manner.

* During an approximate run, the program writes approximate data values to an output file.
* After the approximate run, the `load()` function of the `eval.py` script stores those approximate data values in a data structure that is in the database.
* Finally, the `score()` function computes a correctness score for the approximate run. The correctness score is a metric of error between the values of the data structure filled by the precise run and the corresponding values of the data structure filled by the approximate run. A score of 0.0 indicates complete correctness, while a score of 1.0 indicates complete incorrectness.
