# Τεκμηρίωση Υλοποίησης MLFQ Scheduler - xv6-riscv

## 1. Απαιτήσεις Έργου

H υλοποίηση **MLFQ (Multi-Level Feedback Queue) scheduler** έχει τα εξής χαρακτηριστικά:


#### 1.1 Νέα κλήση συστήματος
- **`int getpinfo(struct pstat *)`** - Επιστρέφει πληροφορίες για όλες τις ενεργές διεργασίες
- Πρέπει να επιστρέφει: pid, ppid, όνομα, προτεραιότητα, κατάσταση, μέγεθος, κλπ.
- Επιστρέφει 0 σε επιτυχία, -1 σε σφάλμα

#### 1.2 Πρόγραμμα χρήστη ps
- Να εμφανίζει πληροφορίες διεργασιών (όπως το Linux ps)
- Να χρησιμοποιεί την κλήση συστήματος `getpinfo()`
- Να δείχνει το επίπεδο προτεραιότητας κάθε διεργασίας

#### 1.3 MLFQ Scheduler - Κανόνες
1. **4 επίπεδα προτεραιότητας**: 0 (υψηλότερη) έως 3 (χαμηλότερη)
2. **Timer tick**: Κάθε 10ms εκτελείται η διεργασία υψηλότερης προτεραιότητας
3. **Priority selection**: Πάντα τρέχει η RUNNABLE διεργασία υψηλότερης προτεραιότητας
4. **Round-robin εντός επιπέδου**: Αν υπάρχουν πολλές διεργασίες στο ίδιο επίπεδο, round-robin
5. **Χρονομερίδια**:
   - Priority 0: 4 timer ticks
   - Priority 1: 8 timer ticks
   - Priority 2: 16 timer ticks
   - Priority 3: 32 timer ticks
6. **Νέες διεργασίες**: Ξεκινούν στο επίπεδο 0
7. **Demotion**: Όταν εξαντληθεί το χρονομερίδιο, πηγαίνει στο επόμενο χαμηλότερο επίπεδο
8. **Διατήρηση χρονομεριδίου**: Αν παραδώσει εθελοντικά τη CPU, το υπόλοιπο χρονομερίδιο διατηρείται
9. **Anti-starvation**: Αν περιμένει 10× το χρονομερίδιο της, ανεβαίνει ένα επίπεδο (priority boost)

---

## 2. Μεθοδολογία υλοποίησης MLFQ

### 2.1 Σχεδιασμός αρχιτεκτονικής
Αποφασίστηκε η εξής δομή:

**Kernel-side:**
- Επέκταση `struct proc` με MLFQ πεδία
- Υλοποίηση `scheduler()` με priority selection + round-robin
- Υλοποίηση `yield()` με tick counting και demotion
- Helper functions: `get_timeslice()`, `update_wait_ticks()`
- Kernel function `getpinfo()` για export δεδομένων

**System call infrastructure:**
- Προσθήκη syscall number στο `syscall.h`
- Registration στο `syscall.c`
- Wrapper `sys_getpinfo()` στο `sysproc.c`
- User-space declaration στο `user.h`
- Entry point στο `usys.pl`

**User-space:**
- Δομή `struct pstat` στο `pstat.h`
- Πρόγραμμα `ps.c` που καλεί `getpinfo()`

### 2.2 Σειρά υλοποίησης (Incremental approach)

Η υλοποίηση έγινε σε λογική σειρά:

**Βήμα 1: Data structures**
- Ορισμός `struct pstat` με όλα τα απαιτούμενα πεδία
- Επέκταση `struct proc` με MLFQ fields:
  - `int priority` - τρέχον επίπεδο (0-3)
  - `int ticks_used` - ticks στο τρέχον χρονομερίδιο
  - `int ticks_total` - συνολικά ticks της διεργασίας
  - `int wait_ticks` - ticks αναμονής (για anti-starvation)

