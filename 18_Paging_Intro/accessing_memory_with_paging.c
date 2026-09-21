#include<stdio.h>
// Not code, this is more like pesudo code
#define VPN_MASK = 0x30;
#define SHIFT = 4;

// Extract the VPN from the virtual address
VPN = (VirtualAddress & VPN_MASK) >> SHIFT;

// From the address of the page-table entr (PTE)
PTEAddr = PageTableBaseRegister + (VPN * sizeof(PTE));

// Fetch the PTE
PTE = AccessMemory(PTEAddr);

// Check if process can access the page
if (PTE.Valid == False)
	RaiseException(SEGMENTATION_FAULT);
else if (CanAccess(PTE.ProtectBits) == False)
	RaiseException(PROTECTION_FAULT);
else
	// Access OK: form  physical address and fetch it
	offset = VirtualAddress & OFFSET_MASK;
	PhysAddr = (PTE.PFN << PFN_SHIFT) | offset;
	Reister = AccessMemory(PhysAddr);

