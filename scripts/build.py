#!/usr/bin/env python
import sys, os, shutil
import build_helper

target='seafile-applet'
num_cpus=str(build_helper.num_cpus)

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
    build_helper.check_string_output(['macdeployqt', target + '.app'])

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
        build_helper.check_string_output(['install_name_tool', '-add_rpath', '@executable_path/../Frameworks', binary])
        deps = build_helper.get_dependencies(binary)
        for dep in deps:
                build_helper.check_string_output(['install_name_tool', '-change', dep, '@executable_path/../Frameworks/%s' % os.path.basename(dep), binary])
    libs = os.listdir(frameworks_path)
    for lib_name in libs:
        lib = os.path.join(frameworks_path, lib_name)
        if os.path.isdir(lib):
            continue
        build_helper.check_string_output(['install_name_tool', '-id', '@loader_path/../Frameworks/%s' % os.path.basename(lib), lib])
        build_helper.check_string_output(['install_name_tool', '-add_rpath', '@loader_path/../Frameworks', lib])
        deps = build_helper.get_dependencies(lib)
        for dep in deps:
                build_helper.check_string_output(['install_name_tool', '-change', dep, '@loader_path/../Frameworks/%s' % os.path.basename(dep), lib])

def postbuild_patchelf():
    lib_path = os.path.join(target, 'lib')
    bin_path = os.path.join(target, 'bin')
    binaries = os.listdir(bin_path)
    for binrary_name in binaries:
        binrary = os.path.join(bin_path, binrary_name)
        if os.path.isdir(binrary):
            continue
        build_helper.check_string_output(['patchelf', '-set-rpath', '\\\$ORIGIN/../lib', binrary])
    libs = os.listdir(lib_path)
    for lib_name in libs:
        lib = os.path.join(lib_path, lib_name)
        if os.path.isdir(lib):
            continue
        build_helper.check_string_output(['patchelf', '-set-rpath', '\\\$ORIGIN/../lib', lib])

def execute_buildscript(generator = 'xcode', build_type = 'Release'):
    print "executing build scripts..."
    if generator == 'xcode':
        command = ['xcodebuild', '-target', target, '-configuration', build_type, '-jobs', num_cpus]
    elif generator == 'ninja':
        command = ['ninja']
    else:
        command = ['make', '-j', num_cpus]
    build_helper.check_string_output(command)
    if generator == 'xcode':
        shutil.copytree(os.path.join(build_type, target+ '.app'), target + '.app')


def generate_buildscript(generator = 'xcode', build_type = 'Release'):
    print "generating build scripts..."
    if not os.path.exists('CMakeLists.txt'):
        print 'Please execute this frome the top dir of the source'
        sys.exit(-1)
    cmake_args = ['cmake', '.', '-DCMAKE_BUILD_TYPE=' + build_type]
    if generator == 'xcode':
        cmake_args.extend(['-G', 'Xcode'])
    elif generator == 'ninja':
        cmake_args.extend(['-G', 'Ninja'])
    else:
        cmake_args.extend(['-G', 'Unix Makefiles'])
    build_helper.check_string_output(cmake_args)

def prebuild_cleanup(Force=False):
    print "cleaning up previous files..."
    if Force:
        build_helper.check_string_output(['git', 'clean', '-xfd'])
        return
    shutil.rmtree('Release', ignore_errors=True)
    shutil.rmtree('Debug', ignore_errors=True)
    shutil.rmtree('CMakeFiles', ignore_errors=True)
    shutil.rmtree(target+ '.app', ignore_errors=True)
    if os.path.exists ('CMakeCache.txt'):
        os.remove('CMakeCache.txt')

if __name__ == '__main__':
    if sys.platform != 'darwin':
        print 'Only support Mac OS X Platfrom!'
        exit(-1)
    if len(sys.argv) >= 2 and (sys.argv[1] == 'Debug' or sys.argv[1] == 'debug'):
        configuration = 'Debug'
    else:
        configuration = 'Release'

    prebuild_cleanup()
    generate_buildscript(build_type=configuration)
    execute_buildscript(build_type=configuration)
    postbuild_copy_libraries()
    postbuild_fix_rpath()
