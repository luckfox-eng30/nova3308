#ifndef ST_ASTERIA_API_H
#define ST_ASTERIA_API_H
#include "st_asteria_common.h"

#define ST_DETECT_FACE              0x00000001  ///< 检测人脸
#define ST_DETECT_FACE_DISTANCE     0x00000002  ///< 检测人脸距离. 需要开启人脸检测
#define ST_DETECT_FACE_ATTRIBUTE    0x00000004  ///< 检测人脸属性. 需要开启人脸检测
#define ST_DETECT_FACE_FEATURE      0x00000008  ///< 检测人脸特征值. 需要开启人脸检测
#define ST_DETECT_HAND              0x00000100  ///< 检测手势
#define ST_DETECT_BODY              0x00001000  ///< 检测肢体

typedef enum {
    ST_MODEL_FACE_RECT,         ///< 人脸检测模型
    ST_MODEL_FACE_POINT,        ///< 人脸关键点模型
    ST_MODEL_FACE_POSE,         ///< 人脸姿态模型
    ST_MODEL_FACE_ATTRIBUTE,    ///< 人脸属性模型
    ST_MODEL_FACE_FEATURE,      ///< 人脸特征值模型
    ST_MODEL_HAND_RECT,         ///< 手势框模型
    ST_MODEL_HAND_POINT,        ///< 手势关键点模型
    ST_MODEL_BODY_RECT,         ///< 肢体检测模型
    ST_MODEL_BODY_POINT,        ///< 肢体关键点模型
    ST_MODEL_UPPER_BODY_RECT,   ///< 上身肢体检测模型
    ST_MODEL_UPPER_BODY_POINT,  ///< 上身肢体关键点模型
    ST_MODEL_SUPER_RESOLUTION   ///< 超分辨率模型
} STModelType;

typedef enum {
    ST_PARAMETER_FACE_DETECT_INTERVAL,      ///< 人脸检测间隔: 设置每多少帧重新检测一次人脸. 有效值 [1, -), 默认值 20. 设置越短新人脸检出越快, CPU/NPU使用率越高
    ST_PARAMETER_FACE_ATTRIBUTE_THRESHOLD,  ///< 检测人脸属性的人脸质量分阈值: 有效值 [0.0, 1.0], 默认值 0.6
    ST_PARAMETER_FACE_ATTRIBUTE_STEP,       ///< 检测人脸属性的人脸质量分步进分值: 有效值 [0.0, 1.0], 默认值 0.1
    ST_PARAMETER_FACE_FEATURE_THRESHOLD,    ///< 提取人脸特征值的人脸质量分阈值: 有效值 [0.0, 1.0], 默认值 0.6
    ST_PARAMETER_FACE_FEATURE_STEP,         ///< 提取人脸特征值的人脸质量分阈值: 有效值 [0.0, 1.0], 默认值 0.1
    ST_PARAMETER_HAND_MULTIPLE,             ///< 是否检测多只手: 有效值 {0(只检测一只手), 1(检测多只手)}, 默认值0
    ST_PARAMETER_BODY_DETECT_INTERVAL,      ///< 肢体检测间隔: 设置每多少帧重新检测一次肢体. 有效值 [1, -), 默认值 20. 设置越短新肢体检出越快, CPU/NPU使用率越高
    ST_PARAMETER_CAMERA_VERTICAL_FOV        ///< 设置摄像头垂直FOV: 默认值 40.0. 检测人脸距离时需要设置
} STParameterType;

ST_API const char* STAsteriaVersion(int *major, int *minor, int *patch);
ST_API STResult STAsteriaSetI2CBusNumber(int number);
ST_API STResult STAsteriaGetActivationCode(const char *license, const char **activationCode);
ST_API STResult STAsteriaCheckActivationCode(const char *license, const char *activationCode);
ST_API STResult STAsteriaAddLicense(const char *license);
ST_API STResult STAsteriaCreate(STHandle *handle);
ST_API STResult STAsteriaAddModel(STHandle handle, STModelType modelType, const char *modelPath);
ST_API STResult STAsteriaInitialize(STHandle handle);
ST_API STResult STAsteriaSetDetection(STHandle handle, int detection);
ST_API STResult STAsteriaSetParameter(STHandle handle, STParameterType type, float value);
ST_API STResult STAsteriaSetCameraFOV(STHandle handle, float value);
ST_API STResult STAsteriaDetect(STHandle handle, const STImage *image, const STDetectResult **detectResult);
ST_API STResult STAsteriaCompare(const unsigned char *feature0, const unsigned char *feature1, int featureLen, float *score);
ST_API STResult STAsteriaSuperResolution(STHandle handle, const STImage *src, STImage *dst);
ST_API STDetectResult* STAsteriaCopyDetectResult(const STDetectResult *detectResult);
ST_API void STAsteriaReleaseDetectResult(STDetectResult *detectResult);
ST_API const char* STAsteriaGetDescription(STResult result);
ST_API void STAsteriaDestroy(STHandle handle);

#endif // ST_ASTERIA_API_H
