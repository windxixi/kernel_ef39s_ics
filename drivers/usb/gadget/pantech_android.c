/*
 * Support function for Android USB Gadget Driver
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNE General Public License for more details.
 *
 * Add comment : 2011-10-28
 * modify from Motorola base source.
 * modify by Pantech Inc. <tarial>
 *
 */

#include <linux/types.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#include "f_pantech_android.h"

int b_support_ms_os_descriptor;
static int enable_android_usb_product_function(char *device_name, int cnt);
static void force_reenumeration(struct android_dev *dev, int dev_type);
static struct usb_configuration android_config_driver;
#define MAX_DEVICE_TYPE_NUM   30
#define MAX_DEVICE_NAME_SIZE  45

enum cable_state {
	CABLE_NONE,
	CABLE_USB,
	CABLE_FACTORY,
};

// define structure for switching mode 
struct device_pid_vid {
	char *name;
	u32 type;
	int vid;
	int pid;
	char *config_name;
	int class;
	int subclass;
	int protocol;
};

static struct device_pid_vid qualcomm_android_vid_pid[MAX_DEVICE_TYPE_NUM] = {
	{"diag_modem_rmnet_msc", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | RMNET_TYPE_FLAG, 
	0x05C6, 0x9026, 
	"Qualcomm Config 3", 
	USB_CLASS_PER_INTERFACE, 0x00, 0x00},

	{"diag_adb_modem_rmnet_msc", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG | RMNET_TYPE_FLAG, 
	0x05C6, 0x9025, 
	"Qualcomm Config 4", 
	USB_CLASS_PER_INTERFACE, 0x00, 0x00},

	{"rndis", 
	RNDIS_TYPE_FLAG, 
	0x05C6, 0xf00e, 
	"Qualcomm RNDIS Device",
	USB_CLASS_MISC, 0x02, 0x01},

	{"rndis_adb", 
	RNDIS_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x05C6, 0x9024,
	"Qualcomm RNDIS ADB Device",
	USB_CLASS_MISC, 0x02, 0x01},

	{"default", 
	DEFAULT_TYPE_FLAG, 
	0, 0, 
	0, 
	0, 0, 0},

	{}
};

static struct device_pid_vid pantech_android_vid_pid[MAX_DEVICE_TYPE_NUM] = {
	{"msc_cdrom", 
	MSC_TYPE_FLAG | CDROM_TYPE_FLAG, 
	0x10A9, 0xE001, 
	"Pantech Config 1",
	USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE},

	{"msc", 
	MSC_TYPE_FLAG, 
	0x10A9, 0xE002,
	"Pantech Config 3", 
	USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE},

	{"cdrom", 
	CDROM_TYPE_FLAG, 
	0x10A9, 0xE003, 
	"Pantech CDROM Device",
	USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE},

	{"eth", 
	ETH_TYPE_FLAG, 
	0x10A9, 0xE004,
	"Pantech Config 4",
	USB_CLASS_COMM, USB_CLASS_COMM, USB_CLASS_PER_INTERFACE},

	{"mtp", 
	MTP_TYPE_FLAG, 
	0x10A9, 0xE005, "Pantech Config 5",
	USB_CLASS_VENDOR_SPEC, USB_SUBCLASS_VENDOR_SPEC, 0},

	{"ptp",
	PTP_TYPE_FLAG,
	0x10A9, 0xE007, "Pantech Config 22",
	USB_CLASS_STILL_IMAGE, 1, 1},

	{"charge", 
	MSC_TYPE_FLAG, 
	0x10A9, 0xE006, 
	"Pantech Config 7",
	USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE},

	{"accessory", 
	ACCESSORY_TYPE_FLAG , 
	0x18D1, 0x2D00,
	"Google Accessory Device",
	USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE},

	{"accessory_adb", 
	ACCESSORY_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x18D1, 0x2D01,
	"Google Accessory ADB Device",
	USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE},

#if defined(CONFIG_PANTECH_VERIZON)
	{"msc_cdrom_adb", 
	MSC_TYPE_FLAG | ADB_TYPE_FLAG | CDROM_TYPE_FLAG, 
	0x10A9, 0x6031,
	"Pantech Config 2", 
	USB_CLASS_MISC, 0x02, 0x01},

	{"msc_adb", 
	MSC_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x6031,
	"Pantech Config 21", 
	USB_CLASS_MISC, 0x02, 0x01},

	{"eth_adb", 
	ETH_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x6031,
	"Pantech Android Composite Device",
	USB_CLASS_COMM, USB_CLASS_COMM, USB_CLASS_PER_INTERFACE},

	{"mtp_adb", 
	MTP_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x6032,
	"Pantech Config 6", 
	USB_CLASS_MISC, 0x02, 0x01},

	{"ptp_adb",
	PTP_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9, 0x6032,
	"Pantech Config 23",
	USB_CLASS_MISC, 0x02, 0x01},

	{"charge_adb", 
	MSC_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x6031,
	"Pantech Config 8", 
	USB_CLASS_MISC, 0x02, 0x01},

	{"rndis", 
	RNDIS_TYPE_FLAG, 
	0x10A9, 0x6031, 
	"Pantech RNDIS Device",
	USB_CLASS_MISC, 0x02, 0x01},

	{"rndis_adb", 
	RNDIS_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x6032,
	"Pantech RNDIS ADB Device",
	USB_CLASS_MISC, 0x02, 0x01},

	{"modem_diag", 
	ACM_TYPE_FLAG | DIAG_TYPE_FLAG , 
	0x10A9, 0x6035, 
	"Pantech Config 10", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"modem_diag_adb", 
	ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG , 
	0x10A9, 0x6035, 
	"Pantech Config 20", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG, 
	0x10A9, 0x6031, 
	"Pantech Config 11", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_adb", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x6032, 
	"Pantech Config 12", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"rndis_diag", 
	RNDIS_TYPE_FLAG | DIAG_TYPE_FLAG, 
	0x10A9, 0x6033, 
	"Pantech Config 13", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"rndis_diag_adb", 
	RNDIS_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x6034, 
	"Pantech Config 14", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_rmnet", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | RMNET_TYPE_FLAG, 
	0x10A9, 0x6036, 
	"Pantech Config 15", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_adb_rmnet", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG | RMNET_TYPE_FLAG, 
	0x10A9, 0x6037, 
	"Pantech Config 16", 
	USB_CLASS_COMM, 0x00, 0x00},

#elif defined(CONFIG_PANTECH_DOMESTIC)
	{"msc_cdrom_adb", 
	MSC_TYPE_FLAG | ADB_TYPE_FLAG | CDROM_TYPE_FLAG, 
	0x10A9, 0x1104, "Pantech Config 2", 
	USB_CLASS_MISC, 0x02, 0x01},

