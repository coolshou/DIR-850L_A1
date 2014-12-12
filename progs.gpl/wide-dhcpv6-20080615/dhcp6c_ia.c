/*	$KAME: dhcp6c_ia.c,v 1.33 2005/07/22 08:50:05 jinmei Exp $	*/

/*
 * Copyright (C) 2003 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dhcp6.h"
#include "config.h"
#include "common.h"
#include "timer.h"
#include "dhcp6c.h"
#include "dhcp6c_ia.h"
#include "prefixconf.h"
#include "addrconf.h"

#ifdef DHCPV6_LOGO
#include <errno.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "libnetlink.h"
#endif


char *xifname = NULL;  //rbj

typedef enum {IAS_ACTIVE, IAS_RENEW, IAS_REBIND , IAS_CNF , IAS_DECLINE} iastate_t;

struct ia {
	TAILQ_ENTRY(ia) link;

	/* back pointer to configuration */
	struct ia_conf *conf;

	/* common parameters of IA */
	u_int32_t t1;		/* duration for renewal */
	u_int32_t t2;		/* duration for rebind  */

	/* internal parameters for renewal/rebinding */
	iastate_t state;
	struct dhcp6_timer *timer;
	struct dhcp6_eventdata *evdata;

	/* DHCP related parameters */
	struct dhcp6_if *ifp;	/* DHCP interface */
	struct duid serverid;	/* the server ID that provided this IA */

	/* control information shared with each particular config routine */
	struct iactl *ctl;

	/* authentication parameters for transaction with servers on this IA */
	struct authparam *authparam;
};

static int update_authparam __P((struct ia *, struct authparam *));
static void reestablish_ia __P((struct ia *));
static void callback __P((struct ia *));
static int release_ia __P((struct ia *));
static void remove_ia __P((struct ia *));
static struct ia *get_ia __P((iatype_t, struct dhcp6_if *, struct ia_conf *,
    struct dhcp6_listval *, struct duid *));
static struct ia *find_ia __P((struct ia_conf *, iatype_t, u_int32_t));
static struct dhcp6_timer *ia_timo __P((void *));

static char *iastr __P((iatype_t));
static char *statestr __P((iastate_t));

#ifdef DHCPV6_LOGO
void release_choose_ia(iatype_t iatype, struct dhcp6_list *ialist, struct dhcp6_if *ifp,struct duid *serverid)
{
	struct ia_conf *iac;
	struct dhcp6_listval *iav;
	struct ia *ia;

	for (iav = TAILQ_FIRST(ialist); iav; iav = TAILQ_NEXT(iav, link)) 
	{
		if ((iac = find_iaconf(&ifp->iaconf_list, iatype, iav->val_ia.iaid)) == NULL) 
			continue;	

		ia = get_ia(iatype, ifp, iac, iav, serverid);
		if(ia)
			remove_ia(ia);
	}
}

//for UNH-IOL (tom, 20110502)
static void renew_ia(struct ia *ia)
{
	struct dhcp6_ia iaparam;
	struct dhcp6_event *ev;
	struct dhcp6_eventdata *evd;
	int dhcpstate;
	struct timeval timo;

	dprintf(LOG_DEBUG, FNAME, "renew IA: %s-%lu", iastr(ia->conf->type), ia->conf->iaid);

	/* cancel the current event for the prefix. */
	if (ia->evdata) 
	{
		TAILQ_REMOVE(&ia->evdata->event->data_list, ia->evdata, link);
		if (ia->evdata->destructor)
			ia->evdata->destructor(ia->evdata);
		free(ia->evdata);
		ia->evdata = NULL;
	}

	ia->state = IAS_RENEW;
	dhcpstate = DHCP6S_RENEW;
	timo.tv_sec = ia->t1 < ia->t2 ? ia->t2 - ia->t1 : 0;
	timo.tv_usec = 0;
	dhcp6_set_timer(&timo, ia->timer);

	if ((ev = dhcp6_create_event(ia->ifp, dhcpstate)) == NULL) {
		dprintf(LOG_NOTICE, FNAME, "failed to create a new event");
		goto fail;
	}
	TAILQ_INSERT_TAIL(&ia->ifp->event_list, ev, link);

	if ((ev->timer = dhcp6_add_timer(client6_timo, ev)) == NULL) {
		dprintf(LOG_NOTICE, FNAME,
		    "failed to create a new event timer");
		goto fail;
	}

	if ((evd = malloc(sizeof(*evd))) == NULL) {
		dprintf(LOG_NOTICE, FNAME,
		    "failed to create a new event data");
		goto fail;
	}
	memset(evd, 0, sizeof(*evd));
	evd->event = ev;
	TAILQ_INSERT_TAIL(&ev->data_list, evd, link);

	if (duidcpy(&ev->serverid, &ia->serverid)) {
		dprintf(LOG_NOTICE, FNAME, "failed to copy server ID");
		goto fail;
	}

	iaparam.iaid = ia->conf->iaid;
	iaparam.t1 = ia->t1;
	iaparam.t2 = ia->t2;

	if (ia->ctl && ia->ctl->renew_data) {
		if ((*ia->ctl->renew_data)(ia->ctl, &iaparam,
		    &ia->evdata, evd)) {
			dprintf(LOG_NOTICE, FNAME,
			    "failed to make renew data");
			goto fail;
		}
	}

	ev->timeouts = 0;
	dhcp6_set_timeoparam(ev);
	dhcp6_reset_timer(ev);

	if (ia->authparam != NULL) {
		if ((ev->authparam = copy_authparam(ia->authparam)) == NULL) {
			dprintf(LOG_WARNING, FNAME,
			    "failed to copy authparam");
			goto fail;
		}
	}

	ia->evdata = evd;
	client6_send(ev);

	return;
fail:
	if (ev)
		dhcp6_remove_event(ev);

	return;
}

