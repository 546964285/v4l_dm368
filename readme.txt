userpointer方式输入和输出

cmem的版本来自这里编译出来的
/home/andy/RidgeRun-SDK-DM36x-Turrialba-Eval/proprietary/dvsdk-4_02_00_06/dvsdk/linuxutils_2_26_01_02/packages/ti/sdo/linuxutils/cmem


即rootfs_ridgerun/lib/modules/2.6.32-17-ridgerun
复制到ti_root/lib/modules，因为kernel是ridgerun版本

启动参数720x480
setenv bootargs davinci_enc_mngr.ch0_output=PRGB davinci_enc_mngr.ch0_mode=PRGB  davinci_display.cont2_bufsize=6291456 vpfe_capture.cont_bufoffset=6291456 vpfe_capture.cont_bufsize=6291456 video=davincifb:osd1=0x0x8:osd0=720x480x16,1800K@0,0:vid0=off:vid1=off console=ttyS0,115200n8  dm365_imp.oper_mode=0  vpfe_capture.interface=1  mem=61M root=/dev/nfs rw ip=192.168.2.100 nfsroot=192.168.2.17:/home/andy/targetfs/rr_rootfs dm365_generic_prgb_encoder.mode=720x480MR-16@60

true

setenv ipaddr 192.168.2.100

true

setenv serverip 192.168.2.17
true

setenv bootfile uImage
true

tftpboot


启动参数1280x720
setenv bootargs davinci_enc_mngr.ch0_output=PRGB davinci_enc_mngr.ch0_mode=PRGB  davinci_display.cont2_bufsize=6291456 vpfe_capture.cont_bufoffset=6291456 vpfe_capture.cont_bufsize=6291456 video=davincifb:osd1=0x0x8:osd0=1280x720x16,1800K@0,0:vid0=off:vid1=off console=ttyS0,115200n8  dm365_imp.oper_mode=0  vpfe_capture.interface=1  mem=61M root=/dev/nfs rw ip=192.168.2.100 nfsroot=192.168.2.17:/home/andy/targetfs/rr_rootfs dm365_generic_prgb_encoder.mode=1280x720MR-16@60

true

setenv ipaddr 192.168.2.100

true

setenv serverip 192.168.2.17
true

setenv bootfile uImage
true

tftpboot




关闭osd
fbset -disable

卸载自动加载的cmemk
rmmod cmemk

这样加载cmemk
modprobe cmemk phys_start=0x84800000 phys_end=0x88000000 pools=8x4172864

参考
modprobe cmemk phys_start=0x84800000 phys_end=0x88000000 pools=4x294912,4x1572864
insmod cmemk.ko phys_start=0x84800000 phys_end=0x88000000 pools=8x1572864
modprobe cmemk phys_start=0x84800000 phys_end=0x88000000 pools=8x1572864


 fmt.fmt.pix.sizeimage= ((fmt.fmt.pix.bytesperline * height) +
			       (fmt.fmt.pix.bytesperline * (height >> 1)));

                   
                   
                   
                   
修改自
capture_prev_rsz_onthe_fly_bayer
do_preview_resize
两个例子





运行对比
先运行v4l2_dm368_r1或者v4l2_dm368
v4l2_dm368_r4

