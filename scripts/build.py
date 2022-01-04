#!/usr/bin/env python
import sys, os, shutil, platform
import build_helper
import argparse

target='seafile-applet'
num_cpus=str(build_helper.num_cpus)
configuration = 'Release'
qtdir = '/usr/local'
codesign_identity = os.getenv('CODESIGN_IDENTITY')
if not codesign_identity:
  codesign_identity = '-'
osx_archs = 'arm64;x86_64'

def postbuild_copy_libraries():
    print 'copying dependent libraries...'
    if sys.platform == 'darwin':
        postbuild_copy_libraries_xcode()
    else:
        postbuild_copy_libraries_posix()

def postbuild_copy_libraries_posix():
    lib_path = os.path.join(target, 'lib')
    bin_path = os.path.join(target, 'bin')
    binaries = [os.path.join(bin_path, target),
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
                os.path.join(resources_path, 'seaf-daemon')]
    libs = []
    for binrary in binaries:
        libs.extend(build_helper.get_dependencies_recursively(binrary))
    if not os.path.isdir(frameworks_path):
        os.makedirs(frameworks_path)
    for lib in libs:
        target_lib = frameworks_path + '/' + os.path.basename(lib)
        if not (os.path.exists(lib) and os.path.exists(target_lib) and os.path.samefile(lib, target_lib)):
            shutil.copyfile(lib, target_lib)
    # official macdeployqt is not supported on apple silicon yet
    # see https://github.com/crystalidea/macdeployqt-universal
    build_helper.write_output(['macdeployqt', target + '.app', '-verbose=%d' % verbose, '-qtdir=' + qtdir])

def postbuild_fix_rpath():
    print 'fixing rpath...'
    if os.name == 'winnt':
        print 'not need to fix rpath'
    elif sys.platform == 'linux':
        postbuild_patchelf()
    elif sys.platform == 'darwin':
        postbuild_install_name_tool()
    else:
        print 'not supported in platform %s' % sys.platform
    print 'fixing rpath...done'

def postbuild_install_name_tool():
    frameworks_path = os.path.join(target + '.app', 'Contents', 'Frameworks')
    resources_path = os.path.join(target + '.app', 'Contents', 'Resources')
    macos_path = os.path.join(target + '.app', 'Contents', 'MacOS')
    binaries = [os.path.join(macos_path, target),
                os.path.join(resources_path, 'seaf-daemon'),
               ]
    for binary in binaries:
        print 'patching binary "%s"' % binary
        build_helper.write_output(['install_name_tool', '-add_rpath', '@executable_path/../Frameworks', binary])
        deps = build_helper.get_dependencies_by_otool(binary)
        for dep in deps:
            build_helper.write_output(['install_name_tool', '-change', dep.origin_name, '@executable_path/../Frameworks/%s' % os.path.basename(dep.name), binary])
        build_helper.write_output(['install_name_tool', '-delete_rpath', '/usr/local/lib', binary], suppress_error = True)
        build_helper.write_output(['install_name_tool', '-delete_rpath', '/opt/local/lib', binary], suppress_error = True)
        build_helper.write_output(['install_name_tool', '-delete_rpath', '/opt/local/libexec/openssl11/lib', binary], suppress_error = True)
    libs = os.listdir(frameworks_path)
    for lib_name in libs:
        print 'patching lib "%s"' % lib_name
        lib = os.path.join(frameworks_path, lib_name)
        # let's fix Framework as well
        if os.path.isdir(lib) and lib.endswith('.framework'):
          lib = os.path.join(lib, 'Versions', 'A', lib_name[:-len('.framework')])
          loader_path = '@loader_path/../../../../Frameworks'
        else:
          loader_path = '@loader_path/../Frameworks'
        build_helper.write_output(['install_name_tool', '-add_rpath', '@loader_path/../Frameworks', lib])
        build_helper.write_output(['install_name_tool', '-delete_rpath', '/usr/local/lib', lib], suppress_error = True)
        build_helper.write_output(['install_name_tool', '-delete_rpath', '/opt/local/lib', lib], suppress_error = True)
        build_helper.write_output(['install_name_tool', '-delete_rpath', '/opt/local/libexec/openssl11/lib', lib], suppress_error = True)
        deps = build_helper.get_dependencies_by_otool(lib)
        for dep in deps:
            build_helper.write_output(['install_name_tool', '-change', dep.origin_name, '%s/%s' % (loader_path, os.path.basename(dep.name)), lib])

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

