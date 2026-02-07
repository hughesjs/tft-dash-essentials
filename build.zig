const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

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
        .files = &.{"src/parser.c"},
        .flags = &.{"-std=c99"},
    });

    testsdl.root_module.addIncludePath(b.path("src"));
    testsdl.root_module.linkSystemLibrary("SDL2", .{});
    testsdl.root_module.linkSystemLibrary("pthread", .{});

    b.installArtifact(testsdl);

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
            "-std=c99",
        },
    });

    test_parser.root_module.addIncludePath(b.path("src"));
    test_parser.root_module.linkSystemLibrary("m", .{});

    b.installArtifact(test_parser);

    // 'zig build test' step - builds and runs parser tests
    const run_tests = b.addRunArtifact(test_parser);
    const test_step = b.step("test", "Run parser tests");
    test_step.dependOn(&run_tests.step);

    // 'zig build run' step - builds and runs the dashboard
    const run_dash = b.addRunArtifact(testsdl);
    if (b.args) |args| {
        run_dash.addArgs(args);
    }
    const run_step = b.step("run", "Run the dashboard");
    run_step.dependOn(&run_dash.step);
}
