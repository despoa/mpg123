#include "icy.h"
#include <stdlib.h>

void init_icy(struct icy_meta *icy)
{
	init_stringbuf(&icy->name);
	init_stringbuf(&icy->url);
	icy->data = NULL;
	icy->interval = 0;
	icy->next = 0;
	icy->changed = 0;
}

void clear_icy(struct icy_meta *icy)
{
	/* if pointers are non-null, they have some memory */
	free_stringbuf(&icy->name);
	free_stringbuf(&icy->url);
	free(icy->data);
	init_icy(icy);
}

void reset_icy(struct icy_meta *icy)
{
	clear_icy(icy);
	init_icy(icy);
}
/*void set_icy(struct icy_meta *icy, char* new_data)
{
	if(icy->data) free(icy->data);
	icy->data = new_data;
	icy->changed = 1;
}*/
