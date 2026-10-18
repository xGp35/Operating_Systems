Your roll-forward idea is right, and it really is that simple in principle: start from the state the on-disk checkpoint region (CR) describes, scan the log forward, and for each imap chunk you find, point the matching CR entry at it, so the latest chunk wins. The difficulty is in four details your version skips. First, the run itself.

## What `-N` shows

After three commands, the CR on disk still says `checkpoint: 3`:

```
create dir  /jm5        → blocks 4–8
create file /jm5/jm2    → blocks 9–12
create dir  /lb9        → blocks 13–17, latest imap at 17
[ 0 ] checkpoint: 3     ← still points at the initial, empty imap
```

All three operations are on disk, but if the machine crashed now, mount would read CR → block 3 → root at block 2 → an empty directory. **The data exists but is unreachable.**

That answers the first two sub-questions:

- **Never updated:** every remount sees the original empty file system. Recovery would have to replay the entire disk, and the cleaner could never safely reclaim anything, because it couldn't tell what's live.
- **Updated periodically:** you lose only the changes since the last checkpoint, around 30 seconds in Sprite LFS, unless you roll forward. The trade-off is how often you pay the seek to block 0 versus how much you replay or lose after a crash.

## The four gaps in your roll-forward

```bash
❯ python3 lfs.py -N -i -o -s 1000 -c

INITIAL file system contents:
[   0 ] live checkpoint: 3 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
[   1 ] live [.,0] [..,0] -- -- -- -- -- -- 
[   2 ] live type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
[   3 ] live chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

create dir  /jm5 

[   4 ] live [.,0] [..,0] [jm5,1] -- -- -- -- -- 
[   5 ] live [.,1] [..,0] -- -- -- -- -- -- 
[   6 ] live type:dir size:1 refs:3 ptrs: 4 -- -- -- -- -- -- -- 
[   7 ] live type:dir size:1 refs:2 ptrs: 5 -- -- -- -- -- -- -- 
[   8 ] live chunk(imap): 6 7 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 


create file /jm5/jm2 

[   9 ] live [.,1] [..,0] [jm2,2] -- -- -- -- -- 
[  10 ] live type:dir size:1 refs:2 ptrs: 9 -- -- -- -- -- -- -- 
[  11 ] live type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
[  12 ] live chunk(imap): 6 10 11 -- -- -- -- -- -- -- -- -- -- -- -- -- 


create dir  /lb9 

[  13 ] live [.,0] [..,0] [jm5,1] [lb9,3] -- -- -- -- 
[  14 ] live [.,3] [..,0] -- -- -- -- -- -- 
[  15 ] live type:dir size:1 refs:4 ptrs: 13 -- -- -- -- -- -- -- 
[  16 ] live type:dir size:1 refs:2 ptrs: 14 -- -- -- -- -- -- -- 
[  17 ] live chunk(imap): 15 10 11 16 -- -- -- -- -- -- -- -- -- -- -- -- 



FINAL file system contents:
[   0 ] live checkpoint: 3 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
[   1 ]      [.,0] [..,0] -- -- -- -- -- -- 
[   2 ]      type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
[   3 ]      chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
[   4 ]      [.,0] [..,0] [jm5,1] -- -- -- -- -- 
[   5 ]      [.,1] [..,0] -- -- -- -- -- -- 
[   6 ]      type:dir size:1 refs:3 ptrs: 4 -- -- -- -- -- -- -- 
[   7 ]      type:dir size:1 refs:2 ptrs: 5 -- -- -- -- -- -- -- 
[   8 ]      chunk(imap): 6 7 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
[   9 ] live [.,1] [..,0] [jm2,2] -- -- -- -- -- 
[  10 ] live type:dir size:1 refs:2 ptrs: 9 -- -- -- -- -- -- -- 
[  11 ] live type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
[  12 ]      chunk(imap): 6 10 11 -- -- -- -- -- -- -- -- -- -- -- -- -- 
[  13 ] live [.,0] [..,0] [jm5,1] [lb9,3] -- -- -- -- 
[  14 ] live [.,3] [..,0] -- -- -- -- -- -- 
[  15 ] live type:dir size:1 refs:4 ptrs: 13 -- -- -- -- -- -- -- 
[  16 ] live type:dir size:1 refs:2 ptrs: 14 -- -- -- -- -- -- -- 
[  17 ] live chunk(imap): 15 10 11 16 -- -- -- -- -- -- -- -- -- -- -- -- 
```

**1. Which CR entry does a found chunk belong to?** In the simulator's printout, an imap chunk doesn't say which chunk it is. With sequential allocation everything is chunk 0, so you didn't notice. After Q10's random run, though, a chunk like `-- ... 6 -- --` only tells you "entry 13 of *some* chunk." Real LFS solves this with the thing you half-remembered: the **segment summary block**. It describes every block in the segment: "this is block 3 of inode 205," "this is imap chunk 12," and so on. **The log has to describe itself**, or roll-forward can't interpret what it finds.

**2. Where to start scanning.** The CR must record where the log ended when it was written. Real LFS stores the last segment written and a timestamp. Otherwise you don't know where "after the checkpoint" begins. And once the cleaner reuses old space, "scan from the start of the disk" stops meaning anything.

**3. An imap chunk is not a segment boundary.** In the simulator every command ends with an imap chunk, but that's a simulator artifact. A real segment holds many operations' worth of blocks. To detect where the valid log ends, you check each segment summary's checksum or timestamp, and the first segment that's torn or stale is the end. The CR is protected the same way. LFS keeps **two** CRs at opposite ends of the disk and alternates between them. Each is written with a timestamp at its head and its tail, and on mount you take the newer CR whose two timestamps match.

**4. You don't need liveness for recovery.** Recovery only has to rebuild the imap. Once you know the latest location of each inode, everything reachable from root is correct automatically. Working out which blocks are dead is the cleaner's job, and it can be derived afterward.

## Why the imap write is the commit point

Look at the order the simulator writes a create in: dir data → dir inode → file inode → **imap chunk**. If the crash happens anywhere before the imap chunk lands, nothing points at the new blocks, so the operation vanishes as a whole and leaves nothing half-done. **The imap chunk acts as the commit record.** Real LFS packs many operations into a segment, so it needed extra machinery for this. Sprite LFS kept a *directory operation log* so that roll-forward keeps directory entries and inode link counts consistent with each other.

`-D` shows what goes wrong without all this. It makes later operations read the stale on-disk CR. In that run, root's newest directory block (block 9) contains only `[lb9,2]`: the `jm5` entry is gone, and `jm2` was never created. Building on a stale CR doesn't just lose recent work; it produces an inconsistent file system.

That's the whole pattern: checkpoint, a self-describing log, replay past the checkpoint, and a commit point per operation. It's the same structure as a database write-ahead log, and as a Raft log with snapshots.