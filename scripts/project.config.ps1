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
                'conf-screentest-host-debug'                    = 'build'
                'conf-screentest-trace'                         = 'build'
                'conf-screentest-release'                       = 'build'
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
                'screentest_po13'          = @{ Preset = 'conf-screentest-debug'; Flash = 'uf2' }
                'light_ui_demo_po13'       = @{ Preset = 'conf-screentest-debug'; Flash = 'uf2' }
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
                'conf-screentest-host-debug'               = @{ LIGHT_PLATFORM = 'HOST'; LIGHT_BOARD = 'pico_hostmode' }
                'conf-screentest-waveshare-touch169-debug' = @{ LIGHT_PLATFORM = 'TARGET'; LIGHT_BOARD = 'waveshare_rp2350_touch_lcd_1.69'; PICO_PLATFORM = 'rp2350-arm-s' }
                'conf-screentest-waveshare-touch169-riscv-debug' = @{ LIGHT_PLATFORM = 'TARGET'; PICO_PLATFORM = 'rp2350-riscv' }
                'conf-screentest-mini-stm32h7-debug'       = @{ LIGHT_SYSTEM = 'CMSIS'; LIGHT_BOARD = 'mini_stm32h7' }
        }

        DefaultTarget = 'light_ui_demo_touch169'

        #   build-host is a HOST_OS tree with no preset behind it -- hand-configured, and the one
        # both mutants.ps1 harnesses default to. Declared here so light-test.ps1 can find it;
        # note that no screen-test preset produces it, so it must already exist or be created by
        # hand until a preset is added.
        Test = @{
                Preset = 'conf-screentest-host-debug'
                Ctest  = $true
        }
}
