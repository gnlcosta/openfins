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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>

#include "omronfins.h"
#include "fins.h"

static omronplc *volatile plc_list;
static int plc_list_dim;
static pthread_mutex_t plc_list_mux;


static int OmronSendReceiv(int plc_id, void *send, int len , void *receiv, int rcv_len)
{
    int ret;
    char client_ko;
    struct sockaddr_in ip;
    socklen_t ip_len;
    finscmd *cmd;
    finsres *resp;
    
    ret = 0;
    ip_len = sizeof(struct sockaddr_in);
    cmd = (finscmd *)send;
    resp = (finsres *)receiv;
    
    pthread_mutex_lock(plc_list[plc_id].mux);
    cmd->hdr.sid = ++plc_list[plc_id].sid;
    if (sendto(plc_list[plc_id].sock, send, len, 0, (struct sockaddr*)plc_list[plc_id].plc_ip, ip_len) != len) {
        ret = -1;
        printf("send error\n");
    }
    else {
        do {
            client_ko = 0;
            if ((len = recvfrom(plc_list[plc_id].sock, receiv, rcv_len, 0, (struct sockaddr*)&ip, &ip_len)) < 0) {
                ret = -1;
            }
            else {
                ret = len;
                if (plc_list[plc_id].plc_ip->sin_addr.s_addr != ip.sin_addr.s_addr || plc_list[plc_id].plc_ip->sin_port != ip.sin_port) {
                    client_ko = 1;
                    ip_len = sizeof(struct sockaddr_in);
                    printf("Message from another client\n");
                }
                else if (cmd->hdr.sid != resp->hdr.sid) {
                    client_ko = 1;
                    ip_len = sizeof(struct sockaddr_in);
                    printf("Message delayed\n");
                }
            }
        } while (client_ko);
    }
    pthread_mutex_unlock(plc_list[plc_id].mux);
    
    if (ret != -1) {
        if (len < 14 || resp->mres != 0 || resp->sres != 0) {
            ret = -1;
            if (len < 14)
                printf("FINS length error\n");
            else
                printf("Response error\n");
        }
    }
    
    return ret;
}


int OmronInit(void)
{
    plc_list = NULL;
    plc_list_dim = 0;
    pthread_mutex_init(&plc_list_mux, NULL);
    
    return 0;
}


