#!/usr/bin/python2
import os
import sys
from collections import defaultdict, OrderedDict


def main():
    if len(sys.argv) == 1:
        filename = os.path.expanduser("/var/log/krait/stdout")
    else:
        filename = sys.argv[1]

    if not os.path.exists(filename):
        print "Log stdout file {} does not exist.".format(filename)
        print "If the stdout output is located elsewhere, rerun this with that filename as argument."
        exit(1)

    times = []
    marker = "[DbgStopwatch] "
    with open(filename, "r") as f:
        for line in (l.strip()[len(marker):] for l in f if l.strip().startswith(marker)):
            time = extract_time(line)
            if time != None:
                times.append(time)

    times_grouped = defaultdict(list)
    for name, time in times:
        times_grouped[name].append(time)

    times_averaged = []
    for name, times in times_grouped.iteritems():
        avg = sum(times) / len(times)
        times_averaged.append((name, avg))

    for name, avg in sorted(times_averaged):
        print "{}: {}ms".format(name, avg)


def extract_time(line):
    if line[0] != '[':
        print "[DbgStopwatch] line missing name '['."
        return None

    name_end = line.rfind(']')
    if name_end == -1:
        print "[DbgStopwatch] line missing ']' marker."
        return None

    name = line[1:name_end]

    time_end = line.rfind("ms", name_end)
    if time_end == -1:
        print "[DbgStopwatch] line missing 'ms' marker."
        return None

    time_str = line[name_end + 1:time_end].strip()
    try:
        time_float = float(time_str)
    except ValueError:
        print "[DbgStopwatch] line time (%1%) not a float!".format(time_str)
        return None

    return name, time_float


if __name__ == '__main__':
    main()
