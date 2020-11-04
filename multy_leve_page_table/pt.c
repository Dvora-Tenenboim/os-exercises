#define _GNU_SOURCE


#include <stdlib.h>
#include <stdio.h>



#include "os.h"
uint64_t check_mapping(uint64_t* pt,uint64_t index)
{
	uint64_t pte=*(pt+index*8);
	if((pte & 0x1)==0)
		return NO_MAPPING;
	else
	{
		return pte;
	}
}
uint64_t mapping(uint64_t* pt,uint64_t index)
{
	uint64_t pte=alloc_page_frame();
	(*(pt+index*8))=pte;
	(*(pt+index*8))=((*(pt+index*8))<<12);
	(*(pt+index*8))=((*(pt+index*8)) | 0x1);
	return *(pt+index*8);
}
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn)
{
	
	uint64_t pt_map;
	pt=(pt<<12);
	for(int i=0;i<=3;i++)
	{
		pt_map=check_mapping(phys_to_virt(pt),(vpn>>(36-9*i))& 0x1ff);
		if(pt_map==NO_MAPPING)
		{
			if(ppn!=NO_MAPPING)
			{
				pt=mapping(phys_to_virt(pt),(vpn>>(36-9*i))& 0x1ff);
				
			}
			else
			{
				return;
			}
		}
		else
		{
			pt=pt_map;
		}
	}
	
	uint64_t vpn4=(vpn & 0x1ff);
	if(ppn!=NO_MAPPING)
	{
		(*((uint64_t*)(phys_to_virt(pt))+vpn4*8))=(ppn<<12);
		(*((uint64_t*)(phys_to_virt(pt))+vpn4*8))=((*((uint64_t*)(phys_to_virt(pt))+vpn4*8)) | 0x1);
	}
	else
	{
		(*((uint64_t*)(phys_to_virt(pt))+vpn4*8))=((*((uint64_t*)(phys_to_virt(pt))+vpn4*8)) & 0x0);
	}
	
}
uint64_t page_table_query(uint64_t pt, uint64_t vpn)
{
	uint64_t pte=check_mapping(phys_to_virt(pt<<12),vpn>>36);
	for(int i=1;i<=4;i++)
	{
		if(pte==NO_MAPPING)
		{
			return NO_MAPPING;
			
		}
		else
		{
			pte=check_mapping(phys_to_virt(pte),(vpn>>(36-9*i))& 0x1ff);
		}
	}
	if(pte!=NO_MAPPING)
	{
		return pte>>12;
	}
	else
	{
		return NO_MAPPING;
	}
	
		
}