void renew_choose_ia(iatype_t iatype, struct dhcp6_list *ialist, struct dhcp6_if *ifp,struct duid *serverid)
{
	struct ia_conf *iac;
	struct dhcp6_listval *iav;
	struct ia *ia;

	for (iav = TAILQ_FIRST(ialist); iav; iav = TAILQ_NEXT(iav, link)) 
	{
		if ((iac = find_iaconf(&ifp->iaconf_list, iatype, iav->val_ia.iaid)) == NULL) 
			continue;	

		ia = get_ia(iatype, ifp, iac, iav, serverid);
		if(ia)
			renew_ia(ia);
	}
}

typedef struct match_cond_t
{
	struct in6_addr addr;
	int if_idx;
	int found;
}match_cond;

#define IFA_F_DADFAILED (0x08)

static int check_address_object(const struct sockaddr_nl *who , struct nlmsghdr *n , void *arg)
{
	match_cond *target = (match_cond*)arg;
	struct ifaddrmsg *ifa = NLMSG_DATA(n);
	struct rtattr * rta_tb[IFA_MAX+1];

	if(n->nlmsg_len < NLMSG_LENGTH(sizeof(ifa)))
		return 0; //skip this

	if(ifa->ifa_index != target->if_idx)
		return 0; //skip this

	if(ifa->ifa_family != AF_INET6)
		return 0; //skip this

	parse_rtattr(rta_tb, IFA_MAX, IFA_RTA(ifa), n->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa)));

	if(rta_tb[IFA_ADDRESS])
	{
		if(memcmp(target->addr.s6_addr , ((struct in6_addr*)RTA_DATA(rta_tb[IFA_ADDRESS]))->s6_addr , 16) == 0)
		{
			//is DAD ok?
			if((ifa->ifa_flags & IFA_F_DADFAILED) == 0)
				target->found = 1;
		}
	}

	return 0;
}

static int check_address_status(const char *ifname , struct in6_addr *addr)
{
	match_cond target;
	int result = 0;
	struct rtnl_handle rth;

	memset(&target , 0 , sizeof(target));
	//prepare our target object
	memcpy(target.addr.s6_addr , addr->s6_addr , 16);
	target.if_idx = if_nametoindex(ifname);
	if(target.if_idx == 0)
	{
		result = -ENODATA;
		goto out;
	}

	if((result = rtnl_open(&rth , 0)) < 0)
		goto out;

	if((result = rtnl_wilddump_request(&rth, AF_INET6, RTM_GETADDR)) < 0)
		goto out_close_connection;

	if((result = rtnl_dump_filter(&rth, check_address_object, &target, NULL, NULL)) < 0)
		goto out_close_connection;
	
	if(target.found == 0)
		result = -ENODATA;

out_close_connection:
	rtnl_close(&rth);
out:
	return result;
}

static void decline_ia(struct ia *ia)
{
	struct dhcp6_ia iaparam;
	struct dhcp6_event *ev;
	struct dhcp6_eventdata *evd;

	ia->state = IAS_DECLINE;
	//console_printf(LOC_FMT"declining IA: %s-%lu\n" , LOC_ARG , iastr(ia->conf->type), ia->conf->iaid);
	dprintf(LOG_DEBUG, FNAME, "declining IA: %s-%lu", iastr(ia->conf->type), ia->conf->iaid);

    /* cancel the current event for the prefix. */
	if (ia->evdata) 
	{
		TAILQ_REMOVE(&ia->evdata->event->data_list, ia->evdata, link);
		if (ia->evdata->destructor)
			ia->evdata->destructor(ia->evdata);
		free(ia->evdata);
		ia->evdata = NULL;
	}

    /* we don't need a timer for the IA (see comments in a_timo()) */
	if (ia->timer)
		dhcp6_remove_timer(&ia->timer);

    if ((ev = dhcp6_create_event(ia->ifp, DHCP6S_DECLINE)) == NULL) {
		//console_printf(LOC_FMT"failed to create a new event\n" , LOC_ARG);
		dprintf(LOG_NOTICE, FNAME, "failed to create a new event");
		goto fail;
	}
	TAILQ_INSERT_TAIL(&ia->ifp->event_list, ev, link);
	//console_printf(LOC_FMT"new event: %p\n" , LOC_ARG , ev);

    if ((ev->timer = dhcp6_add_timer(client6_timo, ev)) == NULL) {
		//console_printf(LOC_FMT"failed to create a new event timer\n" , LOC_ARG);
		dprintf(LOG_NOTICE, FNAME,"failed to create a new event timer");
		goto fail;
	}

	if (duidcpy(&ev->serverid, &ia->serverid)) {
		dprintf(LOG_NOTICE, FNAME, "failed to copy server ID");
		goto fail;
	}

	if ((evd = malloc(sizeof(*evd))) == NULL) 
	{
		//console_printf(LOC_FMT"failed to create a new event data\n" , LOC_ARG);
		dprintf(LOG_NOTICE, FNAME, "failed to create a new event data");
		goto fail;
	}
	memset(evd, 0, sizeof(*evd));
	evd->event = ev;
	TAILQ_INSERT_TAIL(&ev->data_list, evd, link);

	iaparam.iaid = ia->conf->iaid;
	iaparam.t1 = ia->t1;
	iaparam.t2 = ia->t2;

	if (ia->ctl && ia->ctl->reestablish_data) 
	{
		if ((*ia->ctl->reestablish_data)(ia->ctl, &iaparam, &ia->evdata, evd)) 
		{
			//console_printf(LOC_FMT"failed to make reestablish data\n" , LOC_ARG);
			dprintf(LOG_NOTICE, FNAME, "failed to make reestablish data");
			goto fail;
		}
	}

	if (ia->authparam != NULL) 
	{
		if ((ev->authparam = copy_authparam(ia->authparam)) == NULL) 
		{
			//console_printf(LOC_FMT"failed to copy authparam\n" , LOC_ARG);
			dprintf(LOG_WARNING, FNAME, "failed to copy authparam");
			goto fail;
		}
	}

	ev->timeouts = 0;
	dhcp6_set_timeoparam(ev);
	dhcp6_reset_timer(ev);

	ia->evdata = evd;

	client6_send(ev);
	return;

fail:
	if (ev)
		dhcp6_remove_event(ev);
	return;
}

