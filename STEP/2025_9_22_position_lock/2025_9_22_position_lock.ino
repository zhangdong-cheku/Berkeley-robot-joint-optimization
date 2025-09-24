// 引入必要的库：Wire用于I2C通信，Servo用于控制电调（本质是产生PWM信号）
#include <Wire.h>   // I2C通信库，用于与AS5600角度传感器通信
#include <Servo.h>  // 舵机/电调控制库，用于控制ESC电调

// ================== 配置参数 ==================
// AS5600磁编码器的I2C地址
#define AS5600_ADDR      0x36
// AS5600角度值寄存器地址（高字节）
#define ANGLE_REG_H      0x0E
// 配置寄存器高位地址（未直接使用）
#define CONFIG_REG_H     0x1B
// 电调信号线连接的引脚
#define ESC_PIN          9

// 电调控制信号脉宽范围（单位：微秒），不同型号电调脉宽和控制方式会有差异
#define PULSE_STOP       2200 // 电机停止的脉宽，上电时首先给电调发送停止脉宽信号，解锁电调
#define PULSE_NEUTRAL    1470 // 中立位（停止）
//电调的脉宽需要根据电调型号确定
#define PULSE_FWD_MIN    1510 // 正转起始脉宽
#define PULSE_FWD_MAX    1540 // 正转转速最大脉宽
#define PULSE_REV_MIN    1430 // 反转起始脉宽
#define PULSE_REV_MAX    1400 // 反转转速最大脉宽

// 系统安全运行的角度限制（度）
#define ANGLE_MIN        170.0f // 最小允许角度
#define ANGLE_MAX        350.0f // 最大允许角度
#define ANGLE_POWERON    260.0f // 上电后的初始目标角度
#define ANGLE_TOL        2.0f   // 到达目标的容差范围

// 采样与调度参数
#define SAMPLE_RATE      3200UL                   // 角度采样频率 (Hz)
#define SAMPLE_INTERVAL  (1000000UL / SAMPLE_RATE) // 采样间隔 (微秒)
#define FEEDBACK_INTERVAL 1000UL                  // 状态反馈间隔 (毫秒)
#define PID_INTERVAL     1000UL                   // PID计算间隔 (微秒)
#define CONFIRM_COUNT    5                        //  目标到达的确认次数（连续5次满足误差范围则判定到达）

// ================== 数据结构 ==================
// PID参数结构体
struct PIDParam {
  float kp, ki, kd; // PID控制参数，比例、积分、微分系数
};

// 传感器数据结构体
struct SensorData {
  float angle; // 滤波后的当前角度
  float rpm;   // 转速（由角度变化计算）
};

// 控制器状态结构体
struct ControlState {
  float target;        // 目标角度
  float error;         // 当前误差
  float integral;      // 误差积分项
  float derivative;    // 误差微分项
  float output;        // PID输出值（-100% 到 +100%）
  bool running;        // 控制器是否正在运行（电机是否激活）
  bool hold;           // 是否为“保持”模式（与普通定位模式参数不同）
  uint8_t arrivedCount;// 连续到达目标的次数，用于防抖
};

// 统计信息结构体
struct Stats {
  uint32_t loopCount, sampleCount;     // 总循环次数，总采样次数
  uint32_t lastLoopCount, lastSampleCount; // 用于计算每秒次数的上一次快照
};

// ================== 全局变量 ==================
Servo esc; // 创建Servo对象来控制电调
// PID参数：普通定位模式,需要自己调整
PIDParam pid = {0.7f, 0.0f, 0.047f};
// PID参数：保持模式（通常需要更柔和，防止震荡）
PIDParam pidHold = {0.01f, 0.0f, 0.0f};
// 传感器数据
SensorData sensor = {0,0};
// 控制器状态
ControlState ctrl = {-1,0,0,0,0,false,false,0};
// 统计信息
Stats stat = {0,0,0,0};

// 时间戳变量（用于任务调度）
uint32_t lastSampleMicros = 0; // 上一次采样的微秒时间
uint32_t lastPIDMicros    = 0; // 上一次PID计算的微秒时间
uint32_t lastFeedbackMs   = 0; // 上一次状态反馈的毫秒时间
uint32_t lastStatsMs      = 0; // 上一次统计信息输出的毫秒时间
float    lastAngle        = 0; // 上一次的角度值，用于计算转速

// 滑动中值滤波相关变量
#define FILTER_SIZE 7 // 滤波器窗口大小
float filterBuf[FILTER_SIZE] = {0}; // 滤波器缓冲区
uint8_t filterIdx = 0;              // 当前缓冲区索引
bool filterFilled = false;          // 缓冲区是否已填满一次

bool stopFlag = false;   // 全局停止标志