def postbuild_codesign():
    print 'fixing codesign...'
    if sys.platform == 'darwin':
        # reference https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution/resolving_common_notarization_issues?language=objc
        # Hardened runtime is available in the Capabilities pane of Xcode 10 or later
        codesign_command_lines= ['codesign', '--timestamp=none', '--preserve-metadata=entitlements', '--force', '--deep', '--sign', codesign_identity, target + '.app']
        if configuration == 'Release':
            codesign_command_lines.insert(3,'--options=runtime')
        build_helper.write_output(codesign_command_lines)
        build_helper.write_output(['codesign', '-dv', '--deep', '--strict', '--verbose=%d' % verbose, target + '.app'])
        build_helper.write_output(['codesign', '-d', '--entitlements', ':-', target + '.app'])
        build_helper.write_output(['spctl', '-vvv', '--assess', '--type', 'exec', '--raw', target + '.app'])

def postbuild_check_universal_build():
    print 'check universal build...'
    # check if binary is built universally
    if sys.platform == 'darwin':
        build_helper.check_universal_build_darwin(target + '.app', verbose = True)

def execute_buildscript(generator = 'xcode'):
    print 'executing build scripts...'
    if generator == 'xcode':
        command = ['xcodebuild', '-target', 'ALL_BUILD', '-configuration', configuration, '-jobs', num_cpus]
    elif generator == 'ninja':
        command = ['ninja']
    else:
        command = ['make', '-j', num_cpus]
    build_helper.write_output(command)

def generate_buildscript(generator = 'xcode', os_min = '10.14', with_shibboleth = False):
    print 'generating build scripts...'
    if not os.path.exists('CMakeLists.txt'):
        print 'Please execute this frome the top dir of the source'
        sys.exit(-1)
    cmake_args = ['cmake', '.', '-DCMAKE_BUILD_TYPE=' + configuration]
    cmake_args.append('-DCMAKE_OSX_DEPLOYMENT_TARGET=' + os_min)
    if platform.machine() == 'arm64':
        cmake_args.append('-DCMAKE_OSX_ARCHITECTURES=%s' % osx_archs)
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
    print 'cleaning up previous files...'
    if Force:
        build_helper.write_output(['git', 'clean', '-xfd'])
        return
    shutil.rmtree(configuration, ignore_errors=True)
    shutil.rmtree('CMakeFiles', ignore_errors=True)
    shutil.rmtree(target+ '.app', ignore_errors=True)
    shutil.rmtree('seafile-client.xcodeproj', ignore_errors=True)
    shutil.rmtree('seafile-client.build', ignore_errors=True)
    if os.path.exists ('CMakeCache.txt'):
        os.remove('CMakeCache.txt')

if __name__ == '__main__':
    if sys.platform != 'darwin':
        print 'Only support Mac OS X Platfrom!'
        exit(-1)

    parser = argparse.ArgumentParser(description='Script to build Seafile Client and package it')
    parser.add_argument('--build_type', '-t', help='build type', default='Release')
    parser.add_argument('--os_min', '-m', help='osx deploy version', default='10.14')
    parser.add_argument('--with_shibboleth', help='build with shibboleth support', action='store_true')
    parser.add_argument('--output', '-o', help='output file', default='-')
    parser.add_argument('--clean', '-c', help='clean directory before build', action='store_true')
    parser.add_argument('--force_clean', '-f', help='clean directory forcely', action='store_true')
    parser.add_argument('--verbose', '-v', help='verbose logging', action='store_true')
    args = parser.parse_args()

    if args.build_type == 'Debug' or args.build_type == 'debug':
        configuration = 'Debug'

    if args.verbose:
        verbose = 3
    else:
        verbose = 1

    print 'build with type %s' % configuration

    os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))

    if args.clean:
        prebuild_cleanup(args.force_clean)

    if args.output == '-':
        build_helper.set_output(sys.stdout)
    else:
        output = open(args.output, 'wb')
        build_helper.set_output(output)

    generate_buildscript(os_min=args.os_min, with_shibboleth=args.with_shibboleth)
    execute_buildscript()
    postbuild_copy_libraries()
    postbuild_fix_rpath()
    postbuild_codesign()
    postbuild_check_universal_build()

    build_helper.close_output()
