#include "smartbox/config.h"
#include "sport_info_opt.h"
#include "smartbox/event.h"
#include "smartbox_rcsp_manage.h"

#include "sport_info_fall_detection.h"
#include "sport_data/detection_response.h"
#include "watch_common.h"

#if (SMART_BOX_EN && JL_SMART_BOX_SENSORS_DATA_OPT)

void sport_info_fall_detection_attr_set(void *priv, u8 attr, u8 *data, u16 len)
{
    printf("%s", __func__);
    
    u8 fall_switch = data[0];
    u8 fall_mode = data[1];
    put_buf(data,len);
    fall_detection_t fall_detection = {0};
    printf("data[2] = %d %d %d %d %d %d %d %d %d %d %d %d",data[2],data[3],data[4],data[5],data[6], \
    data[7],data[8],data[9],data[10],data[11],data[12],data[13]);
    if (2 == fall_mode) {
        if(data[2])
        {
            printf("data[2] = %d",data[2]);
            memcpy(&fall_detection, data + 2, data[2] + 1);
        }else{
            printf("get vm value put phone number set");
        }
    }
    struct watch_algo algo_hd;
    watch_algo_handle_get(&algo_hd);
    int arg[6];
    arg[0] = FALL;
    arg[1] = fall_switch;
    arg[2] = fall_mode;
    arg[3] = 0;
    algo_hd.detection_ioctrl(3, arg);

    sport_info_switch_record_update(SPORT_INFO_SWTICH_TYPE_FALL_DETECTION, fall_switch, 1);
    sport_info_mode_record_update(SPORT_INFO_MODE_TYPE_FALL_DETECTION, fall_mode);
    
    if (2 == fall_mode ) {
        if(data[2]){
            printf("data[2] = %d",data[2]);
            sport_info_write_vm(VM_SPORT_INFO_FALL_DETECTION, (u8 *)&fall_detection, sizeof(fall_detection_t));
            set_emergency_contact_number((u8 *)&fall_detection.phone_num, fall_detection.phone_len); //保存联系人
        }
        else{
        u8 buf[20];
            memset(buf, 0, 20);
            sport_fall_detection_get((fall_detection_t *)buf);
            printf("buf = %d %d %d %d %d %d %d %d %d %d %d %d",buf[2],buf[3],buf[4] \
        ,buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11]);
            if (buf[0] != 0) {
                set_emergency_contact_number(buf + 1, buf[0]);
            }
            printf("set no number is number");
        }
    }
    
}



u32 sport_info_fall_detection_attr_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    printf("%s777777777777", __func__);
    u32 rlen = 0;

    u8 *fall_data = zalloc(2 + 21);
    fall_data[0] = !!sport_info_swtich_record_get(SPORT_INFO_SWTICH_TYPE_FALL_DETECTION);

    u8 *mode_data = NULL;
    u16 mode_len = sport_info_record_get(SPORT_INFO_MODE_TYPE_FALL_DETECTION, &mode_data);
    printf("mode_data[0]:mode_len %d:%d",mode_data[0],mode_len);
    if (mode_data && mode_len) {
        fall_data[1] = mode_data[0];

    }
    printf("motion_type=%d-- %d", fall_data[0], fall_data[1]);
    mode_len = 2;
    if (2 == fall_data[1]) {
        sport_fall_detection_get((fall_detection_t *)(fall_data + 2));
        printf("fall_data = %d %d %d %d %d %d %d %d %d %d %d %d",fall_data[2],fall_data[3],fall_data[4] \
        ,fall_data[5],fall_data[6],fall_data[7],fall_data[8],fall_data[9],fall_data[10],fall_data[11]);
        mode_len += sizeof(fall_detection_t);
    }
    rlen = add_one_attr(buf, buf_size, offset, attr, fall_data, mode_len);

    if (fall_data) {
        free(fall_data);
    }
    printf("%s %d", __func__, __LINE__);
    return rlen;
}

#endif