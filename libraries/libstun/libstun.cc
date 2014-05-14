#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <net/if.h>
#include "libstun.h"

#define NOSSL

#define STUN_PORT 3478
#define STUN_MAX_STRING 256
#define STUN_MAX_UNKNOWN_ATTRIBUTES 8
#define STUN_MAX_MESSAGE_SIZE 2048
#define MAX_EPOLL_EVENT 16


static StunAddress4 g_stun_srv;
static const int INVALID_SOCKET = -1;
static const int SOCKET_ERROR = -1;

// define some basic types
/// define a structure to hold a stun address 
const uint8_t IPv4Family = 0x01;
const uint8_t IPv6Family = 0x02;

// define  flags  
const uint32_t ChangeIpFlag = 0x04;
const uint32_t ChangePortFlag = 0x02;

// define  stun attribute
const uint16_t MappedAddress = 0x0001;
const uint16_t ResponseAddress = 0x0002;
const uint16_t ChangeRequest = 0x0003;
const uint16_t SourceAddress = 0x0004;
const uint16_t ChangedAddress = 0x0005;
const uint16_t Username = 0x0006;
const uint16_t Password = 0x0007;
const uint16_t MessageIntegrity = 0x0008;
const uint16_t ErrorCode = 0x0009;
const uint16_t UnknownAttribute = 0x000A;
const uint16_t ReflectedFrom = 0x000B;
const uint16_t XorMappedAddress = 0x8020;
const uint16_t XorOnly = 0x0021;
const uint16_t ServerName = 0x8022;
const uint16_t SecondaryAddress = 0x8050; // Non standard extention

// define types for a stun message 
const uint16_t BindRequestMsg = 0x0001;
const uint16_t BindResponseMsg = 0x0101;
const uint16_t BindErrorResponseMsg = 0x0111;
const uint16_t SharedSecretRequestMsg = 0x0002;
const uint16_t SharedSecretResponseMsg = 0x0102;
const uint16_t SharedSecretErrorResponseMsg = 0x0112;

typedef struct {
    unsigned char octet[16];
} uint128_t;

typedef struct {
    uint16_t msgType;
    uint16_t msgLength;
    uint128_t id;
} StunMsgHdr;

typedef struct {
    uint16_t type;
    uint16_t length;
} StunAtrHdr;

typedef struct {
    uint8_t pad;
    uint8_t family;
    StunAddress4 ipv4;
} StunAtrAddress4;

typedef struct {
    uint32_t value;
} StunAtrChangeRequest;

typedef struct {
    uint16_t pad;                 // all 0
    uint8_t errorClass;
    uint8_t number;
    char reason[STUN_MAX_STRING];
    uint16_t sizeReason;
} StunAtrError;

typedef struct {
    uint16_t attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
    uint16_t numAttributes;
} StunAtrUnknown;

typedef struct {
    char value[STUN_MAX_STRING];
    uint16_t sizeValue;
} StunAtrString;

typedef struct {
    char hash[20];
} StunAtrIntegrity;

typedef struct {
    StunMsgHdr msgHdr;

    bool hasMappedAddress;
    StunAtrAddress4 mappedAddress;

    bool hasResponseAddress;
    StunAtrAddress4 responseAddress;

    bool hasChangeRequest;
    StunAtrChangeRequest changeRequest;

    bool hasSourceAddress;
    StunAtrAddress4 sourceAddress;

    bool hasChangedAddress;
    StunAtrAddress4 changedAddress;

    bool hasUsername;
    StunAtrString username;

    bool hasPassword;
    StunAtrString password;

    bool hasMessageIntegrity;
    StunAtrIntegrity messageIntegrity;

    bool hasErrorCode;
    StunAtrError errorCode;

    bool hasUnknownAttributes;
    StunAtrUnknown unknownAttributes;

    bool hasReflectedFrom;
    StunAtrAddress4 reflectedFrom;

    bool hasXorMappedAddress;
    StunAtrAddress4 xorMappedAddress;

    bool xorOnly;

    bool hasServerName;
    StunAtrString serverName;

    bool hasSecondaryAddress;
    StunAtrAddress4 secondaryAddress;
} StunMessage;

// Define enum with different types of NAT 
typedef enum {
    StunTypeUnknown = 0,
    StunTypeFailure,
    StunTypeOpen,
    StunTypeBlocked,
    StunTypeConeNat,
    StunTypeRestrictedNat,
    StunTypePortRestrictedNat,
    StunTypeSymNat,
    StunTypeSymFirewall,
} NatType;


static void
computeHmac(char *hmac, const char *input, int length, const char *key,
            int keySize);

uint64_t stunGetSystemTimeSecs()
{
    uint64_t time = 0;
    struct timeval now;
    gettimeofday(&now, NULL);
    //assert( now );
    time = now.tv_sec;
    return time;
}


int openPort(unsigned short port, unsigned int interfaceIp)
{
    int fd;
    int reuse;

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == INVALID_SOCKET) {
        printf("Could not create a UDP socket: %d", errno);
        return INVALID_SOCKET;
    }

    reuse = 1;
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse))) {
        printf("%s:%d: errno = %d: %s\n", __func__, __LINE__, errno, strerror(errno));
    }

    struct sockaddr_in addr;
    memset((char *)&(addr), 0, sizeof((addr)));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if ((interfaceIp != 0) && (interfaceIp != 0x100007f)) {
        addr.sin_addr.s_addr = htonl(interfaceIp);
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        int e = errno;

        switch (e) {
        case 0:
            {
                fprintf(stderr, "Could not bind socket\n");
                return INVALID_SOCKET;
            }
        case EADDRINUSE:
            {
                fprintf(stderr, "Port %d for receiving UDP is in use\n", port);
                return INVALID_SOCKET;
            }
            break;
        case EADDRNOTAVAIL:
            {
                return INVALID_SOCKET;
            }
            break;
        default:
            {
                fprintf(stderr, "Could not bind UDP receive port Error=%d %s\n", e, strerror(e));
                return INVALID_SOCKET;
            }
            break;
        }
    }

    assert(fd != INVALID_SOCKET);

    return fd;
}

