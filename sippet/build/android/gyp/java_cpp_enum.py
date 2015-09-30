#!/usr/bin/env python
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import re
import optparse
import os
from string import Template
import sys
from StringIO import StringIO

script_dir = os.path.dirname(os.path.realpath(__file__))
checkout_root = os.path.abspath(os.path.join(script_dir, os.pardir, os.pardir,
    os.pardir, os.pardir))

sys.path.insert(0, os.path.join(checkout_root, 'build', 'android', 'gyp'))

from util import build_utils

# List of C++ types that are compatible with the Java code generated by this
# script.
#
# This script can parse .idl files however, at present it ignores special
# rules such as [cpp_enum_prefix_override="ax_attr"].
ENUM_FIXED_TYPE_WHITELIST = ['char', 'unsigned char',
  'short', 'unsigned short',
  'int', 'int8_t', 'int16_t', 'int32_t', 'uint8_t', 'uint16_t']

def DoPreprocess(path, include_path_list=[]):
  gcc_cmd = [ 'gcc' ]  # invoke host gcc.
  if include_path_list:
    gcc_cmd.extend(sum(map(lambda w: ['-I', w], include_path_list), []))
  gcc_cmd.extend([
      '-E',                  # stop after preprocessing.
      '-C',                  # keep comments
      '-D', 'ANDROID',       # Specify ANDROID define for pre-processor.
      '-x', 'c-header',      # treat sources as C header files
      '-P',                  # disable line markers, i.e. '#line 309'
      path
      ])

  contents = build_utils.CheckOutput(gcc_cmd)
  return StringIO(contents).readlines()


class EnumDefinition(object):
  def __init__(self, original_enum_name=None, class_name_override=None,
               enum_package=None, entries=None, fixed_type=None):
    self.original_enum_name = original_enum_name
    self.class_name_override = class_name_override
    self.enum_package = enum_package
    self.entries = collections.OrderedDict(entries or [])
    self.prefix_to_strip = None
    self.fixed_type = fixed_type

  def AppendEntry(self, key, value):
    if key in self.entries:
      raise Exception('Multiple definitions of key %s found.' % key)
    self.entries[key] = value

  @property
  def class_name(self):
    return self.class_name_override or self.original_enum_name

  def Finalize(self):
    self._Validate()
    self._AssignEntryIndices()
    self._StripPrefix()

  def _Validate(self):
    assert self.class_name
    assert self.enum_package
    assert self.entries
    if self.fixed_type and self.fixed_type not in ENUM_FIXED_TYPE_WHITELIST:
      raise Exception('Fixed type %s for enum %s not whitelisted.' %
          (self.fixed_type, self.class_name))

  def _AssignEntryIndices(self):
    # Enums, if given no value, are given the value of the previous enum + 1.
    if not all(self.entries.values()):
      prev_enum_value = -1
      for key, value in self.entries.iteritems():
        if not value:
          self.entries[key] = prev_enum_value + 1
        elif value in self.entries:
          self.entries[key] = self.entries[value]
        else:
          try:
            self.entries[key] = int(value)
          except ValueError:
            raise Exception('Could not interpret integer from enum value "%s" '
                            'for key %s.' % (value, key))
        prev_enum_value = self.entries[key]


  def _StripPrefix(self):
    prefix_to_strip = self.prefix_to_strip
    if not prefix_to_strip:
      prefix_to_strip = self.original_enum_name
      prefix_to_strip = re.sub('(?!^)([A-Z]+)', r'_\1', prefix_to_strip).upper()
      prefix_to_strip += '_'
      if not all([w.startswith(prefix_to_strip) for w in self.entries.keys()]):
        prefix_to_strip = ''

    entries = collections.OrderedDict()
    for (k, v) in self.entries.iteritems():
      stripped_key = k.replace(prefix_to_strip, '', 1)
      if isinstance(v, basestring):
        stripped_value = v.replace(prefix_to_strip, '', 1)
      else:
        stripped_value = v
      entries[stripped_key] = stripped_value

    self.entries = entries

class DirectiveSet(object):
  class_name_override_key = 'CLASS_NAME_OVERRIDE'
  enum_package_key = 'ENUM_PACKAGE'
  prefix_to_strip_key = 'PREFIX_TO_STRIP'

  known_keys = [class_name_override_key, enum_package_key, prefix_to_strip_key]

  def __init__(self):
    self._directives = {}

  def Update(self, key, value):
    if key not in DirectiveSet.known_keys:
      raise Exception("Unknown directive: " + key)
    self._directives[key] = value

  @property
  def empty(self):
    return len(self._directives) == 0

  def UpdateDefinition(self, definition):
    definition.class_name_override = self._directives.get(
        DirectiveSet.class_name_override_key, '')
    definition.enum_package = self._directives.get(
        DirectiveSet.enum_package_key)
    definition.prefix_to_strip = self._directives.get(
        DirectiveSet.prefix_to_strip_key)


