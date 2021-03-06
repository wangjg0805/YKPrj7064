//**********************************************************************
//文件名:global.c
//函数列表
//1  初始化全局变量  		    InitGlobalVarible 
//2  初始化语言变量   		    InitLanguageVarible  
//3  初始化亮度变量  		    InitBrightnessVarible
//4  初始化校准屏幕变量   		InitScreenVarible
//5  初始化满量程变量   		Init_Cal_Varible
//6  初始化 0点变量			    Init_Zer0_Weigh_Varible
//7  线性校准数据计算     		Get_Linecal_Var
//8   初始化线性校准变量		Init_LineCal_Varible
//8a  读取当前记录的存放位置    Search_Rec_Pos
//9   显示已消耗时间   		    Display_elapsed_time
//10  待机画面显示  		    Display_Sleep_Area
//11  待机时间显示   		    Display_Sleep_Time
//12  待机状态     			    Sleep_process
//13  开机系统初始化  			SystemInit

//14  根据XY,构建按键数据结构   Create_ButtonList

//15 放置置零标志    		    Set_Zer0_Flag
//16 放置去皮标志   		    Set_Tare_Flag
//17 放置采集百分比基准标志		Set_Sam100_Flag
//18 放置累加标志        		Set_Totaladd_Flag
//19 放置采集目标值数据标志   	Set_SamTarget_Flag

//**********************************************************************
#include "global.h"
#include "i2c_com.h"
#include "eeprom.h"
#include "graph.h"
#include "ad7843.h"
#include "mainmenu.h"
#include "disp.h"
#include "Font_Lib.h"
#include "Font_Lib2.h"
#include "type_def.h"
#include "alarm_pro.h"

#include "epm7064s.h"

#include "rtc.h"
#include "port.h"


un_rec_data  inner_cal_real;
uint32      lcd_brightness;        //显示屏对比度的值

uint32      Key_Status,key_value;  //按键数据以及状态
struct      KeyStruct  Key[20];

uint32      sleepmode;
uint32      machine_mode;    //开机机器进入哪种状态
uint32      current_mode,mode_run_flag;    //机器的数据状态
uint32      current_unit;    //称量单位
uint32      stable_flag;     //稳定标志
uint32      power_on_flag,power_on_clr_cnt; //开机标志与开机去皮时间
uint32      elapsed_time;                   //开机耗时记录
uint32      do_zer0_flag,do_tare_flag;
uint32      do_sam100_flag;
uint32      clr_data,zer0_data,full_code,new_data,new_data_big,tare_data; //校准零点，工作零点
//uint32      Error_Info_Flag,Warning_Info_Flag;

uint32      positive_flag;
uint32      asd_var,lnt_var,prt_mod;  //机器的三个内部参数
uint32      flag_new_data;            //数据有效标志
uint16      line_cal_para, user_cal_para;  //′?á???2?êy ?ù?Y?úá?3ìà′???¨

uint32      point10_cal_start,Point10_Cal_Buf[CAL_POINT_NUM+1],point10_cal_ok; //真实的

uint32      offset_polar;
uint32      offset_50g,offset_100g,offset_150g;
uint32      expected_100g,expected_50g,expected_25g;
  
float       Point10_k[CAL_POINT_NUM-1];
float       percent_10_data;  //1/10内码数据

uint32      point2_cal_start,point2_cal_ok,point2_cal_step;

uint32      inner_code_step;
uint32      auto_zer0_valid;   //自动跟踪是否生效 用于线性校过程中的回零
float       weigh_coef1;
float       tare_weight,gross_weight,net_weight;  //称重相关
////////////////////以下参数是各种模式下使用到的变量
float       weight_per_pcs;    //单件重
uint32      pcs_number;        //件数                              
float       weight_compare_100,number_100;        //百分比的比较基点
float       limit1_weight,limit2_weight;          //checkmode
float       differ_weight; 
float       total_weight;
uint32      total_times,do_totaladd_flag; 
uint32      animal_weight,animal_real_time,animal_time,animal_start,animal_ok; 
float       peak_weight;
uint32      peak_start,peak_end;
float       target_weight;
uint32      do_samtarget_flag;
float       density_temp,density_value,density_weight1,density_weight2;
uint32      density_start;
////////////////////////////wireless configuration
uint32      wireless_pin1_configuration,wireless_pin2_configuration;

float       temp_float,temp_cmp_point;
uint32      temp_cmp_flag;
////////////////////////////增加自动进入待机功能
uint32      no_key_time;
////////////////////////////增加声音及其屏蔽功能
uint8       speaker_volume,sound_switch;    //按键音开关
uint32      which_para,password,model_id,user_id;                       //开机密码

////////////////////////////增加记录功能
uint32      rec_total_num,rec_real_num;  //记录当前存储的记录位置，初始化的时候设置
uint32      rec_total_pages,rec_curre_index;

/////////////////机器型号可选择 所产生的变量
uint32      system_division,dot_position,inner_code_min,inner_code_max,adc_times;
float       additional_num,display_min_value,full_range,weight_alarm;
//增加内校准
uint32      motor_run_time,motor_err_flag,point2_cal_flag,inner_cal_full,inner_cal_flag;

//增加用户内校准数据，用于调试使用
uint32      user_inncal_data;
//add printer function
uint8      print_busy,Uart_Printer_Flag;
//增加功能如下： 开关多个参数
uint8      logo_display_flag,model_display_flag;
uint8      repeat_check_flag,load_track_flag,adjust_0000_flag;
//增加调试数据输出功能
uint32     debug_data_flag,debug_disp_data1,debug_disp_data2;
///////////////////////////////////////////////////
//functions 
//////////////////////////////////////////////////	

//机器型号对应的字符显示
float const machine_alarm[3] = 
 {
  0.090,  0.0090, 0.00090
 };

//机器型号对应的满量程
//这是默认小数点 3位 或者 4位 的满量程
//        小数点 5位 是在这个基础上除以10

/////////////////////////////////////////
//机器型号对应的分辨率
//这是默认小数点 4位 或者 5位 的分辨率�
//        小数点 3位 是在这个基础上除以10
uint32 const machine_division[] = {
1000,
10000,
100000
};
/////////////////////////////////////////
//机器型号对应的ADC 转换速度
//这是默认小数点 4位 或者 5位 的分辨率�
//        小数点 3位 是在这个基础上除以10
uint32 const machine_speed[] = {
112*3,
112*4,
112*4
};

//机器型号对应的四舍五入
float const machine_4s5r[] = {
0.0004,        //小数点后面三位时候的4舍5入值
0.00004,
0.000004
};
//机器型号对应的最小显示数值
float const machine_display_min[] = {
0.004,       
0.0007,
0.00009
};

//////////////////////////////////////////////////
//马达运动过程，参数为目标位置
//超时则返回错误
///////////////////////////////////////////////////

uint32  motor_run_proc(uint8 target_pos)
{
 uint8 tmp;
  if(target_pos == MOTOR_POSITION)
     return(0);
  else   
    {
     motor_run_time = 0;
     FIO0CLR = EPM7_MOTOR_P1;FIO0SET = EPM7_MOTOR_P2;
     while(1)
        {           
         tmp =  MOTOR_POSITION;
         if(1 == tmp)
           FIO2SET = EO2_LED_CPU_S;
         else
           FIO2CLR = EO2_LED_CPU_S;  
         if((motor_run_time==400)||(tmp==target_pos))
            {
             FIO0CLR = EPM7_MOTOR_P1;FIO0CLR = EPM7_MOTOR_P2;
             break;
            } 
        }
     /////////////////////////
     if(motor_run_time==400)  	       
     {
      return(1);
     }
     else
     {
      return(0);
     }
    }  
}
//////////////////////////////////////////////////
//1
//开机初始化 部分全局变量
///////////////////////////////////////////////////
void InitGlobalVarible(void)
	{
	 //Key_Status = KEY_RELEASE ;   //移到待机中初始化
	 ////////////////////////////added after
	 sleepmode         = FALSE;
	 stable_flag       = FALSE;
	 power_on_flag     = TRUE;
	 point2_cal_start  = FALSE;
	 point2_cal_step   = 0;
	 point10_cal_start = FALSE;
	 power_on_clr_cnt  = POWER_ON_STABLE_TIMES; //进入主画面稳定后5秒内作置零处理
     current_mode      = STAT_WEIGH;
     current_unit      = UNIT_G;
     //////////////////////////////////
     do_zer0_flag      = FALSE;
     do_tare_flag      = FALSE;
     do_sam100_flag    = FALSE;
     tare_data         = 0;
     weight_per_pcs    = 0.0;
     weight_compare_100= 0.0;
     ////////////////////////////////
     no_key_time = 0;
     
     temp_cmp_flag = FALSE;
     
    
	}
 //////////////////////////////////////////////////
 //2
 //开机调取语言设置
 ///////////////////////////////////////////////////
