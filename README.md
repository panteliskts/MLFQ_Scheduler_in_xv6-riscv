# MLFQ Scheduler in xv6-riscv

An extension of the [MIT xv6-riscv](https://github.com/mit-pdos/xv6-riscv) teaching operating system that replaces the default round-robin scheduler with a **Multi-Level Feedback Queue (MLFQ)** scheduler, along with a new `getpinfo()` system call and a user-space `ps` utility.

> Built as part of the Operating Systems course (K22) at the National and Kapodistrian University of Athens (NKUA), 2025–2026.

---

## What is MLFQ?

**Multi-Level Feedback Queue** is a CPU scheduling algorithm that automatically adapts process priorities based on observed behavior — without requiring the programmer to declare them upfront.

- **I/O-bound processes** (e.g. text editors) use little CPU per burst → they stay at high priority → fast response times
- **CPU-bound processes** (e.g. compilers) consume full timeslices → they get demoted → run in the background
- **Starvation is prevented** via an automatic priority boost mechanism

---

## Features Implemented

### 🔧 MLFQ Scheduler (`kernel/proc.c`)

| Rule | Description |
|------|-------------|
| 4 priority levels | 0 = highest, 3 = lowest |
| Priority selection | Always runs the highest-priority RUNNABLE process |
| Round-robin per level | Fair scheduling among processes at the same priority |
| Timeslices | Priority 0: 4 ticks · Priority 1: 8 ticks · Priority 2: 16 ticks · Priority 3: 32 ticks |
| New processes | Always start at priority 0 |
| Demotion | When a timeslice is exhausted, process moves to the next lower priority level |
| Timeslice preservation | Voluntary CPU yields preserve the remaining timeslice (I/O-bound processes stay high) |
| Anti-starvation boost | If a process waits 10× its timeslice without running, it is promoted one level |

### 📞 `getpinfo()` System Call

A new system call that exports process information to user space:

```c
int getpinfo(struct pstat *);
```

Returns for each active process:
- `pid`, `ppid`, `name`
- `priority` (current MLFQ level)
- `state` (RUNNABLE, RUNNING, SLEEPING, etc.)
- `ticks_used`, `ticks_total`
- `size`

Returns `0` on success, `-1` on error.

### 🖥️ `ps` User Program (`user/ps.c`)

A user-space program similar to Linux `ps`, using `getpinfo()` to display a live table of all active processes and their MLFQ priority levels.

```
PID  PPID  NAME      STATE     PRI  TICKS  SIZE
1    0     init      SLEEPING  0    12     16384
2    1     sh        SLEEPING  0    8      28672
3    2     ps        RUNNING   0    1      24576
```

---

## Files Modified

| File | Change |
|------|--------|
| `kernel/proc.h` | Added MLFQ fields to `struct proc`: `priority`, `ticks_used`, `ticks_total`, `wait_ticks` |
| `kernel/proc.c` | Rewrote `scheduler()`, extended `yield()`, added `get_timeslice()`, `update_wait_ticks()`, `getpinfo()` |
| `kernel/pstat.h` | New file — defines `struct pstat`, `MLFQ_LEVELS`, timeslice constants, `BOOST_MULTIPLIER` |
| `kernel/syscall.h` | Added `SYS_getpinfo 22` |
| `kernel/syscall.c` | Registered `sys_getpinfo` in dispatch table |
| `kernel/sysproc.c` | Implemented `sys_getpinfo()` wrapper with `copyout` to user space |
| `user/user.h` | Added `getpinfo(struct pstat*)` declaration |
| `user/usys.pl` | Added `entry("getpinfo")` |
| `user/ps.c` | New file — user-space ps program |
| `Makefile` | Added `$U/_ps` to build targets |

---

## Design Decisions

**Round-robin within a priority level** is implemented using a static `last_scheduled[MLFQ_LEVELS]` array. Each level remembers the last scheduled process index, so the next search starts from there — preventing the same process from monopolizing its level.

**Timeslice preservation** is achieved by only resetting `ticks_used` when demotion occurs (i.e. when `ticks_used >= timeslice`). Voluntary yields leave `ticks_used` intact, so an I/O-bound process that yields early continues from where it left off next time it runs — keeping it at high priority.

**Anti-starvation threshold** is set at `10 × timeslice` of the current level:
- Priority 0 → boost after 40 ticks of waiting
- Priority 3 → boost after 320 ticks of waiting

**Locking strategy**: `update_wait_ticks()` is called *after* releasing the chosen process lock to avoid deadlocks. Each process is locked individually during the scan.

---

## How to Run

### Requirements
- QEMU
- RISC-V GCC toolchain (`riscv64-unknown-elf-gcc`)
- Make

### Build and run (single CPU recommended for testing)

```bash
CPUS=1 make qemu
```

### Run the ps utility inside xv6

```
$ ps
```

### Run all kernel tests

```
$ usertests
```

Expected output:
```
...
ALL TESTS PASSED
```

---

## How MLFQ Differs from Round-Robin

| Scenario | Round-Robin | MLFQ |
|----------|-------------|------|
| Text editor + compiler running together | Both get equal CPU share → editor feels laggy | Editor stays at priority 0 → instant response |
| Long-running CPU job | Runs every turn | Demoted to priority 3 → runs only when nothing else needs CPU |
| Starving process | Can starve if others keep arriving | Boosted after waiting too long |

---

## Base Repository

This project extends the MIT xv6-riscv kernel:
> https://github.com/mit-pdos/xv6-riscv

The original xv6 scheduler is a simple round-robin over all RUNNABLE processes. All MLFQ logic, the `getpinfo` system call, and the `ps` program are original additions.

---

## License

MIT — see [LICENSE](LICENSE)