**Βήμα 2: System call infrastructure**
- Προσθήκη `SYS_getpinfo` (syscall #22) στο `syscall.h`
- Registration στον πίνακα syscalls στο `syscall.c`
- Wrapper function `sys_getpinfo()` στο `sysproc.c`
- User declaration στο `user.h`
- Entry στο `usys.pl`

**Βήμα 3: Kernel getpinfo() implementation**
```c
int getpinfo(struct pstat *ps)
{
  // Iterate through all processes
  // Lock each process
  // Copy: pid, ppid, state, priority, ticks, size, name
  // Handle parent PID carefully
  // Return 0 on success
}
```

**Βήμα 4: Helper functions**
```c
static int get_timeslice(int priority)
{
  // Return: 4, 8, 16, 32 based on priority
}

static void update_wait_ticks(struct proc *running)
{
  // For all RUNNABLE processes except running:
  //   Increment wait_ticks
  //   If wait_ticks >= 10 × timeslice:
  //     Promote to higher priority
  //     Reset ticks_used and wait_ticks
}
```

**Βήμα 5: MLFQ Scheduler**
```c
void scheduler(void)
{
  // Track last_scheduled[4] for round-robin per level
  // For each priority level 0→3:
  //   Round-robin search starting from last_scheduled
  //   Find first RUNNABLE at this priority
  //   If found, run it and break
  // Update wait_ticks for other processes
}
```

**Βήμα 6: Yield with MLFQ accounting**
```c
void yield(void)
{
  // Increment ticks_used and ticks_total
  // Check if timeslice exhausted
  // If exhausted and not at lowest priority:
  //   Demote (priority++)
  //   Reset ticks_used
  // Set RUNNABLE and call sched()
}
```

**Βήμα 7: Process initialization**
```c
static struct proc* allocproc(void)
{
  // ... existing code ...
  // Initialize MLFQ fields:
  p->priority = 0;      // Start at highest priority
  p->ticks_used = 0;
  p->ticks_total = 0;
  p->wait_ticks = 0;
  return p;
}
```

**Βήμα 8: User program ps**
```c
int main(void)
{
  struct pstat ps;
  getpinfo(&ps);  // System call
  
  // Print header
  // For each process in ps:
  //   Print: PID, PPID, STATE, PRIORITY, TICKS, SIZE, NAME
}
```

**Βήμα 9: Makefile integration**
- Προσθήκη `$U/_ps\` στο Makefile

---

## 3. Λεπτομέρειες υλοποίησης (Implementation details)

### 3.1 Αρχεία που τροποποιήθηκαν

**kernel/proc.h (lines 112-115)**
```c
// MLFQ scheduler fields
int priority;                // Current priority level (0=highest, 3=lowest)
int ticks_used;              // Ticks used in current time slice
int ticks_total;             // Total CPU ticks used by process
int wait_ticks;              // Ticks spent waiting (for boost)
```

**kernel/pstat.h (νέο αρχείο)**
- Ορισμοί: `MLFQ_LEVELS`, timeslices, `BOOST_MULTIPLIER`
- `struct pstat` με arrays για όλες τις πληροφορίες διεργασιών

**kernel/proc.c**
- `allocproc()` (line 157-160): Initialize MLFQ fields σε 0
- `get_timeslice()` (lines 431-440): Return timeslice based on priority
- `update_wait_ticks()` (lines 444-467): Increment wait_ticks και priority boost
- `scheduler()` (lines 480-560): MLFQ με round-robin ανά επίπεδο
- `yield()` (lines 580-607): Tick accounting και demotion
- `getpinfo()` (lines 801-841): Export process info

**kernel/syscall.h (line 23)**
```c
#define SYS_getpinfo 22
```

**kernel/syscall.c (lines 104, 130)**
```c
extern uint64 sys_getpinfo(void);
[SYS_getpinfo] sys_getpinfo,
```

**kernel/sysproc.c (lines 115-139)**
```c
uint64 sys_getpinfo(void)
{
  // Get user pointer
  // Call getpinfo()
  // copyout to user space
}
```

**user/user.h (line 28)**
```c
int getpinfo(struct pstat*);
```

**user/usys.pl (line 45)**
```c
entry("getpinfo");
```

**user/ps.c (νέο αρχείο, 58 lines)**
- Complete ps implementation με formatted output

**Makefile (line 148)**
```make
$U/_ps\
```

### 3.2 Κρίσιμες αποφάσεις σχεδιασμού

**Απόφαση 1: Round-robin εντός επιπέδου**
- Χρήση static array `last_scheduled[MLFQ_LEVELS]`
- Κάθε επίπεδο "θυμάται" πού σταμάτησε
- Αποφυγή starvation εντός του ίδιου επιπέδου

**Απόφαση 2: Priority boost threshold**
- `10 × timeslice` είναι το όριο αναμονής
- Priority 0: boost μετά από 40 ticks
- Priority 3: boost μετά από 320 ticks
- Ισορροπία: όχι πολύ συχνά (overhead), όχι πολύ σπάνια (starvation)

**Απόφαση 3: Διατήρηση χρονομεριδίου**
- Το `ticks_used` ΔΕΝ επαναφέρεται σε 0 όταν η διεργασία κοιμάται/μπλοκάρει
- Μόνο όταν `ticks_used >= timeslice` γίνεται reset (μαζί με demotion)
- Αυτό επιτρέπει στις I/O-bound διεργασίες να μένουν σε υψηλή προτεραιότητα

**Απόφαση 4: Locking strategy**
- Κάθε διεργασία κλειδώνεται ξεχωριστά
- `update_wait_ticks()` καλείται ΜΕΤΑ το release του chosen process lock
- Αποφυγή deadlock με σωστή σειρά locking

**Απόφαση 5: State tracking**
- `wait_ticks` επαναφέρεται σε 0 όταν η διεργασία τρέξει
- `wait_ticks` αυξάνεται μόνο για RUNNABLE διεργασίες
- SLEEPING/ZOMBIE διεργασίες δεν συμμετέχουν στο boost

---

## 4. Τι είναι το MLFQ και πώς λειτουργεί

### 4.1 Ορισμός
Το **MLFQ (Multi-Level Feedback Queue)** είναι ένας έξυπνος scheduler που **αυτόματα προσαρμόζει** τις προτεραιότητες των διεργασιών βάσει της συμπεριφοράς τους.

### 4.2 Βασική ιδέα
- Οι διεργασίες ξεκινούν με **υψηλή προτεραιότητα** (fast response)
- Οι **CPU-intensive** διεργασίες σταδιακά **υποβιβάζονται**
- Οι **I/O-bound** διεργασίες **παραμένουν ψηλά**
- **Starvation αποφεύγεται** με automatic boost

### 4.3 Πώς ξεχωρίζει CPU-intensive από I/O-bound

**I/O-bound process (π.χ. text editor):**
```
Start: Priority 0, timeslice=4 ticks
User types character → I/O wait after 1 tick
Wakes up → still Priority 0 (used only 1/4 ticks)
Repeats → stays at Priority 0 → FAST RESPONSE
```

**CPU-bound process (π.χ. video encoding):**
```
Start: Priority 0, timeslice=4 ticks
Uses all 4 ticks → demoted to Priority 1
Uses all 8 ticks → demoted to Priority 2
Uses all 16 ticks → demoted to Priority 3
Stays at Priority 3 → runs when nothing else needs CPU
```

### 4.4 Πλεονεκτήματα MLFQ
1. **Αυτόματη προσαρμογή**: Δεν χρειάζεται ο προγραμματιστής να δηλώσει priority
2. **Γρήγορη απόκριση**: Interactive processes πάντα σε υψηλή προτεραιότητα
3. **Fairness**: CPU-intensive processes δεν πεινάνε, απλά τρέχουν σε χαμηλότερη προτεραιότητα
4. **Anti-starvation**: Automatic boost αν περιμένει πολύ
5. **Scalability**: Λειτουργεί καλά με πολλές διεργασίες

### 4.5 Παράδειγμα real-world scenario

**Σενάριο**: Ταυτόχρονα τρέχουν:
- Text editor (I/O-bound)
- Compiler (CPU-bound)
- Video player (mix)

**Χωρίς MLFQ (round-robin)**:
- Editor παίρνει 33% CPU → laggy typing
- Compiler παίρνει 33% CPU
- Video player παίρνει 33% CPU → dropped frames

**Με MLFQ**:
- Editor: Priority 0 → παίρνει CPU instantly όταν χρειάζεται → smooth typing
- Compiler: Demoted σε Priority 3 → τρέχει όταν editor idle
- Video player: Stays σε Priority 1-2 → smooth playback, compiler δεν επηρεάζει

---

## 5. Προκλήσεις και λύσεις

### 5.1 Πρόκληση 1: Starvation εντός επιπέδου
**Πρόβλημα**: Αν σκανάρεις πάντα από την αρχή του proc[], οι πρώτες διεργασίες μονοπωλούν.

**Λύση**: Static array `last_scheduled[]` ανά επίπεδο, round-robin search.

### 5.2 Πρόκληση 2: Race conditions σε wait_ticks
**Πρόβλημα**: Πολλά processes ενημερώνουν wait_ticks ταυτόχρονα.

**Λύση**: Κάθε process lock ξεχωριστά, `update_wait_ticks()` μετά από release.

### 5.3 Πρόκληση 3: Priority inversion
**Πρόβλημα**: Low priority process κρατάει lock που χρειάζεται high priority.

**Λύση**: Όχι πλήρως solved (απαιτεί priority inheritance), αλλά mitigated με boost mechanism.

### 6.4 Πρόκληση 4: Tuning του boost threshold
**Πρόβλημα**: Πολύ μικρό threshold → πολλά boosts, overhead. Πολύ μεγάλο → starvation.

**Λύση**: 10× timeslice είναι reasonable balance από τη θεωρία MLFQ.

---

## 6. Παράρτημα: Βασικά αρχεία κώδικα

### 6.1 kernel/proc.h (MLFQ fields)
```c
struct proc {
  // ... existing fields ...
  
  // MLFQ scheduler fields
  int priority;       // 0=highest, 3=lowest
  int ticks_used;     // Ticks in current timeslice  
  int ticks_total;    // Total lifetime ticks
  int wait_ticks;     // For anti-starvation boost
};
```

### 6.2 kernel/proc.c (scheduler core)
```c
void scheduler(void) {
  static int last_scheduled[MLFQ_LEVELS];
  
  for(;;) {
    intr_on();
    
    // Find highest priority RUNNABLE process
    for(priority = 0; priority < MLFQ_LEVELS; priority++) {
      // Round-robin within this priority level
      for(i = 0; i < NPROC; i++) {
        idx = (last_scheduled[priority] + i) % NPROC;
        p = &proc[idx];
        
        if(p->state == RUNNABLE && p->priority == priority) {
          // Found one - run it
          chosen = p;
          last_scheduled[priority] = (idx + 1) % NPROC;
          break;
        }
      }
      if(chosen) break;
    }
    
    if(chosen) {
      // Run chosen process
      chosen->state = RUNNING;
      swtch(&c->context, &chosen->context);
      update_wait_ticks(chosen);
    }
  }
}
```

### 6.3 kernel/proc.c (yield with demotion)
```c
void yield(void) {
  struct proc *p = myproc();
  
  p->ticks_used++;
  p->ticks_total++;
  
  int timeslice = get_timeslice(p->priority);
  
  if(p->ticks_used >= timeslice) {
    // Demote to lower priority
    if(p->priority < MLFQ_LEVELS - 1) {
      p->priority++;
    }
    p->ticks_used = 0;
  }
  
  p->state = RUNNABLE;
  sched();
}
```

---