class HeaderParser(object):
  single_line_comment_re = re.compile(r'\s*//')
  multi_line_comment_start_re = re.compile(r'\s*/\*')
  enum_line_re = re.compile(r'^\s*(\w+)(\s*\=\s*([^,\n]+))?,?')
  enum_end_re = re.compile(r'^\s*}\s*;\.*$')
  generator_directive_re = re.compile(
      r'^\s*//\s+GENERATED_JAVA_(\w+)\s*:\s*([\.\w]+)$')
  multi_line_generator_directive_start_re = re.compile(
      r'^\s*//\s+GENERATED_JAVA_(\w+)\s*:\s*\(([\.\w]*)$')
  multi_line_directive_continuation_re = re.compile(
      r'^\s*//\s+([\.\w]+)$')
  multi_line_directive_end_re = re.compile(
      r'^\s*//\s+([\.\w]*)\)$')

  optional_class_or_struct_re = r'(class|struct)?'
  enum_name_re = r'(\w+)'
  optional_fixed_type_re = r'(\:\s*(\w+\s*\w+?))?'
  enum_start_re = re.compile(r'^\s*(?:\[cpp.*\])?\s*enum\s+' +
      optional_class_or_struct_re + '\s*' + enum_name_re + '\s*' +
      optional_fixed_type_re + '\s*{\s*$')

  def __init__(self, lines, path=None):
    self._lines = lines
    self._path = path
    self._enum_definitions = []
    self._in_enum = False
    self._current_definition = None
    self._generator_directives = DirectiveSet()
    self._multi_line_generator_directive = None

  def _ApplyGeneratorDirectives(self):
    self._generator_directives.UpdateDefinition(self._current_definition)
    self._generator_directives = DirectiveSet()

  def ParseDefinitions(self):
    for line in self._lines:
      self._ParseLine(line)
    return self._enum_definitions

  def _ParseLine(self, line):
    if self._multi_line_generator_directive:
      self._ParseMultiLineDirectiveLine(line)
    elif not self._in_enum:
      self._ParseRegularLine(line)
    else:
      self._ParseEnumLine(line)

  def _ParseEnumLine(self, line):
    if HeaderParser.single_line_comment_re.match(line):
      return
    if HeaderParser.multi_line_comment_start_re.match(line):
      raise Exception('Multi-line comments in enums are not supported in ' +
                      self._path)
    enum_end = HeaderParser.enum_end_re.match(line)
    enum_entry = HeaderParser.enum_line_re.match(line)
    if enum_end:
      self._ApplyGeneratorDirectives()
      self._current_definition.Finalize()
      self._enum_definitions.append(self._current_definition)
      self._in_enum = False
    elif enum_entry:
      enum_key = enum_entry.groups()[0]
      enum_value = enum_entry.groups()[2]
      self._current_definition.AppendEntry(enum_key, enum_value)

  def _ParseMultiLineDirectiveLine(self, line):
    multi_line_directive_continuation = (
        HeaderParser.multi_line_directive_continuation_re.match(line))
    multi_line_directive_end = (
        HeaderParser.multi_line_directive_end_re.match(line))

    if multi_line_directive_continuation:
      value_cont = multi_line_directive_continuation.groups()[0]
      self._multi_line_generator_directive[1].append(value_cont)
    elif multi_line_directive_end:
      directive_name = self._multi_line_generator_directive[0]
      directive_value = "".join(self._multi_line_generator_directive[1])
      directive_value += multi_line_directive_end.groups()[0]
      self._multi_line_generator_directive = None
      self._generator_directives.Update(directive_name, directive_value)
    else:
      raise Exception('Malformed multi-line directive declaration in ' +
                      self._path)

  def _ParseRegularLine(self, line):
    enum_start = HeaderParser.enum_start_re.match(line)
    generator_directive = HeaderParser.generator_directive_re.match(line)
    multi_line_generator_directive_start = (
        HeaderParser.multi_line_generator_directive_start_re.match(line))

    if generator_directive:
      directive_name = generator_directive.groups()[0]
      directive_value = generator_directive.groups()[1]
      self._generator_directives.Update(directive_name, directive_value)
    elif multi_line_generator_directive_start:
      directive_name = multi_line_generator_directive_start.groups()[0]
      directive_value = multi_line_generator_directive_start.groups()[1]
      self._multi_line_generator_directive = (directive_name, [directive_value])
    elif enum_start:
      if self._generator_directives.empty:
        return
      self._current_definition = EnumDefinition(
          original_enum_name=enum_start.groups()[1],
          fixed_type=enum_start.groups()[3])
      self._in_enum = True

