from __future__ import print_function
from __future__ import division
from __future__ import absolute_import
import argh
import logging
import sys
from . import experiments


LOCAL_REPS = 1
CLUSTER_REPS = 5
APPS = ['streamcluster', 'blackscholes', 'sobel']


@argh.arg('appnames', metavar='NAME', default=APPS, nargs='*', type=unicode,
          help='applications')
@argh.arg('-r', '--reps', metavar='N', default=None, type=int,
          help='replications')
def exp(appnames, verbose=False, cluster=False, force=False, reps=None):
    if not reps:
        reps = CLUSTER_REPS if cluster else LOCAL_REPS

    logging.getLogger().addHandler(logging.StreamHandler(sys.stderr))

    for appname in appnames:
        print(appname)
        experiments.evaluate(appname, verbose, cluster, force, reps)


if __name__ == '__main__':
    parser = argh.ArghParser()
    parser.add_commands([exp])
    parser.dispatch()
