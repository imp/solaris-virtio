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
#define	VIRTIO_DEV_FEATURES		0x00	/* RO */
#define	VIRTIO_GUEST_FEATURES		0x04	/* RW */
#define	VIRTIO_QUEUE_ADDRESS		0x08	/* RW */
#define	VIRTIO_QUEUE_SIZE		0x0C	/* RO */
#define	VIRTIO_QUEUE_SELECT		0x0E	/* RW */
#define	VIRTIO_QUEUE_NOTIFY		0x10	/* RW */
#define	VIRTIO_DEVICE_STATUS		0x12	/* RW */
#define	VIRTIO_ISR_STATUS		0x13	/* RW */


/* Device Status bits */
#define	VIRTIO_DEV_STATUS_ACK		0x01
#define	VIRTIO_DEV_STATUS_DRIVER	0x02
#define	VIRTIO_DEV_STATUS_DRIVER_OK	0x04
#define	VIRTIO_DEV_STATUS_FAILED	0x80

#endif	/* _SYS_VIRTIO_H */
