#!/usr/bin/env python
import subprocess, sys, os
import re
from multiprocessing import cpu_count

num_cpus=cpu_count()

class Dependency:
    def __init__(self, origin_name, name):
        self.origin_name = origin_name
        self.name = name

    def __eq__(self, other):
        return self.origin_name == other.origin_name \
            and self.name == other.name

    def __hash__(self):
        return hash(self.name)

    def __str__(self):
        return '%s %s' % (self.origin_name, self.name)

def get_dependencies_by_otool(path):
    if path.endswith('.app') and os.path.isdir(path):
        path = os.path.join(path, 'Contents', 'MacOS', os.path.basename(path))
    if path.endswith('.framework') and os.path.isdir(path):
        path = os.path.join(path, 'Contents', 'Versions', 'A', os.path.basename(path))
    lines = check_string_output(['otool', '-L', path]).split('\n')
    striped_lines = []
    for line in lines:
        if '(architecture arm64)' not in line and '(architecture x86_64)' not in line and ':' not in line:
            striped_lines.append(line.strip())
    outputs = []
    path = os.path.abspath(path)
    if path.endswith('.dylib') or '.framework' in path:
        outputs.append(Dependency(path, path));

    if '.framework' in path:
        paths = path.split(os.sep)
        path = '/'
        for path_component in paths:
            if '.framework' in path_component:
                path = os.path.join(path, path_component, 'Versions', 'A', path_component[:-len('.framework')])
                break
            path = os.path.join(path, path_component)

    for line in striped_lines:
        origin_name = name = line.split('(')[0].strip()
        if name.startswith('@executable_path/'):
            if '.framework' in path:
                name = os.path.join(os.path.dirname(path), '..', '..', '..', name[len('@executable_path/'):])
            else:
                name = os.path.join(os.path.dirname(path), name[len('@executable_path/'):])
        if name.startswith('@loader_path/'):
            name = os.path.join(os.path.dirname(path), name[len('@loader_path/'):])
        # TODO: search rpath
        if name.startswith('@rpath/'):
            continue
        name = os.path.normpath(name)
        # skip system libraries
        if name.startswith('/usr/lib') or name.startswith('/System'):
            continue
        # skip Frameworks (TODO: support frameworks, except Qt's framewroks)
        if re.search(r'(\w+)\.framework/Versions/[A-Z0-9]/(\1)$', name):
            continue
        if re.search(r'(\w+)\.framework/Versions/Current/(\1)$', name):
            continue
        if re.search(r'(\w+)\.framework/(\1)$', name):
            continue
        if name.endswith('.framework'):
            continue

        # if file not found
        if not os.path.exists(name):
            raise IOError('broken dependency: expected file "%s" for "%s" was not found for lib "%s"' % (name, origin_name, path))
        outputs.append(Dependency(origin_name, name))

    # remove duplicate items
    return list(set(outputs))

def get_dependencies_by_ldd(path):
    lines = check_string_output(['ldd', '-v', path]).split('Version information:')[1].split('\n\t')
    outputs = []

    for line in lines:
        if not line.startswith('\t'):
            continue
        name = line.strip().split('=>')[1].strip()
        # TODO: search rpath
        if name.startswith('$ORIGIN/'):
            name = os.path.join(path, name[len('$ORIGIN/'):])
        name = os.path.normpath(name)
        # skip system libraries, part2
        if name.startswith('/usr/lib') or name.startswith('/usr/lib64'):
            continue
        # skip system libraries, part3
        if name.startswith('/lib') or name.startswith('/lib64'):
            continue
        # if file not found
        if not os.path.exists(name):
            raise IOError('broken dependency: file %s not found' % name)
        outputs.append(name)

    # remove duplicate items
    return list(set(outputs))

def get_dependencies_by_objdump(path):
    print 'todo'
    return []

# TODO: actually you can use dumpbin /dependents <path to exe> to get dependency
# info
def get_dependencies_by_dependency_walker(path):
    print 'todo'
    return []