void remove_declined_address(struct iactl **ctlp , struct dhcp6_statefuladdr *addr);

void reset_ia_timer(struct dhcp6_listval *iav , iatype_t iatype , struct dhcp6_if *ifp , struct duid *serverid)
{
	struct ia_conf *iac;
	struct ia *ia;
	struct timeval timo;

	if((iac = find_iaconf(&ifp->iaconf_list, iatype, iav->val_ia.iaid)) == NULL)
		return;

	if((ia = get_ia(iatype, ifp, iac, iav, serverid)) == NULL)
		return;

	if(ia->timer == NULL)
		ia->timer = dhcp6_add_timer(ia_timo, ia);

	if(ia->timer == NULL)
		return;

	timo.tv_sec = ia->t2 - ia->t1;
	timo.tv_usec = 0;
	dhcp6_set_timer(&timo, ia->timer);

	ia->state = IAS_ACTIVE;
	//console_printf(LOC_FMT"reset timer (t2: %d, t1: %d)\n" , LOC_ARG , ia->t2 , ia->t1);
}
#endif //DHCPV6_LOGO

#ifdef DHCPV6_LOGO
int
#else
void
#endif
update_ia(iatype, ialist, ifp, serverid, authparam)
	iatype_t iatype;
	struct dhcp6_list *ialist;
	struct dhcp6_if *ifp;
	struct duid *serverid;
	struct authparam *authparam;
{
	struct ia *ia;
	struct ia_conf *iac;
	struct iapd_conf *iapdc;
	struct iana_conf *ianac;
	struct dhcp6_listval *iav, *siav;
	struct timeval timo;

