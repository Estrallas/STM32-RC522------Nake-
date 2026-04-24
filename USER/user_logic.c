#include "stm32f10x.h" 
#include "user_logic.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "Servo.h"
#include "rc522.h"
#include "delay.h"
#include "button4_4.h"
#include "stm32f10x_flash.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern u8 CardA_UID[4];

// 配置：
#define DOOR_COUNT 13 // 柜门编号 0..12
// Flash 存储地址，视设备 flash 大小调整（默认使用 0x0800F800）。
#define FLASH_STORE_ADDR 0x0800F800

static uint32_t codes[DOOR_COUNT]; // 0 表示空
static uint8_t door_occupied[DOOR_COUNT];

typedef enum {MODE_TAKE = 0, MODE_STORE = 1} WorkMode;
static WorkMode mode = MODE_TAKE;

// 最大长度限制
#define TAKE_CODE_MAX_LEN 6 // 取件码显示为 6 位
#define DOOR_NUM_MAX_LEN 2  // 柜门编号最多 2 位

// 输入缓冲
static char input_buf[16];
static int input_len = 0;
static int admin_authenticated = 0;
static int current_open_door = -1; // 打开的门，-1 表示无
static int awaiting_confirm_close = 0;
static int previewed_door = -1; // 当查看已占用柜门的取件码时保存门号，按确认回到状态页

// 按键->数字映射
static const int key_to_digit[17] = {
    -1, // 0
    1, // key1 -> digit 1
    2, // key2 -> 2
    3, // key3 -> 3
    -1, // key4 reserved for enter take mode
    4, // key5 -> 4
    5, // key6 -> 5
    6, // key7 -> 6
    -1, // key8 enter store mode
    7, // key9 -> 7
    8, // key10 -> 8
    9, // key11 -> 9
    -2, // key12 -> backspace
    -3, // key13 unused
    0,  // key14 -> 0
    -4, // key15 unused
    -5  // key16 -> confirm
};

static void load_from_flash(void)
{
    uint32_t *p = (uint32_t*)FLASH_STORE_ADDR;
    for (int i=0;i<DOOR_COUNT;i++) {
        codes[i] = p[i];
        door_occupied[i] = (codes[i] != 0) ? 1 : 0;
    }
}

static int save_to_flash(void)
{
    FLASH_Status status;
    FLASH_Unlock();
    // 擦除目标页
    status = FLASH_ErasePage(FLASH_STORE_ADDR);
    if (status != FLASH_COMPLETE) {
        FLASH_Lock();
        return -1;
    }
    // 写入 codes
    for (int i=0;i<DOOR_COUNT;i++) {
        status = FLASH_ProgramWord(FLASH_STORE_ADDR + i*4, codes[i]);
        if (status != FLASH_COMPLETE) {
            FLASH_Lock();
            return -2;
        }
    }
    FLASH_Lock();
    return 0;
}

static uint32_t gen_code(int door)
{
    // 生成形式: 门号(2位)*100000 + 随机5位, 显示为 6 位数
    int randv = rand() % 100000;
    uint32_t code = (door % 100) * 100000 + randv;
    return code;
}

static int find_code_index(uint32_t code)
{
    for (int i=0;i<DOOR_COUNT;i++) if (codes[i] == code) return i;
    return -1;
}

static void open_door(int door)
{
    if (door < 0 || door >= DOOR_COUNT) return;
    PCA9685_SetAngle((uint8_t)door, 90.0f);
}

static void close_door(int door)
{
    if (door < 0 || door >= DOOR_COUNT) return;
    PCA9685_SetAngle((uint8_t)door, 0.0f);
}

// 在屏幕中心显示当前输入的数字（取件码或门号）
// 使用 OLED_ShowNum 在 (X=0, Y=32) 显示数字，先清除显示区域
static void display_input_number(void)
{
    // 清除中心数字区域（6位 * 8px 宽, 16px 高）
    OLED_ClearArea(0, 32, 6*8, 16);
    if (input_len > 0) {
        uint32_t val = (uint32_t)atoi(input_buf);
        // 长度传入 input_len 保证左侧不补零
        OLED_ShowNum(0, 32, val, input_len, OLED_8X16);
    }
}