bool
getMessage(int fd, char *buf, int *len,
           unsigned int *srcIp, unsigned short *srcPort, bool verbose)
{
    assert(fd != INVALID_SOCKET);

    int originalSize = *len;
    assert(originalSize > 0);

    struct sockaddr_in from;
    int fromLen = sizeof(from);
//    int flag = MSG_DONTWAIT;
    int flag = 0;



    *len = recvfrom(fd,
                    buf,
                    originalSize,
                    flag, (struct sockaddr *)&from, (socklen_t *) & fromLen);

    if (*len == SOCKET_ERROR) {
        int err = errno;

        switch (err) {
        case ENOTSOCK:
            fprintf(stderr, "Error fd not a socket\n");
            break;
        case ECONNRESET:
            fprintf(stderr, "Error connection reset - host not reachable\n");
            break;

        default:
            fprintf(stderr, "socket Error=%d\n", err);
        }

        return false;
    }

    if (*len < 0) {
        fprintf(stderr, "socket closed? negative len\n");
        return false;
    }

    if (*len == 0) {
        fprintf(stderr, "socket closed? zero len\n");
        return false;
    }

    *srcPort = ntohs(from.sin_port);
    *srcIp = ntohl(from.sin_addr.s_addr);

    if ((*len) + 1 >= originalSize) {
        return false;
    }
    buf[*len] = 0;

    return true;
}

bool
sendMessage(int fd, char *buf, int l,
            unsigned int dstIp, unsigned short dstPort, bool verbose)
{
    assert(fd != INVALID_SOCKET);

    int s;
    if (dstPort == 0) {
        // sending on a connected port 
        assert(dstIp == 0);

        s = send(fd, buf, l, 0);
    } else {
        assert(dstIp != 0);
        assert(dstPort != 0);

        struct sockaddr_in to;
        int toLen = sizeof(to);
        memset(&to, 0, toLen);

        to.sin_family = AF_INET;
        to.sin_port = htons(dstPort);
        to.sin_addr.s_addr = htonl(dstIp);

        s = sendto(fd, buf, l, 0, (sockaddr *) & to, toLen);
    }

    if (s == SOCKET_ERROR) {
        int e = errno;
        switch (e) {
        case ECONNREFUSED:
        case EHOSTDOWN:
        case EHOSTUNREACH:
            {
                // quietly ignore this 
            }
            break;
        case EAFNOSUPPORT:
            {
                fprintf(stderr, "err EAFNOSUPPORT in send\n");
            }
            break;
        default:
            {
                fprintf(stderr, "err %d %s in send\n", e, strerror(e));
            }
        }
        return false;
    }

    if (s == 0) {
        fprintf(stderr, "no data sent in send\n");
        return false;
    }

    if (s != l) {
        if (verbose) {
            fprintf(stderr, "only %d out of %d bytes sent\n", s, l);
        }
        return false;
    }

    return true;
}


static bool
stunParseAtrAddress(char *body, unsigned int hdrLen, StunAtrAddress4 & result)
{
    if (hdrLen != 8) {
        fprintf(stderr, "hdrLen wrong for Address\n");
        return false;
    }
    result.pad = *body++;
    result.family = *body++;
    if (result.family == IPv4Family) {
        uint16_t nport;
        memcpy(&nport, body, 2);
        body += 2;
        result.ipv4.port = ntohs(nport);

        uint32_t naddr;
        memcpy(&naddr, body, 4);
        body += 4;
        result.ipv4.addr = ntohl(naddr);
        return true;
    } else if (result.family == IPv6Family) {
        fprintf(stderr, "ipv6 not supported\n");
    } else {
        fprintf(stderr, "bad address family: %d\n", result.family);
    }

    return false;
}

static bool
stunParseAtrChangeRequest(char *body, unsigned int hdrLen,
                          StunAtrChangeRequest & result)
{
    if (hdrLen != 4) {
        fprintf(stderr, "hdr length = %u expecting %lu\n",  hdrLen, sizeof(result));
        fprintf(stderr, "Incorrect size for ChangeRequest\n");
        return false;
    } else {
        memcpy(&result.value, body, 4);
        result.value = ntohl(result.value);
        return true;
    }
}

static bool
stunParseAtrError(char *body, unsigned int hdrLen, StunAtrError & result)
{
    if (hdrLen >= sizeof(result)) {
        fprintf(stderr, "head on Error too large\n");
        return false;
    } else {
        memcpy(&result.pad, body, 2);
        body += 2;
        result.pad = ntohs(result.pad);
        result.errorClass = *body++;
        result.number = *body++;

        result.sizeReason = hdrLen - 4;
        memcpy(&result.reason, body, result.sizeReason);
        result.reason[result.sizeReason] = 0;
        return true;
    }
}

static bool
stunParseAtrUnknown(char *body, unsigned int hdrLen, StunAtrUnknown & result)
{
    if (hdrLen >= sizeof(result)) {
        return false;
    } else {
        if (hdrLen % 4 != 0)
            return false;
        result.numAttributes = hdrLen / 4;
        for (int i = 0; i < result.numAttributes; i++) {
            memcpy(&result.attrType[i], body, 2);
            body += 2;
            result.attrType[i] = ntohs(result.attrType[i]);
        }
        return true;
    }
}