uint8  InitLanguageVarible(void)
	{
	 uint32 i;
	 uint8  LangCalData[4];
	 
	 Read_EEPROM(EEP_LANGUAGE_ADDR, LangCalData, 4);
	 for(i=0;i<1000;i++){;}   //delay
	 
	 //Set_Language_Type(ENGLISH);
	 //return(OK);
	 
	 if(LangCalData[3] != LangCalData[0] + REC_CHECK_DATA1)
	   {
	    Set_Language_Type(CHINESE);
	    return(ERROR);
	   } 
	  else
	   {
	    Set_Language_Type(LangCalData[0]);
	    
	    return(OK);
	   }
	   
	 } 	
 //////////////////////////////////////////////////
 //3a
 //开机调取声音设置
 ///////////////////////////////////////////////////
uint8  InitSpeakerVarible(void)
	{
	 uint32 i,j;
	 uint8  SpeakerData[4];
	 
	 Read_EEPROM(EEP_SPEAK_ADDR, SpeakerData, 4);
	 for(i=0;i<1000;i++){;}   //delay
	 //更改为声音的开关
	 //校验
	 i = (SpeakerData[1]<<8) + SpeakerData[0];
	 j = (SpeakerData[3]<<8) + SpeakerData[2];
	 if((i+REC_CHECK_DATA1)==j)
	  {
	   sound_switch = i;
	   return(OK);
	  }
	 else
	  {
	   sound_switch = ON;  //默认开启
	   return(ERROR);
	  } 
	 /*
	 if((SpeakerData[3]==SpeakerData[0]+REC_CHECK_DATA1)&&(SpeakerData[0]>0)&&(SpeakerData[0]<= SPEAK_CLASS_TOTAL))
	   {
	    speaker_volume = SpeakerData[0];  
	    Set_Speaker_Volume(speaker_volume);
	    return(OK);
	   } 
	  else
	   {
	    speaker_volume = DEFAULT_SPEAKER_VOLUME;          //default 
	    Set_Speaker_Volume(speaker_volume);
	    return(ERROR);
	   }*/
	   
	 } 		
	 
//////////////////////////////////////////////////
 //3a
 //开机调取密码
 ///////////////////////////////////////////////////	 
uint8  InitPasswordVarible(void)
{
	 uint32 i,j;
	 uint8  SpeakerData[4];
	 
	 Read_EEPROM(EEP_PASSWORD_ADDR, SpeakerData, 4);
	 for(i=0;i<1000;i++){;}   //delay
	 //校验
	 i = (SpeakerData[1]<<8) + SpeakerData[0];
	 j = (SpeakerData[3]<<8) + SpeakerData[2];
	 if((i+REC_CHECK_DATA1)==j)
	  {
	    password = i;
	    if(password>9999)
	       password = PASSWORD_DEFAULT;
	    return(OK);
	  }
	 else
	  {
	   password = PASSWORD_DEFAULT;
	   return(ERROR);
	  } 
} 		
	 	 
//////////////////////////////////////////////////
 //3a
 //开机调取用户 ID
 ///////////////////////////////////////////////////	 
uint8  InitUserIDVarible(void)
{
	 uint32 i,j;
	 uint8  SpeakerData[4];
	 
	 Read_EEPROM(EEP_USER_ID_ADDR, SpeakerData, 4);
	 for(i=0;i<1000;i++){;}   //delay
	 //校验
	 i = (SpeakerData[1]<<8) + SpeakerData[0];
	 j = (SpeakerData[3]<<8) + SpeakerData[2];
	 if((i+REC_CHECK_DATA1)==j)
	  {
	    user_id = i;
	    if(user_id>999)
	       user_id = USER_ID_DEFAULT;
	    return(OK);
	  }
	 else
	  {
	   user_id = USER_ID_DEFAULT;
	   return(ERROR);
	  } 
} 	

///////////////////////////////////////////////////
//用于双量程 切换
//////////////////////////////	 
void Set_Relay_Oper(void)
     {
      //根据小数点的设置来切换
      if(DOUBLE_DIVISION&&(WAN10_DIVISION==dot_position))
        {
         FIO0SET = RELAY_OUT_PIN;
        }
      else
        {
         FIO0CLR = RELAY_OUT_PIN;
        } 
      }	
//////////////////////////////////////////////////
 //3a
 //开机调取机器型号
 ///////////////////////////////////////////////////	 
uint8  InitModelIDVarible(void)
{
	 uint32 i,j;
	 uint8  buf[8];
	
 	 ///////////////此处调取所有的工厂配置
 	 Read_EEPROM(EEP_FACT_PARA_ADDR, buf, 8);
     for(i=0;i<80000;i++)	{;}	             //delay
    
     if(buf[7] == (buf[0]+buf[1]+buf[2]+buf[3]+buf[4]+buf[5]+buf[6]+REC_CHECK_DATA1))
      {
       logo_display_flag    = buf[0]&0x01;
       model_display_flag   = buf[1]&0x01;
       adjust_0000_flag     = buf[2]&0x01;
       load_track_flag      = buf[3]&0x01;
       repeat_check_flag    = buf[4]&0x01;
       Uart_Printer_Flag    = buf[5]&0x01;
       debug_data_flag      = buf[6]&0x01;   
	  }
     else
      { 
       logo_display_flag    = 1;
       model_display_flag   = 1;
       adjust_0000_flag     = 1;
       load_track_flag      = 0;
       repeat_check_flag    = 0;
       Uart_Printer_Flag    = 1;
       debug_data_flag      = 0;
	   } 
	 
	 
	 for(i=0;i<8;i++)
	    buf[i] = 0x00;
	 Read_EEPROM(EEP_MODEL_ID_ADDR, buf, 4);
	 for(i=0;i<80000;i++){;}   //delay
	 //校验
	 if(buf[3]==(buf[0]+buf[1]+buf[2]))
	   {
	        model_id = (buf[3]<<24)+(buf[2]<<16)+(buf[1]<<8)+buf[0];
	        return(OK);
	    }
	 else
	   {
	        model_id = MODEL_ID_DEFAULT;
	        return(ERROR);
	   }
}

///////////////////////////////////////

void  get_cal_para(void)
{
 uint16  i;
 i = model_id&0xffff;
 if(i<150)
 {
  line_cal_para = 100;
  user_cal_para = 100;
 }
 else if(i<250)
 {
  line_cal_para = 200;
  user_cal_para = 200;
 }
 else if(i<350)
 {
  line_cal_para = 300;
  user_cal_para = 300;
 }
 else if(i<450)
 {
  line_cal_para = 400;
  user_cal_para = 400;
 }
 else 
 {
  line_cal_para = 500;
  user_cal_para = 500;
 }
 
}