	{"msc_adb", 
	MSC_TYPE_FLAG | ADB_TYPE_FLAG | CDROM_TYPE_FLAG, 
	0x10A9, 0x1104,
	"Pantech Config 21", 
	USB_CLASS_MISC, 0x02, 0x01},

	{"eth_adb", 
	ETH_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104,
	"Pantech Android Composite Device",
	USB_CLASS_COMM, USB_CLASS_COMM, USB_CLASS_PER_INTERFACE},

	{"mtp_adb", 
	MTP_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104,
	"Pantech Config 6", 
	USB_CLASS_MISC, 0x02, 0x01},
	{"ptp_adb",
	PTP_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9, 0x1104,
	"Pantech Config 23",
	USB_CLASS_MISC, 0x02, 0x01},

	{"charge_adb", 
	MSC_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104,
	"Pantech Config 8", 
	USB_CLASS_MISC, 0x02, 0x01},

	{"rndis", 
	RNDIS_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech RNDIS Device",
	USB_CLASS_MISC, 0x02, 0x01},

	{"rndis_adb", 
	RNDIS_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104,
	"Pantech RNDIS ADB Device",
	USB_CLASS_MISC, 0x02, 0x01},

	{"modem_diag", 
	ACM_TYPE_FLAG | DIAG_TYPE_FLAG , 
	0x10A9, 0x1104, 
	"Pantech Config 10", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"modem_diag_adb", 
	ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG , 
	0x10A9, 0x1104, 
	"Pantech Config 20", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"mtp_modem_diag_adb", 
	ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG | MTP_TYPE_FLAG , 
	0x10A9, 0x1104, 
	"Pantech Config 24", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"ptp_modem_diag_adb", 
	ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG | PTP_TYPE_FLAG , 
	0x10A9, 0x1104, 
	"Pantech Config 25", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_obex", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | OBEX_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 11", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_obex_adb", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | OBEX_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 12", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"rndis_diag", 
	RNDIS_TYPE_FLAG | DIAG_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 13", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"rndis_diag_adb", 
	RNDIS_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 14", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_obex_rmnet", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | OBEX_TYPE_FLAG | RMNET_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 15", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_obex_adb_rmnet", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | OBEX_TYPE_FLAG | ADB_TYPE_FLAG | RMNET_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 16", 
	USB_CLASS_COMM, 0x00, 0x00},
#elif defined(CONFIG_PANTECH_ATNT)
	{"msc_cdrom_adb",
	MSC_TYPE_FLAG | ADB_TYPE_FLAG | CDROM_TYPE_FLAG,
	0x10A9, 0x6050,
	"Pantech Config 2",
	USB_CLASS_MISC, 0x02, 0x01},

	{"msc_adb",
	MSC_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9, 0x6050,
	"Pantech Config 21",
	USB_CLASS_MISC, 0x02, 0x01},

	{"eth_adb",
	ETH_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9, 0x6050,
	"Pantech Android Composite Device",
	USB_CLASS_COMM, USB_CLASS_COMM, USB_CLASS_PER_INTERFACE},

	{"mtp_adb",
	MTP_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9, 0x6043,
	"Pantech Config 6",
	USB_CLASS_MISC, 0x02, 0x01},
	{"ptp_adb",
	PTP_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9, 0x6043,
	"Pantech Config 23",
	USB_CLASS_MISC, 0x02, 0x01},

	{"charge_adb",
	MSC_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9, 0x6050,
	"Pantech Config 8",
	USB_CLASS_MISC, 0x02, 0x01},

	{"rndis",
	RNDIS_TYPE_FLAG,
	0x10A9, 0x6050,
	"Pantech RNDIS Device",
	USB_CLASS_MISC, 0x02, 0x01},

	{"rndis_adb",
	RNDIS_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9, 0x6050,
	"Pantech RNDIS ADB Device",
	USB_CLASS_MISC, 0x02, 0x01},

	{"modem_diag",
	ACM_TYPE_FLAG | DIAG_TYPE_FLAG,
	0x10A9,0x6050,
	"Pantech Config 10",
	USB_CLASS_COMM,0x00, 0x00},

	{"modem_diag_adb",
	ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9,0x6050,
	"Pantech Config 20",
	USB_CLASS_COMM,0x00, 0x00},

	{"msc_modem_diag",
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG,
	0x10A9,0x6050,
	"Pantech Config 11",
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_adb",
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9, 0x6050,
	"Pantech Config 12",
	USB_CLASS_COMM, 0x00, 0x00},

	{"rndis_diag",
	RNDIS_TYPE_FLAG | DIAG_TYPE_FLAG,
	0x10A9, 0x6050,
	"Pantech Config 13",
	USB_CLASS_COMM, 0x00, 0x00},
	
	{"rndis_diag_adb",
	RNDIS_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9, 0x6050,
	"Pantech Config 14",
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_rmnet",
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | RMNET_TYPE_FLAG,
	0x10A9, 0x6050,
	"Pantech Config 15",
	USB_CLASS_COMM,0x00, 0x00},
	
	{"msc_modem_diag_adb_rmnet",
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG | RMNET_TYPE_FLAG,
	0x10A9, 0x6050,
	"Pantech Config 16",
	USB_CLASS_COMM, 0x00, 0x00},
#else
	{"msc_cdrom_adb", 
	MSC_TYPE_FLAG | ADB_TYPE_FLAG | CDROM_TYPE_FLAG, 
	0x10A9, 0x1104,
	"Pantech Config 2", 
	USB_CLASS_MISC, 0x02, 0x01},

	{"msc_adb", 
	MSC_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104,
	"Pantech Config 21", 
	USB_CLASS_MISC, 0x02, 0x01},

	{"eth_adb", 
	ETH_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104,
	"Pantech Android Composite Device",
	USB_CLASS_COMM, USB_CLASS_COMM, USB_CLASS_PER_INTERFACE},

	{"mtp_adb", 
	MTP_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104,
	"Pantech Config 6", 
	USB_CLASS_MISC, 0x02, 0x01},
	{"ptp_adb",
	PTP_TYPE_FLAG | ADB_TYPE_FLAG,
	0x10A9, 0x1104,
	"Pantech Config 23",
	USB_CLASS_MISC, 0x02, 0x01},

	{"charge_adb", 
	MSC_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104,
	"Pantech Config 8", 
	USB_CLASS_MISC, 0x02, 0x01},

	{"rndis", 
	RNDIS_TYPE_FLAG, 
	0x10A9, 0x6031, 
	"Pantech RNDIS Device",
	USB_CLASS_MISC, 0x02, 0x01},

	{"rndis_adb", 
	RNDIS_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104,
	"Pantech RNDIS ADB Device",
	USB_CLASS_MISC, 0x02, 0x01},

