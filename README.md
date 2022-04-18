## 功能说明：

- 这个bootloader的主要功能是按照下载的升级包类型升级应用程序，然后跳转到应用程序区运行。
- 支持非压缩的固件。
- 支持lzma压缩后的固件，解压大概需要30K的SRAM空间，如果需要其他压缩方式，可自行改造。
- 支持差分升级包，极大减少升级包体积。
- 所有升级包均附加64字节包头，用来保存升级包名称，长度，校验等信息，类似uImage头。
- 目前支持STM32F103VE和VG芯片。

## FLASH分区说明：

- 目前所有数据均使用芯片内部Flash存储，因此将Flash分为4个分区。
    - 分区1为bootloader分区，固定32K - 64字节，这最后64字节用来保存应用程序镜像头。
    - 分区2为应用程序区，镜像起始地址为0x08007FC0，程序入口地址为0x08008000。
    - 分区3用来保存下载的升级包，差分升级时还用来保存解码出来的新固件。
    - 分区4位于Flash的最后4K，用来保存一些升级信息。
- 当前bootloader支持2种分区参数，一种512K，一种1024K，具体可参见flash.h文件。
- 如果需要将数据保存在外部Flash，可自行改造。

## 升级流程说明：

1. 首先需要使用到2个工具，分别是mkuzimage和make_udiff，前者用来生成非压缩固件或者lzma压缩固件，后者用来生成差分升级包，3种任选1。

    - 非压缩固件
    `mkuzimage -i image.bin -o uimage.bin`
    
    - LZMA方式压缩固件
    `mkuzimage -C lzma -i image.bin -o uzimage.bin`
    
    - 差分固件
    `make_udiff -o old.bin -n new.bin -p patch.bin`
2. 然后将升级包（uimage.bin，uzimage.bin，patch.bin，3种的任意1种即可）通过某种通信方式（串口，网口，4G等）下载到分区3，这部分需要在应用中实现。也可以在bootloader命令行下使用ymodem命令，具体下述。
3. 最后更新位于分区4的升级标志（分区4偏移地址0，值为0xAA55），这一步建议在应用程序中完成。也可在bootloader命令行下，输入`iap-start`命令，更新这个标志。
4. 复位，bootloader会自动解析升级包类型，然后升级。
## 使用ymodem下载固件及补丁包（超级终端使用SecureCRT）
1. 下载应用程序到分区2
    - 分区2固件由bootloader直接引导，所以必须使用mkuzimage生成非压缩固件，然后下载，在超级终端输入命令：
    `#ymodem app `
    - 超级终端窗口收到`CCC`时，用YMODEM发送文件。
- 下载成功后，设备会自动重启，并引导到应用程序运行。
    
2. 下载升级包到分区3
    - 升级包支持上文提到的3种类型，下载时可任选一种，在超级终端输入命令：
    `#ymodem patch `
    - 超级终端窗口收到`CCC`时，用YMODEM发送文件。
    - 下载成功后，设备会自动重启，并自动升级到新版本程序。

## 其他：
这个项目使用了很多源于网络的开源代码，例如bsdiff，lzma，atomlib等，在此表示感谢。

