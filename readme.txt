![pic](https://github.com/ak5k/ndi-audio-io/blob/165bea6fcb464acc7eee2d1c7a7dff254cba4dd9/pic.png)

# NDI Audio IO

NDI Audio IO software can stream audio between compatible audio devices and/or
audio plugin host software using NewTek NDI technology.

Channel count per source and total amount of sources depend on overall system
performance. Modern and relatively powerful PC can handle multiple multichannel
sources operating in standalone mode or under audio plugin host. Theoretical
channel count limit is 256 channels per one NDI Audio IO instance.

Each NDI Audio IO instance can send and/or receive audio. Instance audio
channels are synced. NDI technology dynamically adjusts sync between multiple
instances and over time can achieve sample accuracy depending on overall system
and network performance.

By default NDI Audio IO sends audio channels depending on audio device or track
channel configuration. E.g. NDI Audio IO connected to stereo audio device or
audio plugin host track will send 2 audio channels from device/plugin inputs.

Additional send groups can be assigned in CSV text input format: `MySender;
Group 1,group2,etc3`. NOTE: characters ; and , are interpreted as parameter
separators. To include such characters in literal names use double quotes.

By default NDI Audio IO receives audio channels depending on audio device or
track channel configuration. E.g. NDI Audio IO connected to stereo audio device
or audio plugin host track will receive first 2 audio channels from NDI source.

Additional input channel configuration can be assigned in CSV text input format:
`NDIMACHINE (NDISOURCE); 1,3,13-16` will receive channels 1 and 3 and channels
from 13 to 16 from source, and assign then to local output channels 1,2,3,4,5,6.
Channel zero/0 can be used to skip local output; 1,0,4 would assign source
channel 1 to local output 1, then skip local output 2, and assign source channel
4 to local output 3.

ASIO support can be included simply by building from source. No extra configuration
required. Build like any other JUCE framework CMake project.