void UserLogic_Init(void)
{
		OLED_Init();
    // load stored codes
    for (int i=0;i<DOOR_COUNT;i++) { codes[i]=0; door_occupied[i]=0; }
    load_from_flash();
    mode = MODE_TAKE; // 初始为取件模式
    input_len = 0;
    input_buf[0] = '\0';
    admin_authenticated = 0;
    current_open_door = -1;
    awaiting_confirm_close = 0;
    previewed_door = -1;
    // OLED 初始显示
    OLED_Clear();
    // 使用中文显示函数显示中文提示
    OLED_ShowChinese(0,0,"取件码");
    OLED_ShowChinese(50,0,"：");
    OLED_Update();
}

static int is_admin(u8 *uid)
{
    for (int i=0;i<4;i++) if (uid[i] != CardA_UID[i]) return 0;
    return 1;
}

// 显示存件模式下各门占用状态并提示输入
static void show_store_statuses(void)
{
    OLED_Clear();
    OLED_ShowChinese(0,0,"存件模式 空闲:");
    char buf[32] = {0};
    int line = 1;
    for (int i=0;i<DOOR_COUNT;i++) {
        buf[i%16] = door_occupied[i] ? '1' : '0';
        if ((i%16)==15 || i==DOOR_COUNT-1) {
            buf[(i%16)+1] = '\0';
            OLED_ShowString(0, line, buf, OLED_6X8);
            memset(buf,0,sizeof(buf));
            line++;
        }
    }
    OLED_ShowChinese(0, line, "输入门编号并确认");
    OLED_Update();
}

void UserLogic_HandleRFID(u8 *uid)
{
    // 读取到身份：只有在存件模式并刷到管理员卡才会显示空余柜门
    if (mode == MODE_STORE)
    {
        if (is_admin(uid))
        {
            admin_authenticated = 1;
            previewed_door = -1;
            show_store_statuses();
        }
    }
}

