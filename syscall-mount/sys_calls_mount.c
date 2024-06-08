/*
* 
* This is free software; you can redistribute it and/or modify it under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version.
* 
* This module is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
* @file sys_calls_mount.c 
* @brief This is a module that loads on the system call tables the following system calls
*        to handle operations over the reference monitor:
*        - reference_on     --> switch to on mode
*        - reference_off    --> switch to off mode
*        - reference_rec_on --> switch to rec_on mode
*        - reference_rec_off--> switch to rec_off mode
*
* @author Staccone Simone
*
* @date June 08, 2024
*/


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Staccone Simone <simone.staccone@virgilio.it>");
MODULE_DESCRIPTION("SYS-CALLS-MOUNT");



#define MODNAME "SYS-CALLS-MOUNT"




int init_module(void) {
    printk("%s: module correctly started\n",MODNAME); 
	
    // TODO: Stuff....


    printk("%s: module correctly installed\n",MODNAME);
    return 0;
}

void cleanup_module(void) {
    printk("%s: shutting down\n",MODNAME);   
}