	{"modem_diag", 
	ACM_TYPE_FLAG | DIAG_TYPE_FLAG , 
	0x10A9, 0x1104, 
	"Pantech Config 10", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"modem_diag_adb", 
	ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG , 
	0x10A9, 0x1104, 
	"Pantech Config 20", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 11", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_adb", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 12", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"rndis_diag", 
	RNDIS_TYPE_FLAG | DIAG_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 13", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"rndis_diag_adb", 
	RNDIS_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 14", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_rmnet", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | RMNET_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 15", 
	USB_CLASS_COMM, 0x00, 0x00},

	{"msc_modem_diag_adb_rmnet", 
	MSC_TYPE_FLAG | ACM_TYPE_FLAG | DIAG_TYPE_FLAG | ADB_TYPE_FLAG | RMNET_TYPE_FLAG, 
	0x10A9, 0x1104, 
	"Pantech Config 16", 
	USB_CLASS_COMM, 0x00, 0x00},
#endif

	{"default", DEFAULT_TYPE_FLAG, 0, 0, 0, 0, 0, 0},

	{}
};

static struct device_pid_vid *mode_android_vid_pid = pantech_android_vid_pid;

// structure for mode switching operation
struct device_mode_change_dev {
	int android_enabled_function_flag;
	int adb_mode_changed_flag;
	int tethering_mode_changed_flag;
	int pc_mode_switch_flag;
	int usb_device_cfg_flag;
	int usb_get_desc_flag;
	int usb_data_transfer_flag;
	int pst_req_mode_switch_flag;
	int usb_state_get_flag;
	wait_queue_head_t android_enable_function_wq;
	wait_queue_head_t device_mode_change_wq;
	wait_queue_head_t adb_cb_wq;
	wait_queue_head_t tethering_cb_wq;
	int g_device_type;
	atomic_t device_mode_change_excl;
};

char *pantech_usb_mode_list[MAX_USB_TYPE_NUM] = {
	"pc_mode",
	"mtp_mode",
	"ums_mode",
	"charge_mode",
	"field_mode",
	"factory_mode",
	"ptp_mode",
	""
};

int current_mode_for_pst;

/* usb manager state enabled check for vBus session*/
int usb_manager_state_enabled=0;



static struct device_mode_change_dev *_device_mode_change_dev;
atomic_t tethering_enable_excl;
atomic_t adb_enable_excl;

// function for mode switching 
int get_func_thru_config(int mode)
{
	int i;
	char name[50];

	memset(name, 0, 50);
	sprintf(name, "Pantech Config %d", mode);
	for (i = 0; i < MAX_DEVICE_TYPE_NUM; i++) {
		if (!mode_android_vid_pid[i].config_name)
			break;
		if (!strcmp(mode_android_vid_pid[i].config_name, name))
			return i;
	}
	return -1;
}

static void get_device_pid_vid(int type, int *pid, int *vid)
{
	int i;

	*vid = 0;
	*pid = 0;

	for (i = 0; i < MAX_DEVICE_TYPE_NUM; i++) {
		if (mode_android_vid_pid[i].type == type) {
			*pid = mode_android_vid_pid[i].pid;
			*vid = mode_android_vid_pid[i].vid;
			break;
		}
	}
}

static int get_func_thru_type(int type)
{
	int i;

	for (i = 0; i < MAX_DEVICE_TYPE_NUM; i++) {
		if (mode_android_vid_pid[i].type == type)
			return i;
	}
	return -1;
}

char *get_name_thru_type(int type)
{
	int i;

	for (i = 0; i < MAX_DEVICE_TYPE_NUM; i++) {
		if (mode_android_vid_pid[i].type == type)
			return mode_android_vid_pid[i].name;
	}
	return NULL;
}

int adb_enable_access(void)
{
	return atomic_read(&adb_enable_excl);
}


void usb_connect_cb(void){

	if(usb_manager_state_enabled){
		struct device_mode_change_dev *dev_mode_change =
		    _device_mode_change_dev;
	
		dev_mode_change->usb_state_get_flag = 1;
		wake_up_interruptible(&dev_mode_change->device_mode_change_wq);
		printk(KERN_ERR "Xsemiyas[%s]mode[usb_connected]\n",__func__);
	}
	
}

void mode_switch_cb(int mode)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;
	int ret;

	ret = get_func_thru_config(mode);
	if(ret < 0){
		printk(KERN_ERR "Xsemiyas[%s]mode_switch error[%d]\n",__func__, mode);
		return;
	}
	dev_mode_change->pc_mode_switch_flag = ret + 1;
	wake_up_interruptible(&dev_mode_change->device_mode_change_wq);
	printk(KERN_ERR "Xsemiyas[%s]mode[%d]ret[%d]\n",__func__, mode, ret);
}

void type_switch_cb(int type)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;
	int ret;

	ret = get_func_thru_type(type);
	if(ret < 0){
		printk(KERN_ERR "Xsemiyas[%s]type_switch error[%d]\n",__func__, type);
		return;
	}
	dev_mode_change->pc_mode_switch_flag = ret + 1;
	wake_up_interruptible(&dev_mode_change->device_mode_change_wq);
	printk(KERN_ERR "Xsemiyas[%s]type_mode[0x%x] ret[%d]\n",__func__, type, ret);
}

int mtp_mode_switch_cb(int mtp_enable)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;
	int i;
	char *mtp_mode_name = "mtp_mode";
	char *ptp_mode_name = "ptp_mode";
	char *compare_name;

	if(mtp_enable)
		compare_name = mtp_mode_name;
	else
		compare_name = ptp_mode_name;

	for (i = 0; i < MAX_USB_TYPE_NUM; i++) {
		if (pantech_usb_mode_list[i] == NULL) {
			printk(KERN_ERR "%s - USB mode list Found \n", __func__);
			return -EINVAL;
		}
		if (strcmp(compare_name, pantech_usb_mode_list[i]) == 0) { 
				break;
		}
	}

	if (i == MAX_USB_TYPE_NUM) {
		printk(KERN_ERR "%s - No Matching Function Found \n", __func__);
		return -EINVAL;
	}	

	current_mode_for_pst = i;
	dev_mode_change->pst_req_mode_switch_flag = 1;
	wake_up_interruptible(&dev_mode_change->device_mode_change_wq);
	printk(KERN_ERR "Tarial %s enable\n",pantech_usb_mode_list[i]);

	return 1;
}

void usb_data_transfer_callback(void)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;

	if (dev_mode_change->usb_data_transfer_flag == 0) {
		dev_mode_change->usb_data_transfer_flag = 1;
		dev_mode_change->usb_get_desc_flag = 1;
		wake_up_interruptible
		    (&dev_mode_change->device_mode_change_wq);
	}
	printk(KERN_ERR "Xsemiyas[%s] data_transfer[%d] get_desc[%d]\n",__func__, dev_mode_change->usb_data_transfer_flag, dev_mode_change->usb_get_desc_flag );
}

