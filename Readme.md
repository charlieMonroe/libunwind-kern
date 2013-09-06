# Libunwind for (FreeBSD) kernel

### About

This is project was started in order to bring libunwind to the FreeBSD kernel. Why would you want libunwind in the kernel? To support no-cost exception handling in Objective-C (see https://github.com/charlieMonroe/libobjc-kern).

### Source code

Simply modifying the libunwind code is almost impossible. Hence only relevant parts have been taken and put together in this project - most of the remote-related code has been removed (though some might have been left there) and generally only the x86-64-related code has been left.

### Obstacles

Unfortunately, this project isn't finished - it compiles and runs as a separate kernel module, can create backtraces, but cannot unwind stack. This is because it fails to find sufficient data in the `.eh_frame` section and ends up guessing the stack pointer based on `rsb`, which results in a very wrong value most of the time.

If you want to go have a whack at it, the relevant code is in the `Gfind_proc_info-lsb.c`, where the `linear_search` function fails finding the needed info.