	for (iav = TAILQ_FIRST(ialist); iav; iav = TAILQ_NEXT(iav, link)) {
		/* if we're not interested in this IA, ignore it. */
		if ((iac = find_iaconf(&ifp->iaconf_list, iatype,
		    iav->val_ia.iaid)) == NULL) {
			continue;
		}

		/* validate parameters */
		/*
		 * If a client receives an IA_NA with T1 greater than T2, and
		 * both T1 and T2 are greater than 0, the client discards the
		 * IA_NA option and processes the remainder of the message as
		 * though the server had not included the invalid IA_NA option.
		 * [RFC3315 22.4]
		 * We apply the same rule to IA_PD as well.
		 */
		if (iav->val_ia.t2 != 0 && iav->val_ia.t1 > iav->val_ia.t2) {
			dprintf(LOG_INFO, FNAME,
			    "invalid IA: T1(%lu) > T2(%lu)",
			    iav->val_ia.t1, iav->val_ia.t2);
			continue;
		}

		/* locate the local IA or make a new one */
		ia = get_ia(iatype, ifp, iac, iav, serverid);
		if (ia == NULL) {
			dprintf(LOG_WARNING, FNAME, "failed to get an IA "
			    "type: %s, ID: %u", iastr(iac->type), iac->iaid);
			continue;
		}

		/* update authentication parameters */
		if (update_authparam(ia, authparam)) {
			dprintf(LOG_WARNING, FNAME, "failed to update "
			    "authentication param for IA "
			    "type: %s, ID: %u", iastr(iac->type), iac->iaid);
			remove_ia(ia);
			continue;
		}

		/* update IA configuration information */
		for (siav = TAILQ_FIRST(&iav->sublist); siav;
		    siav = TAILQ_NEXT(siav, link)) {
			switch (siav->type) {
			case DHCP6_LISTVAL_PREFIX6:
				/* add or update the prefix */
				iapdc = (struct iapd_conf *)iac;
				if (update_prefix(ia, &siav->val_prefix6,
				    &iapdc->iapd_pif_list, ifp, &ia->ctl,
				    callback)) {
					dprintf(LOG_NOTICE, FNAME,
					    "failed to update a prefix %s/%d",
					    in6addr2str(&siav->val_prefix6.addr, 0),
					    siav->val_prefix6.plen);
#ifdef DHCPV6_LOGO
					return -EINVAL;
#endif
				}
				break;
			case DHCP6_LISTVAL_STATEFULADDR6:
				ianac = (struct iana_conf *)iac;
				if (update_address(ia, &siav->val_statefuladdr6,
				    ifp, &ia->ctl, callback)) {
					dprintf(LOG_NOTICE, FNAME,
					    "failed to update an address %s",
					    in6addr2str(&siav->val_statefuladdr6.addr, 0));
				}
#ifdef DHCPV6_LOGO
				//wait for kernel DAD procedure, 1 sec should be enough
				sleep(1);
				{
				int result;
				if((result = check_address_status(ifp->ifname , &siav->val_statefuladdr6.addr)) < 0)
				{
					remove_declined_address(&ia->ctl , &siav->val_statefuladdr6);
					decline_ia(ia);
					goto nextia;
				}
				else
				{
				}
				}
#endif
				break;
			case DHCP6_LISTVAL_STCODE:
				dprintf(LOG_INFO, FNAME,
				    "status code for %s-%lu: %s",
				    iastr(iatype), iav->val_ia.iaid,
				    dhcp6_stcodestr(siav->val_num16));
				if ((ia->state == IAS_RENEW ||
				    ia->state == IAS_REBIND) &&
				    siav->val_num16 == DH6OPT_STCODE_NOBINDING) {
					/*
					 * For each IA in the original Renew or
					 * Rebind message, the client
					 * sends a Request message if the IA
					 * contained a Status Code option
					 * with the NoBinding status.
					 * [RFC3315 18.1.8]
					 * XXX: what about the PD case?
					 */
					dprintf(LOG_INFO, FNAME,
					    "receive NoBinding against "
					    "renew/rebind for %s-%lu",
					    iastr(ia->conf->type),
					    ia->conf->iaid);
					reestablish_ia(ia);
					goto nextia;
				}
				break;
			default:
				dprintf(LOG_ERR, FNAME, "impossible case");
				goto nextia;
			}
		}

		/* see if this IA is still valid.  if not, remove it. */
		if (ia->ctl == NULL || !(*ia->ctl->isvalid)(ia->ctl)) {
			dprintf(LOG_DEBUG, FNAME, "IA %s-%lu is invalidated",
			    iastr(ia->conf->type), ia->conf->iaid);
			remove_ia(ia);
			continue;
		}

		/* if T1 or T2 is 0, determine appropriate values locally. */
		if (ia->t1 == 0 || ia->t2 == 0) {
			u_int32_t duration;

			if (ia->ctl && ia->ctl->duration)
				duration = (*ia->ctl->duration)(ia->ctl);
			else
				duration = 1800; /* 30min. XXX: no rationale */

			if (ia->t1 == 0) {
				if (duration == DHCP6_DURATION_INFINITE)
					ia->t1 = DHCP6_DURATION_INFINITE;
				else
					ia->t1 = duration / 2;
			}
			if (ia->t2 == 0) {
				if (duration == DHCP6_DURATION_INFINITE)
					ia->t2 = DHCP6_DURATION_INFINITE;
				else
					ia->t2 = duration * 4 / 5;
			}

			/* make sure T1 <= T2 */
			if (ia->t1 > ia->t2)
				ia->t1 = ia->t2 * 5 / 8;

			dprintf(LOG_INFO, FNAME, "T1(%lu) and/or T2(%lu) "
			    "is locally determined",  ia->t1, ia->t2);
		}

		/*
		 * Be proactive for too-small timeout values.  Note that
		 * the adjusted values may make some information expire
		 * without renewal.
		 */
		if (ia->t2 < DHCP6_DURATION_MIN) {
			dprintf(LOG_INFO, FNAME, "T1 (%lu) or T2 (%lu) "
			    "is too small", ia->t1, ia->t2);
			ia->t2 = DHCP6_DURATION_MIN;
			ia->t1 = ia->t2 * 5 / 8;
			dprintf(LOG_INFO, "", "  adjusted to %lu and %lu",
			    ia->t1, ia->t2);
		}

		/* set up a timer for this IA. */
		if (ia->t1 == DHCP6_DURATION_INFINITE) {
			if (ia->timer)
				dhcp6_remove_timer(&ia->timer);
		} else {
			if (ia->timer == NULL)
				ia->timer = dhcp6_add_timer(ia_timo, ia);
			if (ia->timer == NULL) {
				dprintf(LOG_ERR, FNAME,
				    "failed to add IA timer");
				remove_ia(ia); /* XXX */
				continue;
			}
			timo.tv_sec = ia->t1;
			timo.tv_usec = 0;
			dhcp6_set_timer(&timo, ia->timer);
		}

		ia->state = IAS_ACTIVE;

	  nextia:
		;
	}

#ifdef DHCPV6_LOGO
	return 0;
#endif
}

static int
update_authparam(ia, authparam)
	struct ia *ia;
	struct authparam *authparam;
{
	if (authparam == NULL)
		return (0);

	if (ia->authparam == NULL) {
		if ((ia->authparam = copy_authparam(authparam)) == NULL) {
			dprintf(LOG_WARNING, FNAME,
			    "failed to copy authparam");
			return (-1);
		}
		return (0);
	}

	/* update the previous RD value and flags */
	ia->authparam->prevrd = authparam->prevrd;
	ia->authparam->flags = authparam->flags;

	return (0);
}

static void
reestablish_ia(ia)
	struct ia *ia;
{
	struct dhcp6_ia iaparam;
	struct dhcp6_event *ev;
	struct dhcp6_eventdata *evd;

	dprintf(LOG_DEBUG, FNAME, "re-establishing IA: %s-%lu", 
	    iastr(ia->conf->type), ia->conf->iaid);

	if (ia->state != IAS_RENEW && ia->state != IAS_REBIND) {
		dprintf(LOG_ERR, FNAME, "internal error (invalid IA status)");
		exit(1);	/* XXX */
	}

	/* cancel the current event for the prefix. */
	if (ia->evdata) {
		TAILQ_REMOVE(&ia->evdata->event->data_list, ia->evdata, link);
		if (ia->evdata->destructor)
			ia->evdata->destructor(ia->evdata);
		free(ia->evdata);
		ia->evdata = NULL;
	}

