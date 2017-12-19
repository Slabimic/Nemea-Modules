#ifndef _UR_FIELDS_H_
#define _UR_FIELDS_H_

/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/
#include <unirec/unirec.h>

#define F_DST_IP   0
#define F_DST_IP_T   ip_addr_t
#define F_SRC_IP   1
#define F_SRC_IP_T   ip_addr_t
#define F_BYTES   2
#define F_BYTES_T   uint64_t
#define F_LINK_BIT_FIELD   3
#define F_LINK_BIT_FIELD_T   uint64_t
#define F_TIME   4
#define F_TIME_T   ur_time_t
#define F_TIME_FIRST   5
#define F_TIME_FIRST_T   ur_time_t
#define F_TIME_LAST   6
#define F_TIME_LAST_T   ur_time_t
#define F_DNS_RR_TTL   7
#define F_DNS_RR_TTL_T   uint32_t
#define F_NTP_DELAY   8
#define F_NTP_DELAY_T   uint32_t
#define F_NTP_DISPERSION   9
#define F_NTP_DISPERSION_T   uint32_t
#define F_ODID   10
#define F_ODID_T   uint32_t
#define F_PACKETS   11
#define F_PACKETS_T   uint32_t
#define F_ARP_HA_FORMAT   12
#define F_ARP_HA_FORMAT_T   uint16_t
#define F_ARP_OPCODE   13
#define F_ARP_OPCODE_T   uint16_t
#define F_ARP_PA_FORMAT   14
#define F_ARP_PA_FORMAT_T   uint16_t
#define F_DNS_ANSWERS   15
#define F_DNS_ANSWERS_T   uint16_t
#define F_DNS_CLASS   16
#define F_DNS_CLASS_T   uint16_t
#define F_DNS_ID   17
#define F_DNS_ID_T   uint16_t
#define F_DNS_PSIZE   18
#define F_DNS_PSIZE_T   uint16_t
#define F_DNS_QTYPE   19
#define F_DNS_QTYPE_T   uint16_t
#define F_DNS_RLENGTH   20
#define F_DNS_RLENGTH_T   uint16_t
#define F_DST_PORT   21
#define F_DST_PORT_T   uint16_t
#define F_ETHERTYPE   22
#define F_ETHERTYPE_T   uint16_t
#define F_HTTP_RESPONSE_CODE   23
#define F_HTTP_RESPONSE_CODE_T   uint16_t
#define F_SIP_MSG_TYPE   24
#define F_SIP_MSG_TYPE_T   uint16_t
#define F_SIP_STATUS_CODE   25
#define F_SIP_STATUS_CODE_T   uint16_t
#define F_SRC_PORT   26
#define F_SRC_PORT_T   uint16_t
#define F_DIR_BIT_FIELD   27
#define F_DIR_BIT_FIELD_T   uint8_t
#define F_DNS_DO   28
#define F_DNS_DO_T   uint8_t
#define F_DNS_RCODE   29
#define F_DNS_RCODE_T   uint8_t
#define F_NTP_LEAP   30
#define F_NTP_LEAP_T   uint8_t
#define F_NTP_MODE   31
#define F_NTP_MODE_T   uint8_t
#define F_NTP_POLL   32
#define F_NTP_POLL_T   uint8_t
#define F_NTP_PRECISION   33
#define F_NTP_PRECISION_T   uint8_t
#define F_NTP_STRATUM   34
#define F_NTP_STRATUM_T   uint8_t
#define F_NTP_VERSION   35
#define F_NTP_VERSION_T   uint8_t
#define F_PROTOCOL   36
#define F_PROTOCOL_T   uint8_t
#define F_TCP_FLAGS   37
#define F_TCP_FLAGS_T   uint8_t
#define F_TOS   38
#define F_TOS_T   uint8_t
#define F_TTL   39
#define F_TTL_T   uint8_t
#define F_ARP_DST_HA   40
#define F_ARP_DST_HA_T   char
#define F_ARP_DST_PA   41
#define F_ARP_DST_PA_T   char
#define F_ARP_SRC_HA   42
#define F_ARP_SRC_HA_T   char
#define F_ARP_SRC_PA   43
#define F_ARP_SRC_PA_T   char
#define F_DNS_NAME   44
#define F_DNS_NAME_T   char
#define F_DNS_RDATA   45
#define F_DNS_RDATA_T   char
#define F_DST_MAC   46
#define F_DST_MAC_T   char
#define F_HTTP_CONTENT_TYPE   47
#define F_HTTP_CONTENT_TYPE_T   char
#define F_HTTP_HOST   48
#define F_HTTP_HOST_T   char
#define F_HTTP_METHOD   49
#define F_HTTP_METHOD_T   char
#define F_HTTP_REFERER   50
#define F_HTTP_REFERER_T   char
#define F_HTTP_URL   51
#define F_HTTP_URL_T   char
#define F_HTTP_USER_AGENT   52
#define F_HTTP_USER_AGENT_T   char
#define F_NTP_ORIG   53
#define F_NTP_ORIG_T   char
#define F_NTP_RECV   54
#define F_NTP_RECV_T   char
#define F_NTP_REF   55
#define F_NTP_REF_T   char
#define F_NTP_REF_ID   56
#define F_NTP_REF_ID_T   char
#define F_NTP_SENT   57
#define F_NTP_SENT_T   char
#define F_SIP_CALLED_PARTY   58
#define F_SIP_CALLED_PARTY_T   char
#define F_SIP_CALL_ID   59
#define F_SIP_CALL_ID_T   char
#define F_SIP_CALLING_PARTY   60
#define F_SIP_CALLING_PARTY_T   char
#define F_SIP_CSEQ   61
#define F_SIP_CSEQ_T   char
#define F_SIP_REQUEST_URI   62
#define F_SIP_REQUEST_URI_T   char
#define F_SIP_USER_AGENT   63
#define F_SIP_USER_AGENT_T   char
#define F_SIP_VIA   64
#define F_SIP_VIA_T   char
#define F_SRC_MAC   65
#define F_SRC_MAC_T   char

extern uint16_t ur_last_id;
extern ur_static_field_specs_t UR_FIELD_SPECS_STATIC;
extern ur_field_specs_t ur_field_specs;

#endif

