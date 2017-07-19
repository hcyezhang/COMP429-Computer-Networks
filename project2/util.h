#ifndef _UTIL_H
#define _UTIL_H
#define WINDOW_LEN 8
#define SEQ_LEN 16
#define BUFLEN 32768

typedef unsigned short u_short;
unsigned short chksum(unsigned short *buf, int count){
    register unsigned long sum = 0;
    while(count --){
        sum += *buf++;
        if (sum & 0xFFFF0000){
            sum &= 0xFFFF;
            sum ++;
        }
    }
    return ~(sum & 0xFFFF);
}

int between(int low, int high, int ack_num){//to demo
    if(low <= high){
        if(ack_num >= low && ack_num <= high){
            return 1;
        }
    }
    else{
        if (ack_num >= low || ack_num <= high) {
            return 1;
        }
    }
    return 0;
}

int map_to_window(int low, int ind){
	if(ind < low){
		return ind + (SEQ_LEN - low);
	}
	return ind - low;
}

int mod(int a, int b)
{
  int r = a % b;
  return r < 0 ? r + b : r;
}

#endif
