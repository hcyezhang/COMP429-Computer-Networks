
unsigned short cksum(unsigned short *buf, int count){
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
