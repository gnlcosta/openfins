/**
 * OpenFins is a library that allows communicating with Omron PLC devices through ethernet.
 * 
 * By Gianluca Costa <g.costa@xplico.org>
 * Copyright 2014 Gianluca Costa. Web: www.openfins.io
 * 
 *
 * This  program  is free software:  you can  redistribute it and/or
 * modify  it  under  the terms  of  the  GNU Affero  General Public
 * License  as  published  by the  Free Software Foundation,  either 
 * version 3 of the License,  or (at your option) any later version.
 * 
 * This program is  distributed in  the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;  without even  the implied  warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Affero General Public License for more details.
 * 
 * You should have received a copy of the  GNU Affero General Public
 * License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifndef _FINS_H_
#define _FINS_H_

#include <pthread.h>

#define RESP_TIME_MS_MAX           300  /* ms < 1000 */
#define PLC_MIN_NUMBER               2

/* packets FINS/UDP FINS/TCP */
#define FINS_HD_ICF_BIT_REQ_RESP   0x01
#define FINS_HD_ICF_BIT_RESPONCE   0x40

/* commands */
#define MRC_MEMORY                 0x01
#define SRC_MEMORY_READ            0x01
#define SRC_MEMORY_WRITE           0x02
#define SRC_MEMORY_FILL            0x03
#define SRC_MEMORY_MULTI_READ      0x04
#define SRC_MEMORY_TRANSF          0x05

#define MRC_CONTR_CONNEC_DATA      0x05
#define SRC_CONTROLLER_DATA_READ   0x01
#define SRC_CONNECTION_DATA_READ   0x02



#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */
typedef struct _finsheader_t finsheader;
struct _finsheader_t {
    unsigned char icf;
    unsigned char rsv;
    unsigned char gct;
    unsigned char dna;
    unsigned char da1;
    unsigned char da2;
    unsigned char sna;
    unsigned char sa1;
    unsigned char sa2;
    unsigned char sid;
    /* command */
    unsigned char mrc;
    unsigned char src;
};

typedef struct _finscmd_t finscmd;
struct _finscmd_t {
    finsheader hdr;
    unsigned char data[1998];
};

typedef struct _finsres_t finsres;
struct _finsres_t {
    finsheader hdr;
    /* responce */
    unsigned char mres;   /* main response code */
    unsigned char sres;   /* sub-response code */
    
    unsigned char data[1996];
};

/* memory commands/responce */
typedef struct _finsmemory_cmd_t finsmemory_cmd;
struct _finsmemory_cmd_t {
    finsheader hdr;
    unsigned char area;     /* memory area code */
    unsigned short addr;    /* address */
    unsigned char bit;      /* bit */
    unsigned short num;     /* no. of items */
};

typedef struct _finsmemory_cmd_wr_t finsmemory_cmd_wr;
struct _finsmemory_cmd_wr_t {
    finsheader hdr;
    unsigned char area;     /* memory area code */
    unsigned short addr;    /* address */
    unsigned char bit;      /* bit */
    unsigned short num;     /* no. of items */
    unsigned char data[1994];  /* data */
};

typedef struct _finsmemory_res_t finsmemory_res;
struct _finsmemory_res_t {
    finsheader hdr;
    /* responce */
    unsigned char mres;        /* main response code */
    unsigned char sres;        /* sub-response code */
    /* memory responce */
    unsigned char area;        /* memory area code */
    unsigned short addr;       /* address */
    unsigned char bit;         /* bit */
    unsigned char data[1992];  /* data */
};

/* controller data area */
typedef struct _finscontroller_cmd_t finscontroller_cmd;
struct _finscontroller_cmd_t {
    finsheader hdr;
    unsigned char area;     /* area code */
};

typedef struct _finscontroller_res_t finscontroller_res;
struct _finscontroller_res_t {
    finsheader hdr;
    /* responce */
    unsigned char mres;        /* main response code */
    unsigned char sres;        /* sub-response code */
    /* controller area responce */
    char model[20];            /* controller model */
    char version[20];          /* controller version */
    unsigned char system[40];  /* for system use */
    unsigned char data[12];    /* area data */
};



#pragma pack(pop)   /* restore original alignment from stack */

typedef struct _omronplc_t omronplc;
struct _omronplc_t {
    unsigned char net_addr; /* node address */
    char tcp;      /* tcp or udp */
    int sock;      /* socket */
    struct sockaddr_in *plc_ip;
    unsigned char sid;
    pthread_mutex_t *mux;
};

#endif
