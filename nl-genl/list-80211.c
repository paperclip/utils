#include <stdio.h>
#include <errno.h>

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

#include <linux/nl80211.h>

static int finish_handler(struct nl_msg *msg, void *arg)
{
    int *ret = arg;
    *ret = 0;
    return NL_SKIP;
}

static int list_interface_handler(struct nl_msg *msg, void *arg)
{
    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);

    if (tb_msg[NL80211_ATTR_IFNAME])
        printf("Interface: %s\n", nla_get_string(tb_msg[NL80211_ATTR_IFNAME]));

    return NL_SKIP;
}

int main(void)
{
    struct {
        struct nl_sock *nls;
        int nl80211_id;
    } wifi;

    wifi.nls = nl_socket_alloc();
    if (!wifi.nls) {
        fprintf(stderr, "Failed to allocate netlink socket.\n");
        return EXIT_FAILURE;
    }

    int ret = EXIT_SUCCESS;

    nl_socket_set_buffer_size(wifi.nls, 8192, 8192);

    if (genl_connect(wifi.nls)) {
        fprintf(stderr, "Failed to connect to generic netlink.\n");
        ret = EXIT_FAILURE;
        goto out_connect;
    }

    wifi.nl80211_id = genl_ctrl_resolve(wifi.nls, "nl80211");
    if (wifi.nl80211_id < 0) {
        fprintf(stderr, "nl80211 not found.\n");
        ret = EXIT_FAILURE;
        goto out_connect;
    }

    struct nl_msg *msg = nlmsg_alloc();
    if (!msg) {
        fprintf(stderr, "Failed to allocate netlink message.\n");
        ret = EXIT_FAILURE;
        goto out_connect;
    }

    struct nl_cb *cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        fprintf(stderr, "Failed to allocate netlink callback.\n");
        ret = EXIT_FAILURE;
        goto out_alloc;
    }

    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, list_interface_handler, NULL);

    genlmsg_put(msg, 0, 0, wifi.nl80211_id, 0,
            NLM_F_DUMP, NL80211_CMD_GET_INTERFACE, 0);

    nl_send_auto_complete(wifi.nls, msg);

    int err = 1;

    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);

    while (err > 0)
        nl_recvmsgs(wifi.nls, cb);

    nl_cb_put(cb);

out_alloc:
    nlmsg_free(msg);
out_connect:
    nl_socket_free(wifi.nls);

    return ret;
}