// ================== 函数声明 ==================
float readAngle();                  // 从AS5600读取原始角度
float medianFilter(float newVal);   // 中值滤波函数
float calcAngleDiff(float cur, float tgt); // 计算两个角度间的最短差值（考虑360度循环）
void pidControl(float dt);          // PID控制计算函数
void setTarget(float angle);        // 设置新的目标角度
void stopMotor();                   // 停止电机
void sendStatus();                  // 通过串口发送当前状态
void reportStats();                 // 报告性能统计
void parseCommand();                // 解析串口命令

// ================== 初始化 ==================
void setup() {
  Serial.begin(115200); // 初始化串口通信（波特率115200）
  Wire.begin();         // 初始化I2C总线
  Wire.setClock(400000); // 设置I2C时钟为400kHz，提高读取速度

  //电调初始化
  esc.attach(ESC_PIN);  // 将电调信号线连接到指定引脚
  esc.writeMicroseconds(PULSE_STOP); // 发送停止信号（常用于电调校准）
  delay(1000);          // 等待电调初始化

  Serial.println("=== AS5600 双向电调闭环系统 ===");
  Serial.println("指令: STOP | ANGLE xxx| PID kp ki kd | STATUS | STATS");
  //设置初始目标
  //setTarget(ANGLE_POWERON); // 系统启动后，默认转动到一个安全角度
}

// ================== 主循环 ==================
void loop() {
  stat.loopCount++; // 循环计数器递增
  uint32_t nowMicros = micros(); // 获取当前时间（微秒）
  uint32_t nowMillis = millis(); // 获取当前时间（毫秒）

  // --- 任务1：采样任务 (固定频率) ---
  if (nowMicros - lastSampleMicros >= SAMPLE_INTERVAL) {
    lastSampleMicros = nowMicros; // 更新最后一次采样时间
    stat.sampleCount++; // 采样计数器递增

    // 1. 读取原始角度值
    float raw = readAngle();
    // 2. 进行中值滤波，得到稳定的角度值
    sensor.angle = medianFilter(raw);

    // 3. 计算转速(RPM,基于角度变化率)
    //    计算距离上次采样的时间差（秒）
    float dt = (nowMicros - lastSampleMicros) / 1e6f;// 采样间隔（秒）
    if (dt > 0.0005f) { // 确保时间差有效，避免除零错误或过大误差
      float diff = sensor.angle - lastAngle; // 计算角度差
      // 处理角度跨越360°的边界情况，保证diff是两点间最短路径的差值,（例如：350度到10度的差应为20度，而非-340度）
      if (diff > 180) diff -= 360;
      else if (diff < -180) diff += 360;
      // 角度差(度)/时间(秒) * (60秒/分钟) / (360度/圈) = 转速(RPM)
      sensor.rpm = (diff / dt) * (60.0f / 360.0f);
      lastAngle = sensor.angle; // 更新“上一次的角度”
    }
  }

  // --- 任务2：PID控制任务 (固定频率) ---
  if (nowMicros - lastPIDMicros >= PID_INTERVAL) {
    float dt = (nowMicros - lastPIDMicros) / 1e6f; // 计算距离上次PID计算的时间差（秒）
    lastPIDMicros = nowMicros; // 更新最后一次PID计算时间
    if (ctrl.running) { // 只有控制器处于运行状态时才执行PID
      pidControl(dt);
    }
  }

  // --- 任务3：串口命令处理任务 (异步事件驱动,有数据时执行) ---
  if (Serial.available()) {
    parseCommand();
  }

  // --- 任务4：状态反馈任务 (固定间隔) ---
  if (nowMillis - lastFeedbackMs >= FEEDBACK_INTERVAL) {
    lastFeedbackMs = nowMillis;// 更新反馈时间戳
    sendStatus(); // 向串口发送当前角度、目标、状态等信息
  }

  // --- 任务5：性能统计任务 (固定间隔) ---
  if (nowMillis - lastStatsMs >= 1000) {
    lastStatsMs = nowMillis; // 更新统计时间戳
    reportStats(); // 向串口报告主循环和采样的频率
  }
}
// ================== 功能函数实现 ==================

// 从AS5600读取原始角度值
float readAngle() {
  Wire.beginTransmission(AS5600_ADDR); // 开始I2C传输，指定设备地址
  Wire.write(ANGLE_REG_H);             // 指定要读取的寄存器地址
  Wire.endTransmission(false);         // 发送重启信号（保持I2C连接有效）
  Wire.requestFrom(AS5600_ADDR, 2);    // 请求从设备读取2个字节
  if (Wire.available() < 2) return lastAngle; // 读取失败则，返回上次的角度值
  // 组合两个字节（高8位 + 低8位）成一个16位的原始值
  uint16_t raw = (Wire.read() << 8) | Wire.read();
  // 将原始值(0-4095)转换为角度值(0-359.9)
  return raw * 360.0f / 4096.0f;
}