static bool
stunParseAtrString(char *body, unsigned int hdrLen, StunAtrString & result)
{
    if (hdrLen >= STUN_MAX_STRING) {
        fprintf(stderr, "String is too large\n");
        return false;
    } else {
        if (hdrLen % 4 != 0) {
            fprintf(stderr, "Bad length string %d\n", hdrLen);
            return false;
        }

        result.sizeValue = hdrLen;
        memcpy(&result.value, body, hdrLen);
        result.value[hdrLen] = 0;
        return true;
    }
}

static bool
stunParseAtrIntegrity(char *body, unsigned int hdrLen,
                      StunAtrIntegrity & result)
{
    if (hdrLen != 20) {
        fprintf(stderr, "MessageIntegrity must be 20 bytes\n");
        return false;
    } else {
        memcpy(&result.hash, body, hdrLen);
        return true;
    }
}

bool
stunParseMessage(char *buf, unsigned int bufLen, StunMessage & msg,
                 bool verbose)
{
    if (verbose)
        fprintf(stderr, "Received stun message: %d bytes\n", bufLen);
    memset(&msg, 0, sizeof(msg));

    if (sizeof(StunMsgHdr) > bufLen) {
        fprintf(stderr, "Bad message\n");
        return false;
    }

    memcpy(&msg.msgHdr, buf, sizeof(StunMsgHdr));
    msg.msgHdr.msgType = ntohs(msg.msgHdr.msgType);
    msg.msgHdr.msgLength = ntohs(msg.msgHdr.msgLength);

    if (msg.msgHdr.msgLength + sizeof(StunMsgHdr) != bufLen) {
        fprintf(stderr, "Message header length doesn't match message size: %d - %d\n", msg.msgHdr.msgLength, bufLen);
        return false;
    }

    char *body = buf + sizeof(StunMsgHdr);
    unsigned int size = msg.msgHdr.msgLength;

    while (size > 0) {
        // !jf! should check that there are enough bytes left in the buffer

        StunAtrHdr *attr = (StunAtrHdr *)body;

        unsigned int attrLen = ntohs(attr->length);
        int atrType = ntohs(attr->type);

        if (attrLen + 4 > size) {
            fprintf(stderr, "claims attribute is larger than size of message \"(attribute type= %d \")\n", atrType);
            return false;
        }

        body += 4;              // skip the length and type in attribute header
        size -= 4;

        switch (atrType) {
        case MappedAddress:
            msg.hasMappedAddress = true;
            if (stunParseAtrAddress(body, attrLen, msg.mappedAddress) == false) {
                fprintf(stderr, "problem parsing MappedAddress\n");
                return false;
            } else {
            }

            break;

        case ResponseAddress:
            msg.hasResponseAddress = true;
            if (stunParseAtrAddress(body, attrLen, msg.responseAddress) ==
                false) {
                fprintf(stderr, "problem parsing ResponseAddress\n");
                return false;
            } else {
            }
            break;

        case ChangeRequest:
            msg.hasChangeRequest = true;
            if (stunParseAtrChangeRequest(body, attrLen, msg.changeRequest) ==
                false) {
                fprintf(stderr, "problem parsing ChangeRequest\n");
                return false;
            } else {
                if (verbose)
                    fprintf(stderr, "ChangeRequest = %d\n", msg.changeRequest.value);
            }
            break;

        case SourceAddress:
            msg.hasSourceAddress = true;
            if (stunParseAtrAddress(body, attrLen, msg.sourceAddress) == false) {
                fprintf(stderr, "problem parsing SourceAddress\n");
                return false;
            } else {

            }
            break;

        case ChangedAddress:
            msg.hasChangedAddress = true;
            if (stunParseAtrAddress(body, attrLen, msg.changedAddress) == false) {
                fprintf(stderr, "problem parsing ChangedAddress\n");
                return false;
            }
            break;

        case Username:
            msg.hasUsername = true;
            if (stunParseAtrString(body, attrLen, msg.username) == false) {
                fprintf(stderr, "problem parsing Username\n");
                return false;
            }
            break;

        case Password:
            msg.hasPassword = true;
            if (stunParseAtrString(body, attrLen, msg.password) == false) {
                fprintf(stderr, "problem parsing Password\n");
                return false;
            }
            break;

        case MessageIntegrity:
            msg.hasMessageIntegrity = true;
            if (stunParseAtrIntegrity(body, attrLen, msg.messageIntegrity) ==
                false) {
                fprintf(stderr, "problem parsing MessageIntegrity\n");
                return false;
            }

            // read the current HMAC
            // look up the password given the user of given the transaction id 
            // compute the HMAC on the buffer
            // decide if they match or not
            break;

        case ErrorCode:
            msg.hasErrorCode = true;
            if (stunParseAtrError(body, attrLen, msg.errorCode) == false) {
                fprintf(stderr, "problem parsing ErrorCode\n");
                return false;
            }
            break;

        case UnknownAttribute:
            msg.hasUnknownAttributes = true;
            if (stunParseAtrUnknown(body, attrLen, msg.unknownAttributes) ==
                false) {
                fprintf(stderr, "problem parsing UnknownAttribute\n");
                return false;
            }
            break;

        case ReflectedFrom:
            msg.hasReflectedFrom = true;
            if (stunParseAtrAddress(body, attrLen, msg.reflectedFrom) == false) {
                fprintf(stderr, "problem parsing ReflectedFrom\n");
                return false;
            }
            break;

        case XorMappedAddress:
            msg.hasXorMappedAddress = true;
            if (stunParseAtrAddress(body, attrLen, msg.xorMappedAddress) ==
                false) {
                fprintf(stderr, "problem parsing XorMappedAddress\n");
                return false;
            } else {

            }
            break;

        case XorOnly:
            msg.xorOnly = true;
            if (verbose) {
                fprintf(stderr, "xorOnly = true\n");
            }
            break;

        case ServerName:
            msg.hasServerName = true;
            if (stunParseAtrString(body, attrLen, msg.serverName) == false) {
                fprintf(stderr, "problem parsing ServerName\n");
                return false;
            } else {
                if (verbose)
                    fprintf(stderr, "ServerName = %s\n", msg.serverName.value);
            }
            break;

        case SecondaryAddress:
            msg.hasSecondaryAddress = true;
            if (stunParseAtrAddress(body, attrLen, msg.secondaryAddress) ==
                false) {
                fprintf(stderr, "problem parsing secondaryAddress\n");
                return false;
            } else {

            }
            break;

        default:
            if (verbose)
                fprintf(stderr, "Unknown attribute: %d\n", atrType);
            if (atrType <= 0x7FFF) {
                return false;
            }
        }

        body += attrLen;
        size -= attrLen;
    }

    return true;
}

