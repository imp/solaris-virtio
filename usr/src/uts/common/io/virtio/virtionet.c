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

/*
 * Solaris GLDv3 virtio PCI network driver
 */

#include <sys/types.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/errno.h>
#include <sys/pci.h>
#include <sys/note.h>
#include <sys/conf.h>
#include <sys/devops.h>
#include <sys/modctl.h>
#include <sys/mac.h>
#include <sys/mac_provider.h>
#include <sys/mac_ether.h>
#include <sys/ethernet.h>
#include <sys/virtio.h>
#include <sys/virtio_ring.h>
#include <sys/ddi_intr.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

#include "virtionet.h"

typedef struct {
	ddi_dma_handle_t	hdl;
	ddi_acc_handle_t	acchdl;
	caddr_t			addr;
	size_t			len;
	ddi_dma_cookie_t	cookie;
	uint_t			ccount;
} virtqueue_dma_t;

typedef struct {
	uint16_t		vq_num;
	uint16_t		vq_size;
	virtqueue_dma_t		vq_dma;
	vring_desc_t		*vr_desc;
	vring_avail_t		*vr_avail;
	vring_used_t		*vr_used;
} virtqueue_t;

typedef struct {
	dev_info_t		*dip;
	caddr_t			hdraddr;
	ddi_acc_handle_t	hdrhandle;
	caddr_t			devaddr;
	ddi_acc_handle_t	devhandle;
	uint16_t		rxqsz;
	uint16_t		txqsz;
	uint16_t		ctlqsz;
	virtqueue_t		*rxq;
	virtqueue_t		*txq;
	virtqueue_t		*ctlq;
	ddi_intr_handle_t	ihandle;
	uint32_t		features;
	mac_handle_t		mh;
	ether_addr_t		addr;
} virtionet_state_t;


#define	VIRTIO_GET8(sp, x)	ddi_get8(sp->hdrhandle, \
				    (uint8_t *)(sp->hdraddr + x))
#define	VIRTIO_PUT8(sp, x, v)	ddi_put8(sp->hdrhandle, \
				    (uint8_t *)(sp->hdraddr + x), \
				    (uint8_t)(v))
#define	VIRTIO_GET16(sp, x)	ddi_get16(sp->hdrhandle, \
				    (uint16_t *)(sp->hdraddr + x))
#define	VIRTIO_PUT16(sp, x, v)	ddi_put16(sp->hdrhandle, \
				    (uint16_t *)(sp->hdraddr + x), \
				    (uint16_t)(v))
#define	VIRTIO_GET32(sp, x)	ddi_get32(sp->hdrhandle, \
				    (uint32_t *)(sp->hdraddr + x))
#define	VIRTIO_PUT32(sp, x, v)	ddi_put32(sp->hdrhandle, \
				    (uint32_t *)(sp->hdraddr + x), \
				    (uint32_t)(v))

#define	VIRTIO_DEV_RESET(sp)	\
	VIRTIO_PUT8(sp, VIRTIO_DEVICE_STATUS, 0)
#define	VIRTIO_DEV_ACK(sp)	\
	VIRTIO_PUT8(sp, VIRTIO_DEVICE_STATUS, VIRTIO_DEV_STATUS_ACK)
#define	VIRTIO_DEV_DRIVER(sp)	\
	VIRTIO_PUT8(sp, VIRTIO_DEVICE_STATUS, VIRTIO_DEV_STATUS_DRIVER)
#define	VIRTIO_DEV_DRIVER_OK(sp)\
	VIRTIO_PUT8(sp, VIRTIO_DEVICE_STATUS, VIRTIO_DEV_STATUS_DRIVER_OK)
#define	VIRTIO_DEV_FAILED(sp)	\
	VIRTIO_PUT8(sp, VIRTIO_DEVICE_STATUS, VIRTIO_DEV_STATUS_FAILED)

#define	VIRTIO_ISR(sp)	VIRTIO_GET8(sp, VIRTIO_ISR_STATUS)

static void *virtionet_statep;

static link_state_t
virtionet_link_status(virtionet_state_t *sp)
{
	link_state_t		link;

	/*
	 * If the status field is supported read the link status there,
	 * otherwise link state "should be assumed active".
	 */
	if (sp->features & VIRTIO_NET_F_STATUS) {
		uint16_t	status;

		status = ddi_get16(sp->devhandle,
		    (uint16_t *)(sp->devaddr + VIRTIO_NET_CFG_STATUS));
		if (status & VIRTIO_NET_S_LINK_UP) {
			link = LINK_STATE_UP;
		} else {
			link = LINK_STATE_DOWN;
		}
	} else {
		link = LINK_STATE_UP;
	}

	return (link);
}


/*
 * MAC callbacks
 */
