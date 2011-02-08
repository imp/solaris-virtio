/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Copyright 2011 Grigale Ltd.  All rights reserved.
 */

#ifndef _VIRTIONET_H
#define	_VIRTIONET_H

/*
 * Solaris GLDv3 virtio PCI driver definitions.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef	VLAN_TAGSZ
#define	VLAN_TAGSZ	0x4
#endif

/* Bitfield of features supported by our implementation */
#define	VIRTIONET_GUEST_FEATURES	\
			( \
			VIRTIO_NET_F_MAC \
			| VIRTIO_NET_F_STATUS \
			| VIRTIO_NET_F_CTRL_VQ \
			)
#ifdef __cplusplus
}
#endif

#endif /* _VIRTIONET_H */
