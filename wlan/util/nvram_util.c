/***********************************************************************
 *
 *  Copyright (c) 2008  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

 This program is the proprietary software of Broadcom Corporation and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 *
 ************************************************************************/

/* This file is used to write data wlan params into  nvram */

/* 
Input file format

ID=pci/bus/slot
offset=value
...
ID=pci/bus/slot
offset=value
...
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include "board.h"

#define d_printf(args...) 	{ if (dbg) \
							printf(args);  \
						}


typedef struct entry_struct {
unsigned short offset;
unsigned short value;
} Entry_struct;

typedef struct adapter_struct {
unsigned char id[16];            /*format pci/0/1 */
unsigned short entry_num;
struct entry_struct entry[100];
} Adapter_struct;

typedef struct params_struct {
char tag[4];
unsigned short adapter_num;
struct adapter_struct adapter[4];
} Params_struct;

#define BOARD_DEVICE_NAME  "/dev/brcmboard"
#define NVRAM_WLAN_TAG "WLANDATA" 

unsigned char dbg= 0, readOnly= 0, nvramClean = 0;


/*Write parameter into Flash*/
int  boardIoctl(int boardIoctl,
                         BOARD_IOCTL_ACTION action,
                         char *string,
                         int strLen,
                         int offset,
                         void *data)
{
    BOARD_IOCTL_PARMS ioctlParms;
    int boardFd = 0;
    int rc;
    int ret=0;

   d_printf("%s@%d\n", __FUNCTION__, __LINE__ );

    boardFd = open(BOARD_DEVICE_NAME, O_RDWR);

    if ( boardFd != -1 )
    {
        ioctlParms.string = string;
        ioctlParms.strLen = strLen;
        ioctlParms.offset = offset;
        ioctlParms.action = action;
        ioctlParms.buf    = data;
        ioctlParms.result = -1;

        rc = ioctl(boardFd, boardIoctl, &ioctlParms);
		

           if (rc < 0)
           {
              printf("boardIoctl=0x%x action=%d rc=%d\n", boardIoctl, action, rc);
              ret = -1;
           }
        
    }
    else
    {
       printf("Unable to open device %s", BOARD_DEVICE_NAME);
       ret = -1;
    }

    close(boardFd);
    return ret;
}