///////////////////////////////////////////////
//根据机器型号 初始化系统变量 包含了
// 分辨率
// 满量程重量
// 满量程内码最小数值
// 零点内码最小数值
// 
// 四舍五入数值
// 最小显示数值
/////////////////////////////////////////////// 
void  Init_MachineVarible(void)
{	 
  dot_position = (model_id>>16)&0xff;
  Set_Relay_Oper();
  
  adc_times          = machine_speed[dot_position];
  additional_num     = machine_4s5r[dot_position];
  display_min_value  = machine_display_min[dot_position];
  weight_alarm       = (model_id&0xffff) + machine_alarm[dot_position];  
             
  switch(dot_position)
   {
    case WAN10_DIVISION:
           full_range        = (model_id & 0xffff) / 10;
           system_division   = full_range * machine_division[dot_position];
           break;
    default:
           full_range        = model_id & 0xffff;
           system_division   = full_range * machine_division[dot_position];
           break;
   }
   
   inner_code_min    = system_division*INNER_CODE_STEP_DEF/2;
   inner_code_max    = system_division*INNER_CODE_STEP_DEF*3;   
} 
		 
 //////////////////////////////////////////////////
 //3
 //开机调取屏幕亮度设置
 ///////////////////////////////////////////////////
uint8  InitBrightnessVarible(void)
	{
	 uint32 i;
	 uint8  BriCalData[4];
	 
	 Read_EEPROM(EEP_BRIGHT_ADDR, BriCalData, 4);
	 for(i=0;i<1000;i++){;}   //delay
	 
	 if((BriCalData[3] == BriCalData[0] + REC_CHECK_DATA1)&&(BriCalData[0]<=BRIGHT_CLASS_TOTAL))
	   {
	    lcd_brightness = BriCalData[0];  
	    Set_LCD_Brightness(lcd_brightness);
	    return(OK);
	   } 
	  else
	   {
	    lcd_brightness = DEFAULT_BRIGHTNESS;          //default 
	    Set_LCD_Brightness(lcd_brightness);
	    return(ERROR);
	   }
	 } 		
 //////////////////////////////////////////////////
 //3a
 //开机调取无线配置的设置
 ///////////////////////////////////////////////////
uint8  InitWirelessVarible(void)
	{
	 uint32 i;
	 uint8  WirelessData[4];
	 
	 Read_EEPROM(EEP_WIRELESS_ADDR, WirelessData, 4);
	 for(i=0;i<1000;i++){;}   //delay
	 //标志0x55aa is OK, 则最后两个字节内容可以判定为OK
	 if((WirelessData[3]==REC_CHECK_DATA2)&&(WirelessData[2]==REC_CHECK_DATA1)&&(WirelessData[1]<FUNCTION_END)&&(WirelessData[0]<FUNCTION_END))
	   {
	    wireless_pin1_configuration = WirelessData[0];
	    wireless_pin2_configuration = WirelessData[1];
	    return(OK);
	   } 
	  else
	   {
	    wireless_pin1_configuration = FUNCTION_NULL;          //default 
	    wireless_pin1_configuration = FUNCTION_NULL;
	    return(ERROR);
	   }
	  
	 } 
//////////////////////////////////////////////////
 //3a
 //开机调取内校准数据
 ///////////////////////////////////////////////////	 	
uint8  InitInnerCalVarible(void)
	{
	 uint32 i;
	 uint8  buf[8];
	 
	 un_rec_data tmp;
	 
	 if(DOUBLE_DIVISION&&(WAN10_DIVISION==dot_position))
	    Read_EEPROM(EEP_INNER_CAL2_ADDR, buf, 8);
	 else
	 	Read_EEPROM(EEP_INNER_CAL1_ADDR, buf, 8);
	 	
	 for(i=0;i<1000;i++){;}   //delay
	 //
	  tmp.rec_uint32  = (buf[0]<<24) + (buf[1]<<16) + (buf[2]<<8) + buf[3];
	
	 //判断正常条件
	 if((REC_CHECK_DATA1==buf[4])&&(REC_CHECK_DATA2==buf[5])&&(tmp.rec_float>90)&&(tmp.rec_float<110))
	   {
        //inner_cal_full = i;
        inner_cal_flag = OK; 
	    inner_cal_real = tmp; 
	    return(OK);
	   } 
	 else 
	   {
        inner_cal_flag = FALSE; 
	    inner_cal_real.rec_float = 108.1234; 
	    return(FALSE);
	   } 
	   
	} 		 	  
 //////////////////////////////////////////////////
 //4
 //开机调取屏幕校准数据，如果误差太大，则使用默认数据
 ///////////////////////////////////////////////////
uint8  InitScreenVarible(void)
	{
	 uint32 i;
	 uint16 x00,y00,xmid0,ymid0,xmax0,ymax0;
	 uint8  ScreenCalData[13];
	 uint8  CheckSum;
	 
	 Read_EEPROM(EEP_SCREEN_ADDR, ScreenCalData, 13);
	 for(i=0;i<1000;i++){;}   //delay
	 CheckSum = 0;
	 for(i=0;i<12;i++)
	 CheckSum += ScreenCalData[i];
	 x00   = (ScreenCalData[0] << 8)  + ScreenCalData[1];
     y00   = (ScreenCalData[2] << 8)  + ScreenCalData[3];
	 xmid0 = (ScreenCalData[4] << 8)  + ScreenCalData[5];
	 ymid0 = (ScreenCalData[6] << 8)  + ScreenCalData[7];
	 xmax0 = (ScreenCalData[8] << 8)  + ScreenCalData[9];
	 ymax0 = (ScreenCalData[10] << 8) + ScreenCalData[11];
	 if((CheckSum != ScreenCalData[12])||Set_Originalx0y0(x00,y00)||Set_XmidYmid(xmid0,ymid0)||Set_XmaxYmax(xmax0,ymax0)) 
      {
	  x0 =   X0_DEFAULT;
	  y0 =   Y0_DEFAULT;
	  xmid = XMID_DEFAULT;
	  ymid = YMID_DEFAULT;
	  xmax = XMAX_DEFAULT;
	  ymax = YMAX_DEFAULT;
	  return(ERROR);                 //error
   	  }
	  else
	  return(OK);
	}
 //////////////////////////////////////////////////
 //5
 //开机调取zhongliang校准数据
  ///////////////////////////////////////////////////
uint8  Init_Cal_Varible(void)
	{
	 uint32 i,full_bak;
	 uint8  WeighCalData[12];
	 
	 if(DOUBLE_DIVISION&&(WAN10_DIVISION==dot_position)) //如果是双分度 且量程为十万分之一
	    Read_EEPROM(EEP_WEIFULL2_ADDR, WeighCalData, 12);
	 else
	    Read_EEPROM(EEP_WEIFULL1_ADDR, WeighCalData, 12);
	
	 for(i=0;i<1000;i++){;}   //delay
	 full_code = (WeighCalData[0]<<24) + (WeighCalData[1]<<16) + (WeighCalData[2]<<8) + WeighCalData[3];
	 full_bak  = (WeighCalData[4]<<24) + (WeighCalData[5]<<16) + (WeighCalData[6]<<8) + WeighCalData[7];
	 //把用户内校的数据也读出来
	 user_inncal_data  = (WeighCalData[8]<<24) + (WeighCalData[9]<<16) + (WeighCalData[10]<<8) + WeighCalData[11];
	 
	 //数据有效的条件1      
	 if(full_code==full_bak) //&&(full_code>inner_code_min)&&(full_code<inner_code_max))
	 //if(full_code == full_bak)
	 {
   	  inner_code_step = full_code/system_division;
   	  weigh_coef1     = full_range / full_code;
   	  point2_cal_ok   = TRUE;
	  return(OK);
	  }
	  else if((full_bak>inner_code_min)&&(full_bak<inner_code_max)) 
	  {//数据有效的条件2
	  full_code       = full_bak;
	  inner_code_step = full_bak/system_division;
   	  weigh_coef1     = full_range / full_bak;
   	  point2_cal_ok   = TRUE;
	  if((inner_code_step > INNER_CODE_STEP_MAX)||(inner_code_step < INNER_CODE_STEP_MIN))      
   	  inner_code_step = INNER_CODE_STEP_DEF;
	  return(OK);
	  }
	  else if(QIAN_DIVISION == dot_position)
	  {
	   full_code       = 200000 * INNER_CODE_STEP_MAX;
	   inner_code_step = INNER_CODE_STEP_DEF * 5;
	   weigh_coef1     = 200.0/full_code;    //默认的系数
	   point2_cal_ok   = FALSE;
	   return(ERROR);                        //error
   	  }
   	  else
   	  {
	   full_code       = 2000000 * INNER_CODE_STEP_DEF;
	   inner_code_step = INNER_CODE_STEP_DEF;
	   weigh_coef1     = 200.0/full_code;    //默认的系数
	   point2_cal_ok   = FALSE;
	   return(ERROR);                 //error
   	  }
   	  
    }	
 //////////////////////////////////////////////////
 //6 
 //开机调取 lingdian 线性校准数据 零点
  ///////////////////////////////////////////////////
