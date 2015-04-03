#!/usr/bin/env python
import sys, os, shutil
import build_helper
import argparse

target='seafile-applet'
num_cpus=str(build_helper.num_cpus)
configuration = 'Release'

def postbuild_copy_libraries():
    print "copying dependent libraries..."
    if sys.platform == 'darwin':
        postbuild_copy_libraries_xcode()
    else:
        postbuild_copy_libraries_posix()

def postbuild_copy_libraries_posix():
    lib_path = os.path.join(target, 'lib')
    bin_path = os.path.join(target, 'bin')
    binaries = [os.path.join(bin_path, target),
                os.path.join(bin_path, 'ccnet'),
                os.path.join(bin_path, 'seaf-daemon')]
    libs = []
    for binrary in binaries:
        libs.extend(build_helper.get_dependencies_recursively(binrary))
    if not os.path.isdir(lib_path):
        os.makedirs(lib_path)
    for lib in libs:
        shutil.copyfile(lib, lib_path + '/' + os.path.basename(lib))

def postbuild_copy_libraries_xcode():
    frameworks_path = os.path.join(target + '.app', 'Contents', 'Frameworks')
    resources_path = os.path.join(target + '.app', 'Contents', 'Resources')
    macos_path = os.path.join(target + '.app', 'Contents', 'MacOS')
    binaries = [os.path.join(macos_path, target),
                os.path.join(resources_path, 'ccnet'),
                os.path.join(resources_path, 'seaf-daemon')]
    libs = []
    for binrary in binaries:
        libs.extend(build_helper.get_dependencies_recursively(binrary))
    if not os.path.isdir(frameworks_path):
        os.makedirs(frameworks_path)
    for lib in libs:
        shutil.copyfile(lib, frameworks_path + '/' + os.path.basename(lib))
    build_helper.write_output(['macdeployqt', target + '.app'])

def postbuild_fix_rpath():
    print "fixing rpath..."
    if os.name == 'winnt':
        print 'not need to fix rpath'
    elif sys.platform == 'linux':
        postbuild_patchelf()
    elif sys.platform == 'darwin':
        postbuild_install_name_tool()
    else:
        print 'not supported in platform %s' % sys.platform

def postbuild_install_name_tool():
    frameworks_path = os.path.join(target + '.app', 'Contents', 'Frameworks')
    resources_path = os.path.join(target + '.app', 'Contents', 'Resources')
    macos_path = os.path.join(target + '.app', 'Contents', 'MacOS')
    binaries = [os.path.join(macos_path, target),
                os.path.join(resources_path, 'ccnet'),
                os.path.join(resources_path, 'seaf-daemon')]
    for binary in binaries:
        build_helper.write_output(['install_name_tool', '-add_rpath', '@executable_path/../Frameworks', binary])
        deps = build_helper.get_dependencies(binary)
        for dep in deps:
                build_helper.write_output(['install_name_tool', '-change', dep, '@executable_path/../Frameworks/%s' % os.path.basename(dep), binary])
    libs = os.listdir(frameworks_path)
    for lib_name in libs:
        lib = os.path.join(frameworks_path, lib_name)
        if os.path.isdir(lib):
            continue
        build_helper.write_output(['install_name_tool', '-id', '@loader_path/../Frameworks/%s' % os.path.basename(lib), lib])
        build_helper.write_output(['install_name_tool', '-add_rpath', '@loader_path/../Frameworks', lib])
        deps = build_helper.get_dependencies(lib)
        for dep in deps:
                build_helper.write_output(['install_name_tool', '-change', dep, '@loader_path/../Frameworks/%s' % os.path.basename(dep), lib])

def postbuild_patchelf():
    lib_path = os.path.join(target, 'lib')
    bin_path = os.path.join(target, 'bin')
    binaries = os.listdir(bin_path)
    for binrary_name in binaries:
        binrary = os.path.join(bin_path, binrary_name)
        if os.path.isdir(binrary):
            continue
        build_helper.write_output(['patchelf', '-set-rpath', '\\\$ORIGIN/../lib', binrary])
    libs = os.listdir(lib_path)
    for lib_name in libs:
        lib = os.path.join(lib_path, lib_name)
        if os.path.isdir(lib):
            continue
        build_helper.write_output(['patchelf', '-set-rpath', '\\\$ORIGIN/../lib', lib])

def execute_buildscript(generator = 'xcode'):
    print "executing build scripts..."
    if generator == 'xcode':
        command = ['xcodebuild', '-target', 'ALL_BUILD', '-configuration', configuration, '-jobs', num_cpus]
    elif generator == 'ninja':
        command = ['ninja']
    else:
        command = ['make', '-j', num_cpus]
    build_helper.write_output(command)
    if generator == 'xcode':
        shutil.copytree(os.path.join(configuration, target+ '.app'), target + '.app')


def generate_buildscript(generator = 'xcode', os_min = '10.7', with_shibboleth = False):
    print "generating build scripts..."
    if not os.path.exists('CMakeLists.txt'):
        print 'Please execute this frome the top dir of the source'
        sys.exit(-1)
    cmake_args = ['cmake', '.', '-DCMAKE_BUILD_TYPE=' + configuration]
    cmake_args.append('-DCMAKE_OSX_DEPLOYMENT_TARGET=' + os_min);
    if with_shibboleth:
        cmake_args.append('-DBUILD_SHIBBOLETH_SUPPORT=ON')
    else:
        cmake_args.append('-DBUILD_SHIBBOLETH_SUPPORT=OFF')
    if generator == 'xcode':
        cmake_args.extend(['-G', 'Xcode'])
    elif generator == 'ninja':
        cmake_args.extend(['-G', 'Ninja'])
    else:
        cmake_args.extend(['-G', 'Unix Makefiles'])
    build_helper.write_output(cmake_args)

def prebuild_cleanup(Force=False):
    print "cleaning up previous files..."
    if Force:
        build_helper.write_output(['git', 'clean', '-xfd'])
        return
    shutil.rmtree(configuration, ignore_errors=True)
    shutil.rmtree('CMakeFiles', ignore_errors=True)
    shutil.rmtree(target+ '.app', ignore_errors=True)
    if os.path.exists ('CMakeCache.txt'):
        os.remove('CMakeCache.txt')

if __name__ == '__main__':
    if sys.platform != 'darwin':
        print 'Only support Mac OS X Platfrom!'
        exit(-1)

    parser = argparse.ArgumentParser(description='Script to build Seafile Client and package it')
    parser.add_argument('--build_type', '-t', help='build type', default='Release')
    parser.add_argument('--os_min', '-m', help='osx deploy version', default='10.7')
    parser.add_argument('--with_shibboleth', help='build with shibboleth support', action='store_true')
    parser.add_argument('--output', '-o', help='output file', default='-')
    parser.add_argument('--clean', '-c', help='clean forcely', action='store_true')
    args = parser.parse_args()

    if args.build_type == 'Debug' or args.build_type == 'debug':
        configuration = 'Debug'

    print 'build with type %s' % configuration

    os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))

    prebuild_cleanup(Force=args.clean)

    if args.output == '-':
        build_helper.set_output(sys.stdout)
    else:
        output = open(args.output, 'wb')
        build_helper.set_output(output)

    generate_buildscript(os_min=args.os_min, with_shibboleth=args.with_shibboleth)
    execute_buildscript()
    postbuild_copy_libraries()
    postbuild_fix_rpath()

    build_helper.close_output()
