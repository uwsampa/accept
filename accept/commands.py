from __future__ import print_function
from __future__ import division
from __future__ import absolute_import
import argh
import logging
import sys
from . import experiments
from . import core
from . import cwmemo


LOCAL_REPS = 1
CLUSTER_REPS = 5
APPS = ['streamcluster', 'blackscholes', 'sobel']


# Run the experiments.

@argh.arg('appnames', metavar='NAME', default=APPS, nargs='*', type=unicode,
          help='applications')
@argh.arg('-r', '--reps', metavar='N', default=None, type=int,
          help='replications')
def exp(appnames, verbose=False, cluster=False, force=False, reps=None):
    if not reps:
        reps = CLUSTER_REPS if cluster else LOCAL_REPS

    for appname in appnames:
        print(appname)
        experiments.evaluate(appname, verbose, cluster, force, reps)


# Get the compilation log.

def get_log(directory):
    """Build the benchmark in `directory` and return the contents of the
    compilation log.
    """
    with core.chdir(directory):
        with core.sandbox(True):
            core.build()
            with open('accept_log.txt') as f:
                return f.read()

@argh.arg('appdir', nargs='?', help='application directory')
def log(appdir='.'):
    with cwmemo.get_client(cluster=False, force=False) as client:
        client.submit(get_log, appdir)
        logtxt = client.get(get_log, appdir)
    print(logtxt)


if __name__ == '__main__':
    logging.getLogger().addHandler(logging.StreamHandler(sys.stderr))
    parser = argh.ArghParser()
    parser.add_commands([exp, log])
    parser.dispatch()
