zig build
codesign -s "calm_codesign" ./zig-out/bin/calm
lldb -o run ./zig-out/bin/calm
