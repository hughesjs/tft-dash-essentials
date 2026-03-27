const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Detect cross-compilation (skip test executables when targeting a different arch)
    const is_cross = target.result.cpu.arch != @import("builtin").cpu.arch;

    // SDL3 — built from source via Zig dependency (statically linked)
    const sdl = b.dependency("sdl", .{
        .target = target,
        .optimize = optimize,
        .preferred_linkage = .static,
    });
    const sdl_lib = sdl.artifact("SDL3");

    // --- Main dashboard executable ---
    const testsdl = b.addExecutable(.{
        .name = "testsdl",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    testsdl.root_module.addCSourceFiles(.{
        .files = &.{ "src/testsdl.c", "src/parser.c", "src/assets.c", "src/sensor_feed.c", "src/menu.c", "src/tpms_feed.c" },
        .flags = &.{"-std=c23"},
    });

    testsdl.root_module.addIncludePath(b.path("src"));
    testsdl.linkLibrary(sdl_lib);
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
        test_assets.linkLibrary(sdl_lib);

        b.installArtifact(test_assets);

        // --- Sensor feed tests ---
        const test_sensor_feed = b.addExecutable(.{
            .name = "test_sensor_feed",
            .root_module = b.createModule(.{
                .target = target,
                .optimize = optimize,
                .link_libc = true,
            }),
        });

        test_sensor_feed.root_module.addCSourceFiles(.{
            .files = &.{
                "src/test_sensor_feed.c",
                "src/sensor_feed.c",
                "src/parser.c",
            },
            .flags = &.{
                "-std=c23",
            },
        });

        test_sensor_feed.root_module.addIncludePath(b.path("src"));
        test_sensor_feed.root_module.linkSystemLibrary("pthread", .{});
        test_sensor_feed.root_module.linkSystemLibrary("m", .{});

        b.installArtifact(test_sensor_feed);

        // --- Menu tests ---
        const test_menu = b.addExecutable(.{
            .name = "test_menu",
            .root_module = b.createModule(.{
                .target = target,
                .optimize = optimize,
                .link_libc = true,
            }),
        });

        test_menu.root_module.addCSourceFiles(.{
            .files = &.{
                "src/test_menu.c",
                "src/menu.c",
            },
            .flags = &.{
                "-std=c23",
            },
        });

        test_menu.root_module.addIncludePath(b.path("src"));

        b.installArtifact(test_menu);

        // --- TPMS feed tests ---
        const test_tpms_feed = b.addExecutable(.{
            .name = "test_tpms_feed",
            .root_module = b.createModule(.{
                .target = target,
                .optimize = optimize,
                .link_libc = true,
            }),
        });

        test_tpms_feed.root_module.addCSourceFiles(.{
            .files = &.{
                "src/test_tpms_feed.c",
                "src/tpms_feed.c",
            },
            .flags = &.{
                "-std=c23",
            },
        });

        test_tpms_feed.root_module.addIncludePath(b.path("src"));
        test_tpms_feed.root_module.linkSystemLibrary("pthread", .{});
        test_tpms_feed.root_module.linkSystemLibrary("m", .{});

        b.installArtifact(test_tpms_feed);

        // 'zig build test' step - builds and runs all tests
        const run_parser_tests = b.addRunArtifact(test_parser);
        const run_asset_tests = b.addRunArtifact(test_assets);
        run_asset_tests.setCwd(b.path("."));
        const run_sensor_feed_tests = b.addRunArtifact(test_sensor_feed);
        const run_menu_tests = b.addRunArtifact(test_menu);
        const run_tpms_feed_tests = b.addRunArtifact(test_tpms_feed);
        const test_step = b.step("test", "Run all tests");
        test_step.dependOn(&run_parser_tests.step);
        test_step.dependOn(&run_asset_tests.step);
        test_step.dependOn(&run_sensor_feed_tests.step);
        test_step.dependOn(&run_menu_tests.step);
        test_step.dependOn(&run_tpms_feed_tests.step);

        // 'zig build run' step - builds and runs the dashboard
        const run_dash = b.addRunArtifact(testsdl);
        run_dash.setCwd(b.path("zig-out/bin"));
        if (b.args) |args| {
            run_dash.addArgs(args);
        }
        const run_step = b.step("run", "Run the dashboard");
        run_step.dependOn(&run_dash.step);
    }
}