int OmronOpen(char *ip, unsigned short port, char tcp, unsigned char net_addr)
{
    int i;
    omronplc *next;
    struct timeval tv;
    struct sockaddr_in ws_addr;
    
    pthread_mutex_lock(&plc_list_mux);
    for (i=0; i!=plc_list_dim; i++) {
        if (plc_list[i].sock == -1) {
            break;
        }
    }
    if (i == plc_list_dim) {
        next = realloc(plc_list, (plc_list_dim+PLC_MIN_NUMBER)*sizeof(omronplc));
        if (next == NULL) {
            pthread_mutex_unlock(&plc_list_mux);
            return -1;
        }
        plc_list = next;
        for (i=plc_list_dim; i!=(plc_list_dim+PLC_MIN_NUMBER); i++) {
            plc_list[i].sock = -1;
            plc_list[i].mux = malloc(sizeof(pthread_mutex_t)); /* neccesary to avoid problems after "realloc" */
            pthread_mutex_init(plc_list[i].mux, NULL);
            plc_list[i].plc_ip = malloc(sizeof(struct sockaddr_in));
        }
        i = plc_list_dim;
        plc_list_dim += PLC_MIN_NUMBER;
    }
    
    /* socket */
    if ((plc_list[i].sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        pthread_mutex_unlock(&plc_list_mux);
        printf("can't open datagram socket\n");
        return -1;
    }
    memset((char *)&ws_addr, 0, sizeof(ws_addr));
    ws_addr.sin_family = AF_INET;
    ws_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ws_addr.sin_port = htons(0);
    tv.tv_sec = 0;
    tv.tv_usec = RESP_TIME_MS_MAX*1000;
    if (bind(plc_list[i].sock, (struct sockaddr *)&ws_addr, sizeof(ws_addr)) < 0) {
        close(plc_list[i].sock);
        plc_list[i].sock = -1;
        pthread_mutex_unlock(&plc_list_mux);
        printf("can't bind local address\n");
        return -1;
    }
    if (setsockopt(plc_list[i].sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        close(plc_list[i].sock);
        plc_list[i].sock = -1;
        pthread_mutex_unlock(&plc_list_mux);
        perror("Error");
        return -1;
    }
    
    memset((char *)plc_list[i].plc_ip, 0, sizeof(struct sockaddr_in));
    plc_list[i].plc_ip->sin_family = AF_INET;
    plc_list[i].plc_ip->sin_addr.s_addr = inet_addr(ip);
    plc_list[i].plc_ip->sin_port = htons(port);
    plc_list[i].sid = 1;
    plc_list[i].net_addr = net_addr;
    pthread_mutex_unlock(&plc_list_mux);
    
    return i;
}


int OmronClose(int plc_id)
{
    close(plc_list[plc_id].sock);
    plc_list[plc_id].sock = -1;
    
    return 0;
}


int OmronReadControllerModel(int plc_id, char *model, char *version)
{
    finscontroller_cmd cmd;
    finscontroller_res resp;
    int len, ret;
    
    ret = -1;
    cmd.hdr.icf = 0x80;
    cmd.hdr.rsv = 0x00;
    cmd.hdr.gct = 0x02;
    cmd.hdr.dna = 0x00;
    cmd.hdr.da1 = plc_list[plc_id].net_addr;
    cmd.hdr.da2 = 0x00;
    cmd.hdr.sna = 0x00;
    cmd.hdr.sa1 = 0x63;
    cmd.hdr.sa2 = 0x00;
    cmd.hdr.mrc = MRC_CONTR_CONNEC_DATA;
    cmd.hdr.src = SRC_CONTROLLER_DATA_READ;
    cmd.area = 0x00; /* controller mode, controller version, area data */

    len = sizeof(finscontroller_cmd);
    
    len = OmronSendReceiv(plc_id, &cmd, len, &resp, sizeof(resp));
    
    if (len != -1) {
        if ((cmd.hdr.dna != resp.hdr.sna) || (cmd.hdr.da1 != resp.hdr.sa1) || (cmd.hdr.da2 != resp.hdr.sa2) ) {
            printf("Illegal PLC source address error\n");
        }
        else {
            ret = 0;
            if (model != NULL) {
                memcpy(model, resp.model, 20);
                model[20] = '\0';
            }
            if (version != NULL) {
                memcpy(version, resp.version, 20);
                version[20] = '\0';
            }
        }
    }

    return ret;
}


int OmronReadMem(int plc_id, char type, unsigned short addr, short num, short *data)
{
    finsmemory_cmd cmd;
    finsres resp;
    int i, len, ret;
    
    ret = -1;
    cmd.hdr.icf = 0x80;
    cmd.hdr.rsv = 0x00;
    cmd.hdr.gct = 0x02;
    cmd.hdr.dna = 0x00;
    cmd.hdr.da1 = plc_list[plc_id].net_addr;
    cmd.hdr.da2 = 0x00;
    cmd.hdr.sna = 0x00;
    cmd.hdr.sa1 = 0x63;
    cmd.hdr.sa2 = 0x00;
    cmd.hdr.mrc = MRC_MEMORY;
    cmd.hdr.src = SRC_MEMORY_READ;
    switch (type) {
    case 'A':
    case 'a':
        cmd.area = 0xB3;
        break;
        
    case 'C':
    case 'c':
        cmd.area = 0xB0;
        break;
        
    case 'D':
    case 'd':
        cmd.area = 0x82;
        break;
        
    case 'H':
    case 'h':
        cmd.area = 0xB2 ;
        break;
        
    case 'W':
    case 'w':
        cmd.area = 0xB1;
        break;
    }
    cmd.addr = htons(addr);
    cmd.bit = 0x00;
    cmd.num = htons(num);

    len = sizeof(finsmemory_cmd);
    
    len = OmronSendReceiv(plc_id, &cmd, len, &resp, sizeof(resp));
    
    if (len != -1) {
        if ((cmd.hdr.dna != resp.hdr.sna) || (cmd.hdr.da1 != resp.hdr.sa1) || (cmd.hdr.da2 != resp.hdr.sa2) ) {
            printf("Illegal PLC source address error\n");
        }
        else {
            ret = 0;
            for (i=0; i!=num; i++)
                data[i] = ntohs(((short *)&resp.data)[i]);
        }
    }

    return ret;
}


int OmronWriteMem(int plc_id, char type, unsigned short addr, short num, short *data)
{
    finsmemory_cmd_wr cmd;
    finsres resp;
    int i, len, ret, dim;
    
    ret = -1;
    cmd.hdr.icf = 0x80;
    cmd.hdr.rsv = 0x00;
    cmd.hdr.gct = 0x02;
    cmd.hdr.dna = 0x00;
    cmd.hdr.da1 = plc_list[plc_id].net_addr;
    cmd.hdr.da2 = 0x00;
    cmd.hdr.sna = 0x00;
    cmd.hdr.sa1 = 0x63;
    cmd.hdr.sa2 = 0x00;
    cmd.hdr.mrc = MRC_MEMORY;
    cmd.hdr.src = SRC_MEMORY_WRITE;
    switch (type) {
    case 'A':
    case 'a':
        cmd.area = 0xB3;
        dim = 2;
        break;
        
    case 'C':
    case 'c':
        cmd.area = 0xB0;
        dim = 2;
        break;
        
    case 'D':
    case 'd':
        cmd.area = 0x82;
        dim = 2;
        break;
        
    case 'H':
    case 'h':
        cmd.area =0xB2;
        dim = 2;
        break;
        
    case 'W':
    case 'w':
        cmd.area = 0xB1;
        dim = 2;
        break;
    }
    cmd.addr = htons(addr);
    cmd.bit = 0x00;
    cmd.num = htons(num);
    for (i=0; i!=num; i++) {
        ((short *)&cmd.data)[i] = htons(data[i]);
    }

    len = sizeof(finsmemory_cmd)+dim*num;
    
    len = OmronSendReceiv(plc_id, &cmd, len, &resp, sizeof(resp));
    
    if (len != -1) {
        if ((cmd.hdr.dna != resp.hdr.sna) || (cmd.hdr.da1 != resp.hdr.sa1) || (cmd.hdr.da2 != resp.hdr.sa2) ) {
            printf("Illegal PLC source address error\n");
        }
        else {
            ret = 0;
        }
    }

    return ret;
}


int OmronReadFloat(int plc_id, char type, unsigned short addr, float *data)
{
    return -1;
}


int OmronWriteFloat(int plc_id, char type, unsigned short addr, float *data)
{
    return -1;
}


int OmronReadBitMem(int plc_id, char type, unsigned short addr, char bit, short num, short *data)
{
    return -1;
}


int OmronWriteBitMem(int plc_id, char type, unsigned short addr, char bit, short num, short *data)
{
    return -1;
}


#ifdef MAIN
int main(int argc, char *argv[])
{
    int id;
    short data[2], i;
    char model[30];
    char version[30];
    unsigned short start, error;
    
    i = 0;
    error = 0;
    start = 0;
    OmronInit();
    id = OmronOpen("192.168.250.1", 9600, 0, 1);
    if (id != -1) {
        if (OmronReadControllerModel(id, model, version) == 0)
            printf("PLC: %s [%s]\n", model, version);
        while (1) {
#if 1
            if (OmronReadMem(id, 'C', 100, 2, data) == 0) {
                printf("Time: %i", (unsigned short)data[0]);
                
                if (start == 0)
                    start = (unsigned short)data[0];
                if (start != (unsigned short)data[0]) {
                    printf(" dato: %i [%d]", data[1], data[1]/((unsigned short)data[0]-start));
                    printf(" error: %i", error);
                }
                printf("\n");
            }
            else
                error++;
#endif
            if (i>5) {
                if (OmronWriteMem(id, 'C', 101, 1, &i) == 0)
                    ;//printf("ok\n");
                else
                    error++;
            }
            i++;
            //sleep(1);
        }
    }
    
    return 0;
}
#endif
