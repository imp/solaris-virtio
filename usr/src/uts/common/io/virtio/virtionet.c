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
} virtqueue_t;


typedef struct {
	dev_info_t		*dip;
	caddr_t			hdraddr;
	ddi_acc_handle_t	hdrhandle;
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


static void *virtionet_statep;

/*
 * MAC callbacks
 */
static int
virtionet_getstat(void *arg, uint_t stat, uint64_t *stat_value)
{
	virtionet_state_t	*sp = arg;

	return (0);
}


static int
virtionet_start(void *arg)
{
	virtionet_state_t	*sp = arg;

	return (ENODEV);
}

static void
virtionet_stop(void *arg)
{
	virtionet_state_t	*sp = arg;

}

static int
virtionet_setpromisc(void *arg, boolean_t promisc_mode)
{
	virtionet_state_t	*sp = arg;

	return (0);
}

static int
virtionet_multicst(void *arg, boolean_t add, const uint8_t *mcast_addr)
{
	virtionet_state_t	*sp = arg;

	return (0);
}

static int
virtionet_unicst(void *arg, const uint8_t *ucast_addr)
{
	virtionet_state_t	*sp = arg;

	return (0);
}

static mblk_t *
virtionet_tx(void *arg, mblk_t *mp_chain)
{
	virtionet_state_t	*sp = arg;

	return (NULL);
}


static void
virtionet_ioctl(void *arg, queue_t *q, mblk_t *mp)
{
	virtionet_state_t	*sp = arg;

}

static boolean_t
virtionet_getcapab(void *arg, mac_capab_t cap, void *cap_data)
{
	virtionet_state_t	*sp = arg;

	switch (cap) {
	case MAC_CAPAB_HCKSUM:
	case MAC_CAPAB_LSO:
	default:
		return (B_FALSE);
	}
	return (B_TRUE);
}

static int
virtionet_setprop(void *arg, const char *prop_name, mac_prop_id_t prop_id,
	uint_t prop_val_size, const void *prop_val)
{
	virtionet_state_t	*sp = arg;

	return (ENOTSUP);
}

static int
virtionet_getprop(void *arg, const char *prop_name, mac_prop_id_t prop_id,
	uint_t prop_val_size, void *prop_val)
{
	virtionet_state_t	*sp = arg;

	return (ENOTSUP);
}

static void
virtionet_propinfo(void *arg, const char *prop_name, mac_prop_id_t prop_id,
	mac_prop_info_handle_t prop_handle)
{
	virtionet_state_t	*sp = arg;

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


/* Device attributes */
static ddi_device_acc_attr_t virtio_devattr = {
	.devacc_attr_version		= DDI_DEVICE_ATTR_V0,
	.devacc_attr_endian_flags	= DDI_STRUCTURE_LE_ACC,
	.devacc_attr_dataorder		= DDI_STRICTORDER_ACC,
	.devacc_attr_access		= DDI_DEFAULT_ACC
};


/* virtqueue buffer access attributes */
static ddi_device_acc_attr_t virtio_qattr = {
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
	.dma_attr_burstsizes		= 0x3F,		/* XXX */
	.dma_attr_minxfer		= 1,		/* XXX */
	.dma_attr_maxxfer		= 0xFFFFFFFFU,	/* XXX */
	.dma_attr_seg			= 0xFFFFFFFFU,	/* XXX */
	.dma_attr_sgllen		= 1,		/* XXX */
	.dma_attr_granular		= 1,		/* XXX */
	.dma_attr_flags			= DDI_DMA_FORCE_PHYSICAL
};


static virtqueue_t *
virtio_vq_setup(virtionet_state_t *sp, uint16_t qsize)
{
	virtqueue_t		*vqp = NULL;
	size_t			len = qsize * 16; /* XXX FIX ME */
	int			rc;

	vqp = kmem_zalloc(sizeof (*vqp), KM_SLEEP);

	vq_dma_attr.dma_attr_flags |= DDI_DMA_FORCE_PHYSICAL;

	rc = ddi_dma_alloc_handle(sp->dip, &vq_dma_attr, DDI_DMA_SLEEP,
	    NULL, &vqp->hdl);

	if (rc == DDI_DMA_BADATTR) {
		vq_dma_attr.dma_attr_flags &= (~DDI_DMA_FORCE_PHYSICAL);
		rc = ddi_dma_alloc_handle(sp->dip, &vq_dma_attr, DDI_DMA_SLEEP,
		    NULL, &vqp->hdl);
	}

	if (rc != DDI_SUCCESS) {
		kmem_free(vqp, sizeof (*vqp));
		return (NULL);
	}

	rc = ddi_dma_mem_alloc(vqp->hdl, len, &virtio_qattr, DDI_DMA_CONSISTENT,
	    DDI_DMA_SLEEP, NULL, &vqp->addr, &vqp->len, &vqp->acchdl);
	if (rc != DDI_SUCCESS) {
		ddi_dma_free_handle(&vqp->hdl);
		kmem_free(vqp, sizeof (*vqp));
		return (NULL);
	}

	rc = ddi_dma_addr_bind_handle(vqp->hdl, NULL, vqp->addr, vqp->len,
	    DDI_DMA_RDWR | DDI_DMA_CONSISTENT, DDI_DMA_SLEEP, NULL,
	    &vqp->cookie, &vqp->ccount);
	if (rc != DDI_DMA_MAPPED) {
		ddi_dma_mem_free(&vqp->acchdl);
		ddi_dma_free_handle(&vqp->hdl);
		kmem_free(vqp, sizeof (*vqp));
		return (NULL);
	}

	return (vqp);
}


static void
virtio_vq_teardown(virtqueue_t *vqp)
{
	if (vqp != NULL) {
		(void) ddi_dma_unbind_handle(vqp->hdl);
		ddi_dma_mem_free(&vqp->acchdl);
		ddi_dma_free_handle(&vqp->hdl);
		kmem_free(vqp, sizeof (*vqp));
	}
}


static int
virtio_fixed_intr_setup(virtionet_state_t *sp)
{
	int			rc;
	int			actual;

	rc = ddi_intr_alloc(sp->dip, &sp->ihandle, DDI_INTR_TYPE_FIXED, 0, 1,
	    &actual, DDI_INTR_ALLOC_NORMAL);
	if (rc != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	return (DDI_SUCCESS);
}


static int
virtio_intr_teardown(virtionet_state_t *sp)
{
	int			rc;

	rc = ddi_intr_free(sp->ihandle);
	return (DDI_SUCCESS);
}


static int
virtionet_vq_setup(virtionet_state_t *sp)
{
	int			rc;

	sp->rxq = virtio_vq_setup(sp, 0x100);
	sp->txq = virtio_vq_setup(sp, 0x100);
	sp->ctlq = virtio_vq_setup(sp, 0x10);

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
	rc = virtio_fixed_intr_setup(sp);
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
	rc = ddi_regs_map_setup(sp->dip, 1, &sp->hdraddr, 0, 0,
	    &virtio_devattr, &sp->hdrhandle);
	if (rc != DDI_SUCCESS) {
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}

	/* Reset device - we are going to re-negotiate feature set */
	VIRTIO_DEV_RESET(sp);

	/* Acknowledge the presense of the device */
	VIRTIO_DEV_ACK(sp);

	rc = virtio_validate_netdev(sp);
	if (rc != DDI_SUCCESS) {
		ddi_regs_map_free(&sp->hdrhandle);
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}
	/* We know how to drive this device */
	VIRTIO_DEV_DRIVER(sp);

	rc = virtionet_negotiate_features(sp);
	if (rc != DDI_SUCCESS) {
		ddi_regs_map_free(&sp->hdrhandle);
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}

	rc = virtionet_vq_setup(sp);
	if (rc != DDI_SUCCESS) {
		ddi_regs_map_free(&sp->hdrhandle);
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}

	rc = virtionet_intr_setup(sp);
	if (rc != DDI_SUCCESS) {
		virtionet_vq_teardown(sp);
		ddi_regs_map_free(&sp->hdrhandle);
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}

	/* XXX Should we move it to mc_start() ? */
	VIRTIO_DEV_DRIVER_OK(sp);

	rc = virtionet_mac_register(sp);
	if (rc != DDI_SUCCESS) {
		(void) virtionet_intr_teardown(sp);
		virtionet_vq_teardown(sp);
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