static int
virtionet_getstat(void *arg, uint_t stat, uint64_t *val)
{
	virtionet_state_t	*sp = arg;
	int			rc = 0;

	switch (stat) {
	case MAC_STAT_IFSPEED:
		*val = 1000 * 1000 * 1000;
		break;
	case MAC_STAT_MULTIRCV:
	case MAC_STAT_BRDCSTRCV:
	case MAC_STAT_MULTIXMT:
	case MAC_STAT_BRDCSTXMT:
	case MAC_STAT_NORCVBUF:
	case MAC_STAT_IERRORS:
	case MAC_STAT_UNKNOWNS:
	case MAC_STAT_NOXMTBUF:
	case MAC_STAT_OERRORS:
	case MAC_STAT_COLLISIONS:
	case MAC_STAT_RBYTES:
	case MAC_STAT_IPACKETS:
	case MAC_STAT_OBYTES:
	case MAC_STAT_OPACKETS:
	case MAC_STAT_UNDERFLOWS:
	case MAC_STAT_OVERFLOWS:
	case ETHER_STAT_ALIGN_ERRORS:
	case ETHER_STAT_FCS_ERRORS:
	case ETHER_STAT_FIRST_COLLISIONS:
	case ETHER_STAT_MULTI_COLLISIONS:
	case ETHER_STAT_SQE_ERRORS:
	case ETHER_STAT_DEFER_XMTS:
	case ETHER_STAT_TX_LATE_COLLISIONS:
	case ETHER_STAT_EX_COLLISIONS:
	case ETHER_STAT_MACXMT_ERRORS:
	case ETHER_STAT_CARRIER_ERRORS:
	case ETHER_STAT_TOOLONG_ERRORS:
	case ETHER_STAT_MACRCV_ERRORS:
	case ETHER_STAT_XCVR_ADDR:
	case ETHER_STAT_XCVR_ID:
	case ETHER_STAT_XCVR_INUSE:
	case ETHER_STAT_CAP_1000FDX:
	case ETHER_STAT_CAP_1000HDX:
	case ETHER_STAT_CAP_100FDX:
	case ETHER_STAT_CAP_100HDX:
	case ETHER_STAT_CAP_10FDX:
	case ETHER_STAT_CAP_10HDX:
	case ETHER_STAT_CAP_ASMPAUSE:
	case ETHER_STAT_CAP_PAUSE:
	case ETHER_STAT_CAP_AUTONEG:
	case ETHER_STAT_ADV_CAP_1000FDX:
	case ETHER_STAT_ADV_CAP_1000HDX:
	case ETHER_STAT_ADV_CAP_100FDX:
	case ETHER_STAT_ADV_CAP_100HDX:
	case ETHER_STAT_ADV_CAP_10FDX:
	case ETHER_STAT_ADV_CAP_10HDX:
	case ETHER_STAT_ADV_CAP_ASMPAUSE:
	case ETHER_STAT_ADV_CAP_PAUSE:
	case ETHER_STAT_ADV_CAP_AUTONEG:
	case ETHER_STAT_LP_CAP_1000FDX:
	case ETHER_STAT_LP_CAP_1000HDX:
	case ETHER_STAT_LP_CAP_100FDX:
	case ETHER_STAT_LP_CAP_100HDX:
	case ETHER_STAT_LP_CAP_10FDX:
	case ETHER_STAT_LP_CAP_10HDX:
	case ETHER_STAT_LP_CAP_ASMPAUSE:
	case ETHER_STAT_LP_CAP_PAUSE:
	case ETHER_STAT_LP_CAP_AUTONEG:
	case ETHER_STAT_LINK_ASMPAUSE:
	case ETHER_STAT_LINK_PAUSE:
	case ETHER_STAT_LINK_AUTONEG:
		rc = ENOTSUP;
		break;
	case ETHER_STAT_LINK_DUPLEX:
		*val = LINK_DUPLEX_FULL;
		break;
	default:
		rc = ENOTSUP;
	}
	return (rc);
}


static int
virtionet_start(void *arg)
{
	virtionet_state_t	*sp = arg;

	cmn_err(CE_CONT, "virtionet_start\n");

	VIRTIO_DEV_DRIVER_OK(sp);

	mac_link_update(sp->mh, virtionet_link_status(sp));

	return (0);
}

static void
virtionet_stop(void *arg)
{
	virtionet_state_t	*sp = arg;

	cmn_err(CE_CONT, "virtionet_stop\n");
}

static int
virtionet_setpromisc(void *arg, boolean_t promisc_mode)
{
	virtionet_state_t	*sp = arg;

	cmn_err(CE_CONT, "virtionet_setpromisc\n");

	return (0);
}

static int
virtionet_multicst(void *arg, boolean_t add, const uint8_t *mcast_addr)
{
	virtionet_state_t	*sp = arg;

	cmn_err(CE_CONT, "virtionet_multicst\n");

	return (0);
}

