#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This File is Copied From Chromium Project
# Chilledheart commented
#

"""A tool to generate symbols for a binary suitable for breakpad.

Currently, the tool only supports Linux, Android, and Mac. Support for other
platforms is planned.
"""

import errno
import optparse
import os
import Queue
import re
import shutil
import subprocess
import sys
import threading


CONCURRENT_TASKS=4


def GetCommandOutput(command):
  """Runs the command list, returning its output.

  Prints the given command (which should be a list of one or more strings),
  then runs it and returns its output (stdout) as a string.

  From chromium_utils.
  """
  devnull = open(os.devnull, 'w')
  proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=devnull,
                          bufsize=1)
  output = proc.communicate()[0]
  return output


def GetDumpSymsBinary(build_dir=None):
  """Returns the path to the dump_syms binary."""
  DUMP_SYMS = 'dump_syms'
  dump_syms_bin = os.path.join(os.path.expanduser(build_dir), DUMP_SYMS)
  if not os.access(dump_syms_bin, os.X_OK):
    print 'Cannot find %s.' % dump_syms_bin
    return None

  return dump_syms_bin


def Resolve(path, exe_path, loader_path, rpaths):
  """Resolve a dyld path.

  @executable_path is replaced with |exe_path|
  @loader_path is replaced with |loader_path|
  @rpath is replaced with the first path in |rpaths| where the referenced file
      is found
  """
  path = path.replace('@loader_path', loader_path)
  path = path.replace('@executable_path', exe_path)
  if path.find('@rpath') != -1:
    for rpath in rpaths:
      new_path = Resolve(path.replace('@rpath', rpath), exe_path, loader_path,
                         [])
      if os.access(new_path, os.X_OK):
        return new_path
    return ''
  return path


def GetSharedLibraryDependenciesLinux(binary):
  """Return absolute paths to all shared library dependecies of the binary.

  This implementation assumes that we're running on a Linux system."""
  ldd = GetCommandOutput(['ldd', binary])
  lib_re = re.compile('\t.* => (.+) \(.*\)$')
  result = []
  for line in ldd.splitlines():
    m = lib_re.match(line)
    if m:
      result.append(m.group(1))
  return result


def GetSharedLibraryDependenciesMac(binary, exe_path):
  """Return absolute paths to all shared library dependecies of the binary.

  This implementation assumes that we're running on a Mac system."""
  loader_path = os.path.dirname(binary)
  otool = GetCommandOutput(['otool', '-l', binary]).splitlines()
  rpaths = []
  for idx, line in enumerate(otool):
    if line.find('cmd LC_RPATH') != -1:
      m = re.match(' *path (.*) \(offset .*\)$', otool[idx+2])
      rpaths.append(m.group(1))

  otool = GetCommandOutput(['otool', '-L', binary]).splitlines()
  lib_re = re.compile('\t(.*) \(compatibility .*\)$')
  deps = []
  for line in otool:
    m = lib_re.match(line)
    if m:
      dep = Resolve(m.group(1), exe_path, loader_path, rpaths)
      if dep:
        deps.append(os.path.normpath(dep))
  return deps


def GetSharedLibraryDependencies(options, binary, exe_path):
  """Return absolute paths to all shared library dependecies of the binary."""
  deps = []
  if sys.platform.startswith('linux'):
    deps = GetSharedLibraryDependenciesLinux(binary)
  elif sys.platform == 'darwin':
    deps = GetSharedLibraryDependenciesMac(binary, exe_path)
  else:
    print "Platform not supported."
    sys.exit(1)

  result = []
  build_dir = os.path.abspath(options.build_dir)
  for dep in deps:
    if (os.access(dep, os.X_OK) and
        os.path.abspath(os.path.dirname(dep)).startswith(build_dir)):
      result.append(dep)
  return result


def mkdir_p(path):
  """Simulates mkdir -p."""
  try:
    os.makedirs(path)
  except OSError as e:
    if e.errno == errno.EEXIST and os.path.isdir(path):
      pass
    else: raise


def GenerateSymbols(options, binaries):
  """Dumps the symbols of binary and places them in the given directory."""

  queue = Queue.Queue()
  print_lock = threading.Lock()

  def _Worker():
    while True:
      binary = queue.get()

      should_dump_syms = True
      reason = "no reason"

      output_path = os.path.join(
          options.symbols_dir, os.path.basename(binary))
      if os.path.isdir(output_path):
        if os.path.getmtime(binary) < os.path.getmtime(output_path):
          should_dump_syms = False
          reason = "symbols are more current than binary"

      if not should_dump_syms:
        if options.verbose:
          with print_lock:
            print "Skipping %s (%s)" % (binary, reason)
        queue.task_done()
        continue

      if options.verbose:
        with print_lock:
          print "Generating symbols for %s" % binary

      if os.path.isdir(output_path):
        os.utime(output_path, None)

      syms = GetCommandOutput([GetDumpSymsBinary(options.build_dir), '-r',
                               binary])
      module_line = re.match("MODULE [^ ]+ [^ ]+ ([0-9A-F]+) (.*)\n", syms)
      output_path = os.path.join(options.symbols_dir, module_line.group(2),
                                 module_line.group(1))
      mkdir_p(output_path)
      symbol_file = "%s.sym" % module_line.group(2)
      try:
        f = open(os.path.join(output_path, symbol_file), 'w')
        f.write(syms)
        f.close()
      except Exception, e:
        # Not much we can do about this.
        with print_lock:
          print e

      queue.task_done()

  for binary in binaries:
    queue.put(binary)

  for _ in range(options.jobs):
    t = threading.Thread(target=_Worker)
    t.daemon = True
    t.start()

  queue.join()


def main():
  parser = optparse.OptionParser()
  parser.add_option('', '--build-dir', default='',
                    help='The build output directory.')
  parser.add_option('', '--symbols-dir', default='',
                    help='The directory where to write the symbols file.')
  parser.add_option('', '--binary', default='',
                    help='The path of the binary to generate symbols for.')
  parser.add_option('', '--clear', default=False, action='store_true',
                    help='Clear the symbols directory before writing new '
                         'symbols.')
  parser.add_option('-j', '--jobs', default=CONCURRENT_TASKS, action='store',
                    type='int', help='Number of parallel tasks to run.')
  parser.add_option('-v', '--verbose', action='store_true',
                    help='Print verbose status output.')

  (options, _) = parser.parse_args()

  if not options.symbols_dir:
    print "Required option --symbols-dir missing."
    return 1

  if not options.build_dir:
    print "Required option --build-dir missing."
    return 1

  if not options.binary:
    print "Required option --binary missing."
    return 1

  if not os.access(options.binary, os.X_OK):
    print "Cannot find %s." % options.binary
    return 1

  if options.clear:
    try:
      shutil.rmtree(options.symbols_dir)
    except:
      pass

  if not GetDumpSymsBinary(options.build_dir):
    return 1

  # Build the transitive closure of all dependencies.
  binaries = set([options.binary])
  queue = [options.binary]
  exe_path = os.path.dirname(options.binary)
  while queue:
    deps = GetSharedLibraryDependencies(options, queue.pop(0), exe_path)
    new_deps = set(deps) - binaries
    binaries |= new_deps
    queue.extend(list(new_deps))

  GenerateSymbols(options, binaries)

  return 0


if '__main__' == __name__:
  sys.exit(main())
