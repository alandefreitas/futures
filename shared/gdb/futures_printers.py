#
# Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt
#

# To use it:
#
# * Create a directory and put the file as well as an empty __init__.py in
#   that directory.
# * Create a ~/.gdbinit file, that contains the following:
#      python
#      import sys
#      sys.path.insert(0, '/path/to/futures/printer/directory')
#      from futures_printers import register_futures_printers
#      register_futures_printers (None)
#      end

import gdb
import re
import sys

import itertools
from bisect import bisect_left

# Helpers
have_python_2 = (sys.version_info[0] == 2)
have_python_3 = (sys.version_info[0] == 3)

if have_python_3:
    xrange = range
    intptr = int
elif have_python_2:
    intptr = long

def split_parameter_pack(typename):
    """Split a string represending a comma-separated c++ parameter pack into a list of strings of element types"""

    unmatched = 0
    length = len(typename)
    b = e = 0
    while e < length:
        c = typename[e]
        if c == ',' and unmatched == 0:
            yield typename[b:e].strip()
            b = e = e + 1
        elif c == '<':
            unmatched += 1
        elif c == '>':
            unmatched -= 1
        e += 1
    yield typename[b:e].strip()

def strip_qualifiers(typename):
    """Remove const/volatile qualifiers, references, and pointers of a type"""
    qps = []

    try:
        while True:
            typename = typename.rstrip()
            qual = next(q for q in ['&', '*', 'const', 'volatile'] if typename.endswith(q))
            typename = typename[:-len(qual)]
            qps.append(qual)
    except StopIteration:
        pass

    try:
        while True:
            typename = typename.lstrip()
            qual = next(q for q in ['const', 'volatile'] if typename.startswith(q))
            typename = typename[len(qual):]
            qps.append(qual)
    except StopIteration:
        pass

    return typename, qps[::-1]

def apply_qualifiers(t, qs):
    """Apply the given sequence of references, and pointers to a gdb.Type.
       const and volatile qualifiers are not applied cince they do not affect
       printing. Also it is not possible to make a const+volatile qualified
       type in gdb."""
    for q in qs:
        if q == '*':
            t = t.pointer()
        elif q == '&':
            t = t.reference()
        elif q == 'const':
            t = t.const()
    return t

def reinterpret_cast(value, target_type):
    return value.address.cast(target_type.pointer()).dereference()

class FutureStateTypeIDPrinter:
    "Pretty Printer for futures::detail::future_state<...>::type_id"

    def __init__(self, value):
        self.value = value

    def to_string(self):
        enum_type = self.value.type
        if self.value == 0:
            return 'empty'
        if self.value == 1:
            return 'direct_storage'
        if self.value == 2:
            return 'shared_storage'
        if self.value == 3:
            return 'inline_state'
        if self.value == 4:
            return 'shared_state'
        return 'invalid type'


class MaybeEmptyPrinter:
    "Pretty Printer for futures::detail::maybe_empty<...>"

    regex = re.compile('^futures::detail::maybe_empty<(.*),(.*),(.*)>$')

    def __init__(self, value):
        self.value = value
        self.type_str = underlying_typename(value)
        m = MaybeEmptyPrinter.regex.search(self.type_str)
        self.maybe_empty_type = m.group(1)
        self.maybe_empty_index = m.group(2)
        self.is_empty = m.group(3).find('true') != -1

    def to_string(self):
        if self.is_empty:
            return 'empty type: ' + self.maybe_empty_type
        else:
            return self.maybe_empty_type

    def children(self):
        if not self.is_empty:
            yield self.maybe_empty_type, self.value['value_']
        else:
            yield 'empty', True

class FutureStatePrinter:
    "Pretty Printer for futures::detail::future_state"

    printer_name = 'futures::detail::future_state'
    template_name = 'futures::detail::future_state'
    regex = re.compile('^futures::detail::future_state<(.*)>$')

    def __init__(self, value):
        self.value = value

    def to_string(self):
        resolved_type = self.get_variant_type()
        return '(futures::detail::future_state<...>) type = {}'.format(resolve_type(resolved_type))

    def children(self):
        resolved_type = self.get_variant_type()
        stored_value = reinterpret_cast(self.value['data_']['data_'], resolved_type)
        r = str(resolved_type)
        pos = r.rfind('::')
        if pos != -1:
            r = r[pos+2:]
        yield r, stored_value
        yield 'which', self.value['type_id_']
        yield 'address', self.value['data_']['data_'].address
        # yield 'as_bytes', self.value['data_']['data_']

    def get_variant_type(self):
        """Get a gdb.Type of a template argument"""

        type_index = intptr(self.value['type_id_'])
        assert type_index >= 0, 'Heap backup is not supported'

        # This is a workaround for a GDB issue
        # https://sourceware.org/bugzilla/show_bug.cgi?id=17311.
        # gdb.Type.template_argument() method does not work unless variadic templates
        # are disabled using BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES.
        m = FutureStatePrinter.regex.search(underlying_typename(self.value))
        [R, OpState] = list(split_parameter_pack(m.group(1)))

        # Get current type name
        stored_type_name = ''
        if type_index == 0:
            stored_type_name = underlying_typename(self.value) + '::empty_t'
        elif type_index == 1:
            stored_type_name = underlying_typename(self.value) + '::operation_storage_t'
        elif type_index == 2:
            stored_type_name = underlying_typename(self.value) + '::shared_storage_t'
        elif type_index == 3:
            stored_type_name = underlying_typename(self.value) + '::operation_state_t'
        elif type_index == 4:
            stored_type_name = underlying_typename(self.value) + '::shared_state_t'
        resolved_type = gdb.lookup_type(stored_type_name)
        return resolved_type

def underlying_typename(val):
    t = val.type
    t = resolve_type(t)
    if t is not None:
        return str(t)
    else:
        return str(val.type)

def resolve_type(t):
    if t.code == gdb.TYPE_CODE_REF:
        t = t.target()
    t = t.unqualified().strip_typedefs()

    typename = t.tag
    if typename == None:
        return None
    return t

def lookup_function(val):
    "Look-up and return a pretty-printer that can print va."

    t = underlying_typename(val)
    if t.startswith('futures::detail::future_state'):
        if t.endswith('::type_id'):
            return FutureStateTypeIDPrinter(val)
        else:
            return FutureStatePrinter(val)

    if t.startswith('futures::detail::maybe_empty'):
        return MaybeEmptyPrinter(val)

    return None

def register_futures_printers(obj):
    "Register futures pretty-printers with objfile Obj"

    if obj == None:
        obj = gdb
    obj.pretty_printers.append(lookup_function)

