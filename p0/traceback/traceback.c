/** @file traceback.c
 *  @brief The traceback function
 *
 *  This file contains the traceback function for the traceback library
 *      
 *  @time 2016-01-04 Mon 07:37 PM
 *  @author Harry Q. Bovik (hqbovik)
 *  @bug Could not pass the signal handling test case.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "contracts.h"
#include "traceback_internal.h"

int funcs_find(int addr){
	int index = 0;
	while(index <= FUNCTS_MAX_NUM){
		if(((int)functions[index].addr) == addr){
			return index;
		}
		index++;
	}
	return -1;
}
void print_func(FILE * fp, int ebp, int func_index){
	fprintf(fp, "Function %s(", functions[func_index].name);
	int i = 0;
	int offset = 0;
	char const *name;
	name = functions[func_index].args[i].name;
	if(strlen(name) == 0){
		fprintf(fp, "void");
	} else {
		do{
			offset = ebp + functions[func_index].args[i].offset;
			switch(functions[func_index].args[i].type){
				case TYPE_INT:
					fprintf(fp, "int %s=%d, ", name, *(int *)offset);  
					break;
				case TYPE_CHAR:
					if(isprint(*(char *)(offset))){
						fprintf(fp, "char %s='%c', ", name, 
								*(char *)offset);
					}else{
						fprintf(fp, "char %s='\\%3o', ", name, *(char *)offset);
					}
					break;
				case TYPE_FLOAT:
					fprintf(fp, "float %s=%0.6f, ", name, *(float *)offset);
					break;
				case TYPE_DOUBLE:
					fprintf(fp, "double %s=%0.6f, ", name, *(double *)offset);
					break;
				case TYPE_STRING:
					fprintf(fp, "char *%s=\"", name);
					offset = *(int *)offset;
					//fprintf(fp, "%u %u\n", (unsigned int)offset, 
					//		(unsigned int)getEspRegister());
					//if((unsigned int)offset <= (unsigned int)getEspRegister()){
					//	fprintf(fp, "%p\", ", (void *)offset);
					//	break;
					//}
					int strcount = 0;
				  while(*(char *)offset){
						if(++strcount > 25){
							fprintf(fp, "...");
							break;
						}
						if(isprint(*(char *)offset)){
							fprintf(fp, "%c", *(char *)offset);
						}else{
							fprintf(fp, "\\%3o", *(char *)offset);
						}
						fflush(fp);
						offset++;
					}
					fprintf(fp, "\", ");
					break;
				case TYPE_STRING_ARRAY:
					fprintf(fp, "char **%s={", name);
/*
 *
 * |           |
 * |-----------|
 * |  offset   +------>|-----------------|    _________
 * |-----------|       | string1 address +--->|'s'|'1'|
 *                     |-----------------|    |___|___| 
 *										 | string2 address +--->|'s'|'2'|
 *                     |-----------------|    |___|___| 
 *
 */
					int stroffset = *(int *)offset;
					// now start to print string array
					// 1. only print first 3 strings if possible
					// 2. NULL pointer means end of string array.
					// 3. One string has at most 25 charaters
					int count = 0;
					// The normal data memory starts from 0x08000000
					while((*(int *)stroffset) >= 0x08000000){
						if(++count > 3){
							fprintf(fp, "..., ");
							break;
						}
						if(strlen((char *)(*(int *)stroffset)) <= 25){
							fprintf(fp, "\"%s\", ", (char *)(*(int *)stroffset));
						}else{
							char str[29];
							strncpy(str, (char *)(*(int *)stroffset), 25);
							str[25] = '\0';
							strncat(str, "...", 3);
							fprintf(fp, "\"%s\", ", str);
						}
						stroffset = stroffset + 4;
					}
					fprintf(fp, "\b\b}, ");
					offset = offset + 4;
					break;
				case TYPE_VOIDSTAR:
					fprintf(fp, "void *%s=0v%x, ", name, ebp+offset);
					break;
				case TYPE_UNKNOWN:
					fprintf(fp, "UNKNOWN %s=%p, ", name, (void *)(ebp+offset));
					break;
				default:
					fprintf(fp, "\"Bug here\", ");

			}
			fflush(fp);
			if(++i >= ARGS_MAX_NUM){
				fprintf(fp, "..., ");
				break;
			}
			name = functions[func_index].args[i].name;
		}while(strlen(name) != 0);
		fprintf(fp, "\b\b");
	}
	fprintf(fp, "), in\n");
	fflush(fp);
}

void print_main(FILE * fp, int ebp, int func_index){
	fprintf(fp, "Function %s(", functions[func_index].name);
	fprintf(fp, ")\n");
}

void traceback(FILE *fp)
{
	/* the following just makes a sample access to "functions" array. 
	 * note if "functions" is not referenced somewhere outside the 
	 * file that it's declared in, symtabgen won't be able to find
	 * the symbol. So be sure to always do something with functions */

	/* remove this line once you've got real code here */
	//int count = 0;
	//for(count = 0; count < FUNCTS_MAX_NUM; count++){
	//	printf("functions[%d]: %s at %p\n", count, 
	//			functions[count].name, functions[count].addr);
	//}
	ASSERT(fp != NULL);
	int ebp;
	int eip;
	int func_addr;
	int func_index;
	ebp = getFirstStack();
	while(ebp){
		// Read Eip
		eip = *(int *)(ebp+4);
		// 0x502454ff is the machine code.
		// The machine code may be different from one OS to 
		// another. 
		if(*(int *)(eip-4) == 0x502454ff){
			// Found <main> here.
			func_addr =*(int *) (ebp + 8 + 0x50);
			func_index = funcs_find(func_addr);
			print_main(fp, ebp, func_index);
			return;
		}else{
			func_addr = eip + (*(int *)(eip - 4));
		}
		if((func_index = funcs_find(func_addr)) >= 0){
			print_func(fp, ebp, func_index);
#ifdef DEBUG
			fprintf(fp, "Found function %s(%p) at [%d]\n", 
					functions[func_index].name, (void *)func_addr, func_index);
			fflush(fp);
#endif
		}else{
			// Our functions array didnot hold infor about this function.
			//fprintf(fp, "Unknown %p\n", (void *)func_addr);	
			fprintf(fp, "Function %p(...), in\n", (void *)func_addr);
		}
		ebp = * (int *)ebp;
	}
}