uint8  Init_Zer0_Weigh1_Varible(void)
	{
	 uint32 i,zer0_cal,zer0_cal_bak;
	 uint8  WeighCalData[16];
	 
	 if(DOUBLE_DIVISION&&(WAN10_DIVISION==dot_position)) //如果是双分度 且量程为十万分之一
	    Read_EEPROM(EEP_WEIZERO_L2_ADDR, WeighCalData,16);
	 else
	    Read_EEPROM(EEP_WEIZERO_L1_ADDR, WeighCalData,16);
	 for(i=0;i<1000;i++){;}   //delay
	 zer0_cal     = (WeighCalData[0]<<24) + (WeighCalData[1]<<16) + (WeighCalData[2]<<8) + WeighCalData[3];
	 zer0_cal_bak = (WeighCalData[4]<<24) + (WeighCalData[5]<<16) + (WeighCalData[6]<<8) + WeighCalData[7];
	 
	 //判断正常条件
	 if((zer0_cal==zer0_cal_bak)&&(zer0_cal>USER_ZER0_CODE_MIN)&&(zer0_cal<USER_ZER0_CODE_MAX))
	  {
   	   clr_data  = zer0_cal;       //线性校准零点
   	  }
   	 else if((zer0_cal_bak>USER_ZER0_CODE_MIN)&&(zer0_cal_bak<USER_ZER0_CODE_MAX))
   	  {
   	   clr_data = zer0_cal_bak;    //线性校准零点
   	  }
   	 else 
   	   clr_data = 0;
   	  /////////////////////////////////////////// 
   	   if(clr_data==0)  
   	   {
	    return(ERROR);                 //error
   	   }
   	 else   
   	   {
   	    return(OK);
	   }  
	}   
   	  
   	  
  //////////////////////////////////////////////////
 //6
 //开机调取 lingdian 校准数据 用户校准 零点
  ///////////////////////////////////////////////////
uint8  Init_Zer0_Weigh2_Varible(void)
	{
	 uint32 i,zer0_cal,zer0_cal_bak;
	 uint8  WeighCalData[16];  	  
   	  ///////////////////////////////////////////////////////////////////////////////////////////////
   	 if(DOUBLE_DIVISION&&(WAN10_DIVISION==dot_position)) //如果是双分度 且量程为十万分之一
	    Read_EEPROM(EEP_WEIZERO_U2_ADDR, WeighCalData,16);
	 else
	    Read_EEPROM(EEP_WEIZERO_U1_ADDR, WeighCalData,16);
	 for(i=0;i<1000;i++){;}   //delay
   	 zer0_cal     = (WeighCalData[0]<<24) + (WeighCalData[1]<<16) + (WeighCalData[2]<<8) + WeighCalData[3];
	 zer0_cal_bak = (WeighCalData[4]<<24) + (WeighCalData[5]<<16) + (WeighCalData[6]<<8) + WeighCalData[7];
	 //判断正常条件
	 if((zer0_cal==zer0_cal_bak)&&(zer0_cal>USER_ZER0_CODE_MIN)&&(zer0_cal<USER_ZER0_CODE_MAX))
	  {
   	   zer0_data  = zer0_cal;       //用户校准零点
   	  }
   	 else if((zer0_cal_bak>USER_ZER0_CODE_MIN)&&(zer0_cal_bak<USER_ZER0_CODE_MAX))
   	  {
   	   zer0_data = zer0_cal_bak;    //用户校准零点
   	  }
   	 else if((clr_data>USER_ZER0_CODE_MIN)&&(clr_data<USER_ZER0_CODE_MAX))
   	   zer0_data = clr_data;
   	 else 
   	   zer0_data = 0;  
   	 ////////////////////////////////////////////////////////  
   	 if(zer0_data==0)  
   	   {
   	    point2_cal_ok = FALSE;
	    return(ERROR);                 //error
   	   }
   	 else   
   	   {
   	    point2_cal_ok = TRUE;
	    return(OK);
	   } 
    }	   

 //////////////////////////////////////////////////
 //7
 //重新计算线性变量
 ///////////////////////////////////////////////////
void   Get_Linecal_Var_4(void)
    {
     uint32 i,j;
     // [1] [2]  [3]  [4]
     // 50  100  150  200
     expected_100g = Point10_Cal_Buf[4]/2;
     expected_50g  = expected_100g/2;
     expected_25g  = expected_50g/2;
     
     if(expected_100g > Point10_Cal_Buf[2]) //与理想100克比较
        offset_polar = 0;             //加处理
     else
        offset_polar = 1;             //减处理
     
     offset_100g = abs(expected_100g-Point10_Cal_Buf[2]); //得到100克需要的修正量
     ///////////////////////////////////////////////////////得到 50克需要的修正量
     if(expected_50g > Point10_Cal_Buf[1])
       {
         if(0 == offset_polar)
           {
             i = expected_50g-Point10_Cal_Buf[1];
             if(i> offset_100g/2)
                offset_50g = i-offset_100g/2;
             else
                offset_50g = 0;  
           }
         else 
           offset_50g = 0;
         
       }
     else
       {
         if(1 == offset_polar)
           {
             i = Point10_Cal_Buf[1] - expected_50g;
             if(i> offset_100g/2)
                offset_50g = i-offset_100g/2;
             else
                offset_50g = 0;  
           }
         else 
           offset_50g = 0; 
       }  
     ///////////////////////////////////////////////////////得到150克需要的修正量
     j = expected_100g+expected_50g;
     
     if(j > Point10_Cal_Buf[3])
       {
         if(0 == offset_polar)
           {
             i = j - Point10_Cal_Buf[3];
             if(i> offset_100g/2)
                offset_150g = i-offset_100g/2;
             else
                offset_150g = 0;  
           }
         else 
           offset_150g = 0;
         
       }
     else
       {
         if(1 == offset_polar)
           {
             i = Point10_Cal_Buf[3] - j;
             if(i> offset_100g/2)
                offset_150g = i-offset_100g/2;
             else
                offset_150g = 0;  
           }
         else 
           offset_150g = 0; 
       }  
	}	
	
