# XFS Interface — Failure Analysis & Fix Report

**Date:** 2026-09-03  
**Container:** `nitcbase:ubuntu20.04` (`FROM ubuntu:20.04`, `Dockerfile:1`)  
**Bind mount:** `Host: /home/dev/Developer/DBMS_Lab/NITCbase` → `Container: /home/nitcbase/NITCbase` (`WORKDIR /home/nitcbase/NITCbase`, `Dockerfile:17`)  
**Interface:** `NITCbase/XFS_Interface/xfs-interface` (built via `NITCbase/XFS_Interface/Makefile:4`)

---

## 1. Summary

`xfs-interface` crashed with `Segmentation fault (core dumped)` (exit `139`) when running any dump command:

```
dump bmap
dump relcat
dump attrcat
```

**Root cause:** Missing runtime directories `Files/Input_Files/` and `Files/Output_Files/` combined with unchecked `fopen()` return value (`NULL`) dereferenced in `NITCbase/XFS_Interface/external_fs_commands.cpp:22,74,130,150`.

---

## 2. Why It Crashed

### 2.1 Missing Runtime Directories

*Evidence:*

```bash
docker exec nitcbase ls -R /home/nitcbase/NITCbase/Files
# => /home/nitcbase/NITCbase/Files:
#    Batch_Execution_Files  (only)
#    Batch_Execution_Files/create.txt
#    Batch_Execution_Files/delete.txt
# Missing: Input_Files/, Output_Files/

git ls-tree HEAD -- NITCbase/Files
# => 040000 tree NITCbase/Files
#    100644 blob NITCbase/Files/Batch_Execution_Files/create.txt
#    100644 blob NITCbase/Files/Batch_Execution_Files/delete.txt
# Input_Files/ and Output_Files/ are never tracked in git
```

`NITCbase/XFS_Interface/define/constants.h:8-10` expects:

```c
#define OUTPUT_FILES_PATH "../Files/Output_Files/"
#define INPUT_FILES_PATH  "../Files/Input_Files/"
#define DISK_PATH "../Disk/disk"
```

These paths are relative to `NITCbase/XFS_Interface/` (where `./xfs-interface` is launched). After a `git clean -fd` or fresh container recreation the directories do not exist.

### 2.2 Unchecked `fopen()` NULL Dereference

`NITCbase/XFS_Interface/external_fs_commands.cpp:22,74,130,150` (pre-fix):

```cpp
FILE *fp_export = fopen(fileName, "w"); // returns NULL if parent dir missing
fputc(ch, fp_export); // NULL dereference → SIGSEGV
fputs("Block ", fp_export);
writeHeaderToFile(fp_export, headInfo); // fputs(tmp, fp) inside
```

Similarly `dumpBlockAllocationMap()`:

```cpp
FILE *disk = fopen(&DISK_PATH[0], "rb+"); // no check
fseek(disk, 0, SEEK_SET);
```

And `interface.cpp:155-163` unconditionally printed success:

```cpp
dump_relcat();
cout << "Dumped relation catalog to " << OUTPUT_FILES_PATH << "relation_catalog" << endl;
```

*Reproduction:*

```bash
docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && printf 'dump bmap\nexit\n' | timeout 5 ./xfs-interface; echo \$?"
# => dump bmap
#    timeout: the monitored command dumped core
#    Segmentation fault (139)
# core at NITCbase/XFS_Interface/core.* (978944 bytes)

docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && printf 'dump relcat\nexit\n' | timeout 5 ./xfs-interface; echo \$?"
# => same 139

docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && printf 'dump attrcat\nexit\n' | timeout 5 ./xfs-interface; echo \$?"
# => same 139
```

With a temporary debug build where the guard was commented (`external_fs_commands.cpp:29-32`):

```cpp
FILE *fp_export = fopen(fileName, "w");
// if (!fp_export) { return FAILURE; }  // commented
std::cout << "OK 1" << endl;
writeHeaderToFile(fp_export, headInfo); // crashes here when fp_export==NULL
```

the crash is deferred to `writeHeaderToFile():582 fputs(tmp,fp)` / `fputc(ch,fp_export):50`, proving the `fopen` failure is the trigger, but the unchecked use is the crash site.

**Why this code path fails:** `fopen(fileName,"w")` with `fileName="../Files/Output_Files/relation_catalog"` calls `open()` with `O_CREAT`. If `../Files/Output_Files` does not exist, `open` fails with `ENOENT` and `fopen` returns `NULL`. Any subsequent `fputc`/`fputs`/`fclose` on `NULL` dereferences a null pointer and raises `SIGSEGV`. The fix must not just create the directories, but also handle the `NULL` gracefully to avoid future crashes.

---

