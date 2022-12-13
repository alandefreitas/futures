#!/usr/bin/env python3


"""Combine JSON from multiple -ftime-traces into one.

Run with (e.g.): python combine_traces.py foo.json bar.json.

Source: https://www.snsystems.com/technology/tech-blog/clang-time-trace-feature"""

import json
import sys
import os

# Logging partial results
verbose = True
def log(*args):
    if verbose:
        print(*args)


if __name__ == '__main__':

    start_time = 0

    combined_data = []

    for filename in sys.argv[1:]:
        log('Combine:', os.path.basename(filename))
        with open(filename, 'r') as f:
            file_time = None
            for event in json.load(f)['traceEvents']:
                # log('Event:', event)
                # Skip metadata events
                # Skip total events
                # Filter out very short events to reduce data size
                if event['ph'] == 'M' or event['name'].startswith('Total') or event['dur'] < 5000:
                    continue

                # Keep track of the main ExecuteCompiler event, which exists for each file
                # Also adapt this event to include the file name
                if event['name'] == 'ExecuteCompiler':
                    # Find how long this compilation took for this file
                    file_time = event['dur']
                    log(filename, 'took', file_time)
                    # Set the file name in ExecuteCompiler
                    if not 'args' in event:
                        event['args'] = {}
                    event['args']['detail'] = os.path.basename(filename)

                # Offset start time to make compiles sequential
                event['ts'] += start_time

                # Put all events in the same pid
                del event['pid']
                del event['tid']

                # Add data to combined
                combined_data.append(event)

        # Increase the start time for the next file

        # Add 1 to avoid issues with simultaneous events

        start_time += file_time + 1

with open('combined_traces.json', 'w') as f:
    json.dump({'traceEvents': sorted(combined_data, key=lambda k: k['ts'])}, f)
    log('Saved to ', os.path.abspath("combined_traces.json"))
