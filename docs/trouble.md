Troubleshooting
===============

Here are some solutions to problems you might encounter along the way.


### Wrong "accept" Program

Do you see an error like either of these when trying to run the `accept`
program?

    $ accept -f build
    accept: Error - unknown option "f".

Or:

    $ accept help
    accept: Operation failed: client-error-not-found

This means that you're accidentally invoking a different program named `accept` installed elsewhere on your machine instead of our `accept`. To put the right program on your PATH, go to the ACCEPT directory and type:

    $ source activate.sh

Now the `accept` command should work as expected.

Of course, you can also use the full path to the `accept` program if you prefer not to muck with your `PATH`.


### Finding Build Products

You may find that ACCEPT commands don't produce files like you would expect. For example: where did the built executable go after I typed `accept build`?

ACCEPT commands hide these result files from you because they use [sandboxing](cli.md#sandboxing). You can use the ["keep sandboxes" flag][keep] to avoid automatically deleting the build products.

[keep]: cli.md#-keep-sandboxes-k