## 3. Steps to Reproduce (Focusing on `fopen` Failure)

```bash
# 1. Ensure binary is built (any arch):
docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && make && file xfs-interface"

# 2. Remove runtime dirs to simulate fresh clone / git clean:
docker exec nitcbase bash -c "rm -rf /home/nitcbase/NITCbase/Files/Output_Files /home/nitcbase/NITCbase/Files/Input_Files && ls -R /home/nitcbase/NITCbase/Files"
# => only Batch_Execution_Files remains

# 3. Trigger crash (pre-fix):
docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && printf 'dump bmap\nexit\n' | timeout 5 ./xfs-interface; echo exit:\$?"
# => exit:139 + core dumped

docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && printf 'dump relcat\nexit\n' | timeout 5 ./xfs-interface; echo exit:\$?"
docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && printf 'dump attrcat\nexit\n' | timeout 5 ./xfs-interface; echo exit:\$?"
# All dump commands segfault identically when Output_Files missing

ls -lh /home/dev/Developer/DBMS_Lab/NITCbase/XFS_Interface/core*
# => core.* files appear (not gitignored pre-fix)

# 4. Verify normal commands still work (to isolate dump path):
docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && printf 'ls\nhelp\nexit\n' | timeout 5 ./xfs-interface"
# => lists RELATIONCAT, ATTRIBUTECAT, etc. — no crash, because they don't use OUTPUT_FILES_PATH
```

---

## 4. Fix Applied

### 4.1 Recreate Missing Runtime Directories

```bash
docker exec nitcbase mkdir -p /home/nitcbase/NITCbase/Files/Input_Files /home/nitcbase/NITCbase/Files/Output_Files
ls -R /home/dev/Developer/DBMS_Lab/NITCbase/Files
# => Batch_Execution_Files/ Input_Files/ Output_Files/
```

After this, pre-fix binary immediately succeeds:

```
Dumped block allocation map to ../Files/Output_Files/block_allocation_map
```

### 4.2 Harden `external_fs_commands.cpp` Against Missing Files (Crash Fix)

**File: `NITCbase/XFS_Interface/external_fs_commands.cpp:22,74,130,150`**

Changed signatures from `void` to `int` returning `SUCCESS`/`FAILURE`:

```cpp
// Before (crashes when dir missing):
void dump_relcat() {
  FILE *fp_export = fopen(fileName, "w");
  // ... fputc/fputs without NULL check
}

// After (graceful failure):
int dump_relcat() {
  FILE *fp_export = fopen(fileName, "w");
  if (!fp_export) {
    cout << "Failed to open " << fileName << " for writing. Ensure directory " << OUTPUT_FILES_PATH << " exists." << endl;
    return FAILURE;
  }
  // ...
  fclose(fp_export);
  return SUCCESS;
}
```

Same for `dump_attrcat()` and `dumpBlockAllocationMap()` (also added check for `fopen(DISK_PATH, "rb+")` at `external_fs_commands.cpp:130`):

```cpp
FILE *disk = fopen(&DISK_PATH[0], "rb+");
if (!disk) {
  cout << "Failed to open disk file " << DISK_PATH << endl;
  return FAILURE;
}
```

And the `fopen` for `block_allocation_map` at `external_fs_commands.cpp:150`:

```cpp
FILE *fp_export = fopen(fileName, "w");
if (!fp_export) {
  cout << "Failed to open " << fileName << " for writing. Ensure directory " << OUTPUT_FILES_PATH << " exists." << endl;
  return FAILURE;
}
```

**File: `NITCbase/XFS_Interface/external_fs_commands.h:4-6`**

```cpp
// Before:
void dump_relcat();
void dump_attrcat();
void dumpBlockAllocationMap();

// After:
int dump_relcat();
int dump_attrcat();
int dumpBlockAllocationMap();
```

**File: `NITCbase/XFS_Interface/interface.cpp:155-163`**

Handlers now check return value instead of unconditionally printing success:

```cpp
// Before:
} else if (regex_match(input_command, dump_rel)) {
  dump_relcat();
  cout << "Dumped relation catalog to " << OUTPUT_FILES_PATH << "relation_catalog" << endl;
} else if (regex_match(input_command, dump_attr)) {
  dump_attrcat();
  cout << "Dumped attribute catalog to " << OUTPUT_FILES_PATH << "attribute_catalog" << endl;
} else if (regex_match(input_command, dump_bmap)) {
  dumpBlockAllocationMap();
  cout << "Dumped block allocation map to " << OUTPUT_FILES_PATH << "block_allocation_map" << endl;

// After:
} else if (regex_match(input_command, dump_rel)) {
  int ret = dump_relcat();
  if (ret == SUCCESS) cout << "Dumped relation catalog to " << OUTPUT_FILES_PATH << "relation_catalog" << endl;
} else if (regex_match(input_command, dump_attr)) {
  int ret = dump_attrcat();
  if (ret == SUCCESS) cout << "Dumped attribute catalog to " << OUTPUT_FILES_PATH << "attribute_catalog" << endl;
} else if (regex_match(input_command, dump_bmap)) {
  int ret = dumpBlockAllocationMap();
  if (ret == SUCCESS) cout << "Dumped block allocation map to " << OUTPUT_FILES_PATH << "block_allocation_map" << endl;
```

