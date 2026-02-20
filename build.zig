const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Detect cross-compilation (skip test executables when targeting a different arch)
    const is_cross = target.result.cpu.arch != @import("builtin").cpu.arch;

    // Cross-compilation support: explicit SDL2 paths (avoids pkg-config sysroot conflicts)
    const sdl2_include_path = b.option([]const u8, "sdl2-include-path", "Path to SDL2 headers (for cross-compilation)");
    const sdl2_lib_path = b.option([]const u8, "sdl2-lib-path", "Path to SDL2 libraries (for cross-compilation)");

    // --- Main dashboard executable ---
    const testsdl = b.addExecutable(.{
        .name = "testsdl",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
        }),
    });

    testsdl.root_module.addCSourceFiles(.{
        .files = &.{"src/testsdl.cpp"},
        .flags = &.{"-std=c++14"},
    });

    testsdl.root_module.addCSourceFiles(.{
        .files = &.{ "src/parser.c", "src/assets.c" },
        .flags = &.{"-std=c23"},
    });

    testsdl.root_module.addIncludePath(b.path("src"));

    // SDL2: use explicit paths when provided (cross-compilation), else pkg-config
    if (sdl2_include_path) |p| {
        testsdl.root_module.addSystemIncludePath(.{ .cwd_relative = p });
    }
    if (sdl2_lib_path) |p| {
        testsdl.root_module.addLibraryPath(.{ .cwd_relative = p });
    }
    testsdl.root_module.linkSystemLibrary("SDL2", .{});
    testsdl.root_module.linkSystemLibrary("pthread", .{});

    b.installArtifact(testsdl);

    // Copy assets alongside the binary
    b.installDirectory(.{
        .source_dir = b.path("assets/themes"),
        .install_dir = .bin,
        .install_subdir = "assets/themes",
    });

    // --- Test executables and run steps (native builds only) ---
    if (!is_cross) {
        // --- Parser tests ---
        const test_parser = b.addExecutable(.{
            .name = "test_parser",
            .root_module = b.createModule(.{
                .target = target,
                .optimize = optimize,
                .link_libc = true,
            }),
        });

        test_parser.root_module.addCSourceFiles(.{
            .files = &.{
                "src/test_parser.c",
                "src/parser.c",
            },
            .flags = &.{
                "-std=c23",
            },
        });

        test_parser.root_module.addIncludePath(b.path("src"));
        test_parser.root_module.linkSystemLibrary("m", .{});

        b.installArtifact(test_parser);

        // --- Asset manager tests ---
        const test_assets = b.addExecutable(.{
            .name = "test_assets",
            .root_module = b.createModule(.{
                .target = target,
                .optimize = optimize,
                .link_libc = true,
            }),
        });

        test_assets.root_module.addCSourceFiles(.{
            .files = &.{
                "src/test_assets.c",
                "src/assets.c",
            },
            .flags = &.{
                "-std=c23",
            },
        });

        test_assets.root_module.addIncludePath(b.path("src"));
        test_assets.root_module.linkSystemLibrary("SDL2", .{});

        b.installArtifact(test_assets);

        // 'zig build test' step - builds and runs all tests
        const run_parser_tests = b.addRunArtifact(test_parser);
        const run_asset_tests = b.addRunArtifact(test_assets);
        run_asset_tests.setCwd(b.path("."));
        const test_step = b.step("test", "Run all tests");
        test_step.dependOn(&run_parser_tests.step);
        test_step.dependOn(&run_asset_tests.step);

        // 'zig build run' step - builds and runs the dashboard
        const run_dash = b.addRunArtifact(testsdl);
        run_dash.setCwd(b.path("zig-out/bin"));
        if (b.args) |args| {
            run_dash.addArgs(args);
        }
        const run_step = b.step("run", "Run the dashboard");
        run_step.dependOn(&run_dash.step);
    }

    // 'zig build image' step - cross-compile for Pi then build SD card image
    //
    // This runs the buildroot/build.sh script which:
    //   1. Cross-compiles testsdl for ARM (via zig)
    //   2. Runs Buildroot to produce sdcard.img
    const image_cmd = b.addSystemCommand(&.{
        "/bin/bash",
        b.path("../buildroot/build.sh").getPath(b),
    });
    const image_step = b.step("image", "Cross-compile for Pi and build SD card image via Buildroot");
    image_step.dependOn(&image_cmd.step);
}
