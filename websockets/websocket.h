#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <stdint.h>
#include <string.h>
#include <winsock2.h>
#include "base64.h"
#include "sha1.h"

// Refer to RFC6455 Section 5 for details of a WebSocket frame

/**
 * (some) WebSocket Opcodes
 */
enum WS_OPCODE
{
    OPCODE_TEXT=0x1,
    OPCODE_CLOSE=0x8
    // we do not handle other frame types
};

/**
 * Given a client WebSocket key, construct an accept key in base64
 * @param s Client-given base64 encoded key string
 * @return Generated accept key string (must free after use)
 */
char *make_ws_key(char *req)
{
    /*** THIS FUNCTION IS PROVIDED AS-IS, NO MODIFICATION NEEDED ***/
    char *token = strtok(req, "\r\n");
    while (token)
    {
        if (token == strstr(token, "Sec-WebSocket-Key: "))
        {
            // first 24 bytes of the key will be provided by client, rest is magic number
            char key[60] = "                        258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            strncpy(key, token + 19, 24);
            // key needs to be SHA1-hashed
            char sha1_out[21];
            SHA1(sha1_out, key, 60);
            int flen;
            char *key_out = base64(sha1_out, 20, &flen);
            // CALLER MUST FREE
            return key_out;
        }
        token = strtok(NULL, "\r\n");
    }
    // should not reach here
    return NULL;
}

/**
 * Respond to client upgrade request
 * @param client_fd Socket of requesting client
 * @param accept_key Accept key in base64
 */
void send_ws_accept(SOCKET client_fd, const char *accept_key)
{
    /*
     * Use send() function to send the protocol switch response
     * Note 1: Each HTTP header line must end with \r\n
     * Note 2: Sec-WebSocket-Accept: {content of accept_key string}\r\n
     * Note 3: To append the accept key, you can use multiple send() calls or sprintf()
     * Note 4: HTTP header ends with a line of just \r\n
     */
    /*** YOUR CODE HERE ***/

    char header[0x100];

    const int lenheader = sprintf(header, "HTTP/1.1 101 Switching Protocols\r\n"
                                            "Connection: Upgrade\r\n"
                                            "Upgrade: websocket\r\n"
                                            "Sec-WebSocket-Accept: %s\r\n"
                                            "\r\n",
                                            accept_key);
    send(client_fd,header,lenheader,0);



}

/**
 * Construct a simplified WebSocket frame in buffer from a text payload
 * @param buffer Buffer to contain the frame (to be sent to client)
 * @param msg_buffer Payload (message) string
 * @return size of the resulting frame (message length + 2 for simplified frames)
 */
size_t make_ws_frame(char *buffer, const char *msg_buffer)
{
    size_t msg_len = strlen(msg_buffer);

    if (msg_len > 125)
    {
        printf("Extended payload length is not supported!\n");
        exit(1);
    }

    // no frame fragmentation, so set FIN always
    // reserved bits=000
    // opcode for text frame is 0x1
    /*** YOUR CODE HERE ***/
    buffer[0] = 0b10000001;

    // masking disabled (only required for client)
    // set payload length (7 bits)
    /*** YOUR CODE HERE ***/
    buffer[1] &=~(1 << 7);
    buffer[1] |= (msg_len << 0);

    // copy the message
    /*** YOUR CODE HERE ***/
    strcat(buffer,msg_buffer);
    // simplified header length is 2 bytes
    // frame length is message length + header length
    return msg_len + 2;
}

/**
 * Parse a simplified WebSocket frame and copy the payload
 * @param msg_buffer Buffer to copy the payload string (message)
 * @param buffer Buffer containing the frame
 * @return opcode of the frame
 */
int parse_ws_frame(char *msg_buffer, const char *buffer)
{
    uint8_t fin = buffer[0] & 0x80;
    uint8_t opcode = buffer[0] & 0x0F;
    uint8_t mask = buffer[1] & 0x80;
    uint8_t payload_len = buffer[1] & 0x7F;
    // assuming the simplified format, no extended payload length
    // client sends this, must have masking
    const char *masking_key = buffer + 2;

    // handle only if text frame
    // apply unmasking (same as masking) and save the message inside msg_buffer
    // null-terminate the msg_buffer string
    /*** YOUR CODE HERE ***/
    memset(msg_buffer, 0, sizeof(msg_buffer));

    if ( opcode == OPCODE_TEXT && mask != 0x80){

        for (int i=0; i<payload_len;i++)
        {
            msg_buffer[i]= buffer[i+2];
        }


    }
    else if (opcode == OPCODE_TEXT && mask == 0x80){  //If the MASK bit is set, the frame should contain a 4-byte masking key
        for (int i=0; i<payload_len;i++)
        {
            msg_buffer[i] = buffer[i+6] ^(masking_key[i % 4]);
        }
    }

    return opcode;
}

#endif // WEBSOCKET_H