void usb_interface_enum_cb(int flag)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;

	dev_mode_change->usb_device_cfg_flag |= flag;
	if (dev_mode_change->usb_device_cfg_flag >=
	    dev_mode_change->g_device_type)
		wake_up_interruptible
		    (&dev_mode_change->device_mode_change_wq);
	printk(KERN_ERR "Xsemiyas[%s] device_flag[0x%x], g_device_flag[0x%x]\n",__func__, dev_mode_change->usb_device_cfg_flag, dev_mode_change->g_device_type );
}

void adb_mode_change_cb(void)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;
	int ret;

	ret = wait_event_interruptible(dev_mode_change->adb_cb_wq,
				(!dev_mode_change->adb_mode_changed_flag));
	if (ret < 0)
		printk(KERN_ERR "adb_change_cb: %d\n", ret);

	dev_mode_change->adb_mode_changed_flag = 1;
	wake_up_interruptible(&dev_mode_change->device_mode_change_wq);
	printk(KERN_ERR "Xsemiyas[%s] adb_mode_change[%d]\n",__func__, dev_mode_change->adb_mode_changed_flag);
}

void tethering_mode_change_cb(void)
{
	struct device_mode_change_dev *dev_mode_change =
		_device_mode_change_dev;
	int ret;

	ret = wait_event_interruptible(dev_mode_change->tethering_cb_wq,
			 (!dev_mode_change->tethering_mode_changed_flag));
	if (ret < 0) {
		printk(KERN_ERR "tethering_change_cb: %d\n", ret);
		return;
	}

	dev_mode_change->tethering_mode_changed_flag = 1;
	wake_up_interruptible(&dev_mode_change->device_mode_change_wq);

	printk(KERN_ERR "Xsemiyas[%s]tethering_mode[%d]\n",__func__, dev_mode_change->tethering_mode_changed_flag);
}

int tethering_enable_access(void)
{
	return atomic_read(&tethering_enable_excl);
}

static void android_enable_function_cb(struct android_usb_function **functions)
{
	struct device_mode_change_dev *dev_mode_change =
		_device_mode_change_dev;
	struct android_usb_function *f;
	int ret;

	if(!functions) return;

	while ((f = *functions++)) {
		if((f->request_enabled != f->current_enabled) 
			 && (!strcmp(f->name, "rndis") 
					 || !strcmp(f->name, "adb") 
					 || !strcmp(f->name, "usbnet") 
					 || !strcmp(f->name, "accessory")
					 || !strcmp(f->name, "mtp")
					 || !strcmp(f->name, "ptp"))){

			dev_mode_change->android_enabled_function_flag = 0;
			if(!strcmp(f->name, "rndis")){
				if(f->request_enabled)
					atomic_set(&tethering_enable_excl, 1);
				else
					atomic_set(&tethering_enable_excl, 0);
				tethering_mode_change_cb();
			}
			else if(!strcmp(f->name, "adb")){
				if(f->request_enabled)
					atomic_set(&adb_enable_excl, 1);
				else
					atomic_set(&adb_enable_excl, 0);
				adb_mode_change_cb();
			}
			else if(!strcmp(f->name, "accessory")){
				if(f->request_enabled){
					if(adb_enable_access()){
						type_switch_cb(ACCESSORY_TYPE_FLAG | ADB_TYPE_FLAG);
					}else{
						type_switch_cb(ACCESSORY_TYPE_FLAG);
					}
				}else{
					type_switch_cb(DEFAULT_TYPE_FLAG);
				}
			}
			else if(!strcmp(f->name, "usbnet")){
				if(f->request_enabled){
					if(adb_enable_access()){
						type_switch_cb(ETH_TYPE_FLAG | ADB_TYPE_FLAG);
					}else{
						type_switch_cb(ETH_TYPE_FLAG);
					}
				}else{
					type_switch_cb(DEFAULT_TYPE_FLAG);
				}
			}
			else if(!strcmp(f->name, "mtp")){
				if(f->request_enabled){
					(void)mtp_mode_switch_cb(1);
				}else{
					type_switch_cb(DEFAULT_TYPE_FLAG);
				}
			}
			else if(!strcmp(f->name, "ptp")){
				if(f->request_enabled){
					(void)mtp_mode_switch_cb(0);
				
				}else{
				type_switch_cb(DEFAULT_TYPE_FLAG);
			    }
			}

			ret = wait_event_interruptible(dev_mode_change->android_enable_function_wq,
					(dev_mode_change->android_enabled_function_flag));

			if (ret < 0) {
				printk(KERN_ERR "android_enabled_function_cb: %d\n", ret);
				return;
			}

		}
	}

	printk(KERN_ERR "Xsemiyas[%s][%d]\n",__func__, dev_mode_change->android_enabled_function_flag);
}

#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
void pst_req_mode_switch_cb(int pst_mode)
{
	/* This function is executed SCSI command by received pst tool
	 * execution process like below 
	 * 1. receive SCSI command from PST tool
	 * 2. check usb mode for mode switch
	 * 3. save current mode
	 * 4. send to request usb_manager daemon
	 */

	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;

	switch(pst_mode){
		case PC_MODE:
		case WINDOW_MEDIA_SYNC:
		case USB_MASS_STORAGE:
		case CHARGE_ONLY:
			break;
		default:
			printk(KERN_ERR "Tarial[%s]pst mode change not allowed command : %d\n", __func__, pst_mode);
			return;
	}

	current_mode_for_pst = pst_mode;
	dev_mode_change->pst_req_mode_switch_flag = 1;
	wake_up_interruptible(&dev_mode_change->device_mode_change_wq);
	printk(KERN_ERR "Tarial[%s]pst_mode[%d]\n",__func__, pst_mode);
}

int pst_rsp_current_mode(void)
{
	return current_mode_for_pst;
}
#endif

#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
extern void pantech_set_cdrom_enabled(unsigned char enabled, unsigned char only);
#endif