def GetScriptName():
  script_components = os.path.abspath(sys.argv[0]).split(os.path.sep)
  build_index = script_components.index('build')
  return os.sep.join(script_components[build_index:])


def DoGenerate(output_dir, source_paths, print_output_only=False,
    include_path_list=[]):
  output_paths = []
  for source_path in source_paths:
    enum_definitions = DoParseHeaderFile(source_path, include_path_list)
    if not enum_definitions:
      raise Exception('No enums found in %s\n'
                      'Did you forget prefixing enums with '
                      '"// GENERATED_JAVA_ENUM_PACKAGE: foo"?' %
                      source_path)
    for enum_definition in enum_definitions:
      package_path = enum_definition.enum_package.replace('.', os.path.sep)
      file_name = enum_definition.class_name + '.java'
      output_path = os.path.join(output_dir, package_path, file_name)
      output_paths.append(output_path)
      if not print_output_only:
        build_utils.MakeDirectory(os.path.dirname(output_path))
        DoWriteOutput(source_path, output_path, enum_definition)
  return output_paths


def DoParseHeaderFile(path, include_path_list):
  lines = DoPreprocess(path, include_path_list)
  return HeaderParser(lines, path).ParseDefinitions()


def GenerateOutput(source_path, enum_definition):
  template = Template("""
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is autogenerated by
//     ${SCRIPT_NAME}
// From
//     ${SOURCE_PATH}

package ${PACKAGE};

public class ${CLASS_NAME} {
${ENUM_ENTRIES}
}
""")

  enum_template = Template('  public static final int ${NAME} = ${VALUE};')
  enum_entries_string = []
  for enum_name, enum_value in enum_definition.entries.iteritems():
    values = {
        'NAME': enum_name,
        'VALUE': enum_value,
    }
    enum_entries_string.append(enum_template.substitute(values))
  enum_entries_string = '\n'.join(enum_entries_string)

  values = {
      'CLASS_NAME': enum_definition.class_name,
      'ENUM_ENTRIES': enum_entries_string,
      'PACKAGE': enum_definition.enum_package,
      'SCRIPT_NAME': GetScriptName(),
      'SOURCE_PATH': source_path,
  }
  return template.substitute(values)


def DoWriteOutput(source_path, output_path, enum_definition):
  with open(output_path, 'w') as out_file:
    out_file.write(GenerateOutput(source_path, enum_definition))

def AssertFilesList(output_paths, assert_files_list):
  actual = set(output_paths)
  expected = set(assert_files_list)
  if not actual == expected:
    need_to_add = list(actual - expected)
    need_to_remove = list(expected - actual)
    raise Exception('Output files list does not match expectations. Please '
                    'add %s and remove %s.' % (need_to_add, need_to_remove))

def DoMain(argv):
  usage = 'usage: %prog [options] output_dir input_file(s)...'
  parser = optparse.OptionParser(usage=usage)

  parser.add_option('--assert_file', action="append", default=[],
                    dest="assert_files_list", help='Assert that the given '
                    'file is an output. There can be multiple occurrences of '
                    'this flag.')
  parser.add_option('--print_output_only', help='Only print output paths.',
                    action='store_true')
  parser.add_option('--verbose', help='Print more information.',
                    action='store_true')
  parser.add_option('-I', help='Include path.', default=[], metavar='PATH',
                    action='append', dest='include_path_list')

  options, args = parser.parse_args(argv)
  if len(args) < 2:
    parser.error('Need to specify output directory and at least one input file')
  output_paths = DoGenerate(args[0], args[1:],
                            print_output_only=options.print_output_only,
                            include_path_list=options.include_path_list)

  if options.assert_files_list:
    AssertFilesList(output_paths, options.assert_files_list)

  if options.verbose:
    print 'Output paths:'
    print '\n'.join(output_paths)

  return ' '.join(output_paths)

if __name__ == '__main__':
  DoMain(sys.argv[1:])
