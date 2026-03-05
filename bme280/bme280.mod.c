#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x122c3a7e, "_printk" },
	{ 0x2b21263, "i2c_unregister_device" },
	{ 0xa9329c5c, "i2c_del_driver" },
	{ 0x51ad2efc, "cdev_del" },
	{ 0x639a2613, "device_destroy" },
	{ 0x9a3d88fa, "class_destroy" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x958bbd12, "i2c_smbus_read_byte_data" },
	{ 0x7f06074, "i2c_smbus_read_word_data" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x8f4e90d7, "class_create" },
	{ 0x75f343e5, "device_create" },
	{ 0xf6d7acc3, "cdev_init" },
	{ 0xf5193ee8, "cdev_add" },
	{ 0xab3e760e, "i2c_get_adapter" },
	{ 0xc8585f85, "i2c_new_client_device" },
	{ 0x23c03251, "i2c_register_driver" },
	{ 0xef5c51bc, "i2c_put_adapter" },
	{ 0x80383399, "i2c_smbus_write_byte_data" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x98cf60b3, "strlen" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0xdcb764ad, "memset" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x126bac03, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "C21C93DDCD1E021DADD6FCE");