static void pantech_usb_function_set_enabled(char *name)
{
  struct android_usb_function **functions;
	struct android_usb_function *f;
	
	functions = _android_dev->functions;

	//printk(KERN_ERR "[%s]enabled\n", name);
	
	if((!strcmp(name, "msc"))
		 ||(!strcmp(name, "charge"))){
		while((f = *functions++)){
			if(!strcmp(f->name, "mass_storage")){
				f->current_enabled = 1;
				list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				return;
			}
		}
	}

	if(!strcmp(name, "cdrom")){
		while((f = *functions++)){
			if(!strcmp(f->name, "mass_storage")){
#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
				if(f->current_enabled)//ums_cdrom
					pantech_set_cdrom_enabled(1, 0);
				else
					pantech_set_cdrom_enabled(1, 1);
#endif
				if(!f->current_enabled){
					f->current_enabled = 1;
					list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				}
				return;
			}
		}
	}

	if(!strcmp(name, "eth")){
		while((f = *functions++)){
			if(!strcmp(f->name, "usbnet")){
				f->current_enabled = 1;
				list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				return;
			}
		}
	}

	if(!strcmp(name, "mtp")){
		while((f = *functions++)){
			if(!strcmp(f->name, "mtp")){
				f->current_enabled = 1;
				list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				return;
			}
		}
	}
	
	if(!strcmp(name, "ptp")){
		while((f = *functions++)){
			if(!strcmp(f->name, "ptp")){
				f->current_enabled = 1;
				list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				return;
			}
		}
	}

	if(!strcmp(name, "accessory")){
		while((f = *functions++)){
			if(!strcmp(f->name, "accessory")){
				f->current_enabled = 1;
				list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				return;
			}
		}
	}

	if(!strcmp(name, "adb")){
		while((f = *functions++)){
			if(!strcmp(f->name, "adb")){
				f->current_enabled = 1;
				if(atomic_read(&adb_enable_excl) == 0){
					atomic_set(&adb_enable_excl, 1);// non initialized in booting time.
				}
				list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				return;
			}
		}
	}

	if(!strcmp(name, "rndis")){
		while((f = *functions++)){
			if(!strcmp(f->name, "rndis")){
				f->current_enabled = 1;
				list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				return;
			}
		}
	}

	if(!strcmp(name, "modem")){
		while((f = *functions++)){
			if(!strcmp(f->name, "serial")){
				f->current_enabled = 1;
				list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				return;
			}
		}
	}

	if(!strcmp(name, "diag")){
		while((f = *functions++)){
			if(!strcmp(f->name, "diag")){
				f->current_enabled = 1;
				list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				return;
			}
		}
	}

	if(!strcmp(name, "rmnet")){
		while((f = *functions++)){
			if(!strcmp(f->name, "rmnet")){
				f->current_enabled = 1;
				list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				return;
			}
		}
	}

	if(!strcmp(name, "obex")){
		while((f = *functions++)){
			if(!strcmp(f->name, "obex")){
				f->current_enabled = 1;
				list_add_tail(&f->enabled_list, &_android_dev->enabled_functions);
				return;
			}
		}
	}

}

static int enable_android_usb_product_function(char *device_name, int cnt)
{
  struct android_usb_function **functions;
	struct android_usb_function *f;
	char *name, *b;
	char buffer[128];

	if(!_android_dev) return -1;

 	INIT_LIST_HEAD(&_android_dev->enabled_functions);

#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
	/* initialize cdrom */
	pantech_set_cdrom_enabled(0, 0);
#endif
	functions = _android_dev->functions;
	while((f = *functions++)) {
		f->current_enabled = 0;
	}

	memset(buffer, 0x00,128);
	memcpy(buffer, device_name, strlen(device_name));

	b = buffer;
	while (b) {
		name = strsep(&b, "_");
		if (name) {
			pantech_usb_function_set_enabled(name);
		}
	}

	//printk(KERN_ERR "mode set: %s\n", device_name);
	return 0;
}

static void force_reenumeration(struct android_dev *dev, int dev_type)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;
	int vid, pid, i;
	int gcnum;

	/* using other namespace ??? */
	dev_mode_change->usb_device_cfg_flag = 0;
	dev_mode_change->usb_get_desc_flag = 0;
	dev_mode_change->usb_data_transfer_flag = 0;
	dev_mode_change->pc_mode_switch_flag = 0;

	get_device_pid_vid(dev_type, &pid, &vid);
	device_desc.idProduct = __constant_cpu_to_le16(pid);
	device_desc.idVendor = __constant_cpu_to_le16(vid);

	i = get_func_thru_type(dev_type);
	if (i != -1) {
		device_desc.bDeviceClass = mode_android_vid_pid[i].class;
		device_desc.bDeviceSubClass = mode_android_vid_pid[i].subclass;
		device_desc.bDeviceProtocol = mode_android_vid_pid[i].protocol;

    if(!((device_desc.bDeviceClass == USB_CLASS_COMM)
				&&(device_desc.bDeviceSubClass == 0x00)
				&&(device_desc.bDeviceProtocol == 0x00))
			&& (mode_android_vid_pid[i].type & (MTP_TYPE_FLAG | PTP_TYPE_FLAG))){
			b_support_ms_os_descriptor = true;
    }else{
			b_support_ms_os_descriptor = false;
    }
	}else{
		device_desc.bDeviceClass = USB_CLASS_COMM;
		device_desc.bDeviceSubClass = 0x00;
		device_desc.bDeviceProtocol = 0x00;
	}

	gcnum = usb_gadget_controller_number(dev->cdev->gadget);
	device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum); 

	if (dev->cdev) {
		dev->cdev->desc.idProduct = device_desc.idProduct;
		dev->cdev->desc.idVendor = device_desc.idVendor;
		dev->cdev->desc.bcdDevice = device_desc.bcdDevice;
		dev->cdev->desc.bDeviceClass = device_desc.bDeviceClass;
		dev->cdev->desc.bDeviceSubClass = device_desc.bDeviceSubClass;
		dev->cdev->desc.bDeviceProtocol = device_desc.bDeviceProtocol;
	}
}

static char *pantech_serial_number;
#define MAX_STR_LEN 128

static ssize_t usb_serial_write_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  return 0;
}

static ssize_t usb_serial_write_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
  if(pantech_serial_number == NULL){
    pantech_serial_number = kmalloc(MAX_STR_LEN, GFP_KERNEL);
  }

  memset(pantech_serial_number, 0x00, MAX_STR_LEN);

  memcpy(pantech_serial_number, SYS_MODEL_NAME, strlen(SYS_MODEL_NAME));
  memcpy(pantech_serial_number + strlen(SYS_MODEL_NAME), buf, 14);

  strings_dev[STRING_SERIAL_IDX].s = pantech_serial_number;
	printk(KERN_ERR "[%s] serial_write[%s]\n", __func__, pantech_serial_number);

  return size;
}

static DEVICE_ATTR(usb_serial_write, S_IRUGO | S_IWUSR, usb_serial_write_show, usb_serial_write_store);

// sysfs operation for mode switching 
static ssize_t usb_mode_control_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct device_mode_change_dev *dev_mode_change = _device_mode_change_dev;

	int ret;

	if(dev_mode_change && dev_mode_change -> g_device_type != 0 &&
			((ret = get_func_thru_type(dev_mode_change->g_device_type)) > 0))
	{
		return sprintf(buf, "%s\n", mode_android_vid_pid[ret].name);
	}
	return 0;
}

