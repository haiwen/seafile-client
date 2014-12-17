#!/usr/bin/env python
import subprocess, sys, os
import re
from multiprocessing import cpu_count

num_cpus=cpu_count()

def get_dependencies_by_otool(path):
    if path.endswith('.app') and os.path.isdir(path):
        path = os.path.join(path, 'Contents', 'MacOS', os.path.basename(path))
    lines = check_string_output(['otool', '-L', path]).split('\n')[1:]
    outputs = [path]

    for line in lines:
        name = line.split('(')[0].strip()
        if name.startswith('@executable_path/'):
            name = os.path.join(path, name[len('@executable_path/'):])
        if name.startswith('@loader_path/'):
            name = os.path.join(path, name[len('@loader_path/'):])
        # todo: search rpath
        if not name.startswith('/'):
            continue
        # skip system libraries
        if name.startswith('/usr/lib') or name.startswith('/System'):
            continue
        # skip Frameworks (TODO: support frameworks)
        if re.search(r'(\w+)\.framework/Versions/[A-Z]/(\1)$', name):
            continue
        # if file not found
        if not os.path.exists(name):
            raise IOError('broken dependency: file %s not found' % name)
        outputs.append(os.path.normpath(name))

    return outputs

def get_dependencies_by_ldd(path):
    lines = check_string_output(['ldd', '-v', path]).split('Version information:')[1].split('\n\t')
    outputs = []

    for line in lines:
        if not line.startswith('\t'):
            continue
        name = line.strip().split('=>')[1].strip()
        # todo: search rpath
        if name.startswith('$ORIGIN/'):
            name = os.path.join(path, name[len('$ORIGIN/'):])
        # skip system libraries, such as linux-vdso.so
        if not name.startswith('/'):
            continue
        # skip system libraries, part2
        if name.startswith('/usr/lib') or name.startswith('/usr/lib64'):
            continue
        # skip system libraries, part3
        if name.startswith('/lib') or name.startswith('/lib64'):
            continue
        # if file not found
        if not os.path.exists(name):
            raise IOError('broken dependency: file %s not found' % name)
        outputs.append(os.path.normpath(name))

    # remove duplicate items
    return list(set(outputs))

def get_dependencies_by_objdump(path):
    print 'todo'
    return []

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
            deps_extened = list(deps)
            for dep in deps:
                deps_extened.extend(get_dependencies_by_otool(dep))
            deps_extened = set(deps_extened)
            if deps_extened != deps:
                deps = deps_extened;
            else:
                return list(deps_extened)
    elif sys.platform == 'linux':
        return get_dependencies_by_ldd(path)
    else:
        raise IOError('not supported in platform %s' % sys.platform)

def check_string_output(command):
    return subprocess.check_output(command).decode().strip()

if __name__ == '__main__':
    if (len(sys.argv) < 2):
        print 'Usage: %s <input-file>' % sys.argv[0]
        sys.exit(-1)
    print get_dependencies_recursively(sys.argv[1])
