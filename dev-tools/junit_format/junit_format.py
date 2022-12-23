#!/usr/bin/env python3


"""Reformat junit reports to include file and name attributes

Run with (e.g.): python junit_format.py foo.xml"""

import xml.etree.ElementTree as ET
import sys
import os
import re

# Logging partial results
verbose = True


def log(*args):
    if verbose:
        print(*args)


if __name__ == '__main__':
    print('Main')

    for filename in sys.argv[1:]:
        # Open the XML file and parse it
        tree = ET.parse(filename)
        root = tree.getroot()

        # Remove ".global" suffix from testcases
        testcases = root.findall('.//testcase')
        for testcase in testcases:
            classname = testcase.get('classname')
            if classname.endswith('.global'):
                classname = classname[:-len('.global')]
                testcase.set('classname', classname)

        # Find all the failure elements
        failures = root.findall('.//failure')
        if failures:
            log('Format failures in:', filename)

        # Set the file and line attributes for each failure element
        if failures:
            testcases = root.findall('.//testcase')
            for testcase in testcases:
                failures = testcase.findall('.//failure')
                for failure in failures:
                    lines = failure.text.split('\n')
                    expansion = ''
                    in_expansion = False
                    for line in lines:
                        if not in_expansion:
                            if line.startswith('with expansion:'):
                                in_expansion = True
                        else:
                            if line.startswith('at '):
                                in_expansion = False
                                break
                            else:
                                expansion += line + '\n'
                    expansion = re.sub(r'\s+', ' ', expansion.replace('\n', '').strip())
                    message = re.sub(r'\s+', ' ', failure.get('message').replace('\n', '').strip())
                    if message.find(' expands to ') == -1:
                        failure.set('message', failure.get('type') + '(' + message + ') expands to ' + expansion)

                    last_line = lines[-2]
                    if last_line.startswith('at '):
                        file_and_line = last_line[3:]
                        index = max(file_and_line.rfind(':'), file_and_line.rfind('('))
                        if index != -1:
                            file = file_and_line[:index]
                            if file.startswith('/__w/'):
                                file = file[5:]
                                for i in range(2):
                                    idx = file.find('/')
                                    if idx != -1:
                                        file = file[idx+1:]
                            if file_and_line[index] == ':':
                                line = file_and_line[index + 1:]
                            else:
                                line = file_and_line[index + 1:-1]
                            failure.set('file', file)
                            failure.set('line', line)
                            # Set these attributes in the parent testcase element
                            # The proper element for file and line is the failure element,
                            # but this is a workaround for the github action, which gets
                            # this data from the parent testcase, which is not supposed
                            # to have file and line attributes at all
                            testcase.set('file', file)
                            testcase.set('line', line)

                    log('File:', failure.get('file'))
                    log('Line:', failure.get('line'))
                    log('Message:', failure.get('message'))

        # Write the modified XML tree back to the file
        tree.write(filename, encoding="UTF-8", xml_declaration=True)
