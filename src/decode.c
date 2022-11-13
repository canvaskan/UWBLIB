#include "uwblib.h"

/* get fields (little-endian) ------------------------------------------------*/
#define U1(p) (*((uint8_t *)(p)))
#define I1(p) (*((int8_t *)(p)))
static uint16_t U2(uint8_t *p)
{
    uint16_t u;
    memcpy(&u, p, 2);
    return u;
}
static int16_t I2(uint8_t *p)
{
    int16_t i;
    memcpy(&i, p, 2);
    return i;
}
static uint32_t U4(uint8_t *p)
{
    uint32_t u;
    memcpy(&u, p, 4);
    return u;
}

// 校验算法：
// 校验不包括包头
// 使用check sum校验，校验程序如下：
static uint16_t Checksum_u16(uint8_t *pdata, uint32_t len)
{
    uint16_t sum = 0;
    uint32_t i;
    for (i = 0; i < len; i++)
        sum += pdata[i];
    sum = ~sum;
    return sum;
}

void decodeBinaryObs(const Config *config)
{
    for (int i = 0; i < CONFIG_MAX_OBS_FILE_N; i++)
    {
        if (config->obs_files[i] == NULL)
        {
            continue;
        }
        FILE *fr = fopen(config->obs_files[i], "rb");

        char obs_file[MAX_LINE_LEN] = "";
        strcpy(obs_file, config->obs_files[i]);
        FILE *fw_obs = fopen(strcat(obs_file, DECODE_ASCII_OBS_SUFFIX), "w");

        strcpy(obs_file, config->obs_files[i]);
        FILE *fw_heartbeat = fopen(strcat(obs_file, DECODE_ASCII_HEARTBEAT_SUFFIX), "w");

        if (!fr)
        {
            printf("ERORR: decodeBinaryObs cannot open obs file\n");
            exit(EXIT_FAILURE);
        }
        if (!fw_obs || !fw_heartbeat)
        {
            printf("ERORR: decodeBinaryObs cannot open obs/heartbeat txt file to write\n");
            exit(EXIT_FAILURE);
        }

        // see the file length
        fseek(fr, 0, SEEK_END);
        long fsize = ftell(fr);
        rewind(fr);

        // read entire file
        char *buffer = (char *)malloc(sizeof(char) * fsize);
        if (!buffer)
        {
            printf("ERORR: decodeBinaryObs cannot allocate memory!\n");
            exit(EXIT_FAILURE);
        }
        size_t read_size = fread(buffer, 1, fsize, fr);
        if (read_size != fsize)
        {
            printf("ERORR: decodeBinaryObs reading error!\n");
            exit(EXIT_FAILURE);
        }

        // start decoding
        int heartbeat_i = 0;
        int obs_i = 0;

        // write header
        fprintf(fw_heartbeat, "anchor_id status temp retention flag time\n");
        fprintf(fw_obs, "anchor_id tag_id distance rssi battery SOS mobile retention flag time\n");

        for (char *p = buffer; p - buffer < read_size;)
        {
            // start of frame
            uint16_t SOF = U2(p);
            p += 2;
            if (SOF != 0x66DD)
            {
                continue;
            }

            uint8_t length = U1(p);
            uint16_t checksum = Checksum_u16(p, length - 2 - 2); // except 2 for SOF, 2 for crc itself
            p += 1;
            uint8_t type = U1(p);
            p += 1;

            // TYPE: Anchor Heartbeat
            if (length == 0x0C && type == 0x02)
            {
                uint16_t anchor_id = U2(p);
                p += 2;
                uint8_t status = U1(p);
                p += 1;
                uint8_t temp = U1(p);
                p += 1;
                uint8_t retention = U2(p);
                p += 2;
                uint16_t check = U2(p);
                p += 2;
                // crc check
                int flag = 1; // 1:OK, 0:Fail
                if (checksum != check)
                {
                    flag = 0;
                    printf("Warning: decodeBinaryObs anchor heartbeat CRC check fail!"
                           " anchor_id=%d, status=%d, temp=%d, retention=%d\n",
                           anchor_id, status, temp, retention);
                }
                // time stamp (may be missing)
                if (U2(p) == 0x66DD)
                {
                    fprintf(fw_heartbeat, "%d %d %d %d %d 0 0\n",
                            anchor_id, status, temp, retention, flag);
                }
                else
                {
                    char timestamp[20] = "";
                    memcpy(timestamp, p, 19);
                    p += 19;

                    fprintf(fw_heartbeat, "%d %d %d %d %d %s\n",
                            anchor_id, status, temp, retention, flag, timestamp);
                }

                heartbeat_i++;
            }

            // TYPE: Obs measurement
            else if (length == 0x14 && type == 0x0A)
            {
                uint16_t anchor_id = U2(p);
                p += 2;
                uint16_t tag_id = U2(p);
                p += 2;
                uint32_t distance = U4(p);
                p += 4;
                int16_t rssi = I2(p);
                p += 2;
                uint8_t battery = U1(p);
                p += 1;
                uint8_t SOS = U1(p);
                p += 1;
                uint8_t mobile = U1(p);
                p += 1;
                uint8_t retention = U1(p);
                p += 1;
                uint16_t check = U2(p);
                p += 2;
                // crc check
                int flag = 1; // 1:OK, 0:Fail
                if (checksum != check)
                {
                    flag = 0;
                    printf("Warning: decodeBinaryObs obs measurement CRC check fail!"
                           " anchor_id=%d, tag_id=%d, distance=%d, rssi=%d, battery=%d, SOS=%d, mobile=%d, retention=%d\n",
                           anchor_id, tag_id, distance, rssi, battery, SOS, mobile, retention);
                }
                // time stamp may be missing
                if (U2(p) == 0x66DD)
                {
                    fprintf(fw_obs, "%d %d %d %d %d %d %d %d %d 0 0\n",
                             anchor_id, tag_id, distance, rssi, battery, SOS, mobile, retention, flag);
                }
                else
                {
                    char timestamp[20] = "";
                    memcpy(timestamp, p, 19);
                    p += 19;

                    fprintf(fw_obs, "%d %d %d %d %d %d %d %d %d %s\n",
                            anchor_id, tag_id, distance, rssi, battery, SOS, mobile, retention, flag, timestamp);
                }

                obs_i++;
            }
        }

        // CLOSE!!! and FREE!!!
        fclose(fr);
        fclose(fw_obs);
        fclose(fw_heartbeat);
        free(buffer);
        buffer = NULL;
    }
    return;
}