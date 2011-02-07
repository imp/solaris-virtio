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
#include <sys/note.h>
#include <sys/conf.h>
#include <sys/devops.h>
#include <sys/modctl.h>
#include <sys/mac.h>
#include <sys/mac_provider.h>
#include <sys/mac_ether.h>
#include <sys/virtio.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

#include "virtionet.h"

typedef struct {
	dev_info_t	*dip;
	mac_handle_t	mh;
	uint8_t		addr[8];
} virtionet_state_t;

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

	return (0);
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

	return (B_TRUE);
}

static int
virtionet_setprop(void *arg, const char *prop_name, mac_prop_id_t prop_id,
	uint_t prop_val_size, const void *prop_val)
{
	virtionet_state_t	*sp = arg;

	return (0);
}

static int
virtionet_getprop(void *arg, const char *prop_name, mac_prop_id_t prop_id,
	uint_t prop_val_size, void *prop_val)
{
	virtionet_state_t	*sp = arg;

	return (0);
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

static int
virtionet_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	virtionet_state_t	*sp;
	mac_register_t	*mp;
	int		instance;
	int		rc;

	switch (cmd) {
	case DDI_ATTACH:
		break;
	case DDI_RESUME:
	default:
		return (DDI_FAILURE);
	}

	instance = ddi_get_instance(dip);
	if (ddi_soft_state_zalloc(virtionet_statep, instance) != DDI_SUCCESS) {
		return (DDI_FAILURE);
	}

	sp = ddi_get_soft_state(virtionet_statep, instance);
	ASSERT(sp);
	sp->dip = dip;

	mp = mac_alloc(MAC_VERSION);
	if (mp == NULL) {
		ddi_soft_state_free(virtionet_statep, instance);
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
		ddi_soft_state_free(virtionet_statep, instance);
		return (DDI_FAILURE);
	}

/*
	rc = ddi_create_minor_node(dip, "ctl", S_IFCHR, instance, NT_NET, 0);
*/

	if (rc != DDI_SUCCESS) {
		cmn_err(CE_WARN, "virtionet_attach: failed to create minor node");
		mac_unregister(sp->mh);
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
	int		instance;

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

	ddi_remove_minor_node(dip, 0);
	mac_unregister(sp->mh);
	ddi_soft_state_free(virtionet_statep, instance);

	return (DDI_SUCCESS);
}


static struct dev_ops virtionet_devops = {
	.devo_rev	= DEVO_REV,
	.devo_refcnt	= 0,
	.devo_getinfo	= NULL,
	.devo_identify	= nulldev,
	.devo_probe	= nulldev,
	.devo_attach	= virtionet_attach,
	.devo_detach	= virtionet_detach,
	.devo_reset	= nodev,
	.devo_cb_ops	= NULL,
	.devo_bus_ops	= NULL,
	.devo_power	= NULL,
	.devo_quiesce	= ddi_quiesce_not_supported
};


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

#ifdef	STAP_SOFT_STATE
	error = ddi_soft_state_init(&virtionet_statep, sizeof (virtionet_state_t), 0);
	if (error != 0) {
		return (error);
	}
#endif
	mac_init_ops(&virtionet_devops, "virtionet");
	error = mod_install(&virtionet_modlinkage);
	if (error != 0) {
		mac_fini_ops(&virtionet_devops);
#ifdef	STAP_SOFT_STATE
		ddi_soft_state_fini(&virtionet_statep);
#endif
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
#ifdef	STAP_SOFT_STATE
		ddi_soft_state_fini(&virtionet_statep);
#endif
	}
	return (error);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&virtionet_modlinkage, modinfop));
}
