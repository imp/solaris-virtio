/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Grigale Ltd. nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL GRIGALE LTD OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright 2011 Grigale Ltd. All right reserved.
 */

#ifndef	_SYS_VIRTIO_H
#define	_SYS_VIRTIO_H

/*
 * Virtio Definitions
 *
 * See Virtio PCI Card Specification v0.8.10
 */

/* Virtio PCI configuration definitions */
#define	VIRTIO_PCI_VENDOR		0x1AF4
#define	VIRTIO_PCI_DEVID_MIN		0x1000
#define	VIRTIO_PCI_DEVID_MAX		0x103F
#define	VIRTIO_PCI_SUBSYS_NETWORK	0x0001
#define	VIRTIO_PCI_SUBSYS_BLOCK		0x0002
#define	VIRTIO_PCI_SUBSYS_CONSOLE	0x0003
#define	VIRTIO_PCI_SUBSYS_ENTROPY	0x0004
#define	VIRTIO_PCI_SUBSYS_MEMORY	0x0005
#define	VIRTIO_PCI_SUBSYS_IOMEMORY	0x0006
#define	VIRTIO_PCI_SUBSYS_9P		0x0009
#define	VIRTIO_PCI_REV_ABIV0		0x0000

/* Virtio Header offsets */
#define	VIRTIO_DEVICE_FEATURES		0x00000000	/* RO */
#define	VIRTIO_GUEST_FEATURES		0x00000004	/* RW */
#define	VIRTIO_QUEUE_ADDRESS		0x00000008	/* RW */
#define	VIRTIO_QUEUE_SIZE		0x0000000C	/* RO */
#define	VIRTIO_QUEUE_SELECT		0x0000000E	/* RW */
#define	VIRTIO_QUEUE_NOTIFY		0x00000010	/* RW */
#define	VIRTIO_DEVICE_STATUS		0x00000012	/* RW */
#define	VIRTIO_ISR_STATUS		0x00000013	/* RW */
#define	VIRTIO_MSIX_CONFIG_VECTOR	0x00000014	/* RW */
#define	VIRTIO_MSIX_QUEUE_VECTOR	0x00000016	/* RW */
#define	VIRTIO_DEVICE_SPECIFIC		0x00000018


/* Virtio device-independent features */
#define	VIRTIO_F_NOTIFY_ON_EMPTY	0x01000000U
#define	VIRTIO_F_RING_INDIRECT_DESC	0x10000000U
#define	VIRTIO_F_BAD_FEATURE		0x40000000U


/* Device Status bits */
#define	VIRTIO_DEV_STATUS_ACK		0x01
#define	VIRTIO_DEV_STATUS_DRIVER	0x02
#define	VIRTIO_DEV_STATUS_DRIVER_OK	0x04
#define	VIRTIO_DEV_STATUS_FAILED	0x80


/* Virtio network device features */
#define	VIRTIO_NET_F_CSUM		0x00000001U
#define	VIRTIO_NET_F_GUEST_CSUM		0x00000002U
#define	VIRTIO_NET_F_MAC		0x00000020U
#define	VIRTIO_NET_F_GSO		0x00000040U
#define	VIRTIO_NET_F_GUEST_TSO4		0x00000080U
#define	VIRTIO_NET_F_GUEST_TSO6		0x00000100U
#define	VIRTIO_NET_F_GUEST_ECN		0x00000200U
#define	VIRTIO_NET_F_GUEST_UFO		0x00000400U
#define	VIRTIO_NET_F_HOST_TSO4		0x00000800U
#define	VIRTIO_NET_F_HOST_TSO6		0x00001000U
#define	VIRTIO_NET_F_HOST_ECN		0x00002000U
#define	VIRTIO_NET_F_HOST_UFO		0x00004000U
#define	VIRTIO_NET_F_MRG_RXBUF		0x00008000U
#define	VIRTIO_NET_F_STATUS		0x00010000U
#define	VIRTIO_NET_F_CTRL_VQ		0x00020000U
#define	VIRTIO_NET_F_CTRL_RX		0x00040000U
#define	VIRTIO_NET_F_CTRL_VLAN		0x00080000U

/* Virtio network device configuration status field bits */
#define	VIRTIO_NET_S_LINK_UP		0x0001

typedef struct virtio_net_config {
	uint8_t		mac[6];
	uint16_t	status;
} virtio_net_config_t;

/* Offsets for the above struct */
#define	VIRTIO_NET_CFG_MAC		0x0000
#define	VIRTIO_NET_CFG_STATUS		0x0006

/* Virtio block device features */
#define	VIRTIO_BLK_F_BARRIER		0x00000001
#define	VIRTIO_BLK_F_SIZE_MAX		0x00000002
#define	VIRTIO_BLK_F_SEG_MAX		0x00000004
#define	VIRTIO_BLK_F_GEOMETRY		0x00000010
#define	VIRTIO_BLK_F_RO			0x00000020
#define	VIRTIO_BLK_F_BLK_SIZE		0x00000040
#define	VIRTIO_BLK_F_SCSI		0x00000080
#define	VIRTIO_BLK_F_FLUSH		0x00000200
#define	VIRTIO_BLK_F_SECTOR_MAX		0x00000400

#endif	/* _SYS_VIRTIO_H */