static ssize_t usb_mode_control_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct device_mode_change_dev *dev_mode_change = _device_mode_change_dev;
	int i;

	/* Remove a trailing new line */
	if (size > 0 && buf[size-1] == '\n')
		((char *) buf)[size-1] = 0;

	for (i = 0; i < MAX_DEVICE_TYPE_NUM; i++) {
		if (mode_android_vid_pid[i].name == NULL) {
			printk(KERN_ERR "%s - Function Not Found \n", __func__);
			return -EINVAL;
		}
		if (strcmp(buf, mode_android_vid_pid[i].name) == 0) { 
				break;
		}
	}

	if (i == MAX_DEVICE_TYPE_NUM) {
		printk(KERN_ERR "%s - No Matching Function Found \n", __func__);
		return -EINVAL;
	}

	if(!dev_mode_change)
		return -EINVAL;

	dev_mode_change->pc_mode_switch_flag = i + 1;
	wake_up_interruptible(&dev_mode_change->device_mode_change_wq);
	printk(KERN_ERR "Xsemiyas[%s]name[%s] index[%d]\n", __func__, buf, i);

	return size;
}

static DEVICE_ATTR(usb_mode_control, S_IRUGO | S_IWUSR, usb_mode_control_show, usb_mode_control_store);

static ssize_t usb_manager_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", pantech_usb_mode_list[current_mode_for_pst]);
}

static ssize_t usb_manager_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	/* This function is executed by User set or usb_manage daemon set directly.
	 * execution process like below 
	 * 1. receive mode string by defined f_pantech_android.h
	 * 2. parse mode for mode switch
	 * 3. save current mode
	 * 4. send to request usb_manager daemon
	 */

	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;
	int i;

	/* Remove a trailing new line */
	if (size > 0 && buf[size-1] == '\n')
		((char *) buf)[size-1] = 0;

	for (i = 0; i < MAX_USB_TYPE_NUM; i++) {
		if (pantech_usb_mode_list[i] == NULL) {
			printk(KERN_ERR "%s - USB mode list Found \n", __func__);
			return -EINVAL;
		}
		if (strcmp(buf, pantech_usb_mode_list[i]) == 0) { 
				break;
		}
	}

	if (i == MAX_USB_TYPE_NUM) {
		printk(KERN_ERR "%s - No Matching Function Found \n", __func__);
		return -EINVAL;
	}	

	current_mode_for_pst = i;
	dev_mode_change->pst_req_mode_switch_flag = 1;
	wake_up_interruptible(&dev_mode_change->device_mode_change_wq);
	printk(KERN_ERR "Tarial[%s]name[%s]\n", __func__, buf);

	return size;
}

static DEVICE_ATTR(usb_manager, S_IRUGO | S_IWUSR, usb_manager_show, usb_manager_store);

static struct attribute *android_pantech_usb_control_attrs[] = {
  &dev_attr_usb_serial_write.attr,
  &dev_attr_usb_mode_control.attr,
  &dev_attr_usb_manager.attr,
  NULL,
};

static struct attribute_group android_pantech_usb_control_attr_grp = {
  .attrs = android_pantech_usb_control_attrs,
};

/*
 * Device is used for USB mode switch
 * misc file operation for mode switching
 */
static int device_mode_change_open(struct inode *ip, struct file *fp)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;

	if (atomic_inc_return(&dev_mode_change->device_mode_change_excl) !=
	    1) {
		atomic_dec(&dev_mode_change->device_mode_change_excl);
		return -EBUSY;
	}
	b_pantech_usb_module = true;
	return 0;
}

static int device_mode_change_release(struct inode *ip, struct file *fp)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;

	printk(KERN_INFO "tarial %s : mode change release\n", __func__);

	if(atomic_read(&dev_mode_change->device_mode_change_excl)){
		atomic_dec(&dev_mode_change->device_mode_change_excl);
	}

	wake_up_interruptible(&dev_mode_change->device_mode_change_wq);
	printk(KERN_ERR "%s\n",__func__);

	dev_mode_change->adb_mode_changed_flag = 0;
	b_pantech_usb_module = false;
	return 0;
}