////////////////////////////////////////////////////////////
// 数据 线性拉直 处理
////////////////////////////////////////////////////////////	
uint32  Get_Line_data(uint32 weight1)
{  
  uint32 i,j,k,weight2;
  float  offset_tmp[3];  
  float  offset_temp;
  
  if(weight1 < clr_data)
    return(weight1);
    
  weight2 = weight1 - clr_data;
  
  if(weight2 > Point10_Cal_Buf[4])  //超出范围
    {
      i = weight2 - Point10_Cal_Buf[4];
      offset_temp = i*(offset_100g/(expected_50g+1.01));
      if(0 == offset_polar)       
       return(weight1-(uint32)offset_temp); 
      else
       return(weight1+(uint32)offset_temp);
    }
  else
    { //三个偏移量 累加
      offset_tmp[0] = 0.0;
      offset_tmp[1] = 0.0;
      offset_tmp[2] = 0.0;
      for(i=0;i<3;i++)
      {
        if(i==0)
         {
          j = abs(expected_100g-abs(expected_100g-weight2));
          offset_tmp[0] = j*(offset_100g/(expected_100g+1.01)); 
          k = abs(expected_100g-weight2);
         }
        if(i==1)
         {
          j = abs(expected_50g-abs(expected_50g-k));
          if(weight2 > expected_100g)
          offset_temp = j*(offset_150g/(expected_50g+1.01)); 
          else
          offset_temp = j*(offset_50g/(expected_50g+1.01));
          offset_tmp[1] = offset_temp;  
          k = abs(expected_50g-k);  
         }    
        if(i==2)
         {
          j = abs(expected_25g-abs(expected_25g-k));
          if(weight2 > expected_100g)
          offset_temp = j*(offset_150g/(expected_50g+1.01)); 
          else
          offset_temp = j*(offset_50g/(expected_50g+1.01));
          offset_tmp[2] = offset_temp/2;
          //offset_tmp[2] = 0;  
         }
      }  //偏移量计算完毕  
   
      k = (uint32)(offset_tmp[0]+offset_tmp[1]+offset_tmp[2]);
     
      if(0 == offset_polar)       
        return(weight1+k); 
      else
       {
        return(weight1-k);
         
       }
    } 
}
/////////////////////////////////////////////////
//3 计算线性校准后的数据
//  如果没有线性校准，或者数据不再线性校准的范围内，返回
//  原始数据
//  weight1 含0点的数据
//  weight2 纯重量码
/////////////////////////////////////////////////
uint32  Get_Line_data_10(uint32 data_no_line)
{  
    uint32 where_k,data_offset;
    uint32 pure_wei_code;
    ///////////////μúò????é??
    if(data_no_line < clr_data)  //±ê?¨0??ò??? 2?′|àí
      return(data_no_line);
    /////////////////////′???á??ú??  
    pure_wei_code = data_no_line - clr_data;
    
    if(pure_wei_code > Point10_Cal_Buf[10])   //
     {     //////////////////μú?t???é??￡?ê1ó?×?oóò???D±?ê
      data_offset = pure_wei_code - Point10_Cal_Buf[10]; 
      data_offset = data_offset * Point10_k[9];
      return((uint32)(clr_data + 10*percent_10_data + data_offset));
     }
    ////////////////////////////////////////?y3￡???? ??????ò?á?
    for(where_k=1;where_k<11;where_k++)    //1 2é?ò????
    {                                         
     if(pure_wei_code < Point10_Cal_Buf[where_k])
     	break;
     }
    /////////////////////////////////////////////2 3?3??3??D￡×?μ?μ?êy?Y
    data_offset = pure_wei_code - Point10_Cal_Buf[where_k-1]; //
    /////////////////////////////////////////////3 à-??μ?D￡×?μ?
    data_offset = data_offset * Point10_k[where_k-1]; 
    return((uint32)(clr_data + (where_k-1)*percent_10_data + data_offset));
}	

 //////////////////////////////////////////////////
 //7
 //重新计算线性变量
 ///////////////////////////////////////////////////
void   Get_Linecal10_para(uint32 full_data)
    {
     uint32 i;
     if(TRUE == point10_cal_ok)  //如果有线性校准数据
      {
       percent_10_data =  full_data / 10;  //理想10等份
       for(i=0;i<10;i++)
	    Point10_k[i] = percent_10_data/(Point10_Cal_Buf[i+1]-Point10_Cal_Buf[i]); //比列系数 得到偏移量
	  }
	  else
	  {
	   percent_10_data = 0.0;
	   for(i=0;i<10;i++)
	    Point10_k[i] = 0.0;
	  }   
	  
	}
	

//////////////////////////////////////////////////
//////////////////////////////////////////////////
//8
//开机调取线性校准数据
///////////////////////////////////////////////////	     
///////////////////////////////////////////////////////
uint8  Init_LineCal_Varible(void)
	{
	 uint32 i,j;
	 uint32 checksum;
	 uint8  buf[16];
	 
	 for(i=0;i<3;i++)
	 {
	  if(DOUBLE_DIVISION&&(WAN10_DIVISION==dot_position))
	     Read_EEPROM(EEP_LINCAL2_ADDR01+i*16, buf, 16);
	  else
	     Read_EEPROM(EEP_LINCAL1_ADDR01+i*16, buf, 16);
	  
	  for(j=0;j<1000;j++){;}   //delay
	  for(j=0;j<4;j++)
	  Point10_Cal_Buf[4*i+j] = (buf[4*j+0]<<24) + (buf[4*j+1]<<16) + (buf[4*j+2]<<8) + buf[4*j+3];
	 }
	  //////////////check2
	  checksum = 0;
	  for(j=0;j<11;j++)
	  checksum += Point10_Cal_Buf[j];
	   
	  ///////////////////////////////////////////////////
	  if(checksum == Point10_Cal_Buf[11])
	  {//
	   point10_cal_ok = TRUE;
	   Get_Linecal10_para(Point10_Cal_Buf[10]);
	   return(OK);
	  }
	  else
	  {//????
	   point10_cal_ok = FALSE;
	   return(ERROR);
	  }
   }	    
//////////////////////////////////////////////////
//8a
//开机查找当前记录的存放位置
//每条记录占用16个字节

///////////////////////////////////////////////////	     
void  Search_Rec_Pos(void)
     {
       uint32 i,j;
 	   uint8  BufferData[8];
	 
       Read_EEPROM(EEP_REC_TOTAL_ADDR,BufferData,8);
	   for(j=0;j<1000;j++){;}   //delay
	   
	   i = (BufferData[0]<<24)+(BufferData[1]<<16)+(BufferData[2]<<8)+BufferData[3];
	   j = (BufferData[4]<<24)+(BufferData[5]<<16)+(BufferData[6]<<8)+BufferData[7];
	   
	   if((0xffffffff==i)&&(0xffffffff==j)||(i!=j))
	   {//空芯片
	    for(i=0;i<8;i++)
	       BufferData[i] = 0;
	    Write_EEPROM(EEP_REC_TOTAL_ADDR,BufferData, 8); //初始化为0
		for(j=0;j<40000;j++)	{;}	                 //delay
		//默认状态
		rec_total_num = 0;  //累加数据多少，因此提供了寻址
		rec_real_num  = 0;  //有效数据有多少，最多只能是系统可存储数目
		rec_curre_index = 0;
		rec_total_pages = 1;
	   }
	   ////////////////////////////////////////////检查
	   else if(i==j)
	   {                        //正确,返回当前可以添加的
	    rec_total_num = i;      //当前总记录数
	   
	    if(i < RECNUM_PER_PAGE) //当前记录数小于一页
	     {
	      rec_real_num    = i;
	      rec_curre_index = i;
		  rec_total_pages = 1;
	     }
	    else if(i < REC_MAX_NUM) //当前记录数还未处于覆盖状态
	     {
	      rec_real_num    = i;
	      rec_curre_index = i;
	      rec_total_pages = i/RECNUM_PER_PAGE + 1; //有多页
	     } 
	    else
	     { //当前记录数已经处于覆盖模式
	      rec_real_num    = REC_MAX_NUM;  //有效数固定为最大数
	      rec_curre_index = i%REC_MAX_NUM;  
	      rec_total_pages = REC_MAX_NUM/RECNUM_PER_PAGE; //页数固定
	     }
	   }
	  /* else
	   {//异常处理认为无数据
	    rec_total_num = 0;
		rec_curre_index = 0;
		rec_total_pages = 1;
	   }*/
    }   
