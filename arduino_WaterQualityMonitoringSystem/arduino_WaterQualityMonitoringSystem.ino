#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- Cảm biến pH -----------
#define PH_PIN A0
#define PH_OFFSET 0
#define PH_ARRAY_LENGTH 40
int pHArray[PH_ARRAY_LENGTH];
int pHIndex = 0;

// ---------- Cảm biến TDS ----------
#define TDS_PIN A1
#define VREF 5.0
#define TDS_SAMPLES 30
int tdsBuffer[TDS_SAMPLES];
int tdsIndex = 0;

// ---------- Cảm biến Độ đục (Turbidity) ----
#define TUR_PIN A2
#define TUR_SAMPLES 30
int turBuffer[TUR_SAMPLES];
int turIndex = 0;

// ---------- Cảm biến Nhiệt độ DS18B20 ------
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ---------- Cảm biến DO ----------
#define DO_PIN A3
#define DO_VREF 5000    // mV
#define ADC_RES 1024    // Độ phân giải ADC
#define TWO_POINT_CALIBRATION 0
#define CAL1_V 1600     // mV
#define CAL1_T 25       // °C
#define CAL2_V 1300     // mV
#define CAL2_T 15       // °C

const uint16_t DO_Table[41] = {
  14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
  11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
  9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
  7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410
};

int16_t readDO_raw(uint32_t voltage_mv, uint8_t temperature_c) {
#if TWO_POINT_CALIBRATION == 0
  uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#else
  uint16_t V_saturation = (int16_t)((int8_t)temperature_c - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#endif
}

void setup() {
  Serial.begin(9600);
  pinMode(TDS_PIN, INPUT);
  sensors.begin();
}

void loop() {
  // Lấy dữ liệu từ các cảm biến
  float currentTemp = readTemperature();
  float phValue = readPH();
  float tdsValue = readTDS(currentTemp);
  float turbValue = readTurbidity();
  
  // Đọc DO (raw: μg/L) và chuyển sang mg/L
  float doValue_mgL = readDO_raw(
      (uint32_t)DO_VREF * analogRead(DO_PIN) / ADC_RES,
      (uint8_t)currentTemp
  ) / 1000.0;

  // Gửi dữ liệu qua Serial dưới dạng chuỗi CSV
  Serial.print(currentTemp, 2);
  Serial.print(",");
  Serial.print(phValue, 2);
  Serial.print(",");
  Serial.print(tdsValue, 0);
  Serial.print(",");
  Serial.print(turbValue, 2);
  Serial.print(",");
  Serial.println(doValue_mgL, 2); 

  delay(100);
}

float readTemperature() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

float readPH() {
  pHArray[pHIndex++] = analogRead(PH_PIN);
  if (pHIndex == PH_ARRAY_LENGTH) pHIndex = 0;
  float pHVoltage = (averageArray(pHArray, PH_ARRAY_LENGTH) / 1024.0) * 5.0;
  return 3.50 * pHVoltage + PH_OFFSET;
}

float readTDS(float temperature) {
  tdsBuffer[tdsIndex++] = analogRead(TDS_PIN);
  if (tdsIndex >= TDS_SAMPLES) tdsIndex = 0;
  int tdsFiltered = medianFilter(tdsBuffer, TDS_SAMPLES);
  float voltage = (tdsFiltered / 1024.0) * VREF;
  float compensation = 1.0 + 0.02 * (temperature - 25.0);
  float compVoltage = voltage / compensation;
  return (133.42 * compVoltage * compVoltage * compVoltage
         - 255.86 * compVoltage * compVoltage
         + 857.39 * compVoltage) * 0.05;
}

float readTurbidity() {
  turBuffer[turIndex++] = analogRead(TUR_PIN);
  if (turIndex >= TUR_SAMPLES) turIndex = 0;
  float turVoltage = averageArray(turBuffer, TUR_SAMPLES) * 5.0 / 1024.0;
  float U0 = 3.44; 
  float f = turVoltage / U0;
  float turbValue = (1.0 - f) * 200;
  if (turbValue < 0) turbValue = 0;
  return turbValue;
  // return turVoltage;
}

// Hàm tính trung bình, bỏ qua giá trị min và max
float averageArray(int* arr, int size) {
  if (size <= 2) return 0;
  long sum = 0;
  int maxVal = arr[0];
  int minVal = arr[0];
  for (int i = 0; i < size; i++) {
    if (arr[i] > maxVal) maxVal = arr[i];
    if (arr[i] < minVal) minVal = arr[i];
    sum += arr[i];
  }
  sum = sum - maxVal - minVal;
  return (float)sum / (size - 2);
}

// Hàm lọc trung vị
int medianFilter(int *arr, int len) {
  int sorted[len];
  memcpy(sorted, arr, sizeof(int) * len);
  for (int i = 0; i < len - 1; i++) {
    for (int j = 0; j < len - i - 1; j++) {
      if (sorted[j] > sorted[j + 1]) {
        int temp = sorted[j];
        sorted[j] = sorted[j + 1];
        sorted[j + 1] = temp;
      }
    }
  }
  if (len % 2 == 0)
    return (sorted[len / 2 - 1] + sorted[len / 2]) / 2;
  else
    return sorted[len / 2];
}
