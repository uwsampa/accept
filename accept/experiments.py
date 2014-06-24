from __future__ import print_function
from __future__ import division
import os
import logging
import time
from . import core


APPSDIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'apps')
OPT_KINDS = {
    'loopperf': ('loop',),
    'desync':   ('lock', 'barrier'),
    'aarelax':  ('alias',),
}


def dump_config(config):
    """Given a relaxation configuration and an accompanying description
    map, returning a human-readable string describing it.
    """
    optimizations = [r for r in config if r[1]]
    if not optimizations:
        return u'no optimizations'

    out = []
    for ident, param in optimizations:
        out.append(u'{} @ {}'.format(ident, param))
    return u', '.join(out)


def dump_results_human(results, pout, verbose):
    """Generate human-readable text (as a sequence of lines) for
    the results.
    """
    optimal, suboptimal, bad = core.triage_results(results)

    if verbose and isinstance(pout, str):
        yield 'precise output: {}'.format(pout)
        yield ''

    yield '{} optimal, {} suboptimal, {} bad'.format(
        len(optimal), len(suboptimal), len(bad)
    )
    for res in optimal:
        yield dump_config(res.config)
        yield '{} % error'.format(res.error * 100)
        yield '{} speedup'.format(res.speedup)

        if verbose and isinstance(res.outputs[0], str):
            yield 'output: {}'.format(res.outputs[0])

    if verbose:
        yield '\nsuboptimal configs:'
        for res in suboptimal:
            yield dump_config(res.config)
            yield '{} % error'.format(res.error * 100)
            yield '{} speedup'.format(res.speedup)

        yield '\nbad configs:'
        for res in bad:
            yield dump_config(res.config)
            yield res.desc


def dump_results_json(results):
    """Return a JSON-like representation of the results.
    """
    results, _, _ = core.triage_results(results)
    out = []
    for res in results:
        out.append({
            'config': dump_config(res.config),
            'error_mu': res.error.value,
            'error_sigma': res.error.error,
            'speedup_mu': res.speedup.value,
            'speedup_sigma': res.speedup.error,
        })
    return out


def results_for_base(ev, configs):
    """Given a set of base configurations, return a set of uniqued
    results from all phases of the experiment workflow.
    """
    # The phases.
    base_results = ev.run_approx(configs)
    tuned_results = ev.parameter_search(base_results)
    composite_results = ev.evaluate_composites(tuned_results)

    # Uniquify the results based on their configs.
    results = {}
    for result in base_results + tuned_results + composite_results:
        results[result.config] = result
    return results.values()


def run_experiments(ev, only=None):
    """Run all stages in the Evaluation for producing paper-ready
    results. Returns the main results and a dict of kind-restricted
    results.
    """
    start_time = time.time()
    ev.setup()

    # Main results.
    if only and 'main' not in only:
        main_results = []
    else:
        main_results = results_for_base(ev, ev.base_configs)

    end_time = time.time()

    # "Testing" phase.
    if main_results:
        # FIXME disabled for now.
        # testing_results(ev, main_results)
        pass

    # Experiments with only one optimization type at a time.
    kind_results = {}
    for kind, words in OPT_KINDS.items():
        if only and kind not in only:
            continue

        # Filter all base configs for configs of this kind.
        logging.info('evaluating {} in isolation'.format(kind))
        kind_configs = []
        for config in ev.base_configs:
            for ident, param in config:
                if param and not ident.startswith(words):
                    break
            else:
                kind_configs.append(config)

        # Run the experiment workflow.
        logging.info('isolated configs: {}'.format(len(kind_configs)))
        kind_results[kind] = results_for_base(ev, kind_configs)

    return main_results, kind_results, end_time - start_time


def evaluate(client, appname, verbose=False, reps=1, as_json=False,
             only=None):
    appdir = os.path.join(APPSDIR, appname)
    exp = core.Evaluation(appdir, client, reps)

    setup_script = os.path.join(appdir, 'setup.sh')
    if os.path.exists(setup_script):
        logging.info('running setup script')
        with core.chdir(appdir):
            core.run_cmd(['sh', 'setup.sh'])

    logging.info('starting experiments')
    with client:
        main_results, kind_results, exp_time = run_experiments(exp, only)
    logging.info('all experiments finished')

    if as_json:
        out = {}
        out['main'] = dump_results_json(main_results)
        isolated = {}
        for kind, results in kind_results.items():
            isolated[kind] = dump_results_json(results)
        out['isolated'] = isolated
        out['time'] = exp_time
        return out

    else:
        out = []
        if not only or 'main' in only:
            out += dump_results_human(main_results, exp.pout, verbose)
        if verbose or only:
            for kind, results in kind_results.items():
                if only and kind not in only:
                    continue
                out.append('')
                if results:
                    out.append('ISOLATING {}:'.format(kind))
                    out += dump_results_human(results, exp.pout, verbose)
                else:
                    out.append('No results for isolating {}.'.format(kind))
        return '\n'.join(out)
