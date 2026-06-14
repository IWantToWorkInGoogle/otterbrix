#!/usr/bin/env python3
"""Evict a directory tree's files from the OS page cache via posix_fadvise(DONTNEED).

Used by run_cold.sh to measure true cold-cache scan latency without root/drop_caches.
posix_fadvise(POSIX_FADV_DONTNEED) drops clean pages of the named files; the benchmark
state files are clean after a checkpoint, so the next process reads them from disk.
"""
import os
import sys
import glob


def evict(root: str) -> int:
    evicted = 0
    for path in glob.glob(os.path.join(root, "**", "*"), recursive=True):
        if not os.path.isfile(path):
            continue
        try:
            fd = os.open(path, os.O_RDONLY)
        except OSError:
            continue
        try:
            size = os.fstat(fd).st_size
            if size:
                os.posix_fadvise(fd, 0, size, os.POSIX_FADV_DONTNEED)
            evicted += 1
        finally:
            os.close(fd)
    return evicted


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: evict_cache.py <dir>", file=sys.stderr)
        sys.exit(2)
    n = evict(sys.argv[1])
    print(f"evicted {n} files under {sys.argv[1]}")
