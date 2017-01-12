userpointer方式输入和输出

cmem的版本来自这里编译出来的
/home/andy/RidgeRun-SDK-DM36x-Turrialba-Eval/proprietary/dvsdk-4_02_00_06/dvsdk/linuxutils_2_26_01_02/packages/ti/sdo/linuxutils/cmem


即rootfs_ridgerun/lib/modules/2.6.32-17-ridgerun
复制到ti_root/lib/modules，因为kernel是ridgerun版本


启动参数
setenv bootargs davinci_enc_mngr.ch0_output=PRGB davinci_enc_mngr.ch0_mode=PRGB  davinci_display.cont2_bufsize=6291456 vpfe_capture.cont_bufoffset=6291456 vpfe_capture.cont_bufsize=6291456 video=davincifb:osd1=1024x768x16,1800K@0,0:osd0=1024x768x16,1800K@0,0:vid0=1024x768x16,2025K@0,0:vid1=1024x768x16,2025K@0,0 console=ttyS0,115200n8  dm365_imp.oper_mode=0  vpfe_capture.interface=1  mem=61M root=/dev/nfs rw ip=192.168.2.100 nfsroot=192.168.2.17:/home/andy/targetfs/ti_rootfs dm365_generic_prgb_encoder.mode=1024x768MR-16@60

true

setenv ipaddr 192.168.2.100

true

setenv serverip 192.168.2.17
true

setenv bootfile uImage
true

tftpboot



关闭osd
fbset -fb /dev/fb2 -xres 0
fbset -fb /dev/fb1 -xres 0
fbset -fb /dev/fb0 -xres 0
fbset -fb /dev/fb3 -xres 0

卸载自动加载的cmemk
rmmod cmemk

这样加载cmemk
modprobe cmemk phys_start=0x84800000 phys_end=0x88000000 pools=8x4172864

参考
modprobe cmemk phys_start=0x84800000 phys_end=0x88000000 pools=4x294912,4x1572864
insmod cmemk.ko phys_start=0x84800000 phys_end=0x88000000 pools=8x1572864
modprobe cmemk phys_start=0x84800000 phys_end=0x88000000 pools=8x1572864


设置osd0
fbset -fb /dev/fb0 -xres 1024 -yres 768 -vxres 1024 -vyres 768 -depth 16

运行
./v4l2_dm368_r5

可用看到
640x640 crop @(320,64), black background