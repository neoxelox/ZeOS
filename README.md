# `ZeOS`

**`SO2-FIB Operating System project`**

# Credits

- [**`Alex Rodríguez`**](https://github.com/neoxelox)
- [**`Isma Quiñones`**](https://github.com/ismaqg)

# Setup

This project uses the [**`Bochs`**](http://bochs.sourceforge.net/) emulator to run the Operating System.

You can try to manually install all the dependencies or use the prebuilt Docker image:

- Install [**`docker 19.03.6 >=`**](https://docs.docker.com/get-docker/)
- Run **`chmod +x run.sh`** and then **`./run.sh`**

It will ssh in the created container when it's ready.

In the [**`Dockerfile`**](Dockerfile) and [**`sshd_config`**](sshd_config) you can find all the dependencies the image relies on.

# Running The Operating System

Just execute `make emul` in the root path of the folder.

# Executing Tests

The tests are unitary tests. They cover all the new syscalls and old syscalls that needed to be modified.<br>
The tests can be found in the directory [`test`](test). You can run any test (or bulk of tests) by uncommenting the selected one in the [`user.c`](user.c).<br>

> **NOTE:**<br>
> As there is not enough space in the screen for all the tests outputs please enable the tests you want in order to see them all (uncomment them).<br>
> If you want to see the implementation of each test, just navigate to: **`test/<feature>.c`**<br>
> If a test passes it will output something like: **`[PASSED] : <test title>`** or: **`[FAILED] : <test title>`** if it does not pass.<br>
> If a test requires screen output, that output will be sorrounded by a box like: **`======= [OUTPUT] : <test title> =======`**<br>

# Debugging

The GDB console can be used to debug the system, run `make emuldbg` to launch it.

# Directory structure

```
.
├── bootsect.S
├── build.c
├── devices.c
├── entry.S
├── generate_gdbcmd_file.sh
├── hardware.c
├── include
│   ├── asm.h
│   ├── devices.h
│   ├── entry.h
│   ├── errno.h
│   ├── hardware.h
│   ├── interrupt.h
│   ├── io.h
│   ├── keyboard.h
│   ├── libc.h
│   ├── list.h
│   ├── mm_address.h
│   ├── mm.h
│   ├── p_stats.h
│   ├── sched.h
│   ├── schedperf.h
│   ├── segment.h
│   ├── stats.h
│   ├── system.h
│   ├── test.h
│   ├── types.h
│   ├── utils.h
│   ├── zeos_interrupt.h
│   └── zeos_mm.h
├── interrupt.c
├── io.c
├── kernel-utils.S
├── libauxjp.a
├── libc.c
├── libschedperf.a
├── libuser.a
├── libzeos.a
├── list.c
├── Makefile
├── mm.c
├── p_stats.c
├── README.md
├── sched.c
├── sys.c
├── sys_call_table.S
├── system.c
├── system.lds
├── test
│   ├── exit.c
│   ├── fork.c
│   ├── mutex_destroy.c
│   ├── mutex_init.c
│   ├── mutex_lock.c
│   ├── mutex_unlock.c
│   ├── pthread_create.c
│   ├── pthread_exit.c
│   ├── pthread_getspecific_setspecific.c
│   ├── pthread_join.c
│   ├── pthread_key_create.c
│   └── pthread_key_delete.c
├── user.c
├── user.lds
├── user-utils.S
└── utils.c
```
