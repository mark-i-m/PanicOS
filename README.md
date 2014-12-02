PanicOS
=======

Make
----

To build

```
make
```

To run with QEMU

```
make run
```

To debug with QEMU and GDB

```
make debug
```
then in another window
```
gdb kernel/kernel
(gdb) target remote localhost:1234
(gdb) b <breakpoint>
(gdb) c
```
