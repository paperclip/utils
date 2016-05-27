#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

static int filter_ttys(const struct dirent *de)
{
    return (strlen(de->d_name) >= 6 && !strncmp(de->d_name, "ttyUSB", 6));
}

static char *load_tty_device(char *path)
{
    struct dirent **namelist;
    char *name = NULL;

    int n = scandir(path, &namelist, filter_ttys, alphasort);
    for (int i = 0; i < n; i++) {
        if (!name &&
            strlen(namelist[i]->d_name) >= 7 &&
            !strncmp(namelist[i]->d_name, "ttyUSB", 6)) {
            name = strdup(namelist[i]->d_name);
        }
        free(namelist[i]);
    }
    free(namelist);

    return name;
}

static int filter_interfaces(const struct dirent *de)
{
    unsigned int d0, d1, d2, d3;

    return (strlen(de->d_name) >= 7 &&
            sscanf(de->d_name, "%u-%u:%u.%u", &d0, &d1, &d2, &d3) == 4);
}

bool load_usb_device(char *name, char *sn)
{
    bool found = false;

    /* Look through all interfaces to find TTY devices. */
    struct dirent **namelist;
    int n = scandir(name, &namelist, filter_interfaces, alphasort);
    for (int i = 0; i < n; i++) {
        char *path = NULL;
        if (asprintf(&path, "%s/%s", name, namelist[i]->d_name) > 0) {
            char *dev = load_tty_device(path);
            if (dev) {
                /* Read the serial number */
                char *spath = NULL;
                if (asprintf(&spath, "%s/serial", name) > 0) {
                    FILE *f = fopen(spath, "r");

                    char buf[255];
                    size_t nr = fread(buf, 1, sizeof(buf), f);
                    fclose(f);

                    /* Make sure it's a C string, and chop off the newline */
                    buf[nr] = '\0';
                    char *c = strchr(buf, '\n');
                    if (c)
                        *c = '\0';

                    if (sn && !strcmp(buf, sn)) {
                        /* We were looking for a serial number and this one
                         * matches. Display the corresponding device. */
                        printf("%s\n", dev);
                        found = true;
                    } else if (!sn) {
                        /* We're looking at all devices and this one looks like
                         * a TTY so display the serial number and device. */
                        printf("%s %s\n", buf, dev);
                    }

                    free(spath);
                }
                free(dev);
            }
            free(path);
        }
        free(namelist[i]);
    }

    return found;
}

static int filter_devices(const struct dirent *de)
{
    unsigned int d0, d1;

    return (strlen(de->d_name) >= 3 &&
            strchr(de->d_name, ':') == NULL &&
            sscanf(de->d_name, "%u-%u", &d0, &d1) == 2);
}

static bool scan_devices(char *sn)
{
    bool found = false;
    struct dirent **namelist;
    int n = scandir("/sys/bus/usb/devices", &namelist, filter_devices, alphasort);
    for (int i = 0; i < n; i++) {
        if (!found) {
            char *path = NULL;
            if (asprintf(&path, "/sys/bus/usb/devices/%s",
                        namelist[i]->d_name) > 0) {
                found = load_usb_device(path, sn);
                free(path);
            }
        }
        free(namelist[i]);
    }

    free(namelist);

    return found;
}

int main(int argc, char **argv)
{
    if (argc >= 2) {
        return scan_devices(argv[1]) ? 0 : 1;
    } else {
        scan_devices(NULL);
        return 0;
    }
}
