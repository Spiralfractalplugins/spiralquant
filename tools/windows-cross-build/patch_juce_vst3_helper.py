#!/usr/bin/env python3
"""Make JUCE's VST3 manifest helper work when cross-compiling Windows targets.

The nested configure in _juce_add_vst3_manifest_helper_target only forwards the
compiler names, so the helper gets built as if MSVC were the host compiler, and
the resulting .exe is then invoked directly on the build machine. Both break a
Linux -> Windows cross build. Patched copy: forward the toolchain file and run
the helper through CMAKE_CROSSCOMPILING_EMULATOR.

Applied to the container's copy of vendor/JUCE only. Idempotent.
"""
import sys

PASSTHROUGH_ANCHOR = '''    if(CMAKE_RC_COMPILER)
        list(APPEND PASSTHROUGH_ARGS "-DCMAKE_RC_COMPILER=${CMAKE_RC_COMPILER}")
    endif()
'''

PASSTHROUGH_ADD = '''
    if(CMAKE_TOOLCHAIN_FILE)
        list(APPEND PASSTHROUGH_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
    endif()
'''

RUN_ANCHOR = '''    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "creating ${output_path}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${product}/Contents/Resources"
        COMMAND "${helper_path}" > "${output_path}")
'''

RUN_ADD = '''    set(helper_run_cmd "${helper_path}")
    if(CMAKE_CROSSCOMPILING AND CMAKE_CROSSCOMPILING_EMULATOR)
        list(PREPEND helper_run_cmd "${CMAKE_CROSSCOMPILING_EMULATOR}")
    endif()

'''

RUN_REPLACE_FROM = 'COMMAND "${helper_path}" > "${output_path}")'
RUN_REPLACE_TO = 'COMMAND ${helper_run_cmd} > "${output_path}")'


def patch(path):
    with open(path, encoding='utf-8') as f:
        text = f.read()

    if '-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}' in text:
        print(f'{path}: already patched')
        return

    for anchor in (PASSTHROUGH_ANCHOR, RUN_ANCHOR):
        if text.count(anchor) != 1:
            sys.exit(f'{path}: anchor not found exactly once:\n{anchor}')

    text = text.replace(PASSTHROUGH_ANCHOR, PASSTHROUGH_ANCHOR + PASSTHROUGH_ADD, 1)
    text = text.replace(RUN_ANCHOR, RUN_ADD + RUN_ANCHOR.replace(RUN_REPLACE_FROM, RUN_REPLACE_TO), 1)

    with open(path, 'w', encoding='utf-8') as f:
        f.write(text)
    print(f'{path}: patched')


if __name__ == '__main__':
    for target in sys.argv[1:]:
        patch(target)
