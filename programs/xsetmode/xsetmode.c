/* $XFree86$ */

#include <stdio.h>
#include <X11/Xproto.h>
#include <X11/extensions/XInput.h>

int           event_type;

int
main(int argc, char * argv[])
{
  int           loop, num_extensions, num_devices;
  char          **extensions;
  XDeviceInfo   *devices;
  Window        win;
  Display       *dpy;
  Window        root_win;
  unsigned long screen;
  XEvent        Event;
  int		list = 0;
  
  if (argc != 3) {
    fprintf(stderr, "usage : %s <device name> (ABSOLUTE|RELATIVE)\n", argv[0]);
    exit(1);
  }

  if (strcmp(argv[1], "-l") == 0) {
    list = 1;
  }
  
  dpy = XOpenDisplay(NULL);

  if (!dpy) {
    printf("unable to connect to X Server try to set the DISPLAY variable\n");
    exit(1);
  }

#ifdef DEBUG
  printf("connected to %s\n", XDisplayString(dpy));
#endif

  screen = DefaultScreen(dpy);
  root_win = RootWindow(dpy, screen);

  extensions = XListExtensions(dpy, &num_extensions);
  for (loop = 0; loop < num_extensions &&
         (strcmp(extensions[loop], "XInputExtension") != 0); loop++);
  XFreeExtensionList(extensions);
  if (loop != num_extensions)
    {
      devices = XListInputDevices(dpy, &num_devices);
      for(loop=0; loop<num_devices; loop++)
        {
          if (devices[loop].name &&
              (strcasecmp(devices[loop].name, argv[1]) == 0))
            if ((devices[loop].use == IsXExtensionDevice) ||
		(devices[loop].use == IsXPointer))
              {
                XDevice *device;
              
#ifdef DEBUG
                fprintf(stderr, "opening device %s\n",
                        devices[loop].name ? devices[loop].name : "<noname>");
#endif
                device = XOpenDevice(dpy, devices[loop].id);
                if (device)
                  {
		    XSetDeviceMode(dpy, device, (strcmp("ABSOLUTE", argv[2]) == 0) ? Absolute
				   : Relative);
                    exit(0);
                  }
                else
                  {
                    fprintf(stderr, "error opening device\n");
                    exit(1);
                  }
              }
        }
      XFreeDeviceList(devices);
    }
  else
    {
      fprintf(stderr, "No XInput extension available\n");
      exit(1);
    }
  
  if (list) {
    exit(0);
  }
  else {
    fprintf(stderr, "No device found\n");
    exit(1);
  }
}
