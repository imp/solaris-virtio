/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the followingdisclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution .
 * 3. Neither the name of Grigale Ltd nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright 2011 Grigale Ltd. All rights reserved.
 */

#ifndef VIRTIO_RING_H
#define	VIRTIO_RING_H

#include <sys/types.h>
#ifdef	__cplusplus
extern "C"
{
#endif


/* for PCI the spec states the alignement of virtqueue data is 4096 bytes */
#define	VIRTIO_VQ_PCI_ALIGN		4096

/* The vring descriptor (16 bytes long) */
typedef struct vring_desc {
	uint64_t	addr;		/* Address (guest-physical)	*/
	uint32_t	len;		/* Length			*/
	uint16_t	flags;		/* Flag values; see below	*/
	uint16_t	next;		/* Next field if flags & NEXT	*/
} vring_desc_t;

/* Flags values for the vring descriptor */
/* This marks a buffer as continuing via the next field */
#define	VRING_DESC_F_NEXT		0x0001
/* This marks a buffer as write-only (otherwise read-only) */
#define	VRING_DESC_F_WRITE		0x0002
/* This means the buffer contains a list of buffer descriptors */
#define	VRING_DESC_F_INDIRECT		0x0004

/* Available ring */
typedef struct vring_avail {
	uint16_t	flags;		/* Flags; see below */
	uint16_t	idx;		/* the 'next' descriptor slot to use */
	uint16_t	ring[1];	/* [Queue Size] */
} vring_avail_t;

/* Flags for the vring_avail */
/*
 * DO NOT assert interrupt when the descriptor from available ring is consumed.
 * This is merely a hint, it may not suppress interrupt completely.
 */
#define	VRING_AVAIL_F_NO_INTERRUPT	0x0001


typedef struct vring_used_elem {
	uint32_t	id;
	uint32_t	len;
} vring_used_elem_t;

typedef struct vring_used {
	uint16_t	flags;		/* Flags; see below */
	uint16_t	idx;
	vring_used_elem_t	ring[1];
} vring_used_t;

/* Similar to the vring_avail - means device needs not our notification */
#define	VRING_USED_F_NO_NOTIFY		0x0001

#define	VRING_DTABLE_SIZE(n)	(sizeof (vring_desc_t) * n)
#define	VRING_AVAIL_SIZE(n)	(sizeof (vring_avail_t) + \
					(n - 1) * sizeof (uint16_t))
#define	VRING_USED_SIZE(n)	(sizeof (vring_used_t) + \
					(n - 1) * sizeof (vring_used_elem_t))

#define	VRING_ROUNDUP(n)	((n + VIRTIO_VQ_PCI_ALIGN - 1) & \
					~(VIRTIO_VQ_PCI_ALIGN - 1))
#ifdef	__cplusplus
}
#endif

#endif	/* VIRTIO_RING_H */