///////////////////////////////////////////////////////////
//9      显示已耗费时间
///////////////////////////////////////////////////////////
void  Display_elapsed_time(uint32 time_sec)
   {  
       uint32 i,hour,mint,sec;
       //time_sec = 3600* 100+ time_sec;
       hour = 0;
       mint = 0;
       sec  = 0;        
       if(time_sec>3599)
         {
          hour = time_sec/3600;
          time_sec = time_sec%3600;
          }
       if(time_sec>59)
         {
          mint = time_sec/60;
          time_sec = time_sec%60;
          }        
          sec = time_sec;
         
         if(hour>99)
         Put_Data(515-16,     30,"%3d", hour);
         else   
         Put_Data(515,        30,"%02d", hour);
	    
	     Put_Char(485+4*15+3, 30,0x3a);
         Put_Data(560,        30,"%02d", mint);
	     Put_Char(485+7*15+3, 30,0x3a);
	     Put_Data(605,        30,"%02d", sec); 
     }	     
	    
//////////////////////////////////////////////////
//10  待机画面显示
///////////////////////////////////////////////////
void  Display_Sleep_Area(void)
	{
      Clr_Bar(0,0,640,480,COL_BLACK);
      /*Put_Pixel(0,0,COL_WHITE);
      Put_Pixel(638,478,COL_WHITE);
      Put_Pixel(639,479,COL_WHITE);
      Put_Pixel(640,480,COL_WHITE);
      while(1){;}*/
      //Put_Bmp(0,0,logo22560,0);       //display LOGO
	  if(ENGLISH == Get_Language_Type())
	     Put_Bmp(520,410,start11060e,0);    //display START
      else
         Put_Bmp(520,410,start11060c,0);    //display START
      
      Set_ASC_Lib(Arial16B_Font);
      Set_Chr_Color(COL_WHITE,COL_BLACK);
      Put_Str(Language_Pos(400,370), 5,Language_Str("系统时间"," System Time"));
      Put_Str(Language_Pos(400,370),30,Language_Str("运行时间","Elapsed Time"));
     }

///////////////////////////////////////////////////////////
//11
//    待机时间显示
///////////////////////////////////////////////////////////
void Display_Sleep_Time(void)
  { 
   RTCTime local_time_buf;
   if(Sec_Change_Flag)
        {
         Sec_Change_Flag = FALSE;
      
         local_time_buf = RTCGetTime();
         Set_ASC_Lib(Arial20B_Font);
         Set_Chr_Color(COL_WHITE,COL_BLACK);
         //display time
	     Put_Data(515,        5,"%02d", local_time_buf.RTC_Hour);
	     Put_Char(485+4*15+3, 5,0x3a);
         Put_Data(560,        5,"%02d", local_time_buf.RTC_Min);
	     Put_Char(485+7*15+3, 5,0x3a);
	     Put_Data(605,        5,"%02d", local_time_buf.RTC_Sec);
	     Display_elapsed_time(elapsed_time);
        } 
   }  
///////////////////////////////////////////////////////////
//12
//    开机预热时间不够，进入待机模式
///////////////////////////////////////////////////////////      
void  Sleep_process(void)
    {
      
      uint32 i,X_position,Y_position,Quit_flag;
      
      Quit_flag = FALSE;
         
      InitScreenVarible();         //把屏幕校准数据提前调出来
      InitLanguageVarible();       //把语言参数提前调出来 
      InitBrightnessVarible();     //屏幕亮度
      InitSpeakerVarible();        //声音
      InitPasswordVarible();       //把储存的密码调出来
      
      Key_Status = KEY_RELEASE+KEY_DO_OVER ;   //key release
      elapsed_time = 0;            //
         
      Display_Sleep_Area();  
        
      do
      {
       ///////////////////////////////keyscan
       for(i=0;i<1000;i++){;}   //delay
       key_value = Get_PositionXY();   //键盘扫描
       if(Key_Status == KEY_DO_START)
       {
	     X_position = (key_value>>16)&0x0000ffff;
	     Y_position =  key_value & 0x0000ffff;
	     if((X_position>520)&&(X_position<630)&&(Y_position>410)&&(Y_position<470))
	         Quit_flag = 0x55aa;
	      else
	         Quit_flag = FALSE;
	     Key_Status = KEY_DO_OVER;  
        }
       /////////////////////////////display time
       Display_Sleep_Time();
      
       ////////////////////////////////////////////
       if((0x55aa == Quit_flag)&&(elapsed_time > PREHOT_TIME)) //预热时间大于30分钟
           Quit_flag = TRUE;
       /////////////////////////////////////////////////////////////////////
       if((0x55aa == Quit_flag)&&(elapsed_time < PREHOT_TIME)) //预热时间小于30分钟
        {
         ////////////////////////借用PCS输入状态采集开机密码
         PCSInput_Interface();
         Set_ASC_Lib(Arial16B_Font);
         Set_Chr_Color(COL_DEEPYELLOW,COL_DEEPBLUE);
         Put_Str(Language_Pos(230-100,200-100),85,Language_Str("系统预热时间不足 请输入开机密码","Not Enough Prehot,Please input Password")); 
       
         do
         {
           i = PCSInput_Key_Pro(Get_PositionXY(),PASSWORD_INPUT_ID); //
               PCSInput_Disp_Pro();
           if((i==password)||(i==PASSWORD_DEFAULT))
             {
              Quit_flag = TRUE;  
             }
             else if(i != PASSWORD_INPUT_ID) //密码无效，重新刷新输入
              PCSInput_Interface(); 
          } 
          while(TRUE !=Quit_flag);
         }
      }   
      while(FALSE == Quit_flag);
    }
 
