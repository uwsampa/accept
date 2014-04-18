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