	/* we don't need a timer for the IA (see comments in ia_timo()) */
	if (ia->timer)
		dhcp6_remove_timer(&ia->timer);

	if ((ev = dhcp6_create_event(ia->ifp, DHCP6S_REQUEST)) == NULL) {
		dprintf(LOG_NOTICE, FNAME, "failed to create a new event");
		goto fail;
	}
	TAILQ_INSERT_TAIL(&ia->ifp->event_list, ev, link);

	if ((ev->timer = dhcp6_add_timer(client6_timo, ev)) == NULL) {
		dprintf(LOG_NOTICE, FNAME,
		    "failed to create a new event timer");
		goto fail;
	}

	if ((evd = malloc(sizeof(*evd))) == NULL) {
		dprintf(LOG_NOTICE, FNAME,
		    "failed to create a new event data");
		goto fail;
	}
	memset(evd, 0, sizeof(*evd));
	evd->event = ev;
	TAILQ_INSERT_TAIL(&ev->data_list, evd, link);

	if (duidcpy(&ev->serverid, &ia->serverid)) {
		dprintf(LOG_NOTICE, FNAME, "failed to copy server ID");
		goto fail;
	}

	iaparam.iaid = ia->conf->iaid;
	iaparam.t1 = ia->t1;
	iaparam.t2 = ia->t2;

	if (ia->ctl && ia->ctl->reestablish_data) {
		if ((*ia->ctl->reestablish_data)(ia->ctl, &iaparam,
		    &ia->evdata, evd)) {
			dprintf(LOG_NOTICE, FNAME,
			    "failed to make reestablish data");
			goto fail;
		}
	}

	if (ia->authparam != NULL) {
		if ((ev->authparam = copy_authparam(ia->authparam)) == NULL) {
			dprintf(LOG_WARNING, FNAME,
			    "failed to copy authparam");
			goto fail;
		}
	}

	ev->timeouts = 0;
	dhcp6_set_timeoparam(ev);
	dhcp6_reset_timer(ev);

	ia->evdata = evd;

	client6_send(ev);

	return;

  fail:
	if (ev)
		dhcp6_remove_event(ev);

	return;
}

#ifdef DHCPV6_LOGO
static void confirm_ia(struct ia *ia , int type)
{
	struct dhcp6_ia iaparam;
	struct dhcp6_event *ev;
	struct dhcp6_eventdata *evd;

	if(ia->state != IAS_ACTIVE)
		return; //this ia not active, do nothing

	if(type == IATYPE_PD)
		return; //do nothing, for PD wait until T2

	ia->state = IAS_CNF;

	//console_printf(LOC_FMT"confirming IA: %s-%lu\n" , LOC_ARG , iastr(ia->conf->type), ia->conf->iaid);
	dprintf(LOG_DEBUG, FNAME, "confirming IA: %s-%lu", iastr(ia->conf->type), ia->conf->iaid);

    /* cancel the current event for the prefix. */
	if (ia->evdata) 
	{
		TAILQ_REMOVE(&ia->evdata->event->data_list, ia->evdata, link);
		if (ia->evdata->destructor)
			ia->evdata->destructor(ia->evdata);
		free(ia->evdata);
		ia->evdata = NULL;
	}

    /* we don't need a timer for the IA (see comments in ia_timo()) */
	if (ia->timer)
		dhcp6_remove_timer(&ia->timer);

    if ((ev = dhcp6_create_event(ia->ifp, DHCP6S_CNF)) == NULL) {
		//console_printf(LOC_FMT"failed to create a new event\n" , LOC_ARG);
		dprintf(LOG_NOTICE, FNAME, "failed to create a new event");
		goto fail;
	}

	TAILQ_INSERT_TAIL(&ia->ifp->event_list, ev, link);
	//console_printf(LOC_FMT"new event: %p\n" , LOC_ARG , ev);

    if ((ev->timer = dhcp6_add_timer(client6_timo, ev)) == NULL) {
		//console_printf(LOC_FMT"failed to create a new event timer\n" , LOC_ARG);
		dprintf(LOG_NOTICE, FNAME,"failed to create a new event timer");
		goto fail;
	}

	if ((evd = malloc(sizeof(*evd))) == NULL) 
	{
		//console_printf(LOC_FMT"failed to create a new event data\n" , LOC_ARG);
		dprintf(LOG_NOTICE, FNAME, "failed to create a new event data");
		goto fail;
	}
	memset(evd, 0, sizeof(*evd));
	evd->event = ev;
	TAILQ_INSERT_TAIL(&ev->data_list, evd, link);

	iaparam.iaid = ia->conf->iaid;
	iaparam.t1 = ia->t1;
	iaparam.t2 = ia->t2;

	if (ia->ctl && ia->ctl->reestablish_data) 
	{
		if ((*ia->ctl->reestablish_data)(ia->ctl, &iaparam, &ia->evdata, evd)) 
		{
			//console_printf(LOC_FMT"failed to make reestablish data\n" , LOC_ARG);
			dprintf(LOG_NOTICE, FNAME, "failed to make reestablish data");
			goto fail;
		}
	}

	if (ia->authparam != NULL) 
	{
		if ((ev->authparam = copy_authparam(ia->authparam)) == NULL) 
		{
			//console_printf(LOC_FMT"failed to copy authparam\n" , LOC_ARG);
			dprintf(LOG_WARNING, FNAME, "failed to copy authparam");
			goto fail;
		}
	}

	ev->timeouts = 0;
	dhcp6_set_timeoparam(ev);
	dhcp6_reset_timer(ev);

	ia->evdata = evd;

	client6_send(ev);
	return;

fail:
	if (ev)
		dhcp6_remove_event(ev);

	//console_printf(LOC_FMT"\n" , LOC_ARG);
	return;
}