//////////////////////////////////////////////////
//13 开机初始化所有全局变量
//return 
///////////////////////////////////////////////////
uint8  SystemInit(void)
{   
    uint32 key_times=0;
    uint32 key_buf[4]={0,0,0,0};
    uint32 i,j,key_posx,key_posy;
     
    InitGlobalVarible();
    Search_Rec_Pos();           //查找当前存放的记录位置
     
    //InitModelIDVarible();       //先把机器型号的参数读出来
    //Init_MachineVarible();
    //InitBrightnessVarible();             
    /*///////////////////////////////TEST START
    Clr_Bar(0,0,800,480,COL_BLACK);
    Put_Pixel(0,0,COL_WHITE);
    Put_Pixel(799,479,COL_WHITE);
    Set_Chr_Color(COL_DEEPBLUE,COL_BLACK);
    Set_ASC_Lib(Arial14B_Font);
    Put_Data_Float(200,120,"%7.4f",123.4567);
    while(1){;}
    ////////////////////////////////TEST END*/
    //DRAW TITLE
    Clr_Bar(0,0,640,100,COL_BLACK);								//TITLE
	
	//Put_Bmp(0,0,logo22560,0);
   
	Clr_Bar(0,100,640,380,COL_BLACK);
	
	Set_ASC_Lib(Arial16B_Font);
    Set_Chr_Color(COL_WHITE,COL_BLACK);   
   
   
  	//Put_Str(20,20,Language_Str("Benchmark Scientific","Benchmark Scientific"));
  	 
    if(1 == model_display_flag) 
    {
      if(INNER_CAL_VERSION)
       Put_Str( 280-80,   160,"SYXXXC");
      else
       Put_Str( 280-80,   160,"SYXXX");
     
       Put_Data(280-80+22,160, "%02d", (model_id&0xffff)/10);
       if(1 == dot_position)
       Put_Data(280-80+44,160,"%01d" , 4);
       else
       Put_Data(280-80+44,160,"%01d" , 3);
       
       Put_Str(280,160,Language_Str(" 电子天平"," Electronic Balance"));
    }
    
    
  	Put_Str(280-80,210,Language_Str("系统初始化 ......","System Initiating......"));
	
	Clr_Bar(0,300,640,35,COL_DEEPRED);
	
	for(i=0;i<SCR_XSIZE/2;i++)
	{
	Clr_Bar(0,300,i*2,35,COL_DEEPGREEN);
    if(i < SCR_XSIZE/4)
	Set_Chr_Color(COL_BLACK,COL_DEEPRED);
	else
	Set_Chr_Color(COL_BLACK,COL_DEEPGREEN);
	Put_Numeral(310, 310,i/4,1);
	Put_Char(310+30,310,'%');     //display 88%
	for(j=0;j<300;j++){;}    //调整进度条快慢
//////////////////////////////////////////////////////STEP1:MODEL_data
	/*if(i == 20)      //
	 {
	 if(ERROR == InitScreenVarible())
     {
      Set_Chr_Color(COL_DEEPRED,COL_BLACK);
	  Put_Str(200-100,380,"ScreenCAL ERROR!,Please Recal Screen");
	 }
	else
     {
      Set_Chr_Color(COL_THINGREEN,COL_BLACK);
      Put_Str(200-100,380,"Screen CAL Data OK!");
	 }
	  for(j=0;j<1990000;j++){;}    //warning delay
	  Clr_Bar(200-100,380,400,30,COL_BLACK);
	}*/
///////////////////////////////////////////////////////////////
	if(i == 50)      //
	 {
	 if(ERROR == InitScreenVarible())
     {
      Set_Chr_Color(COL_DEEPRED,COL_BLACK);
	  Put_Str(200-100,380,"ScreenCAL ERROR!,Please Recal Screen");
	 }
	else
     {
      Set_Chr_Color(COL_THINGREEN,COL_BLACK);
      Put_Str(200-100,380,"Screen CAL Data OK!");
	 }
	  for(j=0;j<1990000;j++){;}    //warning delay
	  Clr_Bar(200-100,380,400,30,COL_BLACK);
	}
////////////////////////////////////////////////////STEP2:CAL_DATA
	if(i == 80)
	{
	
     if( ERROR == Init_Cal_Varible())
   	     {
         Set_Chr_Color(COL_DEEPRED,COL_BLACK);
  	     Put_Str(200-100,380,"WeightCal ERROR!,Use Default Data");
	   	 }
	   else
        {
         Set_Chr_Color(COL_THINGREEN,COL_BLACK);
	     Put_Str(200-100,380,"WeighCal Data OK!");
	    }
	  for(j=0;j<1990000;j++){;}    //warning delay
	  Clr_Bar(200-100,380,400,30,COL_BLACK);
	}		
////////////////////////////////////////////////////STEP3:linecal data
	if(i == 100)
	{
	   if( ERROR == Init_Zer0_Weigh1_Varible())
   	     {
         Set_Chr_Color(COL_DEEPRED,COL_BLACK);
  	     Put_Str(200-100,380,"Zer0 data1 ERROR!");
	   	 }
	    else
        {
         Set_Chr_Color(COL_THINGREEN,COL_BLACK);
	     Put_Str(200-100,380,"Zer0 Data1 OK!");
	    }
	  for(j=0;j<1990000;j++){;}    //warning delay
	  Clr_Bar(200-100,380,400,30,COL_BLACK);
	}

	if(i == 120)
	{
	   if( ERROR == Init_Zer0_Weigh2_Varible())
   	     {
         Set_Chr_Color(COL_DEEPRED,COL_BLACK);
  	     Put_Str(200-100,380,"Zer0 data2 ERROR!");
	   	 }
	    else
        {
         Set_Chr_Color(COL_THINGREEN,COL_BLACK);
	     Put_Str(200-100,380,"Zer0 Data2 OK!");
	    }
	  for(j=0;j<1990000;j++){;}    //warning delay
	  Clr_Bar(200-100,380,400,30,COL_BLACK);
	}

	if(i == 140)
	{
	   if( ERROR == Init_LineCal_Varible())
   	     {
         Set_Chr_Color(COL_DEEPRED,COL_BLACK);
  	     Put_Str(200-100,380,"Line Cali ERROR!");
	   	 }
	    else
        {
         Set_Chr_Color(COL_THINGREEN,COL_BLACK);
	     Put_Str(200-100,380,"Line Cali Data OK!");
	    }
	  for(j=0;j<1990000;j++){;}    //warning delay
	  Clr_Bar(200-100,380,400,30,COL_BLACK);
	}

////////////////////////////////////////////////////STEP3:USER ID data
	if(i == 160)
	{
	   if( ERROR == InitUserIDVarible())
   	     {
          Set_Chr_Color(COL_DEEPRED,COL_BLACK);
  	      Put_Str(200-100,380,"User ID Data ERROR!");
	   	 }
	    else
        {
         Set_Chr_Color(COL_THINGREEN,COL_BLACK);
	     Put_Str(200-100,380,"User ID Data OK!");
	    }
	  for(j=0;j<1990000;j++){;}    //warning delay
	  Clr_Bar(200-100,380,400,30,COL_BLACK);
	}
////////////////////////////////////////////////////STEP3:WIRELESS data
	if(i == 200)
	{
	   if( ERROR == InitWirelessVarible())
   	     {
         Set_Chr_Color(COL_DEEPRED,COL_BLACK);
  	     Put_Str(200-100,380,"Wireless Data ERROR!");
	   	 }
	    else
        {
         Set_Chr_Color(COL_THINGREEN,COL_BLACK);
	     Put_Str(200-100,380,"Wireless Data OK!");
	    }
	  for(j=0;j<1990000;j++){;}    //warning delay
	  Clr_Bar(200-100,380,400,30,COL_BLACK);
	}
///////////////////////////////////////////////////////
	if(INNER_CAL_VERSION &&(i == 240))
	{
	   if( ERROR == InitInnerCalVarible())
   	     {
         Set_Chr_Color(COL_DEEPRED,COL_BLACK);
  	     Put_Str(200-100,380,"Inner_CAL Data ERROR!");
	   	 }
	    else
        {
         Set_Chr_Color(COL_THINGREEN,COL_BLACK);
	     Put_Str(200-100,380,"Inner_CAL Data OK!");
	    }
	  for(j=0;j<1990000;j++){;}    //warning delay
	  Clr_Bar(200-100,380,400,30,COL_BLACK);
	}	
	
////////////////////////////////////////////////////STEP4:检测开机键盘并进入相应的界面
	if((i>50) && (key_times<4))  //监测4个按键
	{
	 key_value = Get_PositionXY();   //键盘扫描
	 if(Key_Status == KEY_DO_START)
     {
      Key_Status = KEY_DO_OVER;           //开始处理
      key_times++;
      /*key_posx = (key_value>>16)&0xffff;
      key_posy = key_value&0xffff;
      if((key_posx>0)&&(key_posx<SCR_XSIZE/2)&&(key_posy>0)&&(key_posy<SCR_YSIZE/2))//area1
        { key_buf[key_times] = 1; key_times++;}
      if((key_posx>SCR_XSIZE/2)&&(key_posx<SCR_XSIZE)&&(key_posy>0)&&(key_posy<SCR_YSIZE/2))//area2
        { key_buf[key_times] = 2; key_times++;}
      //if((key_posx>0)&&(key_posx<SCR_XSIZE/2)&&(key_posy>SCR_YSIZE/2)&&(key_posy<SCR_YSIZE))//area3
      //  { key_buf[key_times] = 3; key_times++;}
      //if((key_posx>SCR_XSIZE/2)&&(key_posx<SCR_XSIZE)&&(key_posy>SCR_YSIZE/2)&&(key_posy<SCR_YSIZE))//area4
      //  { key_buf[key_times] = 4; key_times++;}*/
                                            //处理完毕
     }	
	}
   }  
/////////////////////////////////////////////////STEP6:GLOBAL VARIABLE
    if(key_times > 1)
     return(MACHINE_SERVICE_MODE);     //维修模式
    else 
     return(MACHINE_NORMAL_MODE);        //正常模式
}
///////////////////////////////////////////////////
//14  根据X Y位置构建按键数据 
//////////////////////////////////////////////////
void  Create_ButtonList(uint32 (*p)[2],uint32 num)
	{
	         uint32 i;
	 struct  KeyStruct *pKey;
	 pKey = Key;
	 for(i=0;i<num;i++)
	 	{
		(*pKey).X_Position = (*p)[0];
		(*pKey).Y_Position = (*p)[1];
		(*pKey).Value      = i+1;
		 pKey++;
		 p++;
		}		
	}
	