// 中值滤波器：输入新值，返回滤波后的值
float medianFilter(float v) {
  // 将新值放入环形缓冲区
  filterBuf[filterIdx] = v;
  filterIdx = (filterIdx + 1) % FILTER_SIZE;
  // 检查缓冲区是否已被数据填满过一次
  if (!filterFilled && filterIdx == 0) filterFilled = true;

  // 创建临时数组进行排序
  float temp[FILTER_SIZE];
  // 确定当前有效数据的大小
  uint8_t size = filterFilled ? FILTER_SIZE : filterIdx;
  // 将有效数据拷贝到临时数组
  memcpy(temp, filterBuf, size * sizeof(float));

  // 使用插入排序对临时数组进行排序
  for (uint8_t i=1; i<size; i++){
    float key = temp[i];
    int j = i-1;
    while (j>=0 && temp[j] > key) {
      temp[j+1] = temp[j];
      j--;
    }
    temp[j+1] = key;
  }
  // 返回中值（排序后数组的中间值）
  return temp[size/2];
}

// // 计算当前角度到目标角度的最短路径差值（考虑360度循环）
// float calcAngleDiff(float cur, float tgt) {
//   float d = tgt - cur; // 直接差值
//   // 如果差值大于180度，说明反向走更短，例如：目标350，当前10 → 差应为-20（而非340）
//   if (d > 180) d -= 360;
//   // 如果差值小于-180度，说明正向走更短，例如：目标10，当前350 → 差应为20（而非-340）
//   else if (d < -180) d += 360;
//   return d;
// }
float constrainAngle(float angle) {
  // 将输入角度约束在安全范围
  if (angle < ANGLE_MIN) angle = ANGLE_MIN;
  if (angle > ANGLE_MAX) angle = ANGLE_MAX;
  return angle;
}
float calcAngleDiff(float current, float target) {
  target = constrainAngle(target);
  current = constrainAngle(current);
  float diff = target - current;
  if (diff > 180.0) diff -= 360.0;
  else if (diff < -180.0) diff += 360.0;
  return diff;
}

// PID控制计算函数
void pidControl(float dt) {
  // 1. 计算与目标的角度差（考虑360度循环）
  float diff = calcAngleDiff(sensor.angle, ctrl.target);
  float absDiff = fabs(diff); // 误差的绝对值

  // 2.  若未进入保持模式且误差在容忍范围内，计数确认到达目标
  if (!ctrl.hold && absDiff <= ANGLE_TOL) {
    // 如果误差在容差范围内，计数器加一
    if (++ctrl.arrivedCount >= CONFIRM_COUNT) {
      stopMotor(); // 连续多次到达，则认为真正到达，停止电机
      return;      // 退出本次PID计算
    }
  } else {
    // 如果误差变大或不在容差内，重置计数器
    ctrl.arrivedCount = 0;
  }

  // 3. 标准PID计算
  ctrl.error = diff; // 更新误差
  ctrl.integral += diff * dt; // 积分项累加
  ctrl.integral = constrain(ctrl.integral, -20.0f, 20.0f); // 积分限幅，防止积分饱和
  ctrl.derivative = (diff - ctrl.error) / dt; // 微分项

  // 4. 根据模式（定位/保持）选择PID参数并计算输出
  PIDParam &p = ctrl.hold ? pidHold : pid; // 选择参数集
  ctrl.output = p.kp * ctrl.error + p.ki * ctrl.integral + p.kd * ctrl.derivative;
  ctrl.output = constrain(ctrl.output, -100.0f, 100.0f); // 输出限幅到±100%

  // 5. 将PID输出（百分比）映射为电调能理解的脉冲宽度（微秒）
  int pulse;
  if (ctrl.output > 0) // 正转（输出为正）
    //pulse = map(ctrl.output, 0, 100, PULSE_FWD_MIN, PULSE_FWD_MAX);PULSE_FWD_MIN
    pulse = map(ctrl.output, 0, 100, PULSE_FWD_MIN, PULSE_FWD_MIN + 10);
  else if (ctrl.output < 0) // 反转（输出为负）
    //pulse = map(ctrl.output, 0, -100, PULSE_REV_MIN, PULSE_REV_MAX);
    pulse = map(ctrl.output, 0, -100, PULSE_REV_MIN, PULSE_REV_MIN - 10);
  else // 停止,输出为0（中立位置）
    pulse = PULSE_NEUTRAL;
  // 6. 将脉冲信号发送给电调
  esc.writeMicroseconds(pulse);
}

