zig build
codesign -s "calm_codesign" ./zig-out/bin/calm
lldb ./zig-out/bin/calm