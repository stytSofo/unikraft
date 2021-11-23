# FlexOS: Making OS Isolation Flexible

Warning: this README is outdated. We will provide up-to-date documentation
as part of our ASPLOS'22 AE submission.

## Installing from the Docker container

Clone this repository and build the Docker container:

```
$ git clone git@github.com:project-flexos/unikraft.git
$ pushd unikraft/docker
$ docker build -f flexos.dockerfile --tag flexos-dev .
$ popd
```

Run the container as following:

```
$ docker run --privileged -ti flexos-dev bash
```

## Installing from source

Should work on a recent enough Debian-based distro.

Install a recent-enough Coccinelle:

```
$ git clone https://github.com/coccinelle/coccinelle
$ apt-get build-dep coccinelle
$ pushd coccinelle
$ git checkout 99ec612852a88fae85dfad863daafccf55b874ce
$ ./autogen
$ ./configure
$ make
$ sudo make install
$ popd
$ # workaround a Coccinelle bug, if this fails don't worry...
$ mkdir /usr/local/bin/lib
$ ln -s /usr/local/lib/coccinelle /usr/local/bin/lib/coccinelle
```

Install `kraft` for FlexOS:

```
$ git clone git@github.com:project-flexos/kraft.git
$ pushd kraft
$ pip3 install -e .
$ popd
```

Configure `kraft` for FlexOS:

```
$ cat > ~/.kraftrc<< EOF
[fetch]
prioritise_origin = true
mirrors = [
  "https://releases.unikraft.org/mirrors",
]

[configure]
platform = "kvm"
architecture = "x86_64"

[list]
origins = [
  "git@github.com:project-flexos/lib-newlib.git",
  "git@github.com:project-flexos/lib-tlsf.git",
  "git@github.com:project-flexos/app-flexos-example.git",
  "git@github.com:project-flexos/app-flexos-microbenchmarks.git",
  "git@github.com:project-flexos/lib-flexos-example.git",
  "git@github.com:project-flexos/lib-flexos-microbenchmarks.git",
  "git@github.com:project-flexos/lib-lwip.git",
  "git@github.com:project-flexos/lib-nginx.git",
  "git@github.com:project-flexos/app-nginx.git",
  "git@github.com:project-flexos/lib-redis.git",
  "git@github.com:project-flexos/app-redis.git",
  "git@github.com:project-flexos/lib-iperf.git",
  "git@github.com:project-flexos/app-iperf.git",
  "git@github.com:project-flexos/unikraft.git",
  "git@github.com:project-flexos/lib-pthread-embedded.git"
]
EOF
```

Setup the FlexOS build environment:

```
$ kraft list update
$ kraft -v list pull flexos-microbenchmarks@staging iperf@staging \
		  newlib@staging tlsf@staging flexos-example@staging \
		  lwip@staging redis@staging unikraft@staging \
		  pthread-embedded@staging nginx@staging
$ cd ~/.unikraft/ && ls
apps  archs  libs  plats  unikraft
```

## Building FlexOS

Applies to both Docker & installation from source.

Configure Redis with two compartments:

