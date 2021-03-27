# filerewrite
Tool for rewriting file's data in place.

filerewrite works by opening the given file(s) read-write, reading its content (8MB at a time) and writting the same data back at the same offset (using pread(2) and pwrite(2) system calls). Once the file is rewritten, its access and modification times are restored to those from before the file was rewritten. filerewrite operates only on regular files, skipping all the rest, including symbolic links, but it makes no effort to track hard linked files, so those will be rewritten multiple times.

This tool is useful for example with ZFS file system. When selected properties of a ZFS file system like compression or deduplication are updated, the new settings will only apply to newly written blocks without having any impact on alreading existing blocks.

Example usage:

	find -x /directory/to/be/rewritten/ -type f -print0 | xargs -0 filerewrite

filerewrite should not be used on live file systems as there is an obvious race between reading the data and writing the data back.

It also makes little sense to use filerewrite on ZFS file systems with existing snapshots as the rewritten blocks are not going to be freed until the last snapshot referencing them is destroyed.

filerewrite should work on FreeBSD, Linux, macOS and pretty much any other pretty modern UNIX-like operating system.
