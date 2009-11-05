#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

#define MD_PREFIX "TUIO "
#define PDEBUG(msg, ...) if(debug) printf("TMD: "msg,##__VA_ARGS__);

typedef struct _attach_info {
    struct _attach_info *next;

    int slaveid;
    int masterid;
    char *m_name;
} AttachInfo, *AttachInfoPtr;

static AttachInfo ais;
static int debug = 0;
static int foreground = 1;

/**
 * Removes a master device.
 */
int remove_master(Display* dpy, int master_id) {
    XIRemoveMasterInfo c;

    c.type = XIRemoveMaster;
    c.deviceid = master_id;
    c.return_mode = XIFloating;

    return XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&c, 1);
}

/**
 * Creates a new master device.
 */
int create_master(Display* dpy, char* name) {
    XIAddMasterInfo c;

    c.type = XIAddMaster;
    c.name = name;
    c.send_core = 1;
    c.enable = 1;

    return XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&c, 1);
}

/**
 * Attaches a slave device to a master device.
 */
int change_attachment(Display* dpy, int slaveid, int masterid) {
    XIAttachSlaveInfo c;

    c.type = XIAttachSlave;
    c.deviceid = slaveid;
    c.new_master = masterid;

    return XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&c, 1);
}

/**
 * Listens for the creation of slave and master devices.  When a new slave
 * device is found, a new master device will be created and attached to
 * when the message for the new master device comes back around.
 */
void listen(Display *dpy, int xi_opcode) {
    int num_devices;
    unsigned char hmask[2] = { 0, 0 };
    char *m_name, *s_name;
    XEvent ev;
    XIDeviceInfo *info;
    XIEventMask evmask_h;
    AttachInfoPtr ai;

    //Cursor m_cur;
    //m_cur = XCreateFontCursor(dpy, 88);
    //if (m_cur == BadAlloc || m_cur == BadValue) {
        //printf("Unable to create cursor\n");
    //}

    evmask_h.mask = hmask;
    evmask_h.mask_len = sizeof(hmask);
    evmask_h.deviceid = XIAllDevices;
    XISetMask(hmask, XI_HierarchyChanged);
    XFlush(dpy);

    XISelectEvents(dpy, DefaultRootWindow(dpy), &evmask_h, 1);

    PDEBUG("Listening for events...\n");

    /* Start listening for events... */
    while (1) {
        XGenericEventCookie *cookie;
        cookie = &ev.xcookie;
        XNextEvent(dpy, &ev);

        PDEBUG("Event Received\n");

        if (cookie->type != GenericEvent ||
                cookie->extension != xi_opcode)
            continue;

        if (cookie->evtype == XI_HierarchyChanged) {
            int i;
            XIHierarchyEvent *event = cookie->data;

            if (event->flags & XISlaveAdded) {
                PDEBUG("  Slave added\n");

                for (i=0; i < event->num_info; i++) {
                    if (event->info[i].flags & XISlaveAdded) {
                        info = XIQueryDevice(dpy, event->info[i].deviceid, &num_devices);
                        if (num_devices != 1) {
                            PDEBUG("    Couldn't find slave device (XIQueryDevice)\n");
                            XIFreeDeviceInfo(info);
                            continue;
                        }
                        s_name = info->name;

                        PDEBUG("    Found: %s\n", s_name);

                        /* This is a hack.  Xtst slave devices are created and
                         * attached to each new master device within X.  If we 
                         * were to actually create a new master device for this
                         * device we would end up with a recurring creation of
                         * master devices and slave devices. Also, tuio subdevices
                         * will have the word "subdev" in them. */
                        if (strstr(s_name, "XTEST") ||
                                !strstr(s_name, "subdev")) {
                            PDEBUG("      Device is NOT a tuio subdevice\n");
                            XIFreeDeviceInfo(info);
                            continue;
                        } else {
                            PDEBUG("      Device is a tuio subdevice, "
                                    "creating Master Device\n");
                        }

                        asprintf(&m_name, MD_PREFIX "%s", s_name);
                        create_master(dpy, m_name);
                        free(m_name);

                        ai = malloc(sizeof(AttachInfo));
                        ai->next = ais.next;
                        ais.next = ai;

                        /* The new pointer will have the suffix " pointer" */
                        asprintf(&ai->m_name, MD_PREFIX "%s pointer", s_name);
                        ai->slaveid = event->info[i].deviceid;
                        XIFreeDeviceInfo(info);
                    }
                }
            }
            
            if (event->flags & XIMasterAdded) {
                PDEBUG("  Master added\n");

                for (i=0; i < event->num_info; i++) {
                    if(event->info[i].flags & XIMasterAdded) {
                    
                        info = XIQueryDevice(dpy, event->info[i].deviceid, &num_devices);
                        if (num_devices != 1) {
                            PDEBUG("    Couldn't find master device (XIQueryDevice)\n");
                            XIFreeDeviceInfo(info);
                            continue;
                        }
                        m_name = info->name;

                        PDEBUG("    Found: %s, reattaching\n", m_name);

                        ai = ais.next;
                        while (ai != NULL) {
                            if (strcmp(ai->m_name, m_name) == 0) {
                                PDEBUG("      New master found in internal list\n");
                                change_attachment(dpy, ai->slaveid, event->info[i].deviceid);
                                ai->masterid = event->info[i].deviceid;
                                //XIDefineCursor(dpy, event->info[i].deviceid,
                                               //DefaultRootWindow(dpy), m_cur);
                            } 
                            ai = ai->next;
                        }
                        XIFreeDeviceInfo(info);
                    }
                }
            }

            if (event->flags & XISlaveRemoved) {
                PDEBUG("  Slave removed\n");
                for (i=0; i < event->num_info; i++) {
                    if(event->info[i].flags & XISlaveRemoved) {
                        int m_id;
                        AttachInfo *ai_last;

                        ai_last = &ais;
                        ai = ais.next;
                        while (ai != NULL) {
                            if (ai->slaveid == event->info[i].deviceid) {

                                /* Remove corresponding master device */
                                m_id = ai->masterid;
                                PDEBUG("    Removing Master: %d\n", m_id);
                                remove_master(dpy, m_id);

                                ai_last->next = ai->next;

                                free(ai->m_name);
                                free(ai);
                            }
                            ai_last = ai;
                            ai = ai->next;
                        }
                    }
                }
            }
        }
        XFreeEventData(dpy, &ev);
    }
}