void UserLogic_HandleKey(int key)
{
    if (key == 4) { // 进入取件模式
        mode = MODE_TAKE;
        input_len = 0;
        input_buf[0] = '\0';
        admin_authenticated = 0;
        current_open_door = -1;
        awaiting_confirm_close = 0;
        OLED_Clear();
        OLED_ShowChinese(0,0,"取件码:");
        OLED_Update();
        return;
    }
    if (key == 8) { // 进入存件模式
        mode = MODE_STORE;
        input_len = 0;
        input_buf[0] = '\0';
        admin_authenticated = 0;
        current_open_door = -1;
        awaiting_confirm_close = 0;
        // 提示刷管理员卡
        OLED_Clear();
        OLED_ShowChinese(0,0,"存件模式 请刷卡");
        OLED_Update();
        return;
    }

    int map = -99;
    if (key >=1 && key <=16) map = key_to_digit[key];

    if (map == -2) { // backspace
        if (input_len>0) {
            input_len--;
            input_buf[input_len]=0;
        }
        // 更新中心数字显示
        previewed_door = -1;
        display_input_number();
        OLED_Update();
    } else if (map == -5) { // confirm
        // 如果当前有门已打开并等待确认关闭，确认键用于关闭门
        if (awaiting_confirm_close && current_open_door >= 0) {
            close_door(current_open_door);
            // 存件模式下，门已被占用（code 已写入），关闭后返回取件模式
            if (mode == MODE_STORE) {
                // 存件流程：关闭门后返回到存件状态页，允许继续存件/查看
                current_open_door = -1;
                awaiting_confirm_close = 0;
                previewed_door = -1;
                // 保持 admin_authenticated = 1
                show_store_statuses();
                return;
            } else if (mode == MODE_TAKE) {
                // 取件成功，清除存码
                codes[current_open_door] = 0;
                door_occupied[current_open_door] = 0;
                save_to_flash();
                mode = MODE_TAKE;
                current_open_door = -1;
                awaiting_confirm_close = 0;
                OLED_Clear();
                OLED_ShowChinese(0,0,"取件码:");
                OLED_Update();
                return;
            }
        }

        // 如果之前是查看已占用柜门的取件码，按确认回到状态页
        if (mode == MODE_STORE && previewed_door >= 0) {
            previewed_door = -1;
            show_store_statuses();
            return;
        }

        if (mode == MODE_TAKE) {
            if (input_len>0) {
                uint32_t code = (uint32_t)atoi(input_buf);
                int idx = find_code_index(code);
                if (idx >=0 && door_occupied[idx]) {
                    // 成功，开门
                    open_door(idx);
                    OLED_ShowString(0,3,"open",OLED_8X16);
                    OLED_Update();
                    current_open_door = idx;
                    awaiting_confirm_close = 1;
                    // 清空输入缓冲，等待用户确认关闭
                    input_len = 0;
                    input_buf[0] = '\0';
                    display_input_number();
                    OLED_Update();
                } else {
                    OLED_ShowString(0,3,"ERROR", OLED_8X16);
                    OLED_Update();
                }
            }
        } else { // STORE 模式
            // 只有管理员已验证才允许存件
            if (!admin_authenticated) {
                // 使用中文显示并以行号 3 显示（第三行）
                OLED_ShowChinese(0,3,"请刷卡");
                OLED_Update();
                return;
            }
            if (input_len>0) {
                int door = atoi(input_buf);
                if (door < 0 || door >= DOOR_COUNT) {
                    OLED_ShowString(0,3,"ERROR",OLED_8X16);
                    OLED_Update();
                } else if (door_occupied[door]) {
                    // 已被占用：显示该门的取件码（不报错），并进入预览态，按确认回到状态页
                    uint32_t code = codes[door];
                    OLED_Clear();
                    OLED_ShowChinese(0,0,"取件码:");
                    OLED_ShowNum(0,32,code,6,OLED_8X16);
                    OLED_Update();
                    previewed_door = door;
                    // 清空缓冲，便于下一次输入
                    input_len = 0;
                    input_buf[0] = '\0';
                    display_input_number();
                    return;
                } else {
                    uint32_t code = gen_code(door);
                    codes[door] = code;
                    door_occupied[door] = 1;
                    save_to_flash();
                    char s[32];
                    sprintf(s,"Code:");
                    OLED_ShowString(0,1,s,OLED_8X16);
                    // 在屏幕中心显示生成的6位取件码
                    OLED_ShowNum(0,32,code,6,OLED_8X16);
                    OLED_Update();
                    // 开门并等待确认收回
                    open_door(door);
                    current_open_door = door;
                    awaiting_confirm_close = 1;
                    // 存件成功后清空输入缓冲
                    input_len = 0;
                    input_buf[0] = '\0';
                    display_input_number();
                    OLED_Update();
                }
            }
        }
    } else if (map >=0 && map <=9) {
        // append digit to buffer with长度限制：取件码和柜门编号分别有限制
        int max_len = (mode == MODE_TAKE) ? TAKE_CODE_MAX_LEN : DOOR_NUM_MAX_LEN;
            if (input_len < max_len && input_len < (int)(sizeof(input_buf)-1)) {
            previewed_door = -1;
            input_buf[input_len++] = '0' + map;
            input_buf[input_len] = '\0';
            display_input_number();
            OLED_Update();
//        } else {
//            // 可选：当达到最大长度时给出提示（提示在第三行显示）
//            OLED_ShowChinese(0,24, (mode == MODE_TAKE) ? "取件码位数已满" : "门号位数已满");
//            OLED_Update();
        }
    }
}

void UserLogic_Loop(void)
{
    // placeholder: check for confirm to close door when already opened
    // 用户按确认时如果当前有门打开则关闭
    // 简化：不实现门状态检测，这里不做额外处理
}