static char *encode16(char *buf, uint16_t data)
{
    uint16_t ndata = htons(data);
    memcpy(buf, (void *)(&ndata), sizeof(uint16_t));
    return buf + sizeof(uint16_t);
}

static char *encode32(char *buf, uint32_t data)
{
    uint32_t ndata = htonl(data);
    memcpy(buf, (void *)(&ndata), sizeof(uint32_t));
    return buf + sizeof(uint32_t);
}

static char *encode(char *buf, const char *data, unsigned int length)
{
    memcpy(buf, data, length);
    return buf + length;
}

static char *encodeAtrAddress4(char *ptr, uint16_t type,
                               const StunAtrAddress4 & atr)
{
    ptr = encode16(ptr, type);
    ptr = encode16(ptr, 8);
    *ptr++ = atr.pad;
    *ptr++ = IPv4Family;
    ptr = encode16(ptr, atr.ipv4.port);
    ptr = encode32(ptr, atr.ipv4.addr);

    return ptr;
}

static char *encodeAtrChangeRequest(char *ptr, const StunAtrChangeRequest & atr)
{
    ptr = encode16(ptr, ChangeRequest);
    ptr = encode16(ptr, 4);
    ptr = encode32(ptr, atr.value);
    return ptr;
}

static char *encodeAtrError(char *ptr, const StunAtrError & atr)
{
    ptr = encode16(ptr, ErrorCode);
    ptr = encode16(ptr, 6 + atr.sizeReason);
    ptr = encode16(ptr, atr.pad);
    *ptr++ = atr.errorClass;
    *ptr++ = atr.number;
    ptr = encode(ptr, atr.reason, atr.sizeReason);
    return ptr;
}

static char *encodeAtrUnknown(char *ptr, const StunAtrUnknown & atr)
{
    ptr = encode16(ptr, UnknownAttribute);
    ptr = encode16(ptr, 2 + 2 * atr.numAttributes);
    for (int i = 0; i < atr.numAttributes; i++) {
        ptr = encode16(ptr, atr.attrType[i]);
    }
    return ptr;
}

static char *encodeXorOnly(char *ptr)
{
    ptr = encode16(ptr, XorOnly);
    return ptr;
}

static char *encodeAtrString(char *ptr, uint16_t type, const StunAtrString & atr)
{
    assert(atr.sizeValue % 4 == 0);

    ptr = encode16(ptr, type);
    ptr = encode16(ptr, atr.sizeValue);
    ptr = encode(ptr, atr.value, atr.sizeValue);
    return ptr;
}

static char *encodeAtrIntegrity(char *ptr, const StunAtrIntegrity & atr)
{
    ptr = encode16(ptr, MessageIntegrity);
    ptr = encode16(ptr, 20);
    ptr = encode(ptr, atr.hash, sizeof(atr.hash));
    return ptr;
}

unsigned int
stunEncodeMessage(const StunMessage & msg,
                  char *buf,
                  unsigned int bufLen,
                  const StunAtrString & password, bool verbose)
{
    assert(bufLen >= sizeof(StunMsgHdr));
    char *ptr = buf;

    ptr = encode16(ptr, msg.msgHdr.msgType);
    char *lengthp = ptr;
    ptr = encode16(ptr, 0);
    ptr =
        encode(ptr, (const char *)(msg.msgHdr.id.octet),
               sizeof(msg.msgHdr.id));


    if (msg.hasMappedAddress) {

        ptr = encodeAtrAddress4(ptr, MappedAddress, msg.mappedAddress);
    }
    if (msg.hasResponseAddress) {
        ptr = encodeAtrAddress4(ptr, ResponseAddress, msg.responseAddress);
    }
    if (msg.hasChangeRequest) {
        ptr = encodeAtrChangeRequest(ptr, msg.changeRequest);
    }
    if (msg.hasSourceAddress) {
        ptr = encodeAtrAddress4(ptr, SourceAddress, msg.sourceAddress);
    }
    if (msg.hasChangedAddress) {
        ptr = encodeAtrAddress4(ptr, ChangedAddress, msg.changedAddress);
    }
    if (msg.hasUsername) {
        ptr = encodeAtrString(ptr, Username, msg.username);
    }
    if (msg.hasPassword) {
        ptr = encodeAtrString(ptr, Password, msg.password);
    }
    if (msg.hasErrorCode) {
        ptr = encodeAtrError(ptr, msg.errorCode);
    }
    if (msg.hasUnknownAttributes) {
        ptr = encodeAtrUnknown(ptr, msg.unknownAttributes);
    }
    if (msg.hasReflectedFrom) {
        ptr = encodeAtrAddress4(ptr, ReflectedFrom, msg.reflectedFrom);
    }
    if (msg.hasXorMappedAddress) {
        ptr = encodeAtrAddress4(ptr, XorMappedAddress, msg.xorMappedAddress);
    }
    if (msg.xorOnly) {
        ptr = encodeXorOnly(ptr);
    }
    if (msg.hasServerName) {
        ptr = encodeAtrString(ptr, ServerName, msg.serverName);
    }
    if (msg.hasSecondaryAddress) {
        ptr = encodeAtrAddress4(ptr, SecondaryAddress, msg.secondaryAddress);
    }

    if (password.sizeValue > 0) {
        StunAtrIntegrity integrity;
        computeHmac(integrity.hash, buf, int (ptr - buf), password.value,
                    password.sizeValue);
        ptr = encodeAtrIntegrity(ptr, integrity);
    }

    encode16(lengthp, uint16_t(ptr - buf - sizeof(StunMsgHdr)));
    return int (ptr - buf);
}