Now behavior without directories is graceful instead of `SIGSEGV`:

```
Failed to open ../Files/Output_Files/block_allocation_map for writing. Ensure directory ../Files/Output_Files/ exists.
# exit code 0, no core dump
```

### 4.3 Ancillary Fixes

- Removed stale core dumps: `rm -f NITCbase/XFS_Interface/core*`
- Updated `NITCbase/XFS_Interface/.gitignore:1-4` to ignore `core` and `core.*`:
  ```
  *.o
  xfs-interface
  core
  core.*
  ```
- Rebuilt: `docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && make clean && make"` (`g++ *.cpp -o xfs-interface -lreadline`).

---

## 5. Verification (Post-Fix)

```bash
# Dump commands with dirs present (success path):
docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && printf 'dump bmap\ndump relcat\ndump attrcat\nexit\n' | timeout 5 ./xfs-interface; echo exit:\$?; ls -lh ../Files/Output_Files/"
# => Dumped block allocation map to ../Files/Output_Files/block_allocation_map
#    Dumped relation catalog to ../Files/Output_Files/relation_catalog
#    Dumped attribute catalog to ../Files/Output_Files/attribute_catalog
#    3 files created (block_allocation_map 199K, relation_catalog 416B, attribute_catalog 1.1K), exit 0

# Dump commands without dirs (graceful failure, no crash):
docker exec nitcbase bash -c "rm -rf /home/nitcbase/NITCbase/Files/Output_Files && cd /home/nitcbase/NITCbase/XFS_Interface && printf 'dump bmap\nexit\n' | timeout 5 ./xfs-interface; echo exit:\$?; mkdir -p ../Files/Output_Files ../Files/Input_Files"
# => Failed to open ../Files/Output_Files/block_allocation_map for writing. Ensure directory ../Files/Output_Files/ exists., exit 0 (no segfault, no core)

# Other commands unaffected:
docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && printf 'ls\nhelp\nexit\n' | timeout 5 ./xfs-interface"
# => RELATIONCAT, ATTRIBUTECAT, Students, Events, ... + help, exit 0

docker exec nitcbase bash -c "cd /home/nitcbase/NITCbase/XFS_Interface && printf 'run create.txt\nls\nexit\n' | timeout 5 ./xfs-interface"
# => After fdisk: Relation a..k created, ls shows a-k; without fdisk: At line number 1 (relation already exists) — expected state-dependent, not crash
```

---

## 6. Recommendations

1. **Runtime directories:** Add `mkdir -p ../Files/Input_Files ../Files/Output_Files` to `Disk::createDisk()`/`formatDisk()` or to the `fdisk` handler in `interface.cpp:135-140` so first-run never crashes even on fresh clone. Alternatively, create the directories in the `Dockerfile` or an entrypoint script.

2. **Keep `fopen` guards:** All future file operations in `external_fs_commands.cpp` should check `fopen` return value before use. Existing `exportRelation()` already does (`exportRelation:452 if (!fp_export) return FAILURE;`), `dump` functions now do as well.

3. **`.gitignore` for runtime artifacts:** `Files/Output_Files/*` and `Files/Input_Files/*` should remain untracked, but consider adding `Files/Output_Files/.gitkeep` and `Files/Input_Files/.gitkeep` with `.gitignore` inside to ensure directories exist after `git clone`.

4. **Testing:** Add a CI test that removes `Output_Files` and runs `dump bmap` expecting graceful error, not `139`.

---

## 7. Files Changed

- `NITCbase/XFS_Interface/external_fs_commands.cpp:22,74,130,150` — added `NULL` checks after `fopen`, changed `void` → `int` return with `SUCCESS`/`FAILURE`
- `NITCbase/XFS_Interface/external_fs_commands.h:4-6` — signature `void` → `int`
- `NITCbase/XFS_Interface/interface.cpp:155-163` — handlers check return value before printing success
- `NITCbase/XFS_Interface/.gitignore:1-4` — added `core`, `core.*`
- `NITCbase/Files/Input_Files/` and `NITCbase/Files/Output_Files/` — created at runtime (untracked, created via `mkdir -p` inside container, visible on host via bind mount)