void confirm_all_ia(struct dhcp6_if *ifp);
void confirm_all_ia(struct dhcp6_if *ifp)
{   
	struct ia_conf *iac;
	struct ia *ia, *ia_next;

	for (iac = TAILQ_FIRST(&ifp->iaconf_list); iac ; iac = TAILQ_NEXT(iac, link)) 
	{
		for (ia = TAILQ_FIRST(&iac->iadata); ia; ia = ia_next) 
		{
			ia_next = TAILQ_NEXT(ia, link);

			//console_printf(LOC_FMT">>confirm_ia\n" , LOC_ARG);
			confirm_ia(ia , iac->type);
			//console_printf(LOC_FMT"<<confirm_ia\n" , LOC_ARG);
		}
	}
}
#endif

static void
callback(ia)
	struct ia *ia;
{
	/* see if this IA is still valid.  if not, remove it. */
	if (ia->ctl == NULL || !(*ia->ctl->isvalid)(ia->ctl)) {
		dprintf(LOG_DEBUG, FNAME, "IA %s-%lu is invalidated",
		    iastr(ia->conf->type), ia->conf->iaid);
		remove_ia(ia);
	}
}

//add by rbj
//>>>> 
void
renew_all_ia(ifp)
	struct dhcp6_if *ifp;
{
	struct ia_conf *iac;
	struct ia *ia, *ia_next;

	for (iac = TAILQ_FIRST(&ifp->iaconf_list); iac;
	    iac = TAILQ_NEXT(iac, link)) {
		for (ia = TAILQ_FIRST(&iac->iadata); ia; ia = ia_next) {
			ia_next = TAILQ_NEXT(ia, link);

			renew_ia(ia);
		}
	}
}

static void rebind_ia(struct ia *ia)
{
	struct dhcp6_ia iaparam;
	struct dhcp6_event *ev;
	struct dhcp6_eventdata *evd;
	int dhcpstate;
	struct timeval timo;

	dprintf(LOG_DEBUG, FNAME, "rebind IA: %s-%lu", iastr(ia->conf->type), ia->conf->iaid);

	/* cancel the current event for the prefix. */
	if (ia->evdata) 
	{
		TAILQ_REMOVE(&ia->evdata->event->data_list, ia->evdata, link);
		if (ia->evdata->destructor)
			ia->evdata->destructor(ia->evdata);
		free(ia->evdata);
		ia->evdata = NULL;
	}

	ia->state = IAS_REBIND;
	dhcpstate = DHCP6S_REBIND;
	//timo.tv_sec = ia->t1 < ia->t2 ? ia->t2 - ia->t1 : 0;
	//timo.tv_usec = 0;
	//dhcp6_set_timer(&timo, ia->timer);
	dhcp6_remove_timer(&ia->timer);

	if ((ev = dhcp6_create_event(ia->ifp, dhcpstate)) == NULL) {
		dprintf(LOG_NOTICE, FNAME, "failed to create a new event");
		goto fail;
	}
	TAILQ_INSERT_TAIL(&ia->ifp->event_list, ev, link);

	if ((ev->timer = dhcp6_add_timer(client6_timo, ev)) == NULL) {
		dprintf(LOG_NOTICE, FNAME,
		    "failed to create a new event timer");
		goto fail;
	}

	if ((evd = malloc(sizeof(*evd))) == NULL) {
		dprintf(LOG_NOTICE, FNAME,
		    "failed to create a new event data");
		goto fail;
	}
	memset(evd, 0, sizeof(*evd));
	evd->event = ev;
	TAILQ_INSERT_TAIL(&ev->data_list, evd, link);

	//if (duidcpy(&ev->serverid, &ia->serverid)) {
	//	dprintf(LOG_NOTICE, FNAME, "failed to copy server ID");
	//	goto fail;
	//}

	iaparam.iaid = ia->conf->iaid;
	iaparam.t1 = ia->t1;
	iaparam.t2 = ia->t2;

	if (ia->ctl && ia->ctl->rebind_data) {
		if ((*ia->ctl->rebind_data)(ia->ctl, &iaparam,
		    &ia->evdata, evd)) {
			dprintf(LOG_NOTICE, FNAME,
			    "failed to make renew data");
			goto fail;
		}
	}

	ev->timeouts = 0;
	dhcp6_set_timeoparam(ev);
	dhcp6_reset_timer(ev);

	if (ia->authparam != NULL) {
		if ((ev->authparam = copy_authparam(ia->authparam)) == NULL) {
			dprintf(LOG_WARNING, FNAME,
			    "failed to copy authparam");
			goto fail;
		}
	}

	ia->evdata = evd;
	client6_send(ev);

	return;
fail:
	if (ev)
		dhcp6_remove_event(ev);

	return;
}

void
rebind_all_ia(ifp)
	struct dhcp6_if *ifp;
{
	struct ia_conf *iac;
	struct ia *ia, *ia_next;

	for (iac = TAILQ_FIRST(&ifp->iaconf_list); iac;
	    iac = TAILQ_NEXT(iac, link)) {
		for (ia = TAILQ_FIRST(&iac->iadata); ia; ia = ia_next) {
			ia_next = TAILQ_NEXT(ia, link);

			rebind_ia(ia);
		}
	}
}
//<<<<

