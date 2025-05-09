#ifndef ST_ASTERIA_COMMON_H
#define ST_ASTERIA_COMMON_H

#ifdef _MSC_VER
#   ifdef SDK_EXPORTS
#       define ST_API_ __declspec(dllexport)
#   else
#       define ST_API_ __declspec(dllimport)
#   endif
#else
#   ifdef SDK_EXPORTS
#       define ST_API_ __attribute__((visibility ("default")))
#   else
#       define ST_API_
#   endif
#endif

#ifdef __cplusplus
#   define ST_API extern "C" ST_API_
#else
#   define ST_API ST_API_
#endif

#define ST_OK                                0  ///< 运行正常
#define ST_E_INVALID_ARG                    -1  ///< 无效参数
#define ST_E_HANDLE                         -2  ///< 句柄错误
#define ST_E_OUT_OF_MEMORY                  -3  ///< 内存不足
#define ST_E_INTERNAL_ERROR                 -4  ///< 内部错误
#define ST_E_INVALID_PIXEL_FORMAT           -6  ///< 图像格式不支持
#define ST_E_FILE_NOT_EXIST                 -7  ///< 文件不存在
#define ST_E_INVALID_FILE_FORMAT            -8  ///< 模型格式错误
#define ST_E_INVALID_UDID                   -12 ///< 无效的UDID
#define ST_E_INVALID_LICENSE                -13 ///< 授权文件无效
#define ST_E_INVALID_APP_ID                 -14 ///< 包名错误
#define ST_E_LICENSE_EXPIRED                -15 ///< 授权文件过期
#define ST_E_UDID_MISMATCH                  -16 ///< UDID不匹配
#define ST_E_LICENSE_IS_NOT_ACTIVABLE       -20 ///< 授权文件不支持生成激活码
#define ST_E_ACTIVATION_CODE_INVALID        -22 ///< 离线激活码无效
#define ST_E_NO_CAPABILITY                  -23 ///< 授权文件不包含该功能
#define ST_E_PLATFORM_NOT_SUPPORTED         -24 ///< 授权文件不支持该平台
#define ST_E_ONLINE_ACTIVATION_FAIL         -28 ///< 在线激活失败
#define ST_E_ONLINE_ACTIVATION_CODE_INVALID -29 ///< 在线激活码无效
#define ST_E_ONLINE_ACTIVATION_CONNECT_FAIL -30 ///< 在线激活连接失败
#define ST_E_NOT_CONNECTED_TO_THE_NETWORK   -196608 ///< 设备没有连接网络
#define ST_E_UNSUPPORTED                    -1000   ///< 不支持的函数调用方式

#define ST_HAND_ACTION_INVALID          0x00000000  ///< 无效手势
#define ST_HAND_ACTION_THUMBS_UP        0x00000001  ///< 竖大拇指
#define ST_HAND_ACTION_INDEX_FINGER     0x00000002  ///< 食指/一
#define ST_HAND_ACTION_LITTLE_FINGER    0x00000004  ///< 小拇指
#define ST_HAND_ACTION_V                0x00000008  ///< V/Yeah/二
#define ST_HAND_ACTION_THREE            0x00000010  ///< 三
#define ST_HAND_ACTION_FOUR             0x00000020  ///< 四
#define ST_HAND_ACTION_SIX              0x00000040  ///< 六
#define ST_HAND_ACTION_SHOOT            0x00000080  ///< 手枪/八
#define ST_HAND_ACTION_FINGER_HEART     0x00000100  ///< 单手比心
#define ST_HAND_ACTION_OK               0x00000200  ///< OK
#define ST_HAND_ACTION_ROCK_ON          0x00000400  ///< 摇滚
#define ST_HAND_ACTION_I_LOVE_YOU       0x00000800  ///< 我爱你
#define ST_HAND_ACTION_PALM             0x00001000  ///< 手掌/五
#define ST_HAND_ACTION_HOLD_UP          0x00002000  ///< 托手
#define ST_HAND_ACTION_FIST             0x00004000  ///< 拳头
#define ST_HAND_ACTION_HEART            0x00008000  ///< 双手比心
#define ST_HAND_ACTION_PRAY             0x00010000  ///< 祈祷/双手合十
#define ST_HAND_ACTION_CONGRATULATE     0x00020000  ///< 恭贺/抱拳