```
$ cd ~/.unikraft/apps/redis
$ cat ./kraft.yaml
---
specification: '0.6'
name: redis
unikraft:
  version: staging
  kconfig:
    - CONFIG_LIBUK9P=y
    - CONFIG_LIB9PFS=y
    - CONFIG_LIBDEVFS=y
    - CONFIG_LIBDEVFS_AUTOMOUNT=y
    - CONFIG_LIBVFSCORE_AUTOMOUNT_ROOTFS=y
    - CONFIG_LIBVFSCORE_ROOTFS_RAMFS=y
    - CONFIG_LIBUKLIBPARAM=y
    - CONFIG_LIBUKDEBUG=y
    - CONFIG_LIBUKALLOC=y
    - CONFIG_LIBUKSCHED=y
    - CONFIG_LIBPOSIX_SYSINFO=y
    - CONFIG_LIBPOSIX_LIBDL=y
    - CONFIG_LIBFLEXOS=y
targets:
  - architecture: x86_64
    platform: kvm
compartments:
  - name: comp1
    mechanism:
      driver: intel-pku
      noisolstack: false
    default: true
  - name: comp2
    mechanism:
      driver: intel-pku
      noisolstack: false
libraries:
  tlsf:
    version: staging
    kconfig:
      - CONFIG_LIBTLSF=y
  pthread-embedded:
    version: staging
    compartment: comp1
  newlib:
    version: staging
    kconfig:
      - CONFIG_LIBNEWLIBC=y
      - CONFIG_LIBNEWLIBC_WANT_IO_C99_FORMATS=y
      - CONFIG_LIBNEWLIBC_LINUX_ERRNO_EXTENSIONS=y
    compartment: comp1
  lwip:
    version: staging
    kconfig:
      - CONFIG_LWIP_IPV6=y
    compartment: comp2
  redis:
    version: staging
    kconfig:
      - CONFIG_LIBREDIS_SERVER=y
      - CONFIG_LIBREDIS_COMMON=y
      - CONFIG_LIBREDIS_LIBREDIS_LUA=y
      - CONFIG_LIBREDIS_SERVER_MAIN_FUNCTION=y
    compartment: comp1
volumes: {}
networks: {}
$ kraft configure
```

Now we have a fully set up system. We only have to build and run. The following commands are what you would run as part of your development workflow.

Build Redis with two compartments:

```
$ make prepare && kraft -v build --compartmentalize '-j'
```

Run the freshly built image:

```
$ kraft run --initrd ./redis.cpio -M 1024 ""
```

## FlexOS `kraft.yaml` primer

### Declaring Compartments

Declare compartments in `compartments`:

```
compartments:
  - name: comp1
    mechanism:
      driver: intel-pku
      noisolstack: false
    default: true
  - name: comp2
    mechanism:
      driver: intel-pku
      noisolstack: false
```

Each compartment has a `name` and a `mechanism`.