void
release_all_ia(ifp)
	struct dhcp6_if *ifp;
{
	struct ia_conf *iac;
	struct ia *ia, *ia_next;

	for (iac = TAILQ_FIRST(&ifp->iaconf_list); iac;
	    iac = TAILQ_NEXT(iac, link)) {
		for (ia = TAILQ_FIRST(&iac->iadata); ia; ia = ia_next) {
			ia_next = TAILQ_NEXT(ia, link);

			(void)release_ia(ia);

			/*
			 * The client MUST stop using all of the addresses
			 * being released as soon as the client begins the
			 * Release message exchange process.
			 * [RFC3315 Section 18.1.6]
			 */
			remove_ia(ia);
		}
	}
}

static int
release_ia(ia)
	struct ia *ia;
{
	struct dhcp6_ia iaparam;
	struct dhcp6_event *ev;
	struct dhcp6_eventdata *evd;

	dprintf(LOG_DEBUG, FNAME, "release an IA: %s-%lu",
	    iastr(ia->conf->type), ia->conf->iaid);

	if ((ev = dhcp6_create_event(ia->ifp, DHCP6S_RELEASE))
	    == NULL) {
		dprintf(LOG_NOTICE, FNAME, "failed to create a new event");
		goto fail;
	}
	TAILQ_INSERT_TAIL(&ia->ifp->event_list, ev, link);

	if ((ev->timer = dhcp6_add_timer(client6_timo, ev)) == NULL) {
		dprintf(LOG_NOTICE, FNAME,
		    "failed to create a new event timer");
		goto fail;
	}

	if (duidcpy(&ev->serverid, &ia->serverid)) {
		dprintf(LOG_NOTICE, FNAME, "failed to copy server ID");
		goto fail;
	}

	if ((evd = malloc(sizeof(*evd))) == NULL) {
		dprintf(LOG_NOTICE, FNAME,
		    "failed to create a new event data");
		goto fail;
	}
	memset(evd, 0, sizeof(*evd));
	iaparam.iaid = ia->conf->iaid;
	/* XXX: should we set T1/T2 to 0?  spec is silent on this. */
	iaparam.t1 = ia->t1;
	iaparam.t2 = ia->t2;

	if (ia->ctl && ia->ctl->release_data) {
		if ((*ia->ctl->release_data)(ia->ctl, &iaparam, NULL, evd)) {
			dprintf(LOG_NOTICE, FNAME,
			    "failed to make release data");
			goto fail;
		}
	}
	TAILQ_INSERT_TAIL(&ev->data_list, evd, link);

	ev->timeouts = 0;
	dhcp6_set_timeoparam(ev);
	dhcp6_reset_timer(ev);

	if (ia->authparam != NULL) {
		if ((ev->authparam = copy_authparam(ia->authparam)) == NULL) {
			dprintf(LOG_WARNING, FNAME,
			    "failed to copy authparam");
			goto fail;
		}
	}

	client6_send(ev);
	return (0);

  fail:
	if (ev)
		dhcp6_remove_event(ev);

	return (-1);
}

static void
remove_ia(ia)
	struct ia *ia;
{
	struct ia_conf *iac = ia->conf;
	struct dhcp6_if *ifp = ia->ifp;

	dprintf(LOG_DEBUG, FNAME, "remove an IA: %s-%lu",
	    iastr(ia->conf->type), ia->conf->iaid);

	TAILQ_REMOVE(&iac->iadata, ia, link);

	duidfree(&ia->serverid);

	if (ia->timer)
		dhcp6_remove_timer(&ia->timer);

	if (ia->evdata) {
		TAILQ_REMOVE(&ia->evdata->event->data_list, ia->evdata, link);
		if (ia->evdata->destructor)
			ia->evdata->destructor(ia->evdata);
		free(ia->evdata);
		ia->evdata = NULL;
	}

	if (ia->ctl && ia->ctl->cleanup)
		(*ia->ctl->cleanup)(ia->ctl);

	if (ia->authparam != NULL)
		free(ia->authparam);

	free(ia);

	if(ia->conf->type==0) //rebind
		//system("event WAN-4.DOWN");//IOL test
	{
		char tmp[128];
		sprintf(tmp,"event %s.DOWN",xifname);
		system(tmp);
	}

	(void)client6_start(ifp);
}