#define ST_ATTRIBUTE_AGE                "age"
#define ST_ATTRIBUTE_GENDER             "gender"
#define ST_ATTRIBUTE_GENDER_FEMALE      "female"
#define ST_ATTRIBUTE_GENDER_MALE        "male"
#define ST_ATTRIBUTE_RACE               "race"
#define ST_ATTRIBUTE_RACE_YELLOW        "yellow"
#define ST_ATTRIBUTE_RACE_BLACK         "black"
#define ST_ATTRIBUTE_RACE_WHITE         "white"
#define ST_ATTRIBUTE_RACE_BROWN         "brown"
#define ST_ATTRIBUTE_BEARD              "beard"
#define ST_ATTRIBUTE_BEARD_NONE         "none"
#define ST_ATTRIBUTE_BEARD_HAVE         "beard"
#define ST_ATTRIBUTE_FACE_VALUE         "face_value"
#define ST_ATTRIBUTE_GLASSES            "glasses"
#define ST_ATTRIBUTE_GLASSES_NONE       "none"
#define ST_ATTRIBUTE_GLASSES_GLASSES    "glasses"
#define ST_ATTRIBUTE_GLASSES_SUNGLASSES "sunglasses"
#define ST_ATTRIBUTE_MASK               "mask"
#define ST_ATTRIBUTE_MASK_NONE          "none"
#define ST_ATTRIBUTE_MASK_HAVE          "mask"
#define ST_ATTRIBUTE_EYE                "eye"
#define ST_ATTRIBUTE_EYE_OPEN           "open"
#define ST_ATTRIBUTE_EYE_CLOSE          "close"
#define ST_ATTRIBUTE_MOUTH              "mouth"
#define ST_ATTRIBUTE_MOUTH_OPEN         "open"
#define ST_ATTRIBUTE_MOUTH_CLOSE        "close"
#define ST_ATTRIBUTE_SMILE              "smile"
#define ST_ATTRIBUTE_EMOTION            "emotion"
#define ST_ATTRIBUTE_EMOTION_ANGRY      "angry"
#define ST_ATTRIBUTE_EMOTION_CALM       "calm"
#define ST_ATTRIBUTE_EMOTION_DISGUST    "disgust"
#define ST_ATTRIBUTE_EMOTION_HAPPY      "happy"
#define ST_ATTRIBUTE_EMOTION_SAD        "sad"
#define ST_ATTRIBUTE_EMOTION_SCARED     "scared"
#define ST_ATTRIBUTE_EMOTION_SURPRISE   "surprise"
#define ST_ATTRIBUTE_EMOTION_UNCLEAR    "unclear"

typedef int     STResult;
typedef void*   STHandle;

typedef enum {
    ST_PIXEL_FORMAT_GRAY,   ///< GRAY 1        8bpp 单通道灰度
    ST_PIXEL_FORMAT_I420,   ///< YUV  4:2:0   12bpp 3通道, 亮度通道, U分量通道, V分量通道. 所有通道都是连续的
    ST_PIXEL_FORMAT_NV12,   ///< YUV  4:2:0   12bpp 2通道, 亮度通道和UV分量交错通道
    ST_PIXEL_FORMAT_NV21,   ///< YUV  4:2:0   12bpp 2通道, 亮度通道和VU分量交错通道
    ST_PIXEL_FORMAT_BGRA,   ///< BGRA 8:8:8:8 32bpp 4通道
    ST_PIXEL_FORMAT_BGR,    ///< BGR  8:8:8   24bpp 3通道
    ST_PIXEL_FORMAT_RGBA,   ///< RGBA 8:8:8:8 32bpp 4通道
    ST_PIXEL_FORMAT_RGB     ///< RGB  8:8:8   24bpp 3通道
} STPixelFormat;

