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
    u8 buf[21] = {0};
    u8 had_some_value = 0;
    u8 j = 0;
    if (2 == fall_mode) {
        for(u8 i=3;i<len;i++) //不管传进来的值有多少 从第三位开始，不等于0 EF 就记录
        {
            if((data[i]!=0 && data[i]!=0xEF) )
            {
                buf[j+1] = data[i];
                j++;
            }
        }
        if(buf[5]!=0){//判断第五位是否有值，有值说明有号码传入 如果第五位都没有值 那就都是0
            had_some_value = 1;
            buf[0] = j;
            memcpy(&fall_detection, buf, j+1); //有手机码 写入fall_detection 没有就不动
        }
        put_buf(buf,j+1);
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
        if(had_some_value){//判断是否有号码写入fall_detection这个结构体  有就写VM 没有就获取VM的号码进行设置
            had_some_value = 0;
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
    printf("%s", __func__);
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
        // printf("fall_data = %d %d %d %d %d %d %d %d %d %d %d %d",fall_data[2],fall_data[3],fall_data[4] \
        // ,fall_data[5],fall_data[6],fall_data[7],fall_data[8],fall_data[9],fall_data[10],fall_data[11]);
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