static struct dhcp6_timer *
ia_timo(arg)
	void *arg;
{
	struct ia *ia = (struct ia *)arg;
	struct dhcp6_ia iaparam;
	struct dhcp6_event *ev;
	struct dhcp6_eventdata *evd;
	struct timeval timo;
	int dhcpstate;

	dprintf(LOG_DEBUG, FNAME, "IA timeout for %s-%lu, state=%s",
	    iastr(ia->conf->type), ia->conf->iaid, statestr(ia->state));

	/* cancel the current event for the prefix. */
	if (ia->evdata) {
		TAILQ_REMOVE(&ia->evdata->event->data_list, ia->evdata, link);
		if (ia->evdata->destructor)
			ia->evdata->destructor(ia->evdata);
		free(ia->evdata);
		ia->evdata = NULL;
	}

	switch (ia->state) {
	case IAS_ACTIVE:
		ia->state = IAS_RENEW;
		dhcpstate = DHCP6S_RENEW;
		timo.tv_sec = ia->t1 < ia->t2 ? ia->t2 - ia->t1 : 0;
		timo.tv_usec = 0;
		dhcp6_set_timer(&timo, ia->timer);
		break;
	case IAS_RENEW:
		ia->state = IAS_REBIND;
		dhcpstate = DHCP6S_REBIND;

		/*
		 * We need keep DUID for sending Release in this state.
		 * But we don't need a timer for the IA.  We'll just wait for a
		 * reply for the REBIND until all associated configuration
		 * parameters for this IA expire.
		 */
		dhcp6_remove_timer(&ia->timer);
		break;
	default:
		dprintf(LOG_ERR, FNAME, "invalid IA state (%d)",
		    (int)ia->state);
		return (NULL);	/* XXX */
	}

	if ((ev = dhcp6_create_event(ia->ifp, dhcpstate)) == NULL) {
		dprintf(LOG_NOTICE, FNAME, "failed to create a new event");
		goto fail;
	}
	TAILQ_INSERT_TAIL(&ia->ifp->event_list, ev, link);

	if ((ev->timer = dhcp6_add_timer(client6_timo, ev)) == NULL) {
		dprintf(LOG_NOTICE, FNAME,
		    "failed to create a new event timer");
		goto fail;
	}

	if ((evd = malloc(sizeof(*evd))) == NULL) {
		dprintf(LOG_NOTICE, FNAME,
		    "failed to create a new event data");
		goto fail;
	}
	memset(evd, 0, sizeof(*evd));
	evd->event = ev;
	TAILQ_INSERT_TAIL(&ev->data_list, evd, link);

	if (ia->state == IAS_RENEW) {
		if (duidcpy(&ev->serverid, &ia->serverid)) {
			dprintf(LOG_NOTICE, FNAME, "failed to copy server ID");
			goto fail;
		}
	}

	iaparam.iaid = ia->conf->iaid;
	iaparam.t1 = ia->t1;
	iaparam.t2 = ia->t2;
	switch(ia->state) {
	case IAS_RENEW:
		if (ia->ctl && ia->ctl->renew_data) {
			if ((*ia->ctl->renew_data)(ia->ctl, &iaparam,
			    &ia->evdata, evd)) {
				dprintf(LOG_NOTICE, FNAME,
				    "failed to make renew data");
				goto fail;
			}
		}
		break;
	case IAS_REBIND:
		if (ia->ctl && ia->ctl->rebind_data) {
			if ((*ia->ctl->rebind_data)(ia->ctl, &iaparam,
			    &ia->evdata, evd)) {
				dprintf(LOG_NOTICE, FNAME,
				    "failed to make rebind data");
				goto fail;
			}
		}
		break;
	default:
		break;
	}

	ev->timeouts = 0;
	dhcp6_set_timeoparam(ev);
	dhcp6_reset_timer(ev);

	if (ia->authparam != NULL) {
		if ((ev->authparam = copy_authparam(ia->authparam)) == NULL) {
			dprintf(LOG_WARNING, FNAME,
			    "failed to copy authparam");
			goto fail;
		}
	}

	ia->evdata = evd;

	switch(ia->state) {
	case IAS_RENEW:
	case IAS_REBIND:
#ifdef DHCPV6_LOGO
	case IAS_CNF:
	case IAS_DECLINE:
#endif
		client6_send(ev);
		break;
	case IAS_ACTIVE:
		/* what to do? */
		break;
	}

	return (ia->timer);

  fail:
	if (ev)
		dhcp6_remove_event(ev);

	return (NULL);
}

static struct ia *
get_ia(type, ifp, iac, iaparam, serverid)
	iatype_t type;
	struct dhcp6_if *ifp;
	struct ia_conf *iac;
	struct dhcp6_listval *iaparam;
	struct duid *serverid;
{
	struct ia *ia;
	struct duid newserver;
	int create = 0;

	if (duidcpy(&newserver, serverid)) {
		dprintf(LOG_NOTICE, FNAME, "failed to copy server ID");
		return (NULL);
	}

	if ((ia = find_ia(iac, type, iaparam->val_ia.iaid)) == NULL) {
		if ((ia = malloc(sizeof(*ia))) == NULL) {
			dprintf(LOG_NOTICE, FNAME, "memory allocation failed");
			duidfree(&newserver); /* XXX */
			return (NULL);
		}
		memset(ia, 0, sizeof(*ia));
		ia->state = IAS_ACTIVE;

		TAILQ_INSERT_TAIL(&iac->iadata, ia, link);
		ia->conf = iac;

		create = 1;
	} else
		duidfree(&ia->serverid);

	ia->t1 = iaparam->val_ia.t1;
	ia->t2 = iaparam->val_ia.t2;
	ia->ifp = ifp;
	ia->serverid = newserver;

	dprintf(LOG_DEBUG, FNAME, "%s an IA: %s-%lu",
	    create ? "make" : "update", iastr(type), ia->conf->iaid);

	return (ia);
}

static struct ia *
find_ia(iac, type, iaid)
	struct ia_conf *iac;
	iatype_t type;
	u_int32_t iaid;
{
	struct ia *ia;

	for (ia = TAILQ_FIRST(&iac->iadata); ia;
	    ia = TAILQ_NEXT(ia, link)) {
		if (ia->conf->type == type && ia->conf->iaid == iaid)
			return (ia);
	}

	return (NULL);
}

static char *
iastr(type)
	iatype_t type;
{
	switch (type) {
	case IATYPE_PD:
		return ("PD");
	case IATYPE_NA:
		return ("NA");
	default:
		return ("???");	/* should be a bug */
	}
}

static char *
statestr(state)
	iastate_t state;
{
	switch (state) {
	case IAS_ACTIVE:
		return "ACTIVE";
	case IAS_RENEW:
		return "RENEW";
	case IAS_REBIND:
		return "REBIND";
	default:
		return "???";	/* should be a bug */
	}
}
