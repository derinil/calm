const std = @import("std");

const glfw = @import("deps/glfw/build.zig");
const cimgui = @import("deps/cimgui/build.zig");
const yojimbo = @import("deps/yojimbo/build.zig");

var macFrameworks = [_][]const u8{
    "Foundation",
    "CoreGraphics",
    "CoreVideo",
    "VideoToolbox",
    "CoreServices",
    "AVFoundation",
    "CoreMedia",
    "CoreServices",
    "IOSurface",
    "OpenGL",
    "IOKit",
};

var macFiles = [_][]const u8{
    "src/capture/mac.m",
};

var sourceFolders = [_][]const u8{
    basePath("/src"),
    basePath("/src/gui"),
    basePath("/deps/glad/src"),
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "calm",
        .root_source_file = .{ .path = "src/stack_tracer.zig" },
        .target = target,
        .optimize = optimize,
    });

    updateSubmodules(b.allocator);

    exe.linkLibC();
    exe.linkLibCpp();

    glfw.link(b, exe, .{}) catch unreachable;
    exe.addIncludePath("deps/glfw/upstream/glfw/include/");
    exe.addIncludePath("deps/glad/include/");

    cimgui.link(b, exe);
    exe.addIncludePath("deps/cimgui/");
    exe.addIncludePath("deps/cimgui/generator/output/");

    // buildGameNetworkingSockets(b.allocator) catch unreachable;
    yojimbo.link(b, exe);
    exe.addIncludePath("deps/yojimbo/");

    if (target.getOsTag().isDarwin()) {
        const flags = &[_][]const u8{
            "-Wall",
            "-Werror",
            "-Wextra",
            // TODO: Remove these
            "-Wno-unused-variable",
            "-Wno-unused-parameter",
        };

        for (macFrameworks) |f| {
            exe.linkFramework(f);
        }

        for (macFiles) |f| {
            exe.addCSourceFile(f, flags);
        }
    } else unreachable;

    for (sourceFolders) |f| {
        var sf = getSourcesInDir(b.allocator, f) catch unreachable;

        for (sf) |s| {
            exe.addCSourceFile(s, &.{});
        }
    }

    exe.install();

    const run_cmd = exe.run();
    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    const exe_tests = b.addTest(.{
        .root_source_file = .{ .path = "src/main.zig" },
        .target = target,
        .optimize = optimize,
    });

    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&exe_tests.step);
}

fn getSourcesInDir(allocator: std.mem.Allocator, dir_path: []const u8) ![][]const u8 {
    var files = std.ArrayList([]const u8).init(allocator);

    var dir: std.fs.IterableDir = try std.fs.openIterableDirAbsolute(dir_path, .{});
    defer dir.close();
    var iter = dir.iterate();

    while (try iter.next()) |e| {
        if (e.kind != .File) continue;
        if (!std.mem.endsWith(u8, e.name, ".cpp") and !std.mem.endsWith(u8, e.name, ".c") and !std.mem.endsWith(u8, e.name, ".h")) continue;
        var fp = dir.dir.realpathAlloc(allocator, e.name) catch unreachable;
        try files.append(fp);
    }

    return files.items;
}

fn basePath(comptime suffix: []const u8) []const u8 {
    if (suffix[0] != '/') @compileError("suffix must be an absolute path");
    return comptime blk: {
        const root_dir = std.fs.path.dirname(@src().file) orelse ".";
        break :blk root_dir ++ suffix;
    };
}

fn updateSubmodules(allocator: std.mem.Allocator) void {
    var gsu = std.ChildProcess.init(&[_][]const u8{
        "git", "submodule", "update", "--init", "--recursive",
    }, allocator);
    gsu.spawn() catch unreachable;
    _ = gsu.wait() catch unreachable;
}

const GNSBuildError = error{GNSBuildError};
// lazy way of building GameNetworkingSockets
fn buildGameNetworkingSockets(allocator: std.mem.Allocator) !void {
    var er: std.ChildProcess.ExecResult = try std.ChildProcess.exec(.{
        .allocator = allocator,
        .cwd = "deps/GameNetworkingSockets",
        .argv = &.{ "mkdir", "build" },
    });
    std.log.info("gns build: {s} {s}", .{ er.stdout, er.stderr });

    er = try std.ChildProcess.exec(.{
        .allocator = allocator,
        .cwd = "deps/GameNetworkingSockets/build",
        .argv = &.{
            "cmake",
            "-DBUILD_STATIC_LIB=on",
            "-DBUILD_SHARED_LIB=off",
            "-DProtobuf_USE_STATIC_LIBS=on",
            "-DSTEAMNETWORKINGSOCKETS_STATIC_LINK",
            "-DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3/",
            "-DProtobuf_INCLUDE_DIR=/opt/homebrew/Cellar/protobuf/21.12/include",
            "-DProtobuf_LIBRARIES=/opt/homebrew/Cellar/protobuf/21.12/lib",
            "..",
        },
    });
    std.log.info("gns build: {s} {s}", .{ er.stdout, er.stderr });
    if (er.stderr.len > 0)
        return GNSBuildError.GNSBuildError;
}
