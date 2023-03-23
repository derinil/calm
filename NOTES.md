# h264 reading
- https://shopdelta.eu/h-264-image-coding-standard_l2_aid734.html
- https://stackoverflow.com/questions/29525000/how-to-use-videotoolbox-to-decompress-h-264-video-stream
- https://developer.apple.com/documentation/videotoolbox/kvtcompressionpropertykey_h264entropymode?language=objc
- https://developer.apple.com/forums/thread/672710
- https://stackoverflow.com/questions/28396622/extracting-h264-from-cmblockbuffer
- https://www.ti.com/lit/an/sprab88/sprab88.pdf
- https://stackoverflow.com/questions/39784291/how-to-encode-audio-along-with-video-to-h264-format-using-videotoolbox
- https://stackoverflow.com/questions/28396622/extracting-h264-from-cmblockbuffer
- https://gist.github.com/wotjd/a6ed9ace5d72cd53ed62ddafd340735a
- https://yumichan.net/video-processing/video-compression/introduction-to-h264-nal-unit/
- https://www.cardinalpeak.com/blog/the-h-264-sequence-parameter-set
- https://doc-kurento.readthedocs.io/en/latest/knowledge/h264.html
- https://stackoverflow.com/questions/25738680/how-to-parse-access-unit-in-h-264
- https://devstreaming-cdn.apple.com/videos/wwdc/2014/513xxhfudagscto/513/513_direct_access_to_media_encoding_and_decoding.pdf
- https://stackoverflow.com/questions/47217998/h264-encoding-and-decoding-using-videotoolbox

# colors
- http://paulbourke.net/dataformats/yuv/
- https://en.wikipedia.org/wiki/YUV
- https://developer.apple.com/library/archive/technotes/tn2162/_index.html

# competition
- https://parsec.app/blog/a-networking-protocol-built-for-the-lowest-latency-interactive-game-streaming-1fd5a03a6007

# networking
- lan peer discovery https://gamedev.stackexchange.com/questions/30761/solution-for-lightweight-lan-peer-discovering
- use https://github.com/networkprotocol/yojimbo or https://github.com/ValveSoftware/GameNetworkingSockets - list https://stackoverflow.com/questions/118945/best-c-c-network-library/118968#118968
- https://github.com/ValveSoftware/GameNetworkingSockets
- https://isetta.io/compendium/Networking/
- https://www.reddit.com/r/gamedev/comments/93kr9h/recommended_c_udp_networking_library/
- https://github.com/libuv/libuv
- https://github.com/SLikeSoft/SLikeNet

# commands
- to convert the h264 to playable mp4 `ffmpeg -i dump.h264 -c copy test.mp4`, can do it without -codec copy
    - https://superuser.com/questions/710008/how-to-get-rid-of-ffmpeg-pts-has-no-value-error
    - `ffmpeg -i dump.h264 -r 60 -f rawvideo -pix_fmt bgra -s 1920x1080 test.mp4`
- inspect streams `ffprobe -show_streams -i "test.mp4"`
- play via mpv `mpv dump.h264`

# todo
- https://stackoverflow.com/questions/53640949/vtcompressionsession-bitrate-datarate-overshooting
- colors are faded, mp4 is very jittery
- https://fcp.co/forum/4-final-cut-pro-x-fcpx/32772-help-exporting-h264-makes-video-colors-washed-out
[mp4 @ 0x111e09b40] Timestamps are unset in a packet for stream 0. This is deprecated and will stop working in the future. Fix your code to set the timestamps properly
[mp4 @ 0x111e09b40] pts has no value
- converting the h264 to mp4 messes up with the colors, use yuv for compression
- building takes way too long, figure out why
