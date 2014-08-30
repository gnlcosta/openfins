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
 
#ifndef _OMRONFINS_PRIV_H_
#define _OMRONFINS_PRIV_H_
 
int OmronInit(void);
int OmronOpen(char *ip, unsigned short port, char tcp, unsigned char net_addr);
int OmronClose(int plc_id);
int OmronReadControllerModel(int plc_id, char *model, char *version);
int OmronReadMem(int plc_id, char type, unsigned short addr, short num, short *data);
int OmronWriteMem(int plc_id, char type, unsigned short addr, short num, short *data);
int OmronReadFloat(int plc_id, char type, unsigned short addr, float *data);
int OmronWriteFloat(int plc_id, char type, unsigned short addr, float *data);
int OmronReadBitMem(int plc_id, char type, unsigned short addr, char bit, short num, short *data);
int OmronWriteBitMem(int plc_id, char type, unsigned short addr, char bit, short num, short *data);

#endif
