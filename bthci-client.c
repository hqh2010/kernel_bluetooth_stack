#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
// gcc bthci-client.c -lbluetooth  -o test
// bluetooth.h依赖libbluetooth-dev
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
// #include <bluetooth/rfcomm.h>

//#define HCIGETCONNINFO  _IOR('H', 213, int)

// 源码来源自Linux系统 bluez 源码包
// hci_lib中有定义
// struct hci_request {
// 	uint16_t ogf;
// 	uint16_t ocf;
// 	int      event;
// 	void     *cparam;
// 	int      clen;
// 	void     *rparam;
// 	int      rlen;
// };

// int hci_open_dev(int dev_id)
// {
// 	struct sockaddr_hci a;
// 	int dd, err;

// 	/* Check for valid device id */
// 	if (dev_id < 0) {
// 		errno = ENODEV;
// 		return -1;
// 	}

// 	/* Create HCI socket */
// 	dd = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
// 	if (dd < 0)
// 		return dd;

// 	/* Bind socket to the HCI device */
// 	memset(&a, 0, sizeof(a));
// 	a.hci_family = AF_BLUETOOTH;
// 	a.hci_dev = dev_id;
// 	if (bind(dd, (struct sockaddr *) &a, sizeof(a)) < 0)
// 		goto failed;

// 	return dd;

// failed:
// 	err = errno;
// 	close(dd);
// 	errno = err;

// 	return -1;
// }

// int hci_disconnect(int dd, uint16_t handle, uint8_t reason, int to)
// {
// 	evt_disconn_complete rp;
// 	disconnect_cp cp;
// 	struct hci_request rq;

// 	memset(&cp, 0, sizeof(cp));
// 	cp.handle = handle;
// 	cp.reason = reason;

// 	memset(&rq, 0, sizeof(rq));
// 	rq.ogf    = OGF_LINK_CTL;
// 	rq.ocf    = OCF_DISCONNECT;
// 	rq.event  = EVT_DISCONN_COMPLETE;
// 	rq.cparam = &cp;
// 	rq.clen   = DISCONNECT_CP_SIZE;
// 	rq.rparam = &rp;
// 	rq.rlen   = EVT_DISCONN_COMPLETE_SIZE;

// 	if (hci_send_req(dd, &rq, to) < 0)
// 		return -1;

// 	if (rp.status) {
// 		errno = EIO;
// 		return -1;
// 	}
// 	return 0;
// }

static int find_conn(int s, int dev_id, long arg)
{
	struct hci_conn_list_req *cl;
	struct hci_conn_info *ci;
	int i;

	if (!(cl = malloc(10 * sizeof(*ci) + sizeof(*cl)))) {
		perror("Can't allocate memory");
		exit(-1);
	}
	cl->dev_id = dev_id;
	cl->conn_num = 10;
	ci = cl->conn_info;

	if (ioctl(s, HCIGETCONNLIST, (void *) cl)) {
		perror("Can't get connection list");
		exit(-1);
	}

	for (i = 0; i < cl->conn_num; i++, ci++)
		if (!bacmp((bdaddr_t *) arg, &ci->bdaddr)) {
			free(cl);
			return 1;
		}

	free(cl);
	return 0;
}

static void cmd_dc(int dev_id, char *argv)
{
	struct hci_conn_info_req *cr;
	bdaddr_t bdaddr;
	uint8_t reason;
	int opt, dd;

	str2ba(argv, &bdaddr);
	reason = HCI_OE_USER_ENDED_CONNECTION;

	if (dev_id < 0) {
		dev_id = hci_for_each_dev(HCI_UP, find_conn, (long) &bdaddr);
		if (dev_id < 0) {
			fprintf(stderr, "Not connected.\n");
			// exit(1);
            return;
		}
        printf("dev_id:%d\n", dev_id);
	}

	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		perror("HCI device open failed");
		exit(1);
	}

	cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
	if (!cr) {
		perror("Can't allocate memory");
		exit(1);
	}

	bacpy(&cr->bdaddr, &bdaddr);
	cr->type = ACL_LINK;
	if (ioctl(dd, HCIGETCONNINFO, (unsigned long) cr) < 0) {
		perror("Get connection info failed");
		exit(1);
	}

	if (hci_disconnect(dd, htobs(cr->conn_info->handle),
						reason, 10000) < 0)
		perror("Disconnect failed");

	free(cr);

	hci_close_dev(dd);
}

int close_bluetooth_dev(int dev_id)
{
	int dd = hci_open_dev(dev_id);
	if (dd < 0) {
		perror("HCI device open failed");
		return -1;
	}
	hci_disconnect(dd, 1, HCI_OE_USER_ENDED_CONNECTION, 10000);
	hci_close_dev(dd);
}

int main(int argc, char **argv)
{
    // struct sockaddr_rc addr = { 0 };
    // int s, status;
    // char dest[18] = "01:23:45:67:89:AB";

    // // allocate a socket
    // s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // // set the connection parameters (who to connect to)
    // addr.rc_family = AF_BLUETOOTH;
    // addr.rc_channel = (uint8_t) 1;
    // str2ba( dest, &addr.rc_bdaddr );

    // // connect to server
    // status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    // // send a message
    // if( status == 0 ) {
    //     status = write(s, "hello!", 6);
    // }

    // if( status < 0 ) perror("uh oh");
    // close(s);

    // 蓝牙设备id 可以使用 hcitool dev 命令查看，eg: hci0 index = 0
    // uos@uos:~/Desktop$ hcitool dev
    // Devices:
    //         hci0    74:12:B3:96:38:4E
    // uos@uos:~/Desktop$ dpkg -S /usr/bin/hcitool
    // bluez: /usr/bin/hcitool
    
    // close_bluetooth_dev(0);
    cmd_dc(0, "74:12:B3:96:38:4E");

    return 0;
}