static ssize_t device_mode_change_write(struct file *file, const char __user * buffer,
			 size_t count, loff_t *ppos)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;
	unsigned char cmd[MAX_DEVICE_NAME_SIZE + 1];
	int cnt = MAX_DEVICE_NAME_SIZE;
	int i, temp_device_type;
	int ret, result;

	if(!_android_dev){
		printk(KERN_ERR "%s - _android_dev not iniltialized \n", __func__);
		result = -EIO;
		goto _jump;
	}
	if (count <= 0) {
		printk(KERN_ERR "%s - buffer size is 0 \n", __func__);
		result =  -EFAULT;
		goto _jump;
	}

	if (cnt > count)
		cnt = count;

	if (copy_from_user(cmd, buffer, cnt)) {
		printk(KERN_ERR "%s -  Error Copying buffer \n", __func__);
		result =  -EFAULT;
		goto _jump;
	}

	cmd[cnt] = 0;

	printk(KERN_INFO "%s Mode change command=[%s]\n", __func__, cmd);
	if(strncmp(cmd, "[LOG]", 5) == 0){
		printk(KERN_ERR "[%s]\n", cmd);
		return count;
	}

	if(strncmp(cmd, "qualcomm_mode", 13) == 0){
		b_qualcomm_usb_mode = true;
		mode_android_vid_pid = qualcomm_android_vid_pid;
		printk(KERN_INFO "%s - usbmode : Qualcomm\n", __func__);
		return count;
	}

	for(i = 0; i < MAX_USB_TYPE_NUM; i++) {
		if(!strcmp(cmd, pantech_usb_mode_list[i])){
			current_mode_for_pst = i;
			return count;
		}
	}
	
	/* USB cable detached Command */
	if (strncmp(cmd, "usb_cable_detach", 16) == 0) {
		dev_mode_change->usb_data_transfer_flag = 0;
		dev_mode_change->g_device_type = 0;
		dev_mode_change->usb_device_cfg_flag = 0;
		dev_mode_change->usb_get_desc_flag = 0;
		usb_gadget_disconnect(_android_dev->cdev->gadget);
		usb_gadget_vbus_disconnect(_android_dev->cdev->gadget);
		usb_remove_config(_android_dev->cdev, &android_config_driver);
		_android_dev->enabled = false;
		/*
		 * Set the composite switch to 0 during a disconnect.
		 * This is required to handle a few corner cases, where
		 * enumeration has not completed, but the cable is yanked out
		 */
		printk(KERN_INFO "%s - Handled Detach\n", __func__);
		return count;
	}

	/* USB connect/disconnect Test Commands */
	if (strncmp(cmd, "usb_connect", 11) == 0) {
		printk(KERN_INFO "%s - Handled Connect[%d]\n", __func__, _android_dev->enabled);
		if(!_android_dev->enabled){
			_android_dev->cdev->desc.idVendor = device_desc.idVendor;
			_android_dev->cdev->desc.idProduct = device_desc.idProduct;
			_android_dev->cdev->desc.bcdDevice = device_desc.bcdDevice;
			_android_dev->cdev->desc.bDeviceClass = device_desc.bDeviceClass;
			_android_dev->cdev->desc.bDeviceSubClass = device_desc.bDeviceSubClass;
			_android_dev->cdev->desc.bDeviceProtocol = device_desc.bDeviceProtocol;
			if (usb_add_config(_android_dev->cdev, &android_config_driver,
								android_bind_config)){
				printk(KERN_ERR "%s usb add config failed\n", __func__);
				return count;
			}

			usb_gadget_connect(_android_dev->cdev->gadget);
			_android_dev->enabled = true;

		}
		return count;
	}

	if (strncmp(cmd, "usb_disconnect", 14) == 0) {
		printk(KERN_INFO "%s - Handled disconnect[%d]\n", __func__, _android_dev->enabled);
		if(_android_dev->enabled){
			usb_gadget_disconnect(_android_dev->cdev->gadget);
			usb_remove_config(_android_dev->cdev, &android_config_driver);
			_android_dev->enabled = false;
		}
		return count;
	}

	for (i = 0; i < MAX_DEVICE_TYPE_NUM; i++) {
		if (mode_android_vid_pid[i].name == NULL) {
			printk(KERN_ERR "%s - Function Not Found \n" ,
				__func__);
			result = count;
			goto _jump;
		}
		if (strlen(mode_android_vid_pid[i].name) > cnt)
			continue;
		if (strncmp(cmd, mode_android_vid_pid[i].name, cnt - 1) == 0) {
			temp_device_type = mode_android_vid_pid[i].type;
			strings_dev[STRING_CONFIG_IDX].s =
			    mode_android_vid_pid[i].config_name;
			break;
		}
	}

	if (i == MAX_DEVICE_TYPE_NUM) {
		printk(KERN_ERR "%s - No Matching Function Found \n" ,
			__func__);
		result = count;
		goto _jump;
	}

	printk(KERN_ERR "g_device_type[0x%x] == device_cfg_flag[0x%x], temp[0x%x]\n", 
				 dev_mode_change->g_device_type, dev_mode_change->usb_device_cfg_flag, temp_device_type);

	if(dev_mode_change->g_device_type == 0){
		dev_mode_change->g_device_type = dev_mode_change->usb_device_cfg_flag;
	}

	if (temp_device_type == dev_mode_change->g_device_type) {
		printk(KERN_ERR "%s - Function already configured[0x%x] \n" ,
			__func__, temp_device_type);

		result = count;
		goto _jump;
	}

	printk(KERN_ERR "%s - enable cmd : %s, cnt : %d, android_dev enabled[%d]\n", __func__, cmd, cnt, _android_dev->enabled);

	/****** start : force reenumeration  */
	if(_android_dev->enabled){
		usb_gadget_disconnect(_android_dev->cdev->gadget);
		usb_remove_config(_android_dev->cdev, &android_config_driver);
	}

	ret = enable_android_usb_product_function(cmd, cnt);
	if (ret != 0) {
		printk(KERN_ERR "%s - Error Enabling Function \n" ,
			__func__);
		result = -EFAULT;
		goto _jump;
	}

	dev_mode_change->g_device_type = temp_device_type;

	force_reenumeration(_android_dev, dev_mode_change->g_device_type);

	if(_android_dev->enabled){
		_android_dev->cdev->desc.idVendor = device_desc.idVendor;
		_android_dev->cdev->desc.idProduct = device_desc.idProduct;
		_android_dev->cdev->desc.bcdDevice = device_desc.bcdDevice;
		_android_dev->cdev->desc.bDeviceClass = device_desc.bDeviceClass;
		_android_dev->cdev->desc.bDeviceSubClass = device_desc.bDeviceSubClass;
		_android_dev->cdev->desc.bDeviceProtocol = device_desc.bDeviceProtocol;
		if (usb_add_config(_android_dev->cdev, &android_config_driver,
							android_bind_config)){
			printk(KERN_ERR "%s usb add config failed\n", __func__);
			result = -EFAULT;
			goto _jump;
		}
		usb_gadget_connect(_android_dev->cdev->gadget);
	}
	/****** end : force reenumeration */

	printk(KERN_INFO "%s - Successfully enabled function - %s \n",
		__func__, cmd);

	result = count;

_jump:
	if(dev_mode_change->android_enabled_function_flag == 0){
		dev_mode_change->android_enabled_function_flag = 1;
		wake_up_interruptible(&dev_mode_change->android_enable_function_wq);
	}

	/* usb manager state enabled check for vBus session*/
	if(usb_manager_state_enabled == 0){
		usb_manager_state_enabled = 1;
	}

	return result;
}

static int event_pending(void)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;

	if ((dev_mode_change->usb_device_cfg_flag >=
	     dev_mode_change->g_device_type)
	    && (dev_mode_change->g_device_type != 0))
		return 1;
	else if (dev_mode_change->adb_mode_changed_flag)
		return 1;
	else if (dev_mode_change->tethering_mode_changed_flag)
		return 1;
	else if (dev_mode_change->usb_get_desc_flag)
		return 1;
	else if (dev_mode_change->pc_mode_switch_flag)
		return 1;
	else if (dev_mode_change->pst_req_mode_switch_flag)
		return 1;
	else if (dev_mode_change->usb_state_get_flag)
		return 1;	
	else if (!dev_mode_change->android_enabled_function_flag)
		return 1;
	else
		return 0;
}

static unsigned int device_mode_change_poll(struct file *file,
					    struct poll_table_struct *wait)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;

	printk(KERN_ERR "mode_change_poll request\n");
	poll_wait(file, &dev_mode_change->device_mode_change_wq, wait);

	if (event_pending())
		return POLLIN | POLLRDNORM;
	else
		return 0;
}

