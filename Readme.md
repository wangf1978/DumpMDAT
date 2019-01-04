# What is DumpMDAT?
It is a tool which can dump raw AVC/AAC data from MDAT directly, and export them to .h264/.adts files.
In some cases, it can be used to fix the unfinalized or damaged MP4 files.

# How to build?
- Windows<br>
    1. VS2015/vc14<br>
        Open build/DumpMDAT.sln and build it
- Linux<br>
    *Make sure gcc with modern C++11/14 support is installed*<br>
    Here are steps to build and install this application
    ```
    git clone https://github.com/wangf1978/DumpMDAT.git
    cd DumpMDAT
    make
    ```
***(\*) both x64 and x86 are supported***

# How to run it?
*Usage: DumpMDAT.exe MP4Filename h264_filename \[aac_filename\]...*

When running it, please put the MP4Filename.sps, MP4Filename.pps, MP4Filename.adts files under the same folder within MP4Filename.mp4.