// 设置新的目标角度
void setTarget(float angle) {
  // 约束角度在安全范围内
  angle = constrain(angle, ANGLE_MIN, ANGLE_MAX);
  // 更新控制器状态
  ctrl.target = angle;
  ctrl.running = true;
  ctrl.hold = false; // 退出保持模式
  // 重置PID状态，防止历史数据影响新目标
  ctrl.integral = 0;
  ctrl.error = 0;
  ctrl.derivative = 0;
  ctrl.arrivedCount = 0;
  // 打印目标角度
  Serial.print("目标角度: ");
  Serial.println(angle, 1);
}

// 停止电机并重置控制状态
void stopMotor() {
  // 发送停止脉冲给电调
  esc.writeMicroseconds(PULSE_STOP);
  // 重置控制器状态
  ctrl.running = false;       // 停止控制
  ctrl.target = -1;           // 清除目标角度
  ctrl.integral = 0;          // 重置PID项
  ctrl.error = 0;
  ctrl.derivative = 0;
  ctrl.arrivedCount = 0;      // 重置到达计数
  ctrl.output = 0;            //重置电压输出值
  Serial.println("电机已停止");// 打印电机状态
}

// 通过串口发送状态信息
void sendStatus() {
  Serial.print("ANGLE:");
  Serial.print(sensor.angle, 2);// 当前角度
  Serial.print(",TARGET:");// 目标角度（-1表示无）
  // 如果目标无效（已停止）则显示-1，否则显示目标值
  Serial.print(ctrl.target < 0 ? -1 : ctrl.target, 2);
  Serial.print(",STATUS:");
  // 输出状态描述
  if (!ctrl.running)
    Serial.print("STOPPED");
  else if (fabs(calcAngleDiff(sensor.angle, ctrl.target)) <= ANGLE_TOL)
    Serial.print("TARGET"); // 已到达目标
  else
    Serial.print("MOVING"); // 正在移动
  Serial.print(",PID:");
  Serial.println(ctrl.output, 1); // PID输出值
}

// 报告性能统计（主循环频率和采样频率）
void reportStats() {
  uint32_t loops = stat.loopCount - stat.lastLoopCount;       // 过去1秒的循环次数
  uint32_t samples = stat.sampleCount - stat.lastSampleCount; // 过去1秒的采样次数
  stat.lastLoopCount = stat.loopCount;                        // 更新统计基准
  stat.lastSampleCount = stat.sampleCount;
  Serial.print("[Stats] Loop/s:");                            // 每秒循环次数
  Serial.print(loops);
  Serial.print(" Sample/s:");                                 // 每秒采样次数
  Serial.println(samples);
}

// 解析并执行从串口接收到的命令
void parseCommand() {
  static char buf[32]; // 静态缓冲区存储命令
  // 读取串口数据（直到换行符）
  uint8_t len = Serial.readBytesUntil('\n', buf, sizeof(buf) - 1);
  buf[len] = '\0'; // 手动添加字符串结束符

  // -----------【新增：去除末尾回车】-----------
  size_t n = strlen(buf);
  while (n > 0 && (buf[n - 1] == '\r' || buf[n - 1] == ' ')) {
    buf[--n] = '\0';
  }

  // -----------命令解析-----------
  if (strcasecmp(buf, "STOP") == 0) {
    stopMotor();             // 调用停止电机函数
    ctrl.running = false;    // 可选：标记系统已停止
    Serial.println("系统已停止");
    return;
  }

  if (strncasecmp(buf, "ANGLE", 5) == 0) {
    float a = atof(buf + 6); // 提取命令中的角度数值
    if (a >= ANGLE_MIN && a <= ANGLE_MAX)
      setTarget(a);
    else
      Serial.println("角度超出范围");
    return;
  }

  if (strcasecmp(buf, "HOLD") == 0) {
    ctrl.hold = true;
    ctrl.running = true; // 切换到保持模式
    Serial.println("保持模式");
    return;
  }

  if (strncasecmp(buf, "PID", 3) == 0) { // 设置PID参数命令（格式：PID kp ki kd）
    float kp, ki, kd;
    if (sscanf(buf + 3, "%f %f %f", &kp, &ki, &kd) == 3) {
      pid.kp = kp;
      pid.ki = ki;
      pid.kd = kd;
      Serial.print("PID设置: ");
      Serial.print(kp);
      Serial.print(" ");
      Serial.print(ki);
      Serial.print(" ");
      Serial.println(kd);
    } else {
      Serial.println("格式: PID kp ki kd");
    }
    return;
  }
  if (strcasecmp(buf, "STATUS") == 0) {
    sendStatus();
    return;
  }
  if (strcasecmp(buf, "STATS") == 0) {
    reportStats();
    return;
  }
  // 如果以上命令都不匹配
  Serial.println("未知命令");
}