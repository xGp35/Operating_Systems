In the very simple case where I just have one page table for one process. Let's say I don't have the page directory hierarchical structure nor do I have the paging plus segmentation. I just have one page table per process. Forget about VAX also. So just one process running on one operating system and it has one page table. Now let's say the process issues a write on a particular address. It has to write some bits to a particular address. How many memory accesses will it require?Okay. So let's say my thinking is I have been given one virtual address. Now I need to translate this into a physical frame. So what I'll do is I will first go to the page table. The page table will tell me the frame number of this virtual address. So page table itself resides in memory. So one memory access for accessing the page table. Then once I get the frame number, another memory access for accessing the actual location in the physical memory. And then once I have that location, I can write to it. Is that thinking correct?So basically two memory references Forget about TLBs also.

Your reasoning is exactly right. Two memory accesses, no more, no less — this is actually one of the concrete "aha" points OSTEP builds toward on the road to explaining why TLBs exist. Let me walk it with real numbers so there's no hand-waving left.

## Setup (deliberately simple)

- 16-bit virtual addresses, 4KB pages → offset = 12 bits, VPN = 4 bits (16 total pages, small on purpose)
- One flat, linear page table, fully resident in physical memory, entries are 4 bytes each (a frame number + a few valid/protection bits)
- Page-table base register (`PTBR`) = physical address `0x9000` — this is a hardware register, not something you have to look up in memory to know

Say the process executes `write` to virtual address `0x3214`.

```
0x3214 = 0011 0010 0001 0100
         \__/\_______________/
         VPN      offset
         0011      0010 0001 0100
        = 3         = 0x214
```

## The two accesses

**Step 0 (no memory access — just arithmetic):**
The MMU computes *where the PTE lives*: `PTBR + VPN × entry_size = 0x9000 + (3 × 4) = 0x900C`. This is address arithmetic the hardware does with values it already has (base register + VPN extracted from the instruction's operand). Nothing has touched memory yet.

**Memory access #1 — read the PTE:**
Fetch the 4 bytes sitting at physical address `0x900C`. Say it contains `frame = 0x7, valid = 1`. This is the "go to the page table" step you described.

**Step 0.5 (again, arithmetic, no access):**
Physical address = `frame × page_size + offset = 0x7000 + 0x214 = 0x7214`.

**Memory access #2 — do the actual write:**
Store the data at physical address `0x7214`.

**Total: 2 memory references** — exactly your count. One to translate, one to do the thing you actually wanted to do.

## One small clarification, not a correction

The two accesses you're counting are the ones the *data* operation itself needs. The instruction (`write x, ...`) had to be fetched too, which is its own separate translate-then-access pair — but that's normally accounted for separately as instruction fetch, not folded into "the cost of this memory access," so it doesn't change your count. Your two-access answer for the data reference is the standard one.

This is precisely the fact OSTEP uses to motivate the next chapter: a naive linear page table *doubles* every memory reference — 100% overhead, before you even get to hierarchical tables or protection checks. That overhead is the entire reason TLBs exist: cache the VPN→frame mapping so most accesses skip access #1 entirely and go straight to the real data with a single reference. When you get there, this exact 2-access walkthrough is the "before" picture the TLB chapter is solving.