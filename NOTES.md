# capture

- https://developer.nvidia.com/capture-sdk

# h264 encoding

- https://shopdelta.eu/h-264-image-coding-standard_l2_aid734.html
- https://stackoverflow.com/questions/29525000/how-to-use-videotoolbox-to-decompress-h-264-video-stream
- https://developer.apple.com/documentation/videotoolbox/kvtcompressionpropertykey_h264entropymode?language=objc
- https://developer.apple.com/forums/thread/672710
- https://stackoverflow.com/questions/28396622/extracting-h264-from-cmblockbuffer
- https://www.ti.com/lit/an/sprab88/sprab88.pdf
- https://stackoverflow.com/questions/39784291/how-to-encode-audio-along-with-video-to-h264-format-using-videotoolbox
- https://gist.github.com/wotjd/a6ed9ace5d72cd53ed62ddafd340735a
- https://yumichan.net/video-processing/video-compression/introduction-to-h264-nal-unit/
- https://www.cardinalpeak.com/blog/the-h-264-sequence-parameter-set
- https://doc-kurento.readthedocs.io/en/latest/knowledge/h264.html
- https://stackoverflow.com/questions/25738680/how-to-parse-access-unit-in-h-264
- https://devstreaming-cdn.apple.com/videos/wwdc/2014/513xxhfudagscto/513/513_direct_access_to_media_encoding_and_decoding.pdf
- https://stackoverflow.com/questions/47217998/h264-encoding-and-decoding-using-videotoolbox
- https://medium.com/@pinkjuice66/ios-realtime-video-streaming-app-tutorial-part1-d98bda51ca32
- https://github.com/NVIDIA/video-sdk-samples/tree/master/nvEncDXGIOutputDuplicationSample
- https://stackoverflow.com/questions/24884827/possible-locations-for-sequence-picture-parameter-sets-for-h-264-stream

# h264 decoding

- https://github.com/NewChromantics/PopH264
- https://stackoverflow.com/questions/24039345/decoding-h264-videotoolkit-api-fails-with-error-8971-in-vtdecompressionsessionc
- https://forums.developer.apple.com/thread/11637
- https://searchfox.org/mozilla-central/source/dom/media/platforms/apple/AppleVTDecoder.cpp
- http://aviadr1.blogspot.com/2010/05/h264-extradata-partially-explained-for.html
- https://developer.apple.com/documentation/avfoundation/avsamplebufferdisplaylayer?language=objc
- https://stackoverflow.com/questions/26601776/error-code-8969-12909-while-decoding-h264-in-ios-8-with-video-tool-box
- https://github.com/htaiwan/HWDecoder
- https://stackoverflow.com/questions/37091132/ios-yuv-420v-using-gl-texture-2d-shows-wrong-colour-in-opengl-shader
- https://stackoverflow.com/questions/25078364/cmvideoformatdescriptioncreatefromh264parametersets-issues
## vulkan decoding
- https://www.khronos.org/blog/an-introduction-to-vulkan-video
- https://www.khronos.org/blog/khronos-finalizes-vulkan-video-extensions-for-accelerated-h.264-and-h.265-decode
- https://github.com/nvpro-samples/vk_video_samples
- https://www.vulkan.gpuinfo.org/listdevices.php?platform=windows
- https://wickedengine.net/2023/05/07/vulkan-video-decoding/


# nice read

- https://stackoverflow.com/questions/69239660/what-are-conflict-misses-exactly
- https://www.internalpointers.com/post/lock-free-multithreading-atomic-operations

# colors

- http://paulbourke.net/dataformats/yuv/
- https://en.wikipedia.org/wiki/YUV
- https://developer.apple.com/library/archive/technotes/tn2162/_index.html
- https://stackoverflow.com/questions/30191911/is-it-possible-to-draw-yuv422-and-yuv420-texture-using-opengl

# displaying
- https://stackoverflow.com/questions/27513201/how-to-properly-texture-a-quad-in-opengl

# networking

- lan peer discovery https://gamedev.stackexchange.com/questions/30761/solution-for-lightweight-lan-peer-discovering
- use https://github.com/networkprotocol/yojimbo or https://github.com/ValveSoftware/GameNetworkingSockets -> list https://stackoverflow.com/questions/118945/best-c-c-network-library/118968#118968
- https://github.com/ValveSoftware/GameNetworkingSockets
- https://isetta.io/compendium/Networking/
- https://www.reddit.com/r/gamedev/comments/93kr9h/recommended_c_udp_networking_library/
- https://github.com/libuv/libuv
- https://github.com/SLikeSoft/SLikeNet
- enet example https://github.com/cxong/ENetLANChatServer
- https://github.com/Haivision/srt
- https://parsec.app/blog/a-networking-protocol-built-for-the-lowest-latency-interactive-game-streaming-1fd5a03a6007

## libuv notes

- https://groups.google.com/g/libuv/c/fRNQV_QGgaA

# useful misc

- https://www.osstatus.com/

# prior art

- https://github.com/MirrorX-Desktop/MirrorX
- https://github.com/rustdesk/rustdesk
- https://parsec.app

# todo

- cframe stack freer is broken, probably dframe too (is this still the case?)
- link vulkan sdk

# vscode extensions

- clangd
- zls

# hardware support

- https://gpuopen.com/advanced-media-framework/
- https://github.com/intel/media-driver
- https://developer.nvidia.com/video-codec-sdk

# might be an issue

- https://stackoverflow.com/questions/53640949/vtcompressionsession-bitrate-datarate-overshooting
- switch to libxev later

# debugging

- lldb on macos
```
lldb ./zig-out/bin/calm
process launch
kill
gui
step // one line of code
next // one function
breakpoint set --file src/decode/mac.m --line 188
breakpoint delete 1
```

# codesign to debug properly (sip pisses me off)
- follow https://opensource.apple.com/source/lldb/lldb-69/docs/code-signing.txt to create a certificate, no need for the snow leopard stuff
- remove screen recording privileges of calm if already allowed
- codesign via `codesign -s "calm_codesign" ./zig-out/bin/calm`
- you can now use lldb :D

# future todo

- encryption
- clipboard transfer
- much, much fewer allocations

# commands
```
git submodule sync --recursive
git submodule update --recursive --remote
```
