userpointer方式输入和输出

cmem的版本来自这里编译出来的
/home/andy/RidgeRun-SDK-DM36x-Turrialba-Eval/proprietary/dvsdk-4_02_00_06/dvsdk/linuxutils_2_26_01_02/packages/ti/sdo/linuxutils/cmem


即rootfs_ridgerun/lib/modules/2.6.32-17-ridgerun
复制到ti_root/lib/modules，因为kernel是ridgerun版本

启动参数
setenv bootargs console=ttyS0,115200n8 rw dm365_imp.oper_mode=0 video=davincifb:vid0=720x480x16,2025K@0,0:vid1=720x480x16,2025K@0,0:osd0=720x480x16,1350K@0,0:osd1=720x480x4,1350K@0,0 mem=60M vpfe_capture.interface=1 davinci_enc_mngr.ch0_output=PRGB davinci_enc_mngr.ch0_mode=PRGB root=/dev/nfs nfsroot=192.168.2.52:/home/andy/targetfs/ti_root ip=192.168.2.100 dm365_generic_prgb_encoder.mode=1024x768MR-16@60

setenv ipaddr 192.168.2.100
setenv serverip 192.168.2.52
setenv bootfile uImage
tftpboot

进入系统后
fbset -fb /dev/fb2 -xres 0
fbset -fb /dev/fb1 -xres 0
fbset -fb /dev/fb0 -xres 0
fbset -fb /dev/fb3 -xres 0

卸载自动加载的cmemk
rmmod cmemk

这样加载cmemk
modprobe cmemk phys_start=0x84800000 phys_end=0x88000000 pools=4x294912,4x1572864
insmod cmemk.ko phys_start=0x84800000 phys_end=0x88000000 pools=8x1572864
modprobe cmemk phys_start=0x84800000 phys_end=0x88000000 pools=8x1572864