static int
virtionet_unicst(void *arg, const uint8_t *ucast_addr)
{
	virtionet_state_t	*sp = arg;

	cmn_err(CE_CONT, "virtionet_unicst\n");

	return (0);
}

static mblk_t *
virtionet_tx(void *arg, mblk_t *mp_chain)
{
	virtionet_state_t	*sp = arg;

	cmn_err(CE_CONT, "virtionet_tx\n");

	return (NULL);
}


static void
virtionet_ioctl(void *arg, queue_t *q, mblk_t *mp)
{
	virtionet_state_t	*sp = arg;

	cmn_err(CE_CONT, "virtionet_ioctl\n");
}

static boolean_t
virtionet_getcapab(void *arg, mac_capab_t cap, void *cap_data)
{
	virtionet_state_t	*sp = arg;
	boolean_t		result;

	switch (cap) {
	case MAC_CAPAB_HCKSUM:
		result = B_FALSE;
		break;
	case MAC_CAPAB_LSO:
		result = B_FALSE;
		break;
	default:
		result = B_FALSE;
	}
	return (result);
}


#define	VIRTIONET_PROP_FEATURES		"_features"
#define	VIRTIONET_PROP_RECVQSIZE	"_receiveqsize"
#define	VIRTIONET_PROP_XMITQSIZE	"_transmitqsize"
#define	VIRTIONET_PROP_CTRLQSIZE	"_controlqsize"


static int
virtionet_setprop(void *arg, const char *prop_name, mac_prop_id_t pid,
	uint_t pvalsize, const void *pval)
{
	virtionet_state_t	*sp = arg;

	return (ENOTSUP);
}

/* XXX Need to change to from uint32_t to strings */
static int
virtionet_priv_getprop(virtionet_state_t *sp, const char *pname,
	uint_t pvalsize, void *pval)
{
	int			rc = 0;

	ASSERT(pvalsize >= sizeof (uint32_t));

	if (strcmp(pname, VIRTIONET_PROP_FEATURES) == 0) {
		*((uint32_t *)pval) = sp->features;
	} else if (strcmp(pname, VIRTIONET_PROP_RECVQSIZE) == 0) {
		*((uint32_t *)pval) = sp->rxqsz;
	} else if (strcmp(pname, VIRTIONET_PROP_XMITQSIZE) == 0) {
		*((uint32_t *)pval) = sp->txqsz;
	} else if (strcmp(pname, VIRTIONET_PROP_CTRLQSIZE) == 0) {
		*((uint32_t *)pval) = sp->ctlqsz;
	} else {
		rc = ENOTSUP;
	}

	return (rc);
}


static int
virtionet_getprop(void *arg, const char *pname, mac_prop_id_t pid,
	uint_t psize, void *pval)
{
	virtionet_state_t	*sp = arg;
	int			rc = 0;

	switch (pid) {
	case MAC_PROP_DUPLEX:
		ASSERT(psize >= sizeof (link_duplex_t));
		*(link_duplex_t *)pval = LINK_DUPLEX_FULL;
		break;
	case MAC_PROP_SPEED:
		ASSERT(psize >= sizeof (uint64_t));
		*(uint64_t *)pval = 1000 * 1000 * 1000;
		break;
	case MAC_PROP_STATUS:
		ASSERT(psize >= sizeof (link_state_t));
		*(link_state_t *)pval = virtionet_link_status(sp);
		break;
	case MAC_PROP_PRIVATE:
		rc = virtionet_priv_getprop(sp, pname, psize, pval);
		break;
	default:
		rc = ENOTSUP;
	}
	return (rc);
}


static void
virtionet_priv_propinfo(virtionet_state_t *sp, const char *pname,
	mac_prop_info_handle_t ph)
{
	mac_prop_info_set_perm(ph, MAC_PROP_PERM_READ);
	if ((strcmp(pname, VIRTIONET_PROP_FEATURES) == 0) ||
	    (strcmp(pname, VIRTIONET_PROP_RECVQSIZE) == 0) ||
	    (strcmp(pname, VIRTIONET_PROP_XMITQSIZE) == 0) ||
	    (strcmp(pname, VIRTIONET_PROP_CTRLQSIZE) == 0)) {
		mac_prop_info_set_default_uint32(ph, 0);
	} else {
		cmn_err(CE_NOTE, "Unexpected private property %s",
		    pname);
	}
}


static void
virtionet_propinfo(void *arg, const char *pname, mac_prop_id_t pid,
	mac_prop_info_handle_t ph)
{
	virtionet_state_t	*sp = arg;

	switch (pid) {
	case MAC_PROP_DUPLEX:
	case MAC_PROP_SPEED:
	case MAC_PROP_STATUS:
		mac_prop_info_set_perm(ph, MAC_PROP_PERM_READ);
		break;
	case MAC_PROP_PRIVATE:
		cmn_err(CE_CONT, "private propinfo(%s)\n", pname);
		virtionet_priv_propinfo(sp, pname, ph);
		break;
	default:
		/* Do we need to do anything in this case ? */
		;
	}
}


