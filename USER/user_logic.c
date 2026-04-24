st_len = 0;
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
                    OLED_ShowChinese(0,0,"取件码");
										OLED_ShowChinese(98,0,"：");
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
                    OLED_ShowString(0,16,s,OLED_8X16);
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
