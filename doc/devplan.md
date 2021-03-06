##流程图
        *************************
        **     Initialize      **
        **                     **
        *************************
                  |
                  |
                  V
            
        *************************
        **   Inquiry cmd       **
        **                     **
        *************************
                  |
                  |
                  V

        *************************
        **   Acquire Image     **
        **                     **
        *************************
            |                   |
            |                   |
            V                   V
    ******************  *****************
    **x264 Compress **  **  Tracking   **
    **              **  **             **
    ******************  *****************
            |                   |
            |                   |
            V                   V

        ***************************
        **  Prepare Data Package **
        **                       **
        ***************************
                  |
                  |
                  V

        *************************
        **   Gimbal control    **
        **                     **
        *************************
                  |
                  |
                  V

        *************************
        **   Send result       **
        **                     **
        *************************

##指令格式

指令 [keyword,para1,para2]（全s8）

停机 [0,-,-] 

传图像不跟踪 [1,-,-]

开始跟踪    [2,-,-]

跟踪参数配置  [3,颜色编号，结果类型]

摄像头配置    [4，帧率，分辨率]


## 数据包格式

[总长度，图像长度，结果个数 | x264图像 | 结果序列]


  ,和|只是分隔符，不包含在包内

## 数据定义

颜色(存Hue值)：[红 黄 绿 蓝]

结果类型：[最终结果，连通域分析结果] （使用itr_vision::Block存储结果）

分辨率： [320-240,640-480,400-400]

##任务安排

###第一阶段：

#####LM：

使用Qt开发简单控制台，包含功能：

1. 接收图像和数据

1. 放大图像后显示视频

1. 可以设置上述所有参数

1. 可以给跟踪模块发送指令

##### GQP：

1. 获取图像数据

1. 降采样后发送图像

1. 接收控制指令并修改参数

1. 开发颜色跟踪模块（参考AR的代码）

##### GH ZYC：

1. 封装x264压缩、解压缩库

###第二阶段

##### LM

1. 把缩放换成x264解压

1. 在图像上绘制收到的跟踪结果

##### GQP

1. 把降采样换成x264压缩

1. 把跟踪结果加入发送数据包中

1. 控制云台