/**
 * Forks the process sets
 */
int daemonize() {
    int pid;

    pid = fork();
    if (pid < 0) {
        printf("Failed to fork\n");
        return -1;
    } else if (pid > 0) {
        exit(0);
    }

    umask(0);

    if ((chdir("/")) < 0) {
        return -1;
    }

    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
}

/**
 * Prints help information.
 */
void printHelp(char* bin_name) {
    printf("Usage: %s [OPTION...]\n\n", bin_name);
    printf("  -d\tenables debugging output\n");
    printf("  -f\truns process in foreground\n");
    printf("  -h\tprints this help message\n");
}

/**
 * Processes application arguments.
 */
void processArgs(int argc, char **argv) {
    int i;

    for (i=1; i<argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            debug = 1;
            PDEBUG("Debug on\n");
        } else if (strcmp(argv[i], "-f") == 0) {
            foreground = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            printHelp(argv[0]);
            exit(0);
        }
    }
}

int main (int argc, char **argv) {
    Display *dpy;
    int xi_opcode, event, error;
    int major, minor;
    int ret;

    ais.next = NULL;

    processArgs(argc, argv);
    if (!foreground)
        daemonize();

    PDEBUG("Connecting to X\n");
    /* Open X display */
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        printf("Failed to open display.\n");
        return -1;
    }

    /* Make sure the X Input extension is available */
    if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error)) {
        printf("X Input extension not available.\nExiting\n");
        return -1;
    }

    /* Check XI version, need 2.0 */
    major = 2;
    minor = 0;
    ret = XIQueryVersion(dpy, &major, &minor);
    if (ret == BadRequest) {
        printf("XInput 2.0 not supported.  Server supports %i.%i.\nExiting\n", major, minor);
        return -1;
    }

    listen(dpy, xi_opcode);

    XCloseDisplay(dpy);
    return 0;
}
