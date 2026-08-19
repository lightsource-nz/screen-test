# Per-project defaults for the light framework scripts in $env:LIGHT_PATH/scripts.
#
# WHY THIS EXISTS AS DATA: which preset builds a target, and which build tree that preset owns,
# was previously knowable only by reading CMakePresets.json and cross-referencing each tree's
# CMakeCache.txt. Two of those trees (build-host) have no preset at all and their settings
# existed nowhere but in their own cache. Recording it here makes the mapping reviewable, and
# lets the configure script refuse to reuse a tree belonging to a different preset.
#
# NOTE the build directory collisions below are real and deliberate on the preset side --
# conf-screentest-debug, -host-debug, -trace and -release all resolve to ${sourceDir}/build
# because only some presets override binaryDir. That is exactly what light-configure.ps1 guards.
@{
        Name = 'screen-test'

        # preset -> build tree, relative to the project root
        Trees = @{
                'conf-screentest-debug'                         = 'build'
                # split by CHIP, as crossfire's trees are: the po13 rig is a Pico 2 since its
                # 2026-08 rebuild, and its tree coexists with the rp2040 one in build/
                'conf-screentest-pico2-debug'                   = 'build-pico2'
                'conf-screentest-host-debug'                    = 'build'
                'conf-screentest-host-os-debug'                 = 'build-host'
                'conf-screentest-trace'                         = 'build-trace'
                'conf-screentest-release'                       = 'build-release'
                'conf-screentest-waveshare-touch169-debug'      = 'build-waveshare-touch169'
                'conf-screentest-waveshare-touch169-riscv-debug' = 'build-waveshare-touch169-riscv'
                'conf-screentest-mini-stm32h7-debug'            = 'build-mini-stm32h7'
        }

        # target -> which preset builds it. Flash notes the artifact kind: 'uf2' targets can be
        # flashed over USB by light-flash.ps1; 'swd' targets produce only .bin/.hex and need a
        # probe, which is not scripted yet.
        Targets = @{
                'screentest_sh1106_spi4'   = @{ Preset = 'conf-screentest-debug'; Flash = 'uf2' }
                'screentest_sh1106_i2c'    = @{ Preset = 'conf-screentest-debug'; Flash = 'uf2' }
                #   the po13 rig is a Pico 2 in its SWD dock now, so both its targets build with
                # the pico2 preset and reach the board through the probe (scripts/debug.ps1
                # -Batch) rather than BOOTSEL
                'screentest_po13'          = @{ Preset = 'conf-screentest-pico2-debug'; Flash = 'swd' }
                'light_ui_demo_po13'       = @{ Preset = 'conf-screentest-pico2-debug'; Flash = 'swd' }
                'screentest_ws_touch169'   = @{ Preset = 'conf-screentest-waveshare-touch169-debug'; Flash = 'uf2' }
                'light_ui_demo_touch169'   = @{ Preset = 'conf-screentest-waveshare-touch169-debug'; Flash = 'uf2' }
                # no build preset exists for this one; it is a bring-up tool for measuring the
                # panel's corner radius
                'screentest_calib169'      = @{ Preset = 'conf-screentest-waveshare-touch169-debug'; Flash = 'uf2' }
                'screentest_mini_stm32h7'  = @{ Preset = 'conf-screentest-mini-stm32h7-debug'; Flash = 'swd' }
        }

        #   what each preset should produce in the cache. Only needed for presets that SHARE a
        # build directory, which is where an in-place reconfigure would silently reuse the wrong
        # cache -- and only checkable this way for trees configured before these scripts existed,
        # which today is all of them.
        Expect = @{
                'conf-screentest-debug'                    = @{ LIGHT_PLATFORM = 'TARGET'; LIGHT_BOARD = 'pico' }
                'conf-screentest-pico2-debug'              = @{ LIGHT_PLATFORM = 'TARGET'; LIGHT_BOARD = 'pico2'; PICO_PLATFORM = 'rp2350-arm-s' }
                'conf-screentest-host-debug'               = @{ LIGHT_PLATFORM = 'HOST'; LIGHT_BOARD = 'pico_hostmode' }
                'conf-screentest-host-os-debug'            = @{ LIGHT_PLATFORM = 'HOST'; LIGHT_SYSTEM = 'HOST_OS' }
                'conf-screentest-waveshare-touch169-debug' = @{ LIGHT_PLATFORM = 'TARGET'; LIGHT_BOARD = 'waveshare_rp2350_touch_lcd_1.69'; PICO_PLATFORM = 'rp2350-arm-s' }
                'conf-screentest-waveshare-touch169-riscv-debug' = @{ LIGHT_PLATFORM = 'TARGET'; PICO_PLATFORM = 'rp2350-riscv' }
                'conf-screentest-trace'                    = @{ LIGHT_PLATFORM = 'TARGET'; LIGHT_BOARD = 'pico'; LIGHT_RUN_MODE = 'TRACE' }
                'conf-screentest-release'                  = @{ LIGHT_PLATFORM = 'TARGET'; LIGHT_BOARD = 'pico'; LIGHT_RUN_MODE = 'PRODUCTION' }
                'conf-screentest-mini-stm32h7-debug'       = @{ LIGHT_SYSTEM = 'CMSIS'; LIGHT_BOARD = 'mini_stm32h7' }
        }

        #   which OpenOCD config and SVD belong to which board. Getting this pairing wrong is not
        # a clean failure -- attaching an rp2040 configuration to an rp2350 image misbehaves
        # rather than erroring, and this project's launch.json named the rp2040 SVD for every
        # configuration, including both RP2350 ones, until it was corrected alongside this
        Debug = @{
                'conf-screentest-debug' = @{
                        Config = 'openocd.cfg'
                        Svd    = '../pico-sdk/src/rp2040/hardware_regs/RP2040.svd'
                }
                'conf-screentest-pico2-debug' = @{
                        Config = 'openocd-rp2350.cfg'
                        Svd    = '../pico-sdk/src/rp2350/hardware_regs/RP2350.svd'
                }
                'conf-screentest-waveshare-touch169-debug' = @{
                        Config = 'openocd-rp2350.cfg'
                        Svd    = '../pico-sdk/src/rp2350/hardware_regs/RP2350.svd'
                }
                'conf-screentest-waveshare-touch169-riscv-debug' = @{
                        Config = 'openocd-rp2350.cfg'
                        Svd    = '../pico-sdk/src/rp2350/hardware_regs/RP2350.svd'
                }
                #   debugged over an ST-Link rather than CMSIS-DAP, and it is the only target
                # here with no UF2 path at all -- SWD is how an image reaches this board
                'conf-screentest-mini-stm32h7-debug' = @{
                        Config = 'openocd-stm32h7.cfg'
                }
        }

        DefaultTarget = 'light_ui_demo_touch169'

        #   build-host is the HOST_OS tree, and the only configuration that registers this
        # project's tests -- it is also what both mutants.ps1 harnesses default to. It now has a
        # preset (conf-screentest-host-os-debug) so CI and a fresh clone can create it; it used to
        # be hand-configured, which is why this pointed at conf-screentest-host-debug instead.
        #
        #   that was wrong in a way that looked fine: conf-screentest-host-debug is a PICO SDK
        # host build (pico_hostmode) and resolves to the shared ${sourceDir}/build tree, which
        # normally holds an rp2040 target configuration. So test.ps1 rebuilt firmware, ctest found
        # no tests, ctest exited 0, and the script reported "all checks passed" having tested
        # nothing. light-test.ps1 now passes --no-tests=error so that cannot recur silently.
        Test = @{
                Preset = 'conf-screentest-host-os-debug'
                Ctest  = $true
        }

        #   'auto' rather than a glob: this project's host test binaries sit at different
        # depths (light_audio/ and light_framework/test/), so discovery beats enumeration.
        #   HOST_OS explicitly, because the coverage build is a plain Linux build -- the pico_sdk
        # host mode the conf-screentest-host-debug preset uses is not available there. What that
        # measures is the portable module code (light_audio conversion, canvas), which is exactly
        # the part with host tests; the drivers and ports are target-only and will not appear at
        # all
        Coverage = @{
                Objects     = 'auto'
                IgnoreRegex = '(/lib/|/usr/|sanitizers/|_deps/|/freetype/|/jansson/)'
                CMakeArgs   = @('-DLIGHT_SYSTEM=HOST_OS', '-DLIGHT_PLATFORM=HOST')
        }
}
