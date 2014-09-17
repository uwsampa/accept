sourced=$_

# Source this script (don't execute it).
if [ -z "$sourced" -o "$sourced" == "/bin/sh" ] ; then
    echo "please source this script"
fi

base=`dirname $sourced`
base=`cd "$base" ; pwd`

# Extend $PATH.
export PATH=$base/bin:$base/build/built/bin:$PATH

unset base
unset sourced
