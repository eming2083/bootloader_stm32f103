## 功能说明：

- 这个bootloader的主要功能是按照下载的升级包类型升级应用程序，然后跳转到应用程序区运行.
- 支持非压缩的固件.
- 支持lzma压缩后的固件，解压大概需要30K的SRAM空间，如果需要其他压缩方式，可自行改造.
- 支持差分升级包，极大减少升级包体积.
- 所有升级包均附加64字节包头，用来保存升级包名称，长度，校验等信息，类似uImage头.
- 目前支持STM32103VE和VG芯片.

## FLASH分区说明：

- 目前所有数据均使用芯片内部Flash存储，因此将Flash分为4个分区.
    - 分区1为bootloader分区，固定32K - 64字节，这最后64字节用来保存应用程序镜像头.
    - 分区2为应用程序区，镜像起始地址为0x08007FC0，程序入口地址为0x08008000.
    - 分区3用来保存下载的升级包，差分升级时还用来保存解码出来的新固件.
    - 分区4位于Flash的最后4K，用来保存一些升级信息.
- 当前bootloader支持2种分区参数，一种512K，一种1024K，具体可参见flash.h文件.
- 如果需要将数据保存在外部Flash，可自行改造.

## 升级流程说明：

1. 首先需要使用到2个工具，分别是mkuzimage和make_udiff，前者用来生成非压缩固件或者lzma压缩固件，后者用来生成差分升级包，3种任选1.

    - 非压缩固件
    `mkuzimage -i image.bin -o uimage.bin`
    
    - LZMA方式压缩固件
    `mkuzimage -C lzma -i image.bin -o uzimage.bin`
    
    - 差分固件
    `make_udiff -o old.bin -n new.bin -p udiff.bin`
2. 然后将升级包通过某种通信方式下载到分区3，串口，网口，4G通信等等，这部分需要在应用中实现.
3. 复位进入bootloader，在命令窗口提示的1S内按下回车中断程序跳转，输入iap_test命令更新位于分区4的升级信息，这一步也可在应用中自动完成，实现自动升级，本质就是写一个标志.
## 使用ymodem下载固件及补丁包
1. 下载应用程序到分区2
    - 这个固件由bootloader直接引导，所以必须使用mkuzimage生成非压缩固件，然后下载，在超级终端输入命令：
    `#ymodem app `
    - 下载成功后，设备会自动重启，并引导到应用程序运行.

2. 下载升级包到分区3
    - 升级包支持上文提到的3种类型，下载时可任选一种，在超级终端输入命令：
    `#ymodem patch `
    - 下载成功后，设备会自动重启，并自动升级到新版本程序.

## 其他：
这个项目使用了很多源于网络的开源代码，例如bsdiff，lzma，atomlib等，在此表示感谢.