def get_dependencies(path):
    if os.name == 'winnt':
        return get_dependencies_by_objdump(path)
    elif sys.platform == 'darwin':
        return get_dependencies_by_otool(path)
    elif sys.platform == 'linux':
        return get_dependencies_by_ldd(path)
    else:
        raise IOError('not supported in platform %s' % sys.platform)

def get_dependencies_recursively(path):
    if os.name == 'winnt':
        return get_dependencies_by_objdump(path)
    elif sys.platform == 'darwin':
        deps = get_dependencies_by_otool(path)
        while(True):
            deps_extened = []
            for dep in deps:
                deps_extened.append(dep.name)
                for new_dep in get_dependencies_by_otool(dep.name):
                    deps_extened.append(new_dep.name)
            return list(set(deps_extened))
    elif sys.platform == 'linux':
        return get_dependencies_by_ldd(path)
    else:
        raise IOError('not supported in platform %s' % sys.platform)

def check_universal_build_bundle_darwin(bundle_path):
    # export from NSBundle.h
    NSBundleExecutableArchitectureX86_64    = 0x01000007
    NSBundleExecutableArchitectureARM64     = 0x0100000c
    from Foundation import NSBundle

    bundle = NSBundle.bundleWithPath_(bundle_path)
    if not bundle:
        return False

    arches = bundle.executableArchitectures()
    if NSBundleExecutableArchitectureX86_64 in arches and NSBundleExecutableArchitectureARM64 in arches:
        return True

    return False

def check_universal_build_dylib_darwin(lib_path):
    arches = check_string_output(['lipo', '-archs', lib_path])
    if 'x86_64' in arches and 'arm64' in arches:
        return True

    return False

def check_universal_build_darwin(path, verbose = False):
    checked = True
    status_msg = ''
    error_msg = ''

    if path.endswith('.framework') or path.endswith('.app') or path.endswith('.appex'):
        checked = check_universal_build_bundle_darwin(path)
        if not checked:
            status_msg += 'bundle "%s" is not universal built\n' % path
            error_msg += 'bundle "%s" is not universal built\n' % path
        else:
            status_msg += 'bundle "%s" is universal built\n' % path

        for root, dirs, files in os.walk(path):
          for d in dirs:
            full_path = os.path.join(root, d)
            if full_path.endswith('.framework') or full_path.endswith('.app') or full_path.endswith('.appex'):
                if not check_universal_build_bundle_darwin(full_path):
                    error_msg += 'bundle "%s" is not universal built\n' % full_path
                    status_msg += 'bundle "%s" is not universal built\n' % full_path
                    checked = False
                else:
                    status_msg += 'bundle "%s" is universal built\n' % full_path

          for f in files:
            full_path = os.path.join(root, f)
            if full_path.endswith('.dylib') or full_path.endswith('.so'):
                if not check_universal_build_dylib_darwin(full_path):
                    error_msg += 'dylib "%s" is not universal built\n' % full_path
                    status_msg += 'dylib "%s" is not universal built\n' % full_path
                    checked = False
                else:
                    status_msg += 'dylib "%s" is universal built\n' % full_path

    if path.endswith('.dylib') or path.endswith('.so'):
        if not check_universal_build_dylib_darwin(path):
            error_msg += 'dylib "%s" is not universal built\n' % path
            status_msg += 'dylib "%s" is not universal built\n' % path
            checked = False
        else:
            status_msg += 'dylib "%s" is universal built\n' % path


    if verbose:
        print status_msg

    if not checked:
        raise IOError(error_msg)

def check_string_output(command):
    return subprocess.check_output(command, stderr=subprocess.STDOUT).decode().strip()

_output = sys.stdout
_devnull = open(os.devnull, 'w')
def set_output(output=sys.stdout):
    global _output
    _output = output

def close_output():
    if _output != sys.stdout:
        _output.close()

def write_output(command, suppress_error = False):
    print 'exec "%s"' % (' '.join(command))
    proc = subprocess.Popen(command, stdout=_output, stderr=_output if not suppress_error else _devnull, shell=False)
    proc.communicate()

if __name__ == '__main__':
    if (len(sys.argv) < 2):
        print 'Usage: %s <input-file>' % sys.argv[0]
        sys.exit(-1)
    print get_dependencies_recursively(sys.argv[1])
