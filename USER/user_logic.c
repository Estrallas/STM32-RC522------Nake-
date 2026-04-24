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
#define FLASH_MAGIC 0x43424F58UL
#define ERROR_SHOW_MS 800

// codes[] 在空门时保存 10 位随机保护码；占用时保存 6 位取件码
static uint32_t codes[DOOR_COUNT];
static uint8_t door_occupied[DOOR_COUNT];

typedef enum {MODE_TAKE = 0, MODE_STORE = 1} WorkMode;
static WorkMode mode = MODE_TAKE;

// 最大长度限制
#define TAKE_CODE_MAX_LEN 6 // 取件码显示为 6 位
#define DOOR_NUM_MAX_LEN 2  // 柜门编号最多 2 位

// 输入缓冲：取件码与存件门号分离，避免互相覆盖
static char take_input_buf[16];
static int take_input_len = 0;
static char store_input_buf[16];
static int store_input_len = 0;
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

static int save_to_flash(void);
static uint32_t prng_state = 0xA5A5A5A5UL;

static uint32_t prng_next(void)
{
    // xorshift32：体积小，足够用于取件码与空门保护码生成
    uint32_t x = prng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    prng_state = (x == 0) ? 0x1F123BB5UL : x;
    return prng_state;
}

static uint32_t gen_idle_code10(void)
{
    // 生成 10 位随机数（范围 1000000000 ~ 4294967295）
    uint32_t v = prng_next();
    if (v < 1000000000UL) {
        v += 1000000000UL;
        if (v < 1000000000UL) v = 1000000000UL;
    }
    return v;
}

static uint32_t gen_unique_idle_code10(void)
{
    uint32_t code = gen_idle_code10();
    int unique = 0;
    while (!unique) {
        unique = 1;
        for (int i = 0; i < DOOR_COUNT; i++) {
            if (codes[i] == code) {
                unique = 0;
                code = gen_idle_code10();
                break;
            }
        }
    }
    return code;
}