Each mechanism has a driver (for Intel MPK/PKU it's `intel-pku`, for VM-based `vmept`, for simple function calls `fcalls`), and possibly a number of driver-specific options. Intel MPK/PKU, for instance, can isolate or share the stack (`noisolstack`).

There is one `default` compartment. All libraries that have not been assigned a specific compartment will go into the default compartment.

:warning: If you declare a compartment, make sure to actually use it! That is, it should either be default or used by a library (see below).

### Assigned compartments to libraries

Libraries can be assigned a compartment using `compartment`:

```
libraries:
[...]
  lwip:
    version: staging
    kconfig:
      - CONFIG_LWIP_IPV6=y
    compartment: comp2
```

The value has to match the name of a previously declared compartment.

Note that internal libraries (the ones whose source live under `unikraft/`) require an additional `is_core: true` parameter:

```
libraries:
[...]
  uksched:
    is_core: true
    compartment: comp2
  ukschedcoop:
    is_core: true
    compartment: comp2
```

:warning: always put uksched and ukschedcoop together.

## Development Workflow

`kraft` rewrites the source code of microlibraries **in-place** to implement isolation primitives. If you change the isolation profile of the image (by editing `kraft.yaml`), make sure to thoroughly cleanup your setup.

Here is a script that does it:

```
#!/bin/bash

git checkout .
make properclean
# git checkout . && git clean -xdf in all repositories would be fine too
rm -rf ~/.kraftcache ~/.unikraft/libs ~/.unikraft/unikraft
kraft list update
kraft list pull flexos-microbenchmarks@staging iperf@staging newlib@staging \
		tlsf@staging flexos-example@staging lwip@staging redis@staging \
		unikraft@staging pthread-embedded@staging nginx@staging
```

I recommend putting this in `/usr/local/bin/kraftcleanup`.

The usual workflow is then:

```
$ kraftcleanup # clean
$ make prepare && kraft -v build --compartmentalize '-j' # compile
$ kraft run [...] # run
```

If you didn't change `kraft.yaml`, a simple `make -j` should do the trick.

:warning: Once again, if you do not run `kraftcleanup`, running `kraft build --compartmentalize` **will not** rewrite your code again because the rewriting is done in-place!

:warning: `kraftcleanup` will **erase any modification done in your build repositories**! Make sure to have separate repositories that you use for the actual development otherwise you might **loose data**!

## Backend-specific instructions

### Intel MPK/PKU

:warning: You can build on any machine, but **running requires a CPU that supports MPK**!

Generally, if you don't have a Xeon Bronze/Silver/Gold/Platinum, you don't have MPK. You can try running the image, if your CPU doesn't support MPK, FlexOS will abort with an appropriate error message.

### VM/EPT

For the shared memory on KVM, we use a simple shared memory device in QEMU. To add this new device to QEMU, it is necessary to compile it from source code and add a patch. More information on how to build QEMU for Linux can be found [here](https://wiki.qemu.org/Hosts/Linux).
Steps for building QEMU for x86_64 architecture, with support for the shared memory device:

```
$ git clone https://github.com/qemu/qemu.git
$ cd qemu
$ git apply 0001-Myshmem.patch
$ ./configure --target-list=x86_64-softmmu
$ vim build/build.ninja # add -lrt to LINK_ARGS for the target qemu-system-x86_64
$ make -j8
$ build/qemu-system-x86_64 -device myshmem,file=/test,size=0x1000,paddr=0x10000000 # example run to check if the device works
```

The above built QEMU binary should be used to run all Unikraft images that use the VM/EPT backend in FlexOS. You can find 0001-Myshmem.patch in this repository under `flexos-support`.

Additional remark regarding the `LINK_ARGS` edit. The relevant line is the one that begins with `LINK_ARGS = `, following the the line that begins with `build qemu-system-x86_64`.

To build the simple [example application](https://github.com/project-flexos/app-flexos-example), it has to be cloned twice. The two applications, in the `apps/` folder, could have, for example, the names `app-flexos-example` and `rpc-flexos-example`.

Currently, to build any VM/EPT image, the lwip library has to be included in the build (there are wrappers written for functions from lwip, which yield undefined symbols otherwise). TODO: add compiler guards to wrappers to mitigate this issue. To include the lwip library to the build, add `$(UK_LIBS)/lwip` to the `Makefile` and adding `lwip` to the `kraft.yaml`, putting it in `comp1` (make sure to do this in both folders - app and rpc). Also, all internal libraries that have wrapper functions defined (for example, `ukswrand`) also have to be included in the build, in `make menuconfig`.

Steps to build the VM/EPT FlexOS (beginning from `~/.unikraft/apps`):
```
$ cd app-flexos-example
$ make menuconfig # select VM/EPT in Library Configuration -> flexos -> FlexOS backend and also the KVM platform in Platform Configuration
$ make fetch
$ kraft -v build --compartmentalize
$ cd ../rpc-flexos-example
$ make fetch
$ make menuconfig # select VM/EPT in Library Configuration -> flexos -> FlexOS backend
		  # AND select 'Build a library compartment (not main app)'
$ kraft -v build --compartmentalize
```

To run the application, the RPC server has to be run first and then the application (first you run `rpc-flexos-example` and then `app-flexos-example`).
Here is an example command for running the built compartments:
```
<PATH TO QEMU BUILT EARLIER>/qemu/build/qemu-system-x86_64 -enable-kvm -nographic -device isa-debug-exit -gdb tcp::1237 \
	-device myshmem,file=/rpc,paddr=0x800000000,size=0x100000 \
	-device myshmem,file=/heap,paddr=0x4000000000,size=0x8000000 \
	-device myshmem,file=/data_shared,paddr=0x105000,size=<size of data_shared section> \
	-kernel $KERNEL \
	-m 2G \
```

Replace the path to QEMU with the path on your system. To find out the size of the data_shared section, run the following command on the built compartments:
```
$ readelf -SW ~/.unikraft/apps/app-flexos-example/build/app-flexos-example_kvm-x86_64.dbg
[...]
Section Headers:
  [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            0000000000000000 000000 000000 00      0   0  0
  [ 1] .multiboot_header PROGBITS        0000000000100000 001000 005000 00  WA  0   0 4096
  [ 2] .data_shared      PROGBITS        0000000000105000 006000 003000 00  WA  0   0 32
  [ 3] .text             PROGBITS        0000000000108000 009000 0e7604 00 WAX  0   0  1
[...]
```

### Function Calls (Debugging Backend)

The `fcalls` debugging backend will replace all gates with simple function calls. You can build it on any machine and run it on any machine.

## Porting Tips & Tricks

### Gate insertion with MPK (and other intra-AS isolation technologies)

The first part of porting a library to run as isolated component is to insert gates.

It can be hard to ensure that all function calls to the component's API are wrapped with gates if the component doesn't have much internal data; without internal data accesses, technologies such as MPK might not trigger a crash if a gate is missing.

In order to ensure that *all* function calls to the components API trigger a crash if a gate is missing, we can leverage GCC's instrumentation `-finstrument-functions`. With this instrumentation, we can make sure that each function of the target library writes to a library local variable before executing. If a gate is missing, it will crash.

For this you simply have to add the compiler flag to the relevant library, e.g., for uktime:

```
diff --git a/lib/uktime/Makefile.uk b/lib/uktime/Makefile.uk
index 8e05ed7..ca1f085 100644
--- a/lib/uktime/Makefile.uk
+++ b/lib/uktime/Makefile.uk
@@ -12,6 +12,7 @@ LIBUKTIME_SRCS-y += $(LIBUKTIME_BASE)/musl-imported/src/__tm_to_secs.c
 LIBUKTIME_SRCS-y += $(LIBUKTIME_BASE)/musl-imported/src/__year_to_secs.c
 LIBUKTIME_SRCS-y += $(LIBUKTIME_BASE)/time.c
 LIBUKTIME_SRCS-y += $(LIBUKTIME_BASE)/timer.c
+LIBUKTIME_CFLAGS-y += -finstrument-functions -finstrument-functions-exclude-function-list=__cyg_profile_func_enter,__cyg_profile_func_exit
 
 UK_PROVIDED_SYSCALLS-$(CONFIG_LIBUKTIME) += nanosleep-2
 UK_PROVIDED_SYSCALLS-$(CONFIG_LIBUKTIME) += clock_gettime-2
```

Then define the two instrumentation functions:

```
diff --git a/lib/uktime/musl-imported/include/time.h b/lib/uktime/musl-imported/include/time.h
index 7cfcdba..18125a0 100644
--- a/lib/uktime/musl-imported/include/time.h
+++ b/lib/uktime/musl-imported/include/time.h
@@ -7,6 +7,11 @@ extern "C" {
 
 #include <uk/config.h>
 
+void __cyg_profile_func_enter (void *this_fn,
+                               void *call_site);
+void __cyg_profile_func_exit  (void *this_fn,
+                               void *call_site);
+
 #define __NEED_size_t
 #define __NEED_time_t
 #define __NEED_clock_t
diff --git a/lib/uktime/time.c b/lib/uktime/time.c
index 9290635..3917f39 100644
--- a/lib/uktime/time.c
+++ b/lib/uktime/time.c
@@ -51,6 +51,19 @@
 #endif
 #include <uk/essentials.h>
 
+volatile int uktime_local;
+
+void __cyg_profile_func_enter (void *this_fn,
+                               void *call_site)
+{
+       uktime_local = 0;
+}
+void __cyg_profile_func_exit  (void *this_fn,
+                               void *call_site)
+{
+       uktime_local = 1;
+}
+
```

Then compile as usual. The image will crash if gates are missing.