static ssize_t device_mode_change_read(struct file *file, char *buf,
				       size_t count, loff_t *ppos)
{
	struct device_mode_change_dev *dev_mode_change =
	    _device_mode_change_dev;
	int ret, size, cnt;
	/* double check last zero */
	unsigned char no_changed[] = "none:\0";
	unsigned char usb_connected[] = "usb_connected:\0";
	unsigned char adb_en_str[] = "adb_enable:\0";
	unsigned char adb_dis_str[] = "adb_disable:\0";
	unsigned char tethering_en_str[] = "tethering_enable:\0";
	unsigned char tethering_dis_str[] = "tethering_disable:\0";
	unsigned char enumerated_str[] = "enumerated\0";
	unsigned char get_desc_str[] = "get_desc\0";
	unsigned char modswitch_str[50];
	static unsigned char *carrier_str = NULL;

	/* Message format example:
	 * carrier_str:ui_mode_change_str:type_mode_change_str:adb_state_str:tethering_state_str:enumerated
	 */

	if (!event_pending())
		return 0;

	/* append carrier type */ 
	if(carrier_str == NULL){
#if defined(CONFIG_PANTECH_VERIZON)
		carrier_str = "verizon:";
#elif defined(CONFIG_PANTECH_ATNT)
		carrier_str = "atnt:";
#elif defined(CONFIG_PANTECH_JAPAN)
		carrier_str = "japan:";
#else/* domestic */
		carrier_str = "kor:";
#endif
	}
	
	size = strlen(carrier_str);
	ret = copy_to_user(buf, carrier_str, size);
	cnt = size;
	buf += size;

	/* append PST usb mode state */
	if (!dev_mode_change->pst_req_mode_switch_flag) {
	
		if(dev_mode_change->usb_state_get_flag){
			size = strlen(usb_connected);
			ret = copy_to_user(buf, usb_connected, size);
			dev_mode_change->usb_state_get_flag=0;
			printk(KERN_ERR "Tarial usb_connected : %d\n", dev_mode_change->usb_state_get_flag);	
		}else{
			size = strlen(no_changed);
			ret = copy_to_user(buf, no_changed, size);
		}	
	
	}else {
		memset(modswitch_str, 0, 50);
		sprintf(modswitch_str, "%s", pantech_usb_mode_list[current_mode_for_pst]);
		strcat(modswitch_str, ":");
		size = strlen(modswitch_str);
		ret = copy_to_user(buf, modswitch_str, size);
		dev_mode_change->pst_req_mode_switch_flag = 0;
		printk(KERN_ERR "Tarial : %s\n", modswitch_str);
	}
	cnt += size;
	buf += size;

	/* append PC request mode */
	if (!dev_mode_change->pc_mode_switch_flag) {
		size = strlen(no_changed);
		ret = copy_to_user(buf, no_changed, size);
	} else {
		memset(modswitch_str, 0, 50);
		ret = dev_mode_change->pc_mode_switch_flag;
		dev_mode_change->pc_mode_switch_flag = 0;
		if (ret <= 0) {
			size = strlen(no_changed);
			ret = copy_to_user(buf, no_changed, size);
		} else {
			sprintf(modswitch_str, "%s",
				mode_android_vid_pid[ret-1].name);
			strcat(modswitch_str, ":");
			size = strlen(modswitch_str);
			ret = copy_to_user(buf, modswitch_str, size);
		}
	}
	cnt += size;
	buf += size;	

	/* append ADB status */
	if (!dev_mode_change->adb_mode_changed_flag) {
		size = strlen(no_changed);
		ret = copy_to_user(buf, no_changed, size);
	} else {
		if (adb_enable_access()) {
			size = strlen(adb_en_str);
			ret = copy_to_user(buf, adb_en_str, size);
			printk(KERN_ERR "adb_en_str\n");
		} else {
			size = strlen(adb_dis_str);
			ret = copy_to_user(buf, adb_dis_str, size);
			printk(KERN_ERR "adb_dis_str\n");
		}
		dev_mode_change->adb_mode_changed_flag = 0;
		wake_up_interruptible(&dev_mode_change->adb_cb_wq);
	}
	cnt += size;
	buf += size;

	/* append tethering status */
	if (!dev_mode_change->tethering_mode_changed_flag) {
		size = strlen(no_changed);
		ret = copy_to_user(buf, no_changed, size);
	} else {
		if (tethering_enable_access()) {
			size = strlen(tethering_en_str);
			ret = copy_to_user(buf, tethering_en_str, size);
		} else {
			size = strlen(tethering_dis_str);
			ret = copy_to_user(buf, tethering_dis_str, size);
		}
		dev_mode_change->tethering_mode_changed_flag = 0;
		wake_up_interruptible(&dev_mode_change->tethering_cb_wq);
	}
	cnt += size;
	buf += size;

	/* append USB enumerated state */
	if ((dev_mode_change->usb_device_cfg_flag >=
	     dev_mode_change->g_device_type)
	    && (dev_mode_change->g_device_type != 0)) {
		dev_mode_change->usb_device_cfg_flag = 0;
		size = strlen(enumerated_str);
		ret += copy_to_user(buf, enumerated_str, size);

	} else {
		if (dev_mode_change->usb_get_desc_flag == 1) {
			dev_mode_change->usb_get_desc_flag = 0;
			size = strlen(get_desc_str);
			ret += copy_to_user(buf, get_desc_str, size);
		} else {			
			size = strlen(no_changed) - 1;			
			ret += copy_to_user(buf, no_changed, size);
		}
	}
	cnt += size;

	return cnt;
}

static const struct file_operations device_mode_change_fops = {
	.owner = THIS_MODULE,
	.open = device_mode_change_open,
	.write = device_mode_change_write,
	.poll = device_mode_change_poll,
	.read = device_mode_change_read,
	.release = device_mode_change_release,
};

static struct miscdevice mode_change_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "usb_device_mode",
	.fops = &device_mode_change_fops,
};

#ifdef CONFIG_ANDROID_PANTECH_USB_FACTORY_CABLE
int set_factory_mode(bool onoff)
{
  static bool factory_mode = false;
  static char *product_string_factory = "Android(FactoryMode)" ;

  if(factory_mode == onoff) return 0;

  if(onoff == true){
      printk(KERN_ERR "factory_cable mode enable\n");
      factory_mode = true;

      strings_dev[STRING_PRODUCT_IDX].s = product_string_factory;
      if(adb_enable_access()){
        type_switch_cb(DIAG_TYPE_FLAG | ACM_TYPE_FLAG | ADB_TYPE_FLAG);
      }else{
        type_switch_cb(DIAG_TYPE_FLAG | ACM_TYPE_FLAG);
      }
  }else{
      factory_mode = false;
      printk(KERN_ERR "factory_cable mode disable\n");
      strings_dev[STRING_PRODUCT_IDX].s = product_string;
      type_switch_cb(DEFAULT_TYPE_FLAG);
  }

  return 1;
}

static int factory_cable_ctrlrequest(struct usb_composite_dev *cdev,
            const struct usb_ctrlrequest *ctrl)
{
  int value = -EOPNOTSUPP;
  int ret = 0;
  u16 wIndex = le16_to_cpu(ctrl->wIndex);
  u16 wValue = le16_to_cpu(ctrl->wValue);
  u16 wLength = le16_to_cpu(ctrl->wLength);
  struct usb_request *req = cdev->req;

  if(((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR)
     &&(ctrl->bRequest == 0x70) && (wValue == 1) && (wLength == 0) && (wIndex == 0)){
    ret = set_factory_mode(true);
  }

  if(ret){
    value = 0;
    req->zero = 0;
    req->length = value;
    if (usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC))
      printk(KERN_ERR "ep0 in queue failed\n");
  }
  return value;
}
#endif