static void load_from_flash(void)
{
    uint32_t *p = (uint32_t*)FLASH_STORE_ADDR;
    if (p[0] != FLASH_MAGIC) {
        // 首次使用：全部空门，且每门配置独立 10 位随机保护码
        for (int i = 0; i < DOOR_COUNT; i++) {
            door_occupied[i] = 0;
            codes[i] = gen_unique_idle_code10();
        }
        save_to_flash();
        return;
    }

    for (int i=0;i<DOOR_COUNT;i++) {
        uint32_t occ = p[1 + i * 2];
        uint32_t code = p[1 + i * 2 + 1];

        // 数据容错：异常占用位按空门处理并补随机保护码
        if (occ == 1U) {
            door_occupied[i] = 1;
            codes[i] = code;
        } else {
            door_occupied[i] = 0;
            if (code < 1000000000UL) {
                codes[i] = gen_unique_idle_code10();
            } else {
                codes[i] = code;
            }
        }
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
    // 写入魔术字
    status = FLASH_ProgramWord(FLASH_STORE_ADDR, FLASH_MAGIC);
    if (status != FLASH_COMPLETE) {
        FLASH_Lock();
        return -2;
    }

    // 按“占用位 + 码值”写入每个柜门
    for (int i=0;i<DOOR_COUNT;i++) {
        status = FLASH_ProgramWord(FLASH_STORE_ADDR + (1 + i * 2) * 4, (uint32_t)door_occupied[i]);
        if (status != FLASH_COMPLETE) {
            FLASH_Lock();
            return -3;
        }
        status = FLASH_ProgramWord(FLASH_STORE_ADDR + (1 + i * 2 + 1) * 4, codes[i]);
        if (status != FLASH_COMPLETE) {
            FLASH_Lock();
            return -4;
        }
    }
    FLASH_Lock();
    return 0;
}

static uint32_t gen_code(int door)
{
    // 6 位取件码：门号(2 位) + 时间戳扰动与随机数(4 位)
    uint32_t ts = SysTick->VAL;
    uint32_t rnd = prng_next() % 100U;
    uint32_t tail = (ts ^ (ts >> 7) ^ (rnd * 131U)) % 10000U;
    uint32_t code = (uint32_t)(door % 100) * 10000U + tail;

    // 避免与其它已占用柜门取件码冲突
    while (1) {
        int collision = 0;
        for (int i = 0; i < DOOR_COUNT; i++) {
            if (door_occupied[i] && i != door && codes[i] == code) {
                collision = 1;
                ts = (ts << 1) ^ prng_next();
                tail = (ts ^ (ts >> 9) ^ ((prng_next() % 100U) * 17U)) % 10000U;
                code = (uint32_t)(door % 100) * 10000U + tail;
                break;
            }
        }
        if (!collision) break;
    }
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
    if (mode == MODE_TAKE) {
        // 取件模式：整行清除并按输入长度居中显示
        OLED_ClearArea(0, 32, 128, 16);
        if (take_input_len > 0) {
            uint32_t val = (uint32_t)atoi(take_input_buf);
            int x = (128 - take_input_len * 8) / 2;
            if (x < 0) x = 0;
            OLED_ShowNum((uint8_t)x, 32, val, take_input_len, OLED_8X16);
        }
    } else {
        // 存件模式：在“输入门编号：”后显示 1~2 位门号
        OLED_ClearArea(96, 48, 32, 16);
        if (store_input_len > 0) {
            uint32_t val = (uint32_t)atoi(store_input_buf);
            OLED_ShowNum(96, 48, val, store_input_len, OLED_8X16);
        }
    }
}

void UserLogic_Init(void)
{
		OLED_Init();
        prng_state ^= (*(volatile uint32_t*)0x1FFFF7E8) ^ SysTick->VAL;
        if (prng_state == 0) prng_state = 0x6D2B79F5UL;
    // 初始状态强制关闭所有柜门（舵机角度归零）
    for (int i = 0; i < DOOR_COUNT; i++) {
        close_door(i);
    }
    // load stored codes
    for (int i=0;i<DOOR_COUNT;i++) { codes[i]=0; door_occupied[i]=0; }
    load_from_flash();
    mode = MODE_TAKE; // 初始为取件模式
    take_input_len = 0;
    take_input_buf[0] = '\0';
    store_input_len = 0;
    store_input_buf[0] = '\0';
    admin_authenticated = 0;
    current_open_door = -1;
    awaiting_confirm_close = 0;
    previewed_door = -1;
    // OLED 初始显示
    OLED_Clear();
    // 使用中文显示函数显示中文提示
    OLED_ShowChinese(0,0,"请输入取件码");
    OLED_ShowChinese(98,0,"：");
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
    // 标题 (x:0,y:0)
    OLED_ShowChinese(0, 0, "存件模式");

    // 状态区 (y:16..48)，4列x3行，最多显示前12个柜门
    const int max_display = (DOOR_COUNT < 12) ? DOOR_COUNT : 12;
    const int cols = 4;
    const int colWidth = 32; // 128/4
    const int startY = 16;
    const int rowHeight = 10; // 给点垂直间距
    char item[8];
    for (int idx = 0; idx < max_display; idx++) {
        int r = idx / cols;
        int c = idx % cols;
        int x = c * colWidth;
        int y = startY + r * rowHeight;
        // 显示格式 A:0  （字母从 A 开始）
        snprintf(item, sizeof(item), "%c:%d", 'A' + idx, door_occupied[idx] ? 1 : 0);
        OLED_ShowString(x, y, item, OLED_6X8);
    }

    // 输入提示放在 y=48（x:0-128, y:48-64）
    OLED_ShowChinese(0, 48, "输入门编号");
    OLED_ShowChinese(82, 48, "：");
    // 显示当前输入
    display_input_number();
    OLED_Update();
}

void UserLogic_HandleRFID(u8 *uid)
{
    // 只有在存件模式下且刷到管理员卡才显示柜门状态
    if (mode == MODE_STORE)
    {
        // 已认证后忽略后续刷卡，避免状态页被 ERROR 覆盖
        if (admin_authenticated) {
            return;
        }
        if (is_admin(uid))
        {
            admin_authenticated = 1;
            previewed_door = -1;
            show_store_statuses();
        }
        else
        {
            // 非管理员卡：清除“请刷卡”并在同位置显示 ERROR
            OLED_ClearArea(0, 32, 128, 16);
            OLED_ShowString(40,32,"ERROR",OLED_8X16);
            OLED_Update();
            delay_ms(ERROR_SHOW_MS);
            OLED_ClearArea(0, 32, 128, 16);
            OLED_ShowChinese(40,32,"请刷卡");
            OLED_Update();
        }
    }
}

void UserLogic_HandleKey(int key)
{
    if (key == 4) { // 进入取件模式
        mode = MODE_TAKE;
        take_input_len = 0;
        take_input_buf[0] = '\0';
        store_input_len = 0;
        store_input_buf[0] = '\0';
        admin_authenticated = 0;
        current_open_door = -1;
        awaiting_confirm_close = 0;
        OLED_Clear();
				OLED_ShowChinese(0,0,"请输入取件码");
				OLED_ShowChinese(98,0,"：");
        OLED_Update();
        return;
    }
    if (key == 8) { // 进入存件模式
        mode = MODE_STORE;
        take_input_len = 0;
        take_input_buf[0] = '\0';
        store_input_len = 0;
        store_input_buf[0] = '\0';
        admin_authenticated = 0;
        current_open_door = -1;
        awaiting_confirm_close = 0;
        // 提示刷管理员卡
        OLED_Clear();
        OLED_ShowChinese(32,0,"存件模式");
		OLED_ShowChinese(40,32,"请刷卡");
        OLED_Update();
        return;
    }

    int map = -99;
    if (key >=1 && key <=16) map = key_to_digit[key];

    if (map == -2) { // backspace
        if (mode == MODE_TAKE) {
            if (take_input_len > 0) {
                take_input_len--;
                take_input_buf[take_input_len] = 0;
            }
        } else {
            if (store_input_len > 0) {
                store_input_len--;
                store_input_buf[store_input_len] = 0;
            }
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
                codes[current_open_door] = gen_unique_idle_code10();
                door_occupied[current_open_door] = 0;
                save_to_flash();
                mode = MODE_TAKE;
                current_open_door = -1;
                awaiting_confirm_close = 0;
                OLED_Clear();
								OLED_ShowChinese(0,0,"请输入取件码");
								OLED_ShowChinese(98,0,"：");
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
            if (take_input_len > 0) {
                uint32_t code = (uint32_t)atoi(take_input_buf);
                int idx = find_code_index(code);
                if (idx >=0 && door_occupied[idx]) {
                    // 成功，开门
                    open_door(idx);
                    OLED_ShowString(50,48,"open",OLED_8X16);
                    OLED_Update();
                    current_open_door = idx;
                    awaiting_confirm_close = 1;
                    // 清空输入缓冲，等待用户确认关闭
                    take_input_len = 0;
                    take_input_buf[0] = '\0';
                    display_input_number();
                    OLED_Update();
                } else {
                    OLED_ShowString(50,48,"ERROR", OLED_8X16);
                    OLED_Update();
                    delay_ms(ERROR_SHOW_MS);
                    OLED_ClearArea(0, 48, 128, 16);
                    display_input_number();
                    OLED_Update();
                }
            }
        } else { // STORE 模式
            // 只有管理员已验证才允许存件
            if (!admin_authenticated) {
                // 使用中文显示
                OLED_ShowChinese(40,32,"请刷卡");
                OLED_Update();
                return;
            }
            if (store_input_len > 0) {
                int door = atoi(store_input_buf);
                if (door < 0 || door >= DOOR_COUNT) {
                    OLED_ClearArea(0, 48, 128, 16);
                    OLED_ShowString(50,48,"ERROR",OLED_8X16);
                    OLED_Update();
                    delay_ms(ERROR_SHOW_MS);
                    show_store_statuses();
                } else if (door_occupied[door]) {
                    // 已被占用：刷新界面并显示 Code:+取件码，按确认回到状态页
                    uint32_t code = codes[door];
                    char s[16];
                    OLED_Clear();
                    sprintf(s,"Code:");
                    OLED_ShowString(0,16,s,OLED_8X16);
                    OLED_ShowNum(40,32,code,6,OLED_8X16);
                    OLED_Update();
                    previewed_door = door;
                    // 清空缓冲，便于下一次输入
                    store_input_len = 0;
                    store_input_buf[0] = '\0';
                    return;
                } else {
                    uint32_t code = gen_code(door);
                    codes[door] = code;
                    door_occupied[door] = 1;
                    save_to_flash();
                    char s[32];
                    sprintf(s,"Code:");
                    OLED_ShowString(16,16,s,OLED_8X16);
                    // 在屏幕中心显示生成的6位取件码
                    OLED_ShowNum(32,32,code,6,OLED_8X16);
                    OLED_Update();
                    // 开门并等待确认收回
                    open_door(door);
                    current_open_door = door;
                    awaiting_confirm_close = 1;
                    // 存件成功后清空输入缓冲
                    store_input_len = 0;
                    store_input_buf[0] = '\0';
                    display_input_number();
                    OLED_Update();
                }
            }
        }
    } else if (map >=0 && map <=9) {
        // append digit to buffer with长度限制：取件码和柜门编号分别有限制
        int max_len = (mode == MODE_TAKE) ? TAKE_CODE_MAX_LEN : DOOR_NUM_MAX_LEN;
        if (mode == MODE_TAKE) {
            if (take_input_len < max_len && take_input_len < (int)(sizeof(take_input_buf)-1)) {
                previewed_door = -1;
                take_input_buf[take_input_len++] = '0' + map;
                take_input_buf[take_input_len] = '\0';
                display_input_number();
                OLED_Update();
            }
        } else {
            if (store_input_len < max_len && store_input_len < (int)(sizeof(store_input_buf)-1)) {
                previewed_door = -1;
                store_input_buf[store_input_len++] = '0' + map;
                store_input_buf[store_input_len] = '\0';
                display_input_number();
                OLED_Update();
            }
        }
//        } else {
//            // 可选：当达到最大长度时给出提示（提示在第三行显示）
//            OLED_ShowChinese(0,24, (mode == MODE_TAKE) ? "取件码位数已满" : "门号位数已满");
//            OLED_Update();
    }
}

void UserLogic_Loop(void)
{
    // placeholder: check for confirm to close door when already opened
    // 用户按确认时如果当前有门打开则关闭
    // 简化：不实现门状态检测，这里不做额外处理
}