int stunRand()
{
    // return 32 bits of random stuff
    assert(sizeof(int) == 4);
    static bool init = false;
    if (!init) {
        init = true;

        uint64_t tick;

#if defined(__GNUC__) && ( defined(__i686__) || defined(__i386__) )
asm("rdtsc":"=A"(tick));
#elif defined(__MACH__)  || defined(__linux)
        int fd = open("/dev/random", O_RDONLY);
        read(fd, &tick, sizeof(tick));
        close(fd);
#else
#error Need some way to seed the random number generator
#endif
        int seed = int (tick);
        srandom(seed);
    }

    return random();
}

/// return a random number to use as a port 
int stunRandomPort()
{
    int min = 0x4000;
    int max = 0x7FFF;

    int ret = stunRand();
    ret = ret | min;
    ret = ret & max;

    return ret;
}

#ifdef NOSSL
static void
computeHmac(char *hmac, const char *input, int length, const char *key,
            int sizeKey)
{
    strncpy(hmac, "hmac-not-implemented", 20);
}
#else
#include <openssl/hmac.h>

static void
computeHmac(char *hmac, const char *input, int length, const char *key,
            int sizeKey)
{
    unsigned int resultSize = 0;
    HMAC(EVP_sha1(),
         key, sizeKey,
         (const unsigned char *)(input), length,
         (unsigned char *)(hmac), &resultSize);
    assert(resultSize == 20);
}
#endif

static void toHex(const char *buffer, int bufferSize, char *output)
{
    static char hexmap[] = "0123456789abcdef";

    const char *p = buffer;
    char *r = output;
    for (int i = 0; i < bufferSize; i++) {
        unsigned char temp = *p++;

        int hi = (temp & 0xf0) >> 4;
        int low = (temp & 0xf);

        *r++ = hexmap[hi];
        *r++ = hexmap[low];
    }
    *r = 0;
}

void stunCreateUserName(const StunAddress4 & source, StunAtrString * username)
{
    uint64_t time = stunGetSystemTimeSecs();
    time -= (time % 20 * 60);
    //uint64_t hitime = time >> 32;
    uint64_t lotime = time & 0xFFFFFFFF;

    char buffer[1024];
    sprintf(buffer,
            "%08x:%08x:%08x:",
            uint32_t(source.addr), uint32_t(stunRand()), uint32_t(lotime));
    assert(strlen(buffer) < 1024);

    assert(strlen(buffer) + 41 < STUN_MAX_STRING);

    char hmac[20];
    char key[] = "Jason";
    computeHmac(hmac, buffer, strlen(buffer), key, strlen(key));
    char hmacHex[41];
    toHex(hmac, 20, hmacHex);
    hmacHex[40] = 0;

    strcat(buffer, hmacHex);

    int l = strlen(buffer);
    assert(l + 1 < STUN_MAX_STRING);
    assert(l % 4 == 0);

    username->sizeValue = l;
    memcpy(username->value, buffer, l);
    username->value[l] = 0;

}

void
stunCreatePassword(const StunAtrString & username, StunAtrString * password)
{
    char hmac[20];
    char key[] = "Fluffy";
    //char buffer[STUN_MAX_STRING];
    computeHmac(hmac, username.value, strlen(username.value), key, strlen(key));
    toHex(hmac, 20, password->value);
    password->sizeValue = 40;
    password->value[40] = 0;

    //clog << "password=" << password->value << endl;
}

// returns true if it scucceeded
bool
stunParseHostName(char *peerName,
                  uint32_t & ip, uint16_t & portVal, uint16_t defaultPort)
{
    in_addr sin_addr;

    char host[512];
    strncpy(host, peerName, 512);
    host[512 - 1] = '\0';
    char *port = NULL;

    int portNum = defaultPort;

    // pull out the port part if present.
    char *sep = strchr(host, ':');

    if (sep == NULL) {
        portNum = defaultPort;
    } else {
        *sep = '\0';
        port = sep + 1;
        // set port part

        char *endPtr = NULL;

        portNum = strtol(port, &endPtr, 10);

        if (endPtr != NULL) {
            if (*endPtr != '\0') {
                portNum = defaultPort;
            }
        }
    }

    if (portNum < 1024)
        return false;
    if (portNum >= 0xFFFF)
        return false;

    // figure out the host part 
    struct hostent *h;

    h = gethostbyname(host);
    if (h == NULL) {
        fprintf(stderr, "error was %d\n", errno);
        ip = ntohl(0x7F000001L);
        return false;
    } else {
        sin_addr = *(struct in_addr *)h->h_addr;
        ip = ntohl(sin_addr.s_addr);
    }

    portVal = portNum;

    return true;
}

bool stunParseServerName(char *name, StunAddress4 & addr)
{
    assert(name);

    // TODO - put in DNS SRV stuff.

    bool ret = stunParseHostName(name, addr.addr, addr.port, STUN_PORT);
    if (ret != true) {
        addr.port = 0xFFFF;
    }
    return ret;
}

