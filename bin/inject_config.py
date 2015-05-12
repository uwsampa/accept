#!/usr/bin/env python


# These two are copied from accept.core for this one-off.

def parse_relax_config(f):
    """Parse a relaxation configuration from a file-like object.
    Generates (ident, param) tuples.
    """
    for line in f:
        line = line.strip()
        if line:
            param, ident = line.split(None, 1)
            yield ident, int(param)


def dump_relax_config(config, f):
    """Write a relaxation configuration to a file-like object. The
    configuration should be a sequence of tuples.
    """
    for ident, param in config:
        f.write('{} {}\n'.format(param, ident))


def adapt_config(coarse_fn, fine_fn):
    # Load the coarse configuration file.
    params = {}
    with open(coarse_fn) as f:
        for line in f:
            line = line.strip()
            if line:
                func, param = line.split()
                params[func] = int(param)

    # Load ACCEPT config and adjust parameters.
    config = []
    with open(fine_fn) as f:
        for ident, param in parse_relax_config(f):
            # Turn everything off except selected functions.
            param = 0

            # If this is in a function indicated in the parameters file,
            # adjust the parameter accordingly.
            if ident.startswith('instruction'):
                _, i_ident = ident.split()
                func, _, _ = i_ident.split(':')
                if func in params:
                    param = params[func]

            config.append((ident, param))

    # Dump back to the fine (ACCEPT) configuration file.
    with open(fine_fn, 'w') as f:
        dump_relax_config(config, f)


if __name__ == '__main__':
    adapt_config('inject_config.txt', 'accept_config.txt')