#define	VIRTIONET_CALLBACKS	(MC_IOCTL | MC_GETCAPAB | MC_PROPERTIES)

static mac_callbacks_t virtionet_mac_callbacks = {
	.mc_callbacks	= VIRTIONET_CALLBACKS,
	.mc_getstat	= virtionet_getstat,
	.mc_start	= virtionet_start,
	.mc_stop	= virtionet_stop,
	.mc_setpromisc	= virtionet_setpromisc,
	.mc_multicst	= virtionet_multicst,
	.mc_unicst	= virtionet_unicst,
	.mc_tx		= virtionet_tx,
	.mc_ioctl	= virtionet_ioctl,
	.mc_getcapab	= virtionet_getcapab,
	.mc_setprop	= virtionet_setprop,
	.mc_getprop	= virtionet_getprop,
	.mc_propinfo	= virtionet_propinfo
};


static char *virtionet_priv_props[] = {
	VIRTIONET_PROP_FEATURES,
	VIRTIONET_PROP_RECVQSIZE,
	VIRTIONET_PROP_XMITQSIZE,
	VIRTIONET_PROP_CTRLQSIZE,
	NULL
};


/* Register virtionet driver with GLDv3 framework */
static int
virtionet_mac_register(virtionet_state_t *sp)
{
	mac_register_t		*mp;
	int			rc;

	mp = mac_alloc(MAC_VERSION);
	if (mp == NULL) {
		return (DDI_FAILURE);
	}

	mp->m_type_ident	= MAC_PLUGIN_IDENT_ETHER;
	mp->m_driver		= sp;
	mp->m_dip		= sp->dip;
	mp->m_instance		= 0;
	mp->m_src_addr		= sp->addr;
	mp->m_dst_addr		= NULL;
	mp->m_callbacks		= &virtionet_mac_callbacks;
	mp->m_min_sdu		= 0;
	mp->m_max_sdu		= ETHERMTU;
	mp->m_margin		= VLAN_TAGSZ;
	mp->m_priv_props	= virtionet_priv_props;

	rc = mac_register(mp, &sp->mh);
	mac_free(mp);
	if (rc != 0) {
		return (DDI_FAILURE);
	}

	return (DDI_SUCCESS);
}


/* Unregister virtionet driver from GLDv3 framework */
static int
virtionet_mac_unregister(virtionet_state_t *sp)
{
	int			rc;

	rc = mac_unregister(sp->mh);
	if (rc != 0) {
		return (DDI_FAILURE);
	}

	return (DDI_SUCCESS);
}


/*
 * Validate that the device in hand is indeed virtio network device
 */