void
stunBuildReqSimple(StunMessage * msg,
                   const StunAtrString & username,
                   bool changePort, bool changeIp, unsigned int id)
{
    assert(msg);
    memset(msg, 0, sizeof(*msg));

    msg->msgHdr.msgType = BindRequestMsg;

    for (int i = 0; i < 16; i = i + 4) {
        assert(i + 3 < 16);
        int r = stunRand();
        msg->msgHdr.id.octet[i + 0] = r >> 0;
        msg->msgHdr.id.octet[i + 1] = r >> 8;
        msg->msgHdr.id.octet[i + 2] = r >> 16;
        msg->msgHdr.id.octet[i + 3] = r >> 24;
    }

    if (id != 0) {
        msg->msgHdr.id.octet[0] = id;
    }

    msg->hasChangeRequest = true;
    msg->changeRequest.value = (changeIp ? ChangeIpFlag : 0) |
        (changePort ? ChangePortFlag : 0);

    if (username.sizeValue > 0) {
        msg->hasUsername = true;
        msg->username = username;
    }
}

static void
stunSendTest(int myFd, StunAddress4 & dest,
             const StunAtrString & username, const StunAtrString & password,
             int testNum, bool verbose)
{
    assert(dest.addr != 0);
    assert(dest.port != 0);

    bool changePort = false;
    bool changeIP = false;
    bool discard = false;

    switch (testNum) {
    case 1:
    case 10:
    case 11:
        break;
    case 2:
        //changePort=true;
        changeIP = true;
        break;
    case 3:
        changePort = true;
        break;
    case 4:
        changeIP = true;
        break;
    case 5:
        discard = true;
        break;
    default:
        fprintf(stderr, "Test %d is unkown\n", testNum);
        assert(0);
    }

    StunMessage req;
    memset(&req, 0, sizeof(StunMessage));

    stunBuildReqSimple(&req, username, changePort, changeIP, testNum);

    char buf[STUN_MAX_MESSAGE_SIZE];
    int len = STUN_MAX_MESSAGE_SIZE;

    len = stunEncodeMessage(req, buf, len, password, verbose);

    sendMessage(myFd, buf, len, dest.addr, dest.port, verbose);

    // add some delay so the packets don't get sent too quickly 

//    usleep(10 * 1000);

}

void
stunGetUserNameAndPassword(const StunAddress4 & dest,
                           StunAtrString * username, StunAtrString * password)
{
    // !cj! This is totally bogus - need to make TLS connection to dest and get a
    // username and password to use 
    stunCreateUserName(dest, username);
    stunCreatePassword(*username, password);
}

void
stunTest(StunAddress4 & dest, int testNum, bool verbose, StunAddress4 * sAddr)
{
    assert(dest.addr != 0);
    assert(dest.port != 0);

    int port = stunRandomPort();
    uint32_t interfaceIp = 0;
    if (sAddr) {
        interfaceIp = sAddr->addr;
        if (sAddr->port != 0) {
            port = sAddr->port;
        }
    }
    int myFd = openPort(port, interfaceIp);

    StunAtrString username;
    StunAtrString password;

    username.sizeValue = 0;
    password.sizeValue = 0;

#ifdef USE_TLS
    stunGetUserNameAndPassword(dest, username, password);
#endif

    stunSendTest(myFd, dest, username, password, testNum, verbose);

    char msg[STUN_MAX_MESSAGE_SIZE];
    int msgLen = STUN_MAX_MESSAGE_SIZE;

    StunAddress4 from;
    getMessage(myFd, msg, &msgLen, &from.addr, &from.port, verbose);

    StunMessage resp;
    memset(&resp, 0, sizeof(StunMessage));

    stunParseMessage(msg, msgLen, resp, verbose);

    if (sAddr) {
        sAddr->port = resp.mappedAddress.ipv4.port;
        sAddr->addr = resp.mappedAddress.ipv4.addr;
    }
}

