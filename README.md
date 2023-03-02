# 背景

使用安全管理服务PermissionManager禁用蓝牙后，使用sudo systemctl start bluetooth.service可以重新开启蓝牙．

# 分析过程

通过查看源码(hciconfig hcitool是蓝牙包bluez提供)或者strace追踪发现，hciconfig hci0 down是通过ioctl系统调用关断蓝牙．当前实现是在net/bluetooth/hci_sock.c中的

```
`static int hci_sock_ioctl(struct socket *sock, unsigned int cmd,`

​              `unsigned long arg) {`

`......`

​    `switch (cmd) {`

​    `case HCIGETDEVLIST:`

​        `return hci_get_dev_list(argp);`



​    `case HCIGETDEVINFO:`

​        `return hci_get_dev_info(argp);`



​    `case HCIGETCONNLIST:`

​        `return hci_get_conn_list(argp);`



​    `case HCIDEVUP:`

​        `if (!capable(CAP_NET_ADMIN))`

​            `return -EPERM;`

​        `// control whether can up hci device by selinux sid2`

​        `if (hci_has_perm(arg, O_RDWR)) {`

​            `return -EPERM;`

​        `}`

​        `return hci_dev_open(arg);`



​    `case HCIDEVDOWN:`

​        `if (!capable(CAP_NET_ADMIN))`

​            `return -EPERM;`

​        `// control whether can down hci device by selinux sid2`

​        `if (hci_has_perm(arg, O_RDWR)) {`

​            `return -EPERM;`

​        `}`

​        `return hci_dev_close(arg);`

`......`

`}`
```

当用户使用hciconfig hci0 down时，被hci_has_perm拦截，所以能够管控蓝牙设备，通过strace追踪sudo systemctl start bluetooth.service命令发现，问题场景是通过sendmsg系统调用来控制蓝牙的，所以可以在hci_sock_sendmsg函数中加勾子函数来管控．hci_sock_sendmsg调用了hci_mgmt_cmd．测试截图如下：

```bash
sudo journalctl -f -k
2月 28 18:08:23 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:23 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:5
2月 28 18:08:23 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 00000000893fd017, sk 00000000bbf2df9f
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 65535 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 65535 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 65535 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:4
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:17
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:52
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:47
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:61
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:14
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:39
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:51
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:18
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:19
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:48
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:53
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:40
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:14
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:15
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:7
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:7
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:6
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:16
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:15
2月 28 18:08:24 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:25 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:25 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:5
2月 28 18:08:25 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:25 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:25 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:25 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:7
2月 28 18:08:25 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:25 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:25 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:6
2月 28 18:08:25 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
2月 28 18:08:26 uos kernel: Bluetooth: tttt hci_mgmt_cmd 0 channel:3
2月 28 18:08:26 uos kernel: Bluetooth: tttt hci_mgmt_cmd index:0 name:hci0 opcode:24
2月 28 18:08:26 uos kernel: Bluetooth: tttt hci_sock_recvmsg sock 0000000079985a52, sk 00000000e0c49ce0
```

其中opcode 51 52表示添加删作设备

```
// include/net/bluetooth/mgmt.h

\#define MGMT_OP_ADD_DEVICE      0x0033

\#define MGMT_OP_REMOVE_DEVICE       0x0034
```



# 参考文档

* [蓝牙核心技术概述
](https://blog.csdn.net/xubin341719/article/details/38305331)

* [linux蓝牙驱动代码阅读笔记
](https://blog.51cto.com/u_15314083/3190495)

* [剖析Linux内核蓝牙子系统架构
](https://www.bilibili.com/video/BV15A411P77D/)

* [蓝牙主机控制接口HCI介绍
](https://zhuanlan.zhihu.com/p/574265396)

* [蓝牙编程参考
](https://people.csail.mit.edu/albert/bluez-intro/index.html)

* [基于qemu tap(NAT网络)、debootstrap 调试内核、根文件系统
](https://github.com/realwujing/linux-learning/blob/main/kernel/qemu/%E5%9F%BA%E4%BA%8Eqemu%20tap(NAT%E7%BD%91%E7%BB%9C)%E3%80%81debootstrap%20%E8%B0%83%E8%AF%95%E5%86%85%E6%A0%B8%E3%80%81%E6%A0%B9%E6%96%87%E4%BB%B6%E7%B3%BB%E7%BB%9F.md)

https://stackoverflow.com/questions/45044504/bluetooth-programming-in-c-secure-connection-and-data-transfer

https://blog.csdn.net/wangzhen209/category_5885205.html