static int
virtio_validate_pcidev(dev_info_t *dip)
{
	ddi_acc_handle_t	pcihdl;
	int			rc;

	rc = pci_config_setup(dip, &pcihdl);
	if (rc != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	if (pci_config_get16(pcihdl, PCI_CONF_VENID) != VIRTIO_PCI_VENDOR) {
		cmn_err(CE_WARN, "Incorrect PCI vendor id");
		rc = DDI_FAILURE;
	}

	uint16_t devid = pci_config_get16(pcihdl, PCI_CONF_DEVID);

	if ((devid < VIRTIO_PCI_DEVID_MIN) && (devid > VIRTIO_PCI_DEVID_MAX)) {
		cmn_err(CE_WARN, "Incorrect PCI device id");
		rc = DDI_FAILURE;
	}

	if (pci_config_get16(pcihdl, PCI_CONF_REVID) != VIRTIO_PCI_REV_ABIV0) {
		cmn_err(CE_WARN, "Unsupported virtio ABI detected");
		rc = DDI_FAILURE;
	}

	pci_config_teardown(&pcihdl);
	return (rc);
}


/*
 * Validate that the device in hand is indeed virtio network device
 */
static int
virtio_validate_netdev(virtionet_state_t *sp)
{
	cmn_err(CE_CONT, "Device Features 0x%X\n",
	    VIRTIO_GET32(sp, VIRTIO_DEVICE_FEATURES));
	cmn_err(CE_CONT, "Guest Features 0x%X\n",
	    VIRTIO_GET32(sp, VIRTIO_GUEST_FEATURES));
	cmn_err(CE_CONT, "Device Status 0x%X\n",
	    VIRTIO_GET8(sp, VIRTIO_DEVICE_STATUS));
	cmn_err(CE_CONT, "ISR Status 0x%X\n",
	    VIRTIO_GET8(sp, VIRTIO_ISR_STATUS));

	cmn_err(CE_CONT, "Selecting queue 0\n");
	VIRTIO_PUT16(sp, VIRTIO_QUEUE_SELECT, 0);
	cmn_err(CE_CONT, "Queue size 0x%X\n",
	    VIRTIO_GET16(sp, VIRTIO_QUEUE_SIZE));

	cmn_err(CE_CONT, "Selecting queue 1\n");
	VIRTIO_PUT16(sp, VIRTIO_QUEUE_SELECT, 1);
	cmn_err(CE_CONT, "Queue size 0x%X\n",
	    VIRTIO_GET16(sp, VIRTIO_QUEUE_SIZE));

	cmn_err(CE_CONT, "Selecting queue 2\n");
	VIRTIO_PUT16(sp, VIRTIO_QUEUE_SELECT, 2);
	cmn_err(CE_CONT, "Queue size 0x%X\n",
	    VIRTIO_GET16(sp, VIRTIO_QUEUE_SIZE));

	return (DDI_SUCCESS);
}


/*
 * Negotiate the feature set with the device
 * Return
 *        DDI_SUCCESS if succesfuly negotiated non-zero feature set
 *        DDI_FAILURE otherwise
 */
static int
virtionet_negotiate_features(virtionet_state_t *sp)
{
	sp->features = VIRTIO_GET32(sp, VIRTIO_DEVICE_FEATURES);
	sp->features &= VIRTIONET_GUEST_FEATURES;
	if (sp->features != 0) {
		/* If there any features we support let device know them */
		VIRTIO_PUT32(sp, VIRTIO_GUEST_FEATURES, sp->features);
		return (DDI_SUCCESS);
	} else {
		/* otherwise report failure to negotiate anything */
		return (DDI_FAILURE);
	}
}


static void
virtionet_get_macaddr(virtionet_state_t *sp)
{
	if (sp->features & VIRTIO_NET_F_MAC) {
		ddi_rep_get8(sp->devhandle, sp->addr,
		    (uint8_t *)(sp->devaddr + VIRTIO_NET_CFG_MAC),
		    ETHERADDRL, DDI_DEV_AUTOINCR);
	} else {
		/* TODO Should be random, but hardcoded for now */
		sp->addr[0] = 0;
		sp->addr[1] = 1;
		sp->addr[2] = 2;
		sp->addr[3] = 3;
		sp->addr[4] = 4;
		sp->addr[5] = 5;
	}

#if defined(DEBUG)
	cmn_err(CE_NOTE, "Using mac address = %x:%x:%x:%x:%x:%x",
	    sp->addr[0], sp->addr[1], sp->addr[2],
	    sp->addr[3], sp->addr[4], sp->addr[5]);
#endif
}


static uint_t
virtionet_intr(caddr_t arg1, caddr_t arg2)
{
	virtionet_state_t	*sp = (virtionet_state_t *)arg1;
	uint8_t			intr;

	/* Autoclears the ISR */
	intr = VIRTIO_ISR(sp);

	if (intr) {
		cmn_err(CE_NOTE, "intr=0x%X", intr);
		if (intr & VIRTIO_ISR_VQ) {
			/* VQ update */
			intr &= (~VIRTIO_ISR_VQ);
		}
		if (intr & VIRTIO_ISR_CFG) {
			/* Configuration update */
			intr &= (~VIRTIO_ISR_CFG);
		}
		/* Let us know if there is still something interrupting */
		if (intr) {
			cmn_err(CE_WARN, "Unknown interrupt 0x%X", intr);
		}
		return (DDI_INTR_CLAIMED);
	} else {
		return (DDI_INTR_UNCLAIMED);
	}
}


/* Device attributes */
static ddi_device_acc_attr_t virtio_devattr = {
	.devacc_attr_version		= DDI_DEVICE_ATTR_V0,
	.devacc_attr_endian_flags	= DDI_STRUCTURE_LE_ACC,
	.devacc_attr_dataorder		= DDI_STRICTORDER_ACC,
	.devacc_attr_access		= DDI_DEFAULT_ACC
};


/* virtqueue buffer access attributes */
static ddi_device_acc_attr_t virtio_native_attr = {
	.devacc_attr_version		= DDI_DEVICE_ATTR_V0,
	.devacc_attr_endian_flags	= DDI_NEVERSWAP_ACC,
	.devacc_attr_dataorder		= DDI_STRICTORDER_ACC,
	.devacc_attr_access		= DDI_DEFAULT_ACC
};


static ddi_dma_attr_t vq_dma_attr = {
	.dma_attr_version		= DMA_ATTR_V0,
	.dma_attr_addr_lo		= 0,
	.dma_attr_addr_hi		= 0xFFFFFFFFU,
	.dma_attr_count_max		= 0xFFFFFFFFU,
	.dma_attr_align			= 4096,
	.dma_attr_burstsizes		= 1,
	.dma_attr_minxfer		= 1,
	.dma_attr_maxxfer		= 0xFFFFFFFFU,
	.dma_attr_seg			= 0xFFFFFFFFU,
	.dma_attr_sgllen		= 1,
	.dma_attr_granular		= 1,
	.dma_attr_flags			= DDI_DMA_FORCE_PHYSICAL
};


static virtqueue_t *
virtio_vq_setup(virtionet_state_t *sp, uint16_t qsize)
{
	virtqueue_t		*vqp = NULL;
	size_t			len = 2 * 4096; /* XXX FIX ME !!! */
	size_t			desc_size;
	size_t			avail_size;
	size_t			used_size;
	size_t			part1;
	size_t			part2;
	int			rc;

	desc_size = VRING_DTABLE_SIZE(qsize);
	avail_size = VRING_AVAIL_SIZE(qsize);
	used_size = VRING_USED_SIZE(qsize);

	part1 = VRING_ROUNDUP(desc_size + avail_size);
	part2 = VRING_ROUNDUP(used_size);

	len = part1 + part2;

	vqp = kmem_zalloc(sizeof (*vqp), KM_SLEEP);

	vq_dma_attr.dma_attr_flags |= DDI_DMA_FORCE_PHYSICAL;

	rc = ddi_dma_alloc_handle(sp->dip, &vq_dma_attr, DDI_DMA_SLEEP,
	    NULL, &vqp->vq_dma.hdl);

	if (rc == DDI_DMA_BADATTR) {
		vq_dma_attr.dma_attr_flags &= (~DDI_DMA_FORCE_PHYSICAL);
		rc = ddi_dma_alloc_handle(sp->dip, &vq_dma_attr, DDI_DMA_SLEEP,
		    NULL, &vqp->vq_dma.hdl);
	}

	if (rc != DDI_SUCCESS) {
		kmem_free(vqp, sizeof (*vqp));
		return (NULL);
	}

	rc = ddi_dma_mem_alloc(vqp->vq_dma.hdl, len, &virtio_native_attr,
	    DDI_DMA_CONSISTENT, DDI_DMA_SLEEP, NULL, &vqp->vq_dma.addr,
	    &vqp->vq_dma.len, &vqp->vq_dma.acchdl);
	if (rc != DDI_SUCCESS) {
		ddi_dma_free_handle(&vqp->vq_dma.hdl);
		kmem_free(vqp, sizeof (*vqp));
		return (NULL);
	}

	rc = ddi_dma_addr_bind_handle(vqp->vq_dma.hdl, NULL, vqp->vq_dma.addr,
	    vqp->vq_dma.len, DDI_DMA_RDWR | DDI_DMA_CONSISTENT, DDI_DMA_SLEEP,
	    NULL, &vqp->vq_dma.cookie, &vqp->vq_dma.ccount);
	if (rc != DDI_DMA_MAPPED) {
		ddi_dma_mem_free(&vqp->vq_dma.acchdl);
		ddi_dma_free_handle(&vqp->vq_dma.hdl);
		kmem_free(vqp, sizeof (*vqp));
		return (NULL);
	}
	ASSERT(&vqp->vq_dma.ccount == 1);

	vqp->vq_size = qsize;
	vqp->vr_desc = (vring_desc_t *)vqp->vq_dma.addr;
	vqp->vr_avail = (vring_avail_t *)(vqp->vq_dma.addr + desc_size);
	vqp->vr_used = (vring_used_t *)(vqp->vq_dma.addr + part1);

	return (vqp);
}


static void
virtio_vq_teardown(virtqueue_t *vqp)
{
	if (vqp != NULL) {
		(void) ddi_dma_unbind_handle(vqp->vq_dma.hdl);
		ddi_dma_mem_free(&vqp->vq_dma.acchdl);
		ddi_dma_free_handle(&vqp->vq_dma.hdl);
		kmem_free(vqp, sizeof (*vqp));
	}
}


static int
virtio_fixed_intr_setup(virtionet_state_t *sp, ddi_intr_handler_t inthandler)
{
	int			rc;
	int			nintr;
	uint_t			pri;

	rc = ddi_intr_get_nintrs(sp->dip, DDI_INTR_TYPE_FIXED, &nintr);
	if (rc != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	ASSERT(nintr == 1);

	rc = ddi_intr_alloc(sp->dip, &sp->ihandle, DDI_INTR_TYPE_FIXED, 0, 1,
	    &nintr, DDI_INTR_ALLOC_NORMAL);
	if (rc != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	ASSERT(actual == 1);

	rc = ddi_intr_get_pri(sp->ihandle, &pri);
	if (rc != DDI_SUCCESS) {
		(void) ddi_intr_free(sp->ihandle);
		return (DDI_FAILURE);
	}

	cmn_err(CE_NOTE, "Supported interrupt priority = %d(%d)",
	    pri, ddi_intr_get_hilevel_pri());

	/* Test for high level mutex */
	if (pri >= ddi_intr_get_hilevel_pri()) {
		cmn_err(CE_WARN, "Hi level interrupt not supported");
		(void) ddi_intr_free(sp->ihandle);
		return (DDI_FAILURE);
	}

	rc = ddi_intr_add_handler(sp->ihandle, inthandler, sp, NULL);
	if (rc != DDI_SUCCESS) {
		(void) ddi_intr_free(sp->ihandle);
		return (DDI_FAILURE);
	}

	rc = ddi_intr_enable(sp->ihandle);
	if (rc != DDI_SUCCESS) {
		(void) ddi_intr_remove_handler(sp->ihandle);
		(void) ddi_intr_free(sp->ihandle);
		return (DDI_FAILURE);
	}
	return (DDI_SUCCESS);
}


static int
virtio_intr_teardown(virtionet_state_t *sp)
{
	int			rc;

	rc = ddi_intr_disable(sp->ihandle);
	rc = ddi_intr_remove_handler(sp->ihandle);
	rc = ddi_intr_free(sp->ihandle);
	return (DDI_SUCCESS);
}


static int
virtionet_vq_setup(virtionet_state_t *sp)
{
	/* Receive queue */
	VIRTIO_PUT16(sp, VIRTIO_QUEUE_SELECT, 0);
	sp->rxqsz = VIRTIO_GET16(sp, VIRTIO_QUEUE_SIZE);
	sp->rxq = virtio_vq_setup(sp, sp->rxqsz);

	/* Transmit queue */
	VIRTIO_PUT16(sp, VIRTIO_QUEUE_SELECT, 1);
	sp->txqsz = VIRTIO_GET16(sp, VIRTIO_QUEUE_SIZE);
	sp->txq = virtio_vq_setup(sp, sp->txqsz);

	/* Control queue */
	VIRTIO_PUT16(sp, VIRTIO_QUEUE_SELECT, 2);
	sp->ctlqsz = VIRTIO_GET16(sp, VIRTIO_QUEUE_SIZE);
	sp->ctlq = virtio_vq_setup(sp, sp->ctlqsz);

	if ((sp->rxq == NULL) || (sp->txq == NULL) || (sp->ctlq == NULL)) {
		virtio_vq_teardown(sp->rxq);
		virtio_vq_teardown(sp->txq);
		virtio_vq_teardown(sp->ctlq);
		sp->rxq = sp->txq = sp->ctlq = NULL;
		return (DDI_FAILURE);
	}

	return (DDI_SUCCESS);
}


static void
virtionet_vq_teardown(virtionet_state_t *sp)
{
	virtio_vq_teardown(sp->rxq);
	virtio_vq_teardown(sp->txq);
	virtio_vq_teardown(sp->ctlq);
	sp->rxq = sp->txq = sp->ctlq = NULL;
}


static int
virtionet_intr_setup(virtionet_state_t *sp)
{
	int			rc;
	int			itypes;


	rc = ddi_intr_get_supported_types(sp->dip, &itypes);
	if (rc != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	if (itypes & DDI_INTR_TYPE_MSIX) {
		cmn_err(CE_NOTE, "Detected MSIX interrupt support");
	}
	if (itypes & DDI_INTR_TYPE_MSI) {
		cmn_err(CE_NOTE, "Detected MSI interrupt support");
	}
	if (itypes & DDI_INTR_TYPE_FIXED) {
		cmn_err(CE_NOTE, "Detected FIXED interrupt support");
	}
	rc = virtio_fixed_intr_setup(sp, virtionet_intr);
	return (DDI_SUCCESS);
}


static int
virtionet_intr_teardown(virtionet_state_t *sp)
{
	int			rc;

	rc = virtio_intr_teardown(sp);
	return (DDI_SUCCESS);
}


static int
virtionet_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	virtionet_state_t	*sp;
	int			instance;
	int			rc;

	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
	default:
		return (DDI_FAILURE);
	}

	/* Sanity check - make sure this is indeed virtio PCI device */
	if (virtio_validate_pcidev(dip) != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	instance = ddi_get_instance(dip);
	if (ddi_soft_state_zalloc(virtionet_statep, instance) != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	sp = ddi_get_soft_state(virtionet_statep, instance);
	ASSERT(sp);
	sp->dip = dip;

	/* Map virtionet PCI header */
	rc = ddi_regs_map_setup(sp->dip, 1, &sp->hdraddr, 0,
	    VIRTIO_DEVICE_SPECIFIC, &virtio_devattr, &sp->hdrhandle);
	if (rc != DDI_SUCCESS) {
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}

	/*
	 * The device specific portion is *always* in guest native mode,
	 * so it can be accessed directly, w/o ddi_get()/ddi_put() machinery.
	 */
	/* Map virtionet device specific configuration area */
	off_t	len;
	if (ddi_dev_regsize(sp->dip, 1, &len) != DDI_SUCCESS) {
		ddi_regs_map_free(&sp->hdrhandle);
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}
	rc = ddi_regs_map_setup(sp->dip, 1, &sp->devaddr,
	    VIRTIO_DEVICE_SPECIFIC, len - VIRTIO_DEVICE_SPECIFIC,
	    &virtio_devattr, &sp->devhandle);
	if (rc != DDI_SUCCESS) {
		ddi_regs_map_free(&sp->hdrhandle);
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}

	cmn_err(CE_CONT, "PCI header %p, device specific %p\n",
	    sp->hdraddr, sp->devaddr);
/*
	sp->devcfg =
	    (virtio_net_config_t *)(sp->hdraddr + VIRTIO_DEVICE_SPECIFIC);
*/

	/* Reset device - we are going to re-negotiate feature set */
	VIRTIO_DEV_RESET(sp);

	/* Acknowledge the presense of the device */
	VIRTIO_DEV_ACK(sp);

	rc = virtio_validate_netdev(sp);
	if (rc != DDI_SUCCESS) {
		ddi_regs_map_free(&sp->devhandle);
		ddi_regs_map_free(&sp->hdrhandle);
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}
	/* We know how to drive this device */
	VIRTIO_DEV_DRIVER(sp);

	rc = virtionet_negotiate_features(sp);
	if (rc != DDI_SUCCESS) {
		ddi_regs_map_free(&sp->devhandle);
		ddi_regs_map_free(&sp->hdrhandle);
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}

	virtionet_get_macaddr(sp);
	
	rc = virtionet_vq_setup(sp);
	if (rc != DDI_SUCCESS) {
		ddi_regs_map_free(&sp->devhandle);
		ddi_regs_map_free(&sp->hdrhandle);
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}

	rc = virtionet_intr_setup(sp);
	if (rc != DDI_SUCCESS) {
		virtionet_vq_teardown(sp);
		ddi_regs_map_free(&sp->devhandle);
		ddi_regs_map_free(&sp->hdrhandle);
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}

	rc = virtionet_mac_register(sp);
	if (rc != DDI_SUCCESS) {
		(void) virtionet_intr_teardown(sp);
		virtionet_vq_teardown(sp);
		ddi_regs_map_free(&sp->devhandle);
		ddi_regs_map_free(&sp->hdrhandle);
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}

	ddi_report_dev(dip);

	return (DDI_SUCCESS);
}


static int
virtionet_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	virtionet_state_t	*sp;
	int			instance;
	int			rc;

	switch (cmd) {
	case DDI_DETACH:
		break;
	case DDI_SUSPEND:
	default:
		return (DDI_FAILURE);
	}

	instance = ddi_get_instance(dip);
	sp = ddi_get_soft_state(virtionet_statep, instance);

	ASSERT(sp);

	rc = virtionet_mac_unregister(sp);
	if (rc != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	(void) virtionet_intr_teardown(sp);
	virtionet_vq_teardown(sp);
	ddi_regs_map_free(&sp->devhandle);
	ddi_regs_map_free(&sp->hdrhandle);
	ddi_soft_state_free(virtionet_statep, instance);

	return (DDI_SUCCESS);
}

/*
 * Stream information
 */
DDI_DEFINE_STREAM_OPS(virtionet_devops, nulldev, nulldev, virtionet_attach,
    virtionet_detach, nodev, NULL, D_MP, NULL, ddi_quiesce_not_supported);


static struct modldrv virtionet_modldrv = {
	.drv_modops	= &mod_driverops,
	.drv_linkinfo	= "virtionet driver v0",
	.drv_dev_ops	= &virtionet_devops
};

static struct modlinkage virtionet_modlinkage = {
	.ml_rev		= MODREV_1,
	.ml_linkage	= {&virtionet_modldrv, NULL, NULL, NULL}
};


/*
 * Loadable module entry points.
 */
int
_init(void)
{
	int error;

	error = ddi_soft_state_init(&virtionet_statep,
	    sizeof (virtionet_state_t), 0);
	if (error != 0) {
		return (error);
	}

	mac_init_ops(&virtionet_devops, "virtionet");
	error = mod_install(&virtionet_modlinkage);
	if (error != 0) {
		mac_fini_ops(&virtionet_devops);
		ddi_soft_state_fini(&virtionet_statep);
	}
	return (error);
}

int
_fini(void)
{
	int error;

	error = mod_remove(&virtionet_modlinkage);
	if (error == 0) {
		mac_fini_ops(&virtionet_devops);
		ddi_soft_state_fini(&virtionet_statep);
	}
	return (error);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&virtionet_modlinkage, modinfop));
}