///////////////////////////////////////////////////////////
//15 放置置零标志
///////////////////////////////////////////////////////////	
void  Set_Zer0_Flag(void)
{
  do_zer0_flag = TRUE;
}
///////////////////////////////////////////////////////////
//16 放置去皮标志
///////////////////////////////////////////////////////////
void  Set_Tare_Flag(void)
{
  do_tare_flag = TRUE;
}
///////////////////////////////////////////////////////////
//17 放置采集百分比基准标志
///////////////////////////////////////////////////////////
void  Set_Sam100_Flag(void)
{
  do_sam100_flag = TRUE;
}
///////////////////////////////////////////////////////////
//18 放置累加标志
///////////////////////////////////////////////////////////
void  Set_Totaladd_Flag(void)
{
  do_totaladd_flag = TRUE;
}
///////////////////////////////////////////////////////////
//19 放置采集目标值数据标志
///////////////////////////////////////////////////////////
void  Set_SamTarget_Flag(void)
{
  do_samtarget_flag = TRUE;
}

///////////////////////////////////////////////////////////
//20 保存到指定位置一条记录
//并更新相关变量
//返回0:  ok
//    1:  not stable
//    2:  ee2prom fault
///////////////////////////////////////////////////////////
void  Save_Rec(uint32 mode)
    {
     un_rec_data rec_data;
     uint32 i;
     uint8 check_data;
     uint8 Send_Data[16];
     RTCTime LocalTime;   
     if((FALSE == positive_flag)||(FALSE == stable_flag))
     {
      Log_Alm(&tips_save_fault,ALM_LEV_TIPS);  //
     }
     else
     { 
       LocalTime = RTCGetTime();
       Send_Data[0] = REC_CHECK_DATA1;
       Send_Data[1] = REC_CHECK_DATA2;
       Send_Data[2] = LocalTime.RTC_Year-2000;
       Send_Data[3] = LocalTime.RTC_Mon;
       Send_Data[4] = LocalTime.RTC_Day;
       Send_Data[5] = LocalTime.RTC_Hour;
       Send_Data[6] = LocalTime.RTC_Min;
       Send_Data[7] = LocalTime.RTC_Sec;
       Send_Data[8] = mode;      //存放模式
       switch(mode)
        {
         case STAT_PCS:
              Send_Data[9]  = (pcs_number>>24)&0xff;
              Send_Data[10] = (pcs_number>>16)&0xff;
              Send_Data[11] =  (pcs_number>>8)&0xff;
              Send_Data[12] =       pcs_number&0xff;
              break;
         case STAT_100:
              rec_data.rec_float = number_100;
              Send_Data[9]  = (rec_data.rec_uint32>>24)&0xff;
              Send_Data[10] = (rec_data.rec_uint32>>16)&0xff;
              Send_Data[11] =  (rec_data.rec_uint32>>8)&0xff;
              Send_Data[12] =       rec_data.rec_uint32&0xff;
              break;
         case STAT_TOTAL:
              rec_data.rec_float = total_weight;
              Send_Data[9]  = (rec_data.rec_uint32>>24)&0xff;
              Send_Data[10] = (rec_data.rec_uint32>>16)&0xff;
              Send_Data[11] =  (rec_data.rec_uint32>>8)&0xff;
              Send_Data[12] =       rec_data.rec_uint32&0xff;
              break; 
         default:
              rec_data.rec_float = net_weight;
              Send_Data[9]  = (rec_data.rec_uint32>>24)&0xff;
              Send_Data[10] = (rec_data.rec_uint32>>16)&0xff;
              Send_Data[11] =  (rec_data.rec_uint32>>8)&0xff;
              Send_Data[12] =       rec_data.rec_uint32&0xff;
              break;
         }
       ////////////////////////增加USERID 字段
       Send_Data[13] = (user_id>>8)&0xff;
       Send_Data[14] =  user_id&0xff;
       
       ////////////////////////计算13个字节的 校验和
       check_data = 0;
       for(i=0;i<15;i++)
       check_data += Send_Data[i];
       
       Send_Data[15] = check_data; 
       //save one rec
       Write_EEPROM(EEP_REC_START_ADDR+rec_curre_index*EEP_BYTE_PER_REC,Send_Data, 16); 
	   for(i=0;i<40000;i++)	{;}	         //delay
	  
	   for(i=0;i<16;i++) Send_Data[i] = 0;	 //clear buf
       //readback
	   Read_EEPROM(EEP_REC_START_ADDR+rec_curre_index*EEP_BYTE_PER_REC,Send_Data, 16);	 
	   for(i=0;i<1000;i++)	{;}	         //delay
       check_data = 0;
       for(i=0;i<15;i++)
       check_data += Send_Data[i];
       if(Send_Data[15] == check_data)
       {
        rec_total_num++;               //更新总记录数
        
        if(rec_real_num < REC_MAX_NUM) //更新有效条数
        rec_real_num++;
                                       //更新总页数 
        rec_total_pages =  rec_real_num/RECNUM_PER_PAGE + 1;
                                      
        rec_curre_index++;             //更新索引
        if(REC_MAX_NUM == rec_curre_index)
        rec_curre_index = 0;
                                       
        Send_Data[0] = (rec_total_num>>24)&0xff;
        Send_Data[1] = (rec_total_num>>16)&0xff;
        Send_Data[2] =  (rec_total_num>>8)&0xff;
        Send_Data[3] =       rec_total_num&0xff;
        Send_Data[4] = Send_Data[0];
        Send_Data[5] = Send_Data[1];
        Send_Data[6] = Send_Data[2];
        Send_Data[7] = Send_Data[3];
        Write_EEPROM(EEP_REC_TOTAL_ADDR,Send_Data, 8); 
        for(i=0;i<20000;i++)	{;}	
        Log_Alm(&tips_save_ok,ALM_LEV_TIPS);  //正常保存
       }
       else
       {
        Log_Alm(&tips_EE_bad,ALM_LEV_TIPS);
       }
    }
  }  

 ////////////////////////////////////
//对整克数进行修正 修正原则为
// < 2    0
// < 3    2
// < 5    3
/////////////////////////////////
static  float display_to_int(float net_tmp)
 {
    uint32 tmp,tmp2;
	if((WAN_DIVISION==dot_position)&&(1==adjust_0000_flag))
	{
      tmp  = (uint32)(net_tmp * 10000);
      tmp  = tmp % 10000;
      if((tmp<2)||(tmp>9998))
       {
        return(net_tmp);
       } 
      else if(tmp<4)
       {
        return(net_tmp-0.0002);
       }
      else if(tmp<6)
       {
        return(net_tmp-0.0003);
       }
      else if(tmp>9996)
       {
        return(net_tmp+0.0002);
       }
      else if(tmp>9994)
       {
        return(net_tmp+0.0003);
       }
      else 
        return(net_tmp);
     }
	/*
  else if(WAN10_DIVISION == dot_position) 
     {
      tmp  = (Uint32)(net_tmp * 100000);
      tmp  = tmp % 10000;
      if((tmp<2)||(tmp>9998))
       {
        return(net_tmp);
       } 
      else if(tmp<4)
       {
        return(net_tmp-0.00002);
       }
      else if(tmp<6)
       {
        return(net_tmp-0.00003);
       }
      else if(tmp>9996)
       {
        return(net_tmp+0.00002);
       }
      else if(tmp>9994)
       {
        return(net_tmp+0.00003);
       }
      else 
        return(net_tmp);
     }*/
  else
        return(net_tmp);   
 }
 
