const std = @import("std");

export fn setup_debug_handlers() void {
    std.debug.maybeEnableSegfaultHandler();
}

export fn dump_stack_trace() void {
    std.debug.dumpCurrentStackTrace(@returnAddress());
}

/// Communicate to zig that main() will be provided elsewhere.
pub const _start = {};