typedef struct STPoint {
    float x;
    float y;
} STPoint;

typedef struct STRect {
    int left;
    int top;
    int right;
    int bottom;
} STRect;

typedef struct STImage {
    unsigned char *data;    ///< 图像数据
    STPixelFormat format;   ///< 像素格式
    int width;              ///< 图像宽度 (单位: 像素)
    int height;             ///< 图像高度 (单位: 像素)
    int reserved0;          ///< 保留字段
    long reserved1;         ///< 保留字段
    long reserved2;         ///< 保留字段
} STImage;

typedef struct STAttribute {
    char *category;         ///< 属性类别
    char *label;            ///< 属性标签
    float score;            ///< 属性置信度
} STAttribute;

typedef struct STFace {
    int id;                 ///< 人脸序号 (从1开始)
    STRect rect;            ///< 人脸框
    STPoint *points;        ///< 人脸关键点
    int pointCount;         ///< 人脸关键点个数
    float *visibilities;    ///< 人脸关键点可见度
    float yaw;              ///< 水平转角
    float pitch;            ///< 俯仰角
    float roll;             ///< 旋转角
    float quality;          ///< 人脸质量
    float distance;         ///< 人脸距离
    STAttribute *attributes;///< 人脸属性
    int attributeCount;     ///< 人脸属性个数
    unsigned char* feature; ///< 人脸特征值
    int featureLen;         ///< 人脸特征值长度
} STFace;

typedef enum {
    ST_HAND_EVENT_NONE,     ///< 无动态手势
    ST_HAND_EVENT_UP,       ///< 向上手势
    ST_HAND_EVENT_DOWN,     ///< 向下手势
    ST_HAND_EVENT_LEFT,     ///< 向左手势
    ST_HAND_EVENT_RIGHT,    ///< 向右手势
    ST_HAND_EVENT_CIRCLE,   ///< 转圈手势
    ST_HAND_EVENT_HOLD,     ///< 手势保持不动
    ST_HAND_EVENT_SILENT    ///< 静音(嘘)手势
} STHandEventType;

typedef struct STHandEvent {
    STHandEventType type;   ///< 手势事件类型
    float velocity;         ///< 手势事件速度
    float value;            ///< 手势事件值
} STHandEvent;

typedef struct STHand {
    int id;                 ///< 手势序号
    int faceId;             ///< 关联的人脸id. 没有关联的人脸时值为0 (无效人脸id) (暂不支持)
    STRect rect;            ///< 手势框
    STPoint *points;        ///< 手势关键点
    int pointCount;         ///< 手势关键点个数
    int action;             ///< 手势类型
    STHandEvent event;      ///< 动态手势 (暂不支持)
} STHand;

typedef struct STBody {
    int id;                 ///< 肢体序号 (从1开始)
    int faceId;             ///< 关联的人脸id. 没有关联的人脸时值为0 (无效人脸id) (暂不支持)
    STRect rect;            ///< 肢体框
    STPoint *points;        ///< 肢体关键点
    float *visibilities;    ///< 肢体关键点可见度
    int pointCount;         ///< 肢体关键点个数
    float quality;          ///< 肢体质量
} STBody;

typedef struct STDetectResult {
    STFace *faces;          ///< 人脸
    int faceCount;          ///< 人脸个数
    STHand *hands;          ///< 手势
    int handCount;          ///< 手势个数
    STBody *bodys;          ///< 肢体
    int bodyCount;          ///< 肢体个数
} STDetectResult;

#endif // ST_ASTERIA_COMMON_H
