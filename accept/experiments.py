from __future__ import print_function
from __future__ import division
import os
import logging
from . import core

APPSDIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'apps')

def dump_config(config, descs):
    """Given a relaxation configuration and an accompanying description
    map, returning a human-readable string describing it.
    """
    optimizations = [r for r in config if r[1]]
    if not optimizations:
        return u'no optimizations'

    out = []
    for ident, param in optimizations:
        out.append(u'{} @ {}'.format(descs[ident], param))
    return u', '.join(out)

def evaluate(client, appname, verbose=False, reps=1):
    exp = core.Evaluation(os.path.join(APPSDIR, appname), client, reps)
    logging.info('starting experiments')
    with client:
        exp.run()
    logging.info('all experiments finished')
    results, descs = exp.results, exp.descs

    optimal, suboptimal, bad = core.triage_results(results)
    print('{} optimal, {} suboptimal, {} bad'.format(
        len(optimal), len(suboptimal), len(bad)
    ))
    for res in optimal:
        print(dump_config(res.config, descs))
        print('{:.1%} error'.format(res.error))
        print('{} speedup'.format(res.speedup))

    if verbose:
        print('\nsuboptimal configs:')
        for res in suboptimal:
            print(dump_config(res.config, descs))
            print('{:.1%} error'.format(res.error))
            print('{} speedup'.format(res.speedup))

        print('\nbad configs:')
        for res in bad:
            print(dump_config(res.config, descs))
            print(res.desc)