int main(int argc, char** argv)
{
    FILE *fp;
    char name[32], array[2048];
    unsigned int offset, value;
    char line[64], *ptr;
    int cnt=0;
	
    struct params_struct params;

    struct adapter_struct *adapter_ptr=NULL;
    struct entry_struct *entry_ptr = NULL;
    int i=0, j=0;
    
    int size =0;
    unsigned short *data_ptr;

    memset(&params, 0, sizeof(params) );

    /*Parse the argv parameters*/
    if ( argc < 2 ) {
	 printf("Usage: \n");
	 printf("nvramUpdate {DataFileName | Clean} \n");
	 printf("Clean: Clean up Nvram\n");
	 printf("DataFileName: Nvram data file name, formated as:\n");
	 printf("ID=pci/bus/slot/\n");
	 printf("offset1=value1\n");
	 printf("...\n");
	 printf("ID=pci/bus/slot/\n");
	 printf("offset1=value1\n");
	 printf("......\n");
	 printf("offsetn=valuen\n");
	 printf("\n\n Offset is decimal, 16bit aligment offset(not 8bit alignment offset)\n");
	 printf("\n\n value is 16bit hexical data\n");
	 
    	 return 1;
    }

    if ( argc > 1 ) {
    		if ( !strcmp( argv[1], "Clean" ) ) {
		    nvramClean = 1;
		}
    		if ( !strcmp( argv[1], "Read" ) ) {
		    dbg =1;
		    readOnly = 1;
		}
    }

    if ( argc > 2 ) {
		if ( !strcmp(argv[2], "Debug") )
		    dbg = 1;
    }

    /* Cleanup the Cal data Nvram*/
    if ( nvramClean ) { 	
        ptr = malloc( sizeof(params) );
           size = 0;
           if ( ptr == NULL ) {
                 printf("Memory Allocation Failure\n");
                 return 1;
           }

           memset( ptr, 0, sizeof(params) );
           strncpy(ptr, NVRAM_WLAN_TAG, strlen(NVRAM_WLAN_TAG) );
		   
	    /*Save to flash*/
          boardIoctl(BOARD_IOCTL_FLASH_WRITE,
                                       NVRAM,
                                       ptr,
                                       sizeof(params),
                                       0, 0);

           free(ptr);
           printf("Cal Data Nvram Clean up\n");
           return 0;
    }

    /*Flash the cal data parameter*/
    if ( !readOnly ) { 	
           fp=fopen(argv[1],"r");
           if ( fp == NULL ) {
                printf("File %s could not be read\n", argv[1]);
                return 1;
           }

           /*Parse the input file and create the data structure written into Flash*/
           while( !feof(fp) ){
                 cnt = fscanf(fp, "%s\n", line );
	    	
                 if ( cnt != 1 ) /*Skip empty line*/
                       continue;

                 d_printf("line=%s\n", line );    		
                 ptr= strchr(line, '=');

                 /*Comment line*/
                 if ( ptr == NULL )
                      continue;
	    	
                 *ptr = '\0';
	    	
                 strcpy( name, line );

                 d_printf("[%s]=", name);

                 /*Start a new Adapter parameters*/
                 if ( !strcmp(name, "ID") ) {
                     adapter_ptr= (params.adapter + params.adapter_num );
		
                     strcpy((char *)(adapter_ptr->id), (ptr+1) );
                     d_printf("[%s]\n", ptr+1);
                     adapter_ptr->entry_num =0;
	        
                     entry_ptr = adapter_ptr->entry;
                     params.adapter_num++;
                     if ( params.adapter_num > 4 ) {
                            printf("Too many Adapters in the config file\n");
                            fclose(fp);
                            return 1;
                    }
                    continue;
               }
	      
               if ( params.adapter_num == 0 ) {
                      printf("The fist line should be ID=xxx to specify the adapter ID\n");
                      fclose(fp);
                      return -1;
               }

               /*Fetch a new entry and save it*/
               sscanf( name, "%d", &offset );
               sscanf( (ptr+1), "%x", &value );
	    		
               d_printf("[%x]\n", value );

               entry_ptr->offset = (unsigned short)(offset&0xffff) ;
               entry_ptr->value = (unsigned short)(value&0xffff) ;
               adapter_ptr->entry_num++;
               if ( adapter_ptr->entry_num > 100 ) {
                     printf("Too many Entry in the config file\n");
                     fclose(fp);
                     return 1;
               }
               entry_ptr++;
	   		
            }		

            fclose(fp);
	    

            /*Transfer into a memory area, in order to save to flash*/
           ptr = malloc( sizeof(params) );
           size = 0;
           if ( ptr == NULL ) {
                 printf("Memory Allocation Failure\n");
                 return 1;
           }

           memset( ptr, 0, sizeof(params) );
		
           strncpy(ptr, NVRAM_WLAN_TAG, strlen(NVRAM_WLAN_TAG) );

           data_ptr = (unsigned short *)( ptr + strlen(NVRAM_WLAN_TAG) ) ;
		
           *data_ptr = params.adapter_num;
           data_ptr++;
		
           for ( i=0; i< params.adapter_num; i++ ){
                d_printf("\nadapter[%d] parames\n", i);
           	  d_printf("ID=%s\n", params.adapter[i].id );
                strcpy( (char *)data_ptr, (char *)(params.adapter[i].id) );
                data_ptr += 8;

                d_printf("entry_num=%d\n", params.adapter[i].entry_num);
                *data_ptr = params.adapter[i].entry_num;
                data_ptr++;
                for ( j=0; j<params.adapter[i].entry_num; j++ ) {
                      d_printf("offset=%d value =0x%x\n",
              	        params.adapter[i].entry[j].offset,
              	        params.adapter[i].entry[j].value);
                      *data_ptr = params.adapter[i].entry[j].offset;
                      data_ptr++;
                      *data_ptr = params.adapter[i].entry[j].value;
                      data_ptr++;
              }
          }

          size = (char *)data_ptr - ptr;
		
           if ( dbg ) {
                  d_printf("mem data[%x]:\n", size);

                  for ( i=0; i<size; i++ ) {
                          if ( i%16 == 0 ) {
                               d_printf("\n%04x--", i);
                          }
                          d_printf("[%02x]", (unsigned char)*(ptr+i) );
		    }
          }
		
	    /*Save to flash*/
          boardIoctl(BOARD_IOCTL_FLASH_WRITE,
                                       NVRAM,
                                       ptr,
                                       size,
                                       0, 0);

           free(ptr);
       }

       if ( dbg ) {
	    /*Dbg? read it back and check*/
	    boardIoctl(BOARD_IOCTL_FLASH_READ,
	                                NVRAM,
	                                array,
	                                1024,
	                                0, 0);
	   
	    d_printf("Cfe Data:\n");
			
	    for ( i=0; i<1024; i++ ) {
			if ( i%16 == 0 ) {
				d_printf("\n%04x--", i);
			}
			d_printf("[%02x]", (unsigned char)array[i] );
	    }
    	}

	
    return 0;
}


