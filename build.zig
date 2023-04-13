const std = @import("std");
const time = std.time;
const log = std.log;
const enet = @import("deps/enet/build.zig");
const glfw = @import("deps/glfw/build.zig");
const cimgui = @import("deps/cimgui/build.zig");
const libuv = @import("deps/libuv/build.zig");

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
    "src/decode/mac.m",
};

var sourceFolders = [_][]const u8{
    basePath("/src"),
    basePath("/src/gui"),
    basePath("/src/net"),
    basePath("/src/data"),
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

    // updateSubmodules(b.allocator);

    exe.linkLibC();
    exe.linkLibCpp();

    glfw.link(b, exe, .{}) catch unreachable;
    exe.addIncludePath("deps/glfw/upstream/glfw/include/");
    exe.addIncludePath("deps/glad/include/");

    cimgui.link(b, exe);
    exe.addIncludePath("deps/cimgui/");
    exe.addIncludePath("deps/cimgui/generator/output/");

    enet.link(b, exe);

    _ = libuv.link(b, exe) catch unreachable;

    if (target.getOsTag().isDarwin()) {
        const flags = &[_][]const u8{
            "-Wall",
            "-Werror",
            "-Wextra",
            // "-static",
            // TODO: add static
            // TODO: Remove these
            "-Wno-unused-variable",
            "-Wno-unused-parameter",
            "-Wno-unused-but-set-variable",
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

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
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
