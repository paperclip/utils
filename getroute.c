/* This is an example of using rtnetlink directly to ask the kernel for the
 * default route for an interface. To accomplish this we ask for a dump of the
 * routing table and for each entry see if it matches our interface index. If
 * it does, we find the highest-metric route that has no destination (is a
 * default route) and then we print that.
 *
 * Andrey Yurovsky <yurovsky@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <asm/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <net/if.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <interface>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    unsigned int interface = if_nametoindex(argv[1]);
    if (interface == 0) {
        fprintf(stderr, "No interface named \"%s\"\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    int s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    assert(s != -1);

    struct {
        struct nlmsghdr nh;
        struct ifinfomsg inf;
    } req = {
        .nh.nlmsg_len = NLMSG_LENGTH(sizeof(req.inf)),
        .nh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST,
        .nh.nlmsg_type = RTM_GETROUTE,
        .nh.nlmsg_seq = 1,
        .nh.nlmsg_pid = getpid(),
        .inf.ifi_flags = IFF_UP,
        .inf.ifi_family = AF_UNSPEC, /* or AF_INET for v4 or AF_INET6 for v6 */
        .inf.ifi_type = ARPHRD_ETHER,
    };

    send(s, &req, req.nh.nlmsg_len, 0);

    char buf[32768];
    ssize_t nr = recv(s, buf, sizeof(buf), 0);
    close(s);

    struct {
        char addr[INET6_ADDRSTRLEN];
        unsigned int priority;
    } default_route = {
        .addr = { 0 },
        .priority = 0,
    };

    struct nlmsghdr *h = (struct nlmsghdr *)buf;

    /* Parse each message */
    for(; NLMSG_OK(h, nr); h = NLMSG_NEXT(h, nr)) {
        struct rtmsg *msg = (struct rtmsg *)NLMSG_DATA(h);

        if (msg->rtm_family != AF_INET || msg->rtm_table != RT_TABLE_MAIN)
            continue;

        struct rtattr *attr = (struct rtattr *)RTM_RTA(msg);
        int len = RTM_PAYLOAD(h);

        struct {
            bool match;
            char dst[INET6_ADDRSTRLEN];
            char gateway[INET6_ADDRSTRLEN];
            unsigned int priority;
        } report = {
            .match      = false,
            .dst        = { 0 },
            .gateway    = { 0 },
            .priority   = 0,
        };

        /* Parse each attribute */
        for(; RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
            switch (attr->rta_type) {
                case RTA_DST:
                    inet_ntop((attr->rta_len > 8) ? AF_INET6 : AF_INET,
                            RTA_DATA(attr), report.dst, sizeof(report.dst));
                    break;

                case RTA_PRIORITY:
                    report.priority = *(unsigned int *)RTA_DATA(attr);
                    break;

                /* We want the output interface index to match the interface
                 * we're looking up */
                case RTA_OIF:
                    if (*(unsigned int *)RTA_DATA(attr) == interface)
                        report.match = true;
                    break;

                case RTA_GATEWAY:
                    inet_ntop((attr->rta_len > 8) ? AF_INET6 : AF_INET,
                            RTA_DATA(attr), report.gateway, sizeof(report.gateway));
                    break;

                default:
                    break;
            }
        }

        /* If this message is for our interface (match) and has a gateway,
         * store it so long as it has a higher metric than what we already
         * know (if anything) */
        if (report.match && !strlen(report.dst) && strlen(report.gateway)) {
            if (report.priority > default_route.priority) {
                strcpy(default_route.addr, report.gateway);
                default_route.priority = report.priority;
            }
        }
    }

    printf("Default route for %s is %s\n", argv[1],
            strlen(default_route.addr) ? default_route.addr : "unknown");

    return EXIT_SUCCESS;
}
