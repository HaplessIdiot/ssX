# Super Sonic X: The X Server for Super Sonic Freedom

**Super Sonic X** is a high-performance, hardened fork of the final **XFree86** lineage. This project exists to reclaim the desktop from GPU-centric corporate churn and return it to the users of embedded systems, retro hardware, and lean workstations.

We are restoring the classic 2D rendering pipeline and preserving **XAA (XFree86 Acceleration Architecture)** as a primary engine—not a "legacy" feature marked for deletion.

---

### The NetBSD Foundation
This project is built upon the monumental, often-overlooked work of the **NetBSD Project**. While the mainstream Linux ecosystem abandoned XFree86 in the mid-2000s, the NetBSD developers maintained, secured, and refined this codebase within their `xsrc` tree until **2015**. 

Super Sonic X is an enshrining of that work. We recognize NetBSD as the final, true stewards of the XFree86 architecture. Without their decade of "quiet" maintenance, this codebase would have been lost to bit-rot. **We carry their torch forward.**

---

### Standing on the Shoulders of Giants
This fork is a tribute to the hackers who did the real work in the trenches:
* **The NetBSD `xsrc` Maintainers:** For keeping the fire burning until 2015.
* **Alan Coopersmith & Friends:** For the architectural brilliance that defined the X11 era.
* **The Original XFree86 Team:** For building the foundation that still outperforms modern "composed" stacks on real-world 2D hardware.

---

### The Mission
* **No Corporate Bureaucracy:** We don't answer to Freedesktop.org or IBM/Red Hat. We answer to the hardware.
* **XAA Restoration:** Bringing back first-class acceleration for chips that don't do "Glamor."
* **Lean & Mean:** If a feature requires a massive dependency tree just to draw a window, it doesn't belong here.

---

### Repository Info
* **Upstream:** XFree86 4.x via NetBSD `xsrc`
* **Focus:** 2D Acceleration, Stability, and Low-Latency Input.
* **Contribution:** Send us clean code. We value technical merit and hardware support over "Contributor Covenants" and corporate boilerplate BS
