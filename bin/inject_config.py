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
    """Take a coarse (per-function) configuration file and apply it to a
    fine (per-instruction) ACCEPT configuration file. The latter is
    modified on disk.

    The first argument is the filename of a text file containing
    entries like this:

        function_name parameter

    where the parameter is an integer. There can also be a line where
    the function_name is "default", in which case all other instructions
    (outside of the listed functions) are given this parameter.

    All non-instruction parameters are left as 0.

    The second argument is the filename of an ACCEPT configuration file.
    """
    # hack to deal with LVA PC
    lva_pc = 1;

    # Load the coarse configuration file.
    params = {}
    default_param = 0
    with open(coarse_fn) as f:
        for line in f:
            line = line.strip()
            if line:
                func, param = line.split()
                param = int(param)
                if func == 'default':
                    default_param = param
                else:
                    params[func] = param

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
                else:
                    param = default_param

	    if (param >= 0x40000 and param <= 0x50000):
		config.append((ident, param + lva_pc))
		lva_pc += 1
	    else:
		config.append((ident, param))

    # Dump back to the fine (ACCEPT) configuration file.
    with open(fine_fn, 'w') as f:
        dump_relax_config(config, f)


if __name__ == '__main__':
    adapt_config('inject_config.txt', 'accept_config.txt')