NatType stunNatType(StunAddress4 & dest, bool verbose, bool * preservePort, // if set, is return for if NAT preservers ports or not
                    bool * hairpin, // if set, is the return for if NAT will hairpin packets
                    int port,   // port to use for the test, 0 to choose random port
                    StunAddress4 * sAddr    // NIC to use 
    )
{
    assert(dest.addr != 0);
    assert(dest.port != 0);

    if (hairpin) {
        *hairpin = false;
    }

    if (port == 0) {
        port = stunRandomPort();
    }
    uint32_t interfaceIp = 0;
    if (sAddr) {
        interfaceIp = sAddr->addr;
    }
    int myFd1 = openPort(port, interfaceIp);
    int myFd2 = openPort(port + 1, interfaceIp);

    if ((myFd1 == INVALID_SOCKET) || (myFd2 == INVALID_SOCKET)) {
        fprintf(stderr, "Some problem opening port/interface to send on\n");
        return StunTypeFailure;
    }

    assert(myFd1 != INVALID_SOCKET);
    assert(myFd2 != INVALID_SOCKET);

    bool respTestI = false;
    bool isNat = true;
    StunAddress4 testIchangedAddr;
    StunAddress4 testImappedAddr;
    bool respTestI2 = false;
    bool mappedIpSame = true;
    StunAddress4 testI2mappedAddr;
    StunAddress4 testI2dest = dest;
    bool respTestII = false;
    bool respTestIII = false;

    bool respTestHairpin = false;
    bool respTestPreservePort = false;

    memset(&testImappedAddr, 0, sizeof(testImappedAddr));

    StunAtrString username;
    StunAtrString password;

    username.sizeValue = 0;
    password.sizeValue = 0;

#ifdef USE_TLS
    stunGetUserNameAndPassword(dest, username, password);
#endif

    int count = 0;
    while (count < 7) {
        struct timeval tv;
        fd_set fdSet;

        int fdSetSize;
        FD_ZERO(&fdSet);
        fdSetSize = 0;
        FD_SET(myFd1, &fdSet);
        fdSetSize = (myFd1 + 1 > fdSetSize) ? myFd1 + 1 : fdSetSize;
        FD_SET(myFd2, &fdSet);
        fdSetSize = (myFd2 + 1 > fdSetSize) ? myFd2 + 1 : fdSetSize;
        tv.tv_sec = 0;
        tv.tv_usec = 150 * 1000;    // 150 ms 
        if (count == 0)
            tv.tv_usec = 0;

        int err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
        int e = errno;
        if (err == SOCKET_ERROR) {
            // error occured
            fprintf(stderr, "Error %d %s in select\n", e, strerror(e));
            return StunTypeFailure;
        } else if (err == 0) {
            // timeout occured 
            count++;

            if (!respTestI) {
                stunSendTest(myFd1, dest, username, password, 1, verbose);
            }

            if ((!respTestI2) && respTestI) {
                // check the address to send to if valid 
                if ((testI2dest.addr != 0) && (testI2dest.port != 0)) {
                    stunSendTest(myFd1, testI2dest, username, password, 10,
                                 verbose);
                }
            }

            if (!respTestII) {
                stunSendTest(myFd2, dest, username, password, 2, verbose);
            }

            if (!respTestIII) {
                stunSendTest(myFd2, dest, username, password, 3, verbose);
            }

            if (respTestI && (!respTestHairpin)) {
                if ((testImappedAddr.addr != 0) && (testImappedAddr.port != 0)) {
                    stunSendTest(myFd1, testImappedAddr, username, password, 11,
                                 verbose);
                }
            }
        } else {
            //if (verbose) clog << "-----------------------------------------" << endl;
            assert(err > 0);
            // data is avialbe on some fd 

            for (int i = 0; i < 2; i++) {
                int myFd;
                if (i == 0) {
                    myFd = myFd1;
                } else {
                    myFd = myFd2;
                }

                if (myFd != INVALID_SOCKET) {
                    if (FD_ISSET(myFd, &fdSet)) {
                        char msg[STUN_MAX_MESSAGE_SIZE];
                        int msgLen = sizeof(msg);

                        StunAddress4 from;

                        getMessage(myFd,
                                   msg,
                                   &msgLen, &from.addr, &from.port, verbose);

                        StunMessage resp;
                        memset(&resp, 0, sizeof(StunMessage));

                        stunParseMessage(msg, msgLen, resp, verbose);

                        switch (resp.msgHdr.id.octet[0]) {
                        case 1:
                            {
                                if (!respTestI) {

                                    testIchangedAddr.addr =
                                        resp.changedAddress.ipv4.addr;
                                    testIchangedAddr.port =
                                        resp.changedAddress.ipv4.port;
                                    testImappedAddr.addr =
                                        resp.mappedAddress.ipv4.addr;
                                    testImappedAddr.port =
                                        resp.mappedAddress.ipv4.port;

                                    respTestPreservePort =
                                        (testImappedAddr.port == port);
                                    if (preservePort) {
                                        *preservePort = respTestPreservePort;
                                    }

                                    testI2dest.addr =
                                        resp.changedAddress.ipv4.addr;

                                    if (sAddr) {
                                        sAddr->port = testImappedAddr.port;
                                        sAddr->addr = testImappedAddr.addr;
                                    }

                                    count = 0;
                                }
                                respTestI = true;
                            }
                            break;
                        case 2:
                            {
                                respTestII = true;
                            }
                            break;
                        case 3:
                            {
                                respTestIII = true;
                            }
                            break;
                        case 10:
                            {
                                if (!respTestI2) {
                                    testI2mappedAddr.addr =
                                        resp.mappedAddress.ipv4.addr;
                                    testI2mappedAddr.port =
                                        resp.mappedAddress.ipv4.port;

                                    mappedIpSame = false;
                                    if ((testI2mappedAddr.addr ==
                                         testImappedAddr.addr)
                                        && (testI2mappedAddr.port ==
                                            testImappedAddr.port)) {
                                        mappedIpSame = true;
                                    }

                                }
                                respTestI2 = true;
                            }
                            break;
                        case 11:
                            {

                                if (hairpin) {
                                    *hairpin = true;
                                }
                                respTestHairpin = true;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    // see if we can bind to this address 
    //cerr << "try binding to " << testImappedAddr << endl;
    int s = openPort(0 /*use ephemeral */ , testImappedAddr.addr);
    if (s != INVALID_SOCKET) {
        close(s);
        isNat = false;
        //cerr << "binding worked" << endl;
    } else {
        isNat = true;
        //cerr << "binding failed" << endl;
    }

    // implement logic flow chart from draft RFC
    if (respTestI) {
        if (isNat) {
            if (respTestII) {
                return StunTypeConeNat;
            } else {
                if (mappedIpSame) {
                    if (respTestIII) {
                        return StunTypeRestrictedNat;
                    } else {
                        return StunTypePortRestrictedNat;
                    }
                } else {
                    return StunTypeSymNat;
                }
            }
        } else {
            if (respTestII) {
                return StunTypeOpen;
            } else {
                return StunTypeSymFirewall;
            }
        }
    } else {
        return StunTypeBlocked;
    }
    return StunTypeUnknown;
}

int
stunOpenSocket(StunAddress4 & dest, StunAddress4 * mapAddr,
               int port, StunAddress4 * srcAddr, bool verbose)
{
    assert(dest.addr != 0);
    assert(dest.port != 0);
    assert(mapAddr);

    StunAddress4 from;
    int err;
    int fdSetSize;
    struct timeval tv;
    fd_set fdSet; 

    if (port == 0) {
        port = stunRandomPort();
    }
    unsigned int interfaceIp = 0;
    if (srcAddr) {
        interfaceIp = srcAddr->addr;
    }

    int myFd = openPort(port, interfaceIp);
    if (myFd == INVALID_SOCKET) {
        return myFd;
    }

    char msg[STUN_MAX_MESSAGE_SIZE];
    int msgLen = sizeof(msg);

    StunAtrString username;
    StunAtrString password;

    username.sizeValue = 0;
    password.sizeValue = 0;

#ifdef USE_TLS
    stunGetUserNameAndPassword(dest, username, password);
#endif

    stunSendTest(myFd, dest, username, password, 1, 0 /*false */ );

    while (1) {
        FD_ZERO(&fdSet);
        FD_SET(myFd,&fdSet);
        tv.tv_sec=0;
        tv.tv_usec=2000*1000;
        fdSetSize = (myFd+1>fdSetSize) ? myFd+1 : fdSetSize;

        err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
        if (-1 == err) {
            perror("select");
        } else if (0 == err) {
            fprintf(stderr, "no data to recv\n");
            return -1;
        } else {
            break;
        }
    }

    getMessage(myFd, msg, &msgLen, &from.addr, &from.port, verbose);

    StunMessage resp;
    memset(&resp, 0, sizeof(StunMessage));

    bool ok = stunParseMessage(msg, msgLen, resp, verbose);
    if (!ok) {
        return -1;
    }

    StunAddress4 mappedAddr = resp.mappedAddress.ipv4;
    StunAddress4 changedAddr = resp.changedAddress.ipv4;

    *mapAddr = mappedAddr;

    return myFd;
}

bool
stunOpenSocketPair(StunAddress4 & dest, StunAddress4 * mapAddr,
                   int *fd1, int *fd2,
                   int port, StunAddress4 * srcAddr, bool verbose)
{
    assert(dest.addr != 0);
    assert(dest.port != 0);
    assert(mapAddr);

    const int NUM = 3;

    if (port == 0) {
        port = stunRandomPort();
    }

    *fd1 = -1;
    *fd2 = -1;

    char msg[STUN_MAX_MESSAGE_SIZE];
    int msgLen = sizeof(msg);

    StunAddress4 from;
    int fd[NUM];
    int i;

    unsigned int interfaceIp = 0;
    if (srcAddr) {
        interfaceIp = srcAddr->addr;
    }

    for (i = 0; i < NUM; i++) {
        fd[i] = openPort((port == 0) ? 0 : (port + i), interfaceIp);
        if (fd[i] < 0) {
            while (i > 0) {
                close(fd[--i]);
            }
            return false;
        }
    }

    StunAtrString username;
    StunAtrString password;

    username.sizeValue = 0;
    password.sizeValue = 0;

#ifdef USE_TLS
    stunGetUserNameAndPassword(dest, username, password);
#endif

    for (i = 0; i < NUM; i++) {
        stunSendTest(fd[i], dest, username, password, 1 /*testNum */ , verbose);
    }

    StunAddress4 mappedAddr[NUM];
    for (i = 0; i < NUM; i++) {
        msgLen = sizeof(msg) / sizeof(*msg);
        getMessage(fd[i], msg, &msgLen, &from.addr, &from.port, verbose);

        StunMessage resp;
        memset(&resp, 0, sizeof(StunMessage));

        bool ok = stunParseMessage(msg, msgLen, resp, verbose);
        if (!ok) {
            return false;
        }

        mappedAddr[i] = resp.mappedAddress.ipv4;
        StunAddress4 changedAddr = resp.changedAddress.ipv4;
    }

    if (mappedAddr[0].port % 2 == 0) {
        if (mappedAddr[0].port + 1 == mappedAddr[1].port) {
            *mapAddr = mappedAddr[0];
            *fd1 = fd[0];
            *fd2 = fd[1];
            close(fd[2]);
            return true;
        }
    } else {
        if ((mappedAddr[1].port % 2 == 0)
            && (mappedAddr[1].port + 1 == mappedAddr[2].port)) {
            *mapAddr = mappedAddr[1];
            *fd1 = fd[1];
            *fd2 = fd[2];
            close(fd[0]);
            return true;
        }
    }

    // something failed, close all and return error
    for (i = 0; i < NUM; i++) {
        close(fd[i]);
    }

    return false;
}

int stun_init(const char *ip)
{
    return stunParseServerName((char *)ip, g_stun_srv);
}

int stun_socket(const char *ip, uint16_t port, StunAddress4 *map)
{
    int fd;
    StunAddress4 src;
    if (ip == NULL) {
        fd = stunOpenSocket(g_stun_srv, map, port, NULL, 0);
    } else {
        stunParseServerName((char *)ip, src);
        fd = stunOpenSocket(g_stun_srv, map, port, &src, 0);
    }

    return fd;
}

int stun_nat_type()
{
    bool verbose = false;
    bool presPort = false;
    bool hairpin = false;

    StunAddress4 sAddr;
    sAddr.port = 0;
    sAddr.addr = 0;

    NatType stype = stunNatType(g_stun_srv, verbose, &presPort, &hairpin, 0, &sAddr);
    switch (stype) {
        case StunTypeOpen:
            printf("No NAT detected - P2P should work\n");
	    break;
	case StunTypeConeNat:
	    printf("NAT type: Full Cone Nat detect - P2P will work with STUN\n");
	    break;
	case StunTypeRestrictedNat:
	    printf("NAT type: Address restricted - P2P will work with STUN\n");
	    break;
	case StunTypePortRestrictedNat:
	    printf("NAT type: Port restricted - P2P will work with STUN\n");
	    break;
	case StunTypeSymNat:
	    printf("NAT type: Symetric - P2P will NOT work\n");
	    break;
	case StunTypeBlocked:
	    printf("Could not reach the stun server - check server name is correct\n");
	    break;
        case StunTypeFailure:
	    printf("Local network is not work!\n");
	    break;
	default:
	    printf("Unkown NAT type\n");
	    break;
    }
    return stype;
}

void stun_keep_alive(int fd)
{
    char buf[32] = "keep alive";
    sendMessage(fd, buf, strlen(buf), g_stun_srv.addr, g_stun_srv.port, 0);
}
