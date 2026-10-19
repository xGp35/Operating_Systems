I'll take "performance" to mean both **speed** and **checking ability** (how many kinds of corruption each one catches), since those trade off against each other.

To compare fairly, assume byte-wise versions as in the simulator. Additive and XOR give an 8-bit result. Fletcher gives a 16-bit (a, b) pair. CRC-16 gives a 16-bit result.

## Speed

| Method | Work per data word | Why | Relative speed |
|---|---|---|---|
| XOR | 1 XOR | Independent operations, easy to process 8+ bytes at once with SIMD (one CPU instruction acting on many values at once) | Fastest. Usually limited by how fast memory can feed data, not by the CPU |
| Additive | 1 add | Same as XOR | Fastest, same tier |
| Fletcher | 2 adds + modulo | `b` depends on the freshly updated `a`, so each step waits on the previous one. Doing `% 255` every byte is slow; real implementations defer it and apply it occasionally | Fast, a bit behind XOR and add |
| CRC (naive) | 8 shift-and-XOR steps per byte | Processes one bit at a time | Slow |
| CRC (lookup table) | 1 table lookup + XOR per byte | Precomputes a 256-entry table so each byte is handled in one step | Moderate |
| CRC (hardware) | Dedicated instruction | x86 (SSE4.2) and ARM have a built-in CRC32C instruction, processing several bytes per cycle | Near XOR speed |

Caveat: the hardware instruction exists for **CRC32C** specifically (one particular 32-bit variant), not for CRC-16. A CRC-16 you write for the homework would run at table-driven speed.

## Checking ability

| Corruption type | Additive | XOR | Fletcher | CRC-16 |
|---|---|---|---|---|
| Single flipped bit | ✅ Caught | ✅ Caught | ✅ Caught | ✅ Caught |
| Two flipped bits, same bit position in different words | ❌ Can miss (one flips 0→1, the other 1→0, so the changes cancel) | ❌ Always missed (the column parity flips twice, back to the original) | ✅ Caught for short-to-moderate data lengths | ✅ Caught (with a well-chosen generator polynomial, up to a long length limit) |
| Reordered or swapped words | ❌ Missed | ❌ Missed | ✅ Caught (position weighting) | ✅ Caught |
| Burst error (a run of nearby corrupted bits) up to the checksum's width | ⚠️ Usually caught, but carries can make changes cancel | ✅ Caught (each column is hit at most once) | ✅ Mostly caught | ✅ Guaranteed for bursts ≤ 16 bits |
| A byte changing between 0x00 and 0xFF | ✅ Caught | ✅ Caught | ❌ **Missed** (0 ≡ 255 under mod 255). An all-zero block and an all-0xFF block get the same checksum | ✅ Caught |
| Random garbage (large-scale corruption) | Missed ~1 in 256 | Missed ~1 in 256 | Missed ~1 in 65,025 | Missed ~1 in 65,536 |

The last row is mostly about **width**, not cleverness. For totally random corruption, any n-bit checksum misses about 1 in 2^n. The algorithm matters for *structured* errors (flips, swaps, bursts), which are what real hardware actually produces.

## Summary ranking

- **Speed:** XOR ≈ Additive > Fletcher > CRC (table). CRC with a hardware instruction closes the gap.
- **Checking ability:** CRC > Fletcher > Additive ≈ XOR.

## Where each shows up in practice

| Method | Real use |
|---|---|
| Additive | IP, TCP and UDP header checksums use a 16-bit additive variant. It's cheap, and the link layer below (Ethernet) also runs a CRC |
| XOR | RAID parity. There it's used to **rebuild** a lost disk, not to detect corruption |
| Fletcher | ZFS's long-standing default data checksum (a 4-word Fletcher variant) |
| CRC | Ethernet frames (CRC-32), SATA, and ext4/btrfs metadata (CRC32C, using the hardware instruction) |

The pattern: CRC wins wherever hardware accelerates it, and the weaker checksums survive where speed mattered historically, or where another layer already provides a strong check.