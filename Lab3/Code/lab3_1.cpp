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
    int version = pBuffer[0] >> 4;                             // �汾�� ��ȡ����λ
    int head_length = pBuffer[0] & 0xf;                        // ͷ������ ��ȡ����λ
    short ttl = (unsigned short)pBuffer[8];                    // ����ʱ��
    short checksum = ntohs(*(unsigned short *)(pBuffer + 10)); // ͷ��У���
    int destination = ntohl(*(unsigned int *)(pBuffer + 16));  // Ŀ�ĵ�ַ

    if (ttl == 0)
    {
        // ����ʱ��Ϊ0, �ѹ���
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
        return 1;
    }
    if (version != 4)
    {
        // �汾�Ų�Ϊ4
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
        return 1;
    }
    if (head_length < 5)
    {
        // ͷ�����ȵ���20�ֽ�
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
        return 1;
    }
    if (destination != getIpv4Address() && destination != 0xffff) 
    {
	// ���Ǳ�����ַ��㲥��ַ
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR);
        return 1;
    }
    // ����ͷ��У���
    unsigned long sum = 0;
    unsigned long temp = 0;
    int i;
    for (i = 0; i < head_length * 2; i++)
    {
	// ȡ16λ ���
        temp = ((unsigned char)pBuffer[i * 2] << 8) + (unsigned char)pBuffer[i * 2 + 1];
        sum += temp;
        temp = 0;
    }
    unsigned short low = sum & 0xffff;
    unsigned short high = sum >> 16;
    while(high != 0)
    {
	// ����16λ��Ϊ0, ����16λ���16λ���
	low += high;
	high = low >> 16;
    }
    sum = low;
    if (sum != 0xffff) 
    {
        ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
        return 1;
    }
    // �ɱ������� ���͵��ϲ�
    ip_SendtoUp(pBuffer, length);
    return 0;
}

int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,
				   unsigned int dstAddr,byte protocol,byte ttl)
{
    // Ĭ��ͷ������Ϊ20Bytes
    short ip_length = len + 20;
    char *buffer = (char *)malloc(ip_length * sizeof(char));
    memset(buffer, 0, ip_length);
    buffer[0] = 0x45;  // �汾�� ͷ������
    buffer[8] = ttl;  // ����ʱ��
    buffer[9] = protocol;   // �ϲ�Э���
    // ת��Ϊ�����ֽ���
    unsigned short network_length = htons(ip_length); // �ܳ���
    memcpy(buffer + 2, &network_length, 2);

    unsigned int src = htonl(srcAddr);  // Դ��ַ
    unsigned int dst = htonl(dstAddr);  // Ŀ�ĵ�ַ
    memcpy(buffer + 12, &src, 4);
    memcpy(buffer + 16, &dst, 4);
    // ����ͷ��У���
    unsigned long sum = 0;
    unsigned long temp = 0;
    int i;
    for (i = 0; i < 10; i++)
    {
	  // ȡ16λ ���
        temp = ((unsigned char)buffer[i * 2] << 8) + (unsigned char)buffer[i * 2 + 1];
        sum += temp;
        temp = 0;
    }
    unsigned short low = sum & 0xffff;
    unsigned short high = sum >> 16;
    while(high != 0)
    {
	// ����16λ��Ϊ0, ����16λ���16λ���
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
