/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"
#include <stdio.h>
#include <malloc.h>

extern void ip_DiscardPkt(char* pBuffer,int type);

extern void ip_SendtoLower(char*pBuffer,int length);

extern void ip_SendtoUp(char *pBuffer,int length);

extern unsigned int getIpv4Address();

// implemented by students

int stud_ip_recv(char *pBuffer,unsigned short length)
{
    int version = pBuffer[0] >> 4;                             // 版本号 提取高四位
    int head_length = pBuffer[0] & 0xf;                        // 头部长度 提取低四位
    short ttl = (unsigned short)pBuffer[8];                    // 生存时间
    short checksum = ntohs(*(unsigned short *)(pBuffer + 10)); // 头部校验和
    int destination = ntohl(*(unsigned int *)(pBuffer + 16));  // 目的地址

    if (ttl == 0)
    {
        // 生存时间为0, 已过期
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
        return 1;
    }
    if (version != 4)
    {
        // 版本号不为4
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
        return 1;
    }
    if (head_length < 5)
    {
        // 头部长度低于20字节
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
        return 1;
    }
    if (destination != getIpv4Address() && destination != 0xffff) 
    {
	// 不是本机地址或广播地址
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR);
        return 1;
    }
    // 计算头部校验和
    unsigned long sum = 0;
    unsigned long temp = 0;
    int i;
    for (i = 0; i < head_length * 2; i++)
    {
	// 取16位 相加
        temp = ((unsigned char)pBuffer[i * 2] << 8) + (unsigned char)pBuffer[i * 2 + 1];
        sum += temp;
        temp = 0;
    }
    unsigned short low = sum & 0xffff;
    unsigned short high = sum >> 16;
    while(high != 0)
    {
	// 若高16位不为0, 将高16位与低16位相加
	low += high;
	high = low >> 16;
    }
    sum = low;
    if (sum != 0xffff) 
    {
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
        return 1;
    }
    // 由本机接收 发送到上层
    ip_SendtoUp(pBuffer, length);
    return 0;
}

int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,
				   unsigned int dstAddr,byte protocol,byte ttl)
{
    // 默认头部长度为20Bytes
    short ip_length = len + 20;
    char *buffer = (char *)malloc(ip_length * sizeof(char));
    memset(buffer, 0, ip_length);
    buffer[0] = 0x45;  // 版本号 头部长度
    buffer[8] = ttl;  // 生存时间
    buffer[9] = protocol;   // 上层协议号
    // 转换为网络字节序
    unsigned short network_length = htons(ip_length); // 总长度
    memcpy(buffer + 2, &network_length, 2);

    unsigned int src = htonl(srcAddr);  // 源地址
    unsigned int dst = htonl(dstAddr);  // 目的地址
    memcpy(buffer + 12, &src, 4);
    memcpy(buffer + 16, &dst, 4);
    // 计算头部校验和
    unsigned long sum = 0;
    unsigned long temp = 0;
    int i;
    for (i = 0; i < 10; i++)
    {
	  // 取16位 相加
        temp = ((unsigned char)buffer[i * 2] << 8) + (unsigned char)buffer[i * 2 + 1];
        sum += temp;
        temp = 0;
    }
    unsigned short low = sum & 0xffff;
    unsigned short high = sum >> 16;
    while(high != 0)
    {
	// 若高16位不为0, 将高16位与低16位相加
	low += high;
	high = low >> 16;
    }
    unsigned short checksum = low;
    checksum = ~checksum;
    unsigned short header_checksum = htons(checksum);
    
    memcpy(buffer + 10, &header_checksum, 2);
    memcpy(buffer + 20, pBuffer, len);

    ip_SendtoLower(buffer, len + 20);
    return 0;
}
