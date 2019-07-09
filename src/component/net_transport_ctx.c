//
// Created by hujianzhe on 19-7-9.
//

#include "../../inc/component/net_transport_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

NetPacket_t* netpacketSharding(const Iobuf_t iov[], unsigned int iovcnt, unsigned short mtu, int(*get_hdrlen)(int)) {
	unsigned int i, nbytes;
	if (!iov || !iovcnt) {
		iovcnt = 0;
		nbytes = 0;
	}
	else {
		for (nbytes = 0, i = 0; i < iovcnt; ++i)
			nbytes += iobufLen(iov + i);
		if (0 == nbytes)
			iovcnt = 0;
	}
	if (s->ctx.reliable) {
		if (ESTABLISHED_STATUS == s->ctx.m_status) {
			NetPacket_t* packet = NULL;
			int hdrlen;
			if (nbytes) {
				unsigned int offset, packetlen, copy_off, i_off;
				List_t packetlist;
				listInit(&packetlist);
				for (i = i_off = offset = 0; offset < nbytes; offset += packetlen) {
					packetlen = nbytes - offset;
					hdrlen = get_hdrlen ? get_hdrlen(RELIABLE_DGRAM_DATA_HDR_LEN + packetlen) : 0;
					if (hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN + packetlen > s->ctx.mtu) {
						packetlen = s->ctx.mtu - hdrlen - RELIABLE_DGRAM_DATA_HDR_LEN;
						hdrlen = s->ctx.hdrlen ? s->ctx.hdrlen(RELIABLE_DGRAM_DATA_HDR_LEN + packetlen) : 0;
					}
					packet = (Packet_t*)malloc(sizeof(Packet_t) + hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN + packetlen);
					if (!packet)
						break;
					packet->msg.type = SESSION_PACKET_MESSAGE;
					packet->s = s;
					packet->resendtimes = 0;
					packet->hdrlen = hdrlen;
					packet->len = hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN + packetlen;
					packet->data[hdrlen] = HDR_DATA;
					if (hdrlen && s->ctx.encode) {
						s->ctx.encode(packet->data, RELIABLE_DGRAM_DATA_HDR_LEN + packetlen);
					}
					copy_off = 0;
					while (i < iovcnt) {
						unsigned int copy_len;
						if (iobufLen(iov + i) - i_off > packetlen - copy_off) {
							copy_len = packetlen - copy_off;
							memcpy(packet->data + hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN + copy_off, ((char*)iobufPtr(iov + i)) + i_off, copy_len);
							i_off += copy_len;
							break;
						}
						else {
							copy_len = iobufLen(iov + i) - i_off;
							memcpy(packet->data + hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN + copy_off, ((char*)iobufPtr(iov + i)) + i_off, copy_len);
							copy_off += copy_len;
							i_off = 0;
							++i;
						}
					}
					listInsertNodeBack(&packetlist, packetlist.tail, &packet->msg.m_listnode);
				}
				if (offset >= nbytes) {
					packet->data[packet->hdrlen] |= HDR_DATA_END_FLAG;
					sessionloop_exec_msglist(s->m_loop, &packetlist);
				}
				else {
					ListNode_t* cur, *next;
					for (cur = packetlist.head; cur; cur = next) {
						next = cur->next;
						free(cur);
					}
					return NULL;
				}
			}
			else {
				hdrlen = s->ctx.hdrlen ? s->ctx.hdrlen(RELIABLE_DGRAM_DATA_HDR_LEN) : 0;
				packet = (Packet_t*)malloc(sizeof(Packet_t) + hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN);
				if (!packet)
					return NULL;
				packet->msg.type = SESSION_PACKET_MESSAGE;
				packet->s = s;
				packet->resendtimes = 0;
				packet->hdrlen = hdrlen;
				packet->len = hdrlen + RELIABLE_DGRAM_DATA_HDR_LEN;
				packet->data[hdrlen] = HDR_DATA | HDR_DATA_END_FLAG;
				if (hdrlen && s->ctx.encode) {
					s->ctx.encode(packet->data, RELIABLE_DGRAM_DATA_HDR_LEN);
				}
				sessionloop_exec_msg(s->m_loop, &packet->msg.m_listnode);
			}
		}
	}
	else {
		size_t hdrlen = s->ctx.hdrlen ? s->ctx.hdrlen(nbytes) : 0;
		Packet_t* packet = (Packet_t*)malloc(sizeof(Packet_t) + hdrlen + nbytes);
		if (!packet)
			return NULL;
		packet->msg.type = SESSION_PACKET_MESSAGE;
		packet->s = s;
		packet->type = HDR_DATA;
		packet->need_ack = 0;
		packet->seq = 0;
		packet->offset = 0;
		packet->hdrlen = hdrlen;
		packet->len = hdrlen + nbytes;
		if (hdrlen && s->ctx.encode) {
			s->ctx.encode(packet->data, nbytes);
		}
		for (nbytes = 0, i = 0; i < iovcnt; ++i) {
			memcpy(packet->data + hdrlen + nbytes, iobufPtr(iov + i), iobufLen(iov + i));
			nbytes += iobufLen(iov + i);
		}
		sessionloop_exec_msg(s->m_loop, &packet->msg.m_listnode);
	}
	return s;
}

void netpacketMerge(List_t* pklist, Iobuf_t* iov) {
	
}

#ifdef __cplusplus
}
#endif