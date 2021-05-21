#include <Arduino.h>
#include "Wire.h"
#include <stdlib.h>
#include  <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"


#define POLYNOMIAL_8 0x2F // wielomian 8 bitowy (używany w przemyśle motoryzacyjnym)
const int MPU_ADDR = 0x68; // adres I2C dla MPU-6050


int msg[2];
RF24 radio(2,10);
const uint64_t pipe = 0xE8E8F0F0E1LL;

typedef struct data{
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t temp;
    uint16_t adcValue[5];
};

char str[128];
char nadawane[4];
data sensors;
int sum_x, sum_y, sum_z; 
int loop_counter=0, sum_counter=0;
uint8_t str_len=0;
byte arr[128];
byte sum_crc;
long gyro_x_cal, gyro_y_cal, gyro_z_cal;


// source: https://www.elektroda.pl/rtvforum/topic1768322.html#8510622
/*
byte create_crc(byte *data, uint8_t n){
    byte suma=0;
    for(int i = 0; i<n; i++){
        suma += (data[i] & POLYNOMIAL_8);
    }
    return ~suma;
}
*/
// konwersja tablicy char do tablicy unsigned int 8-bit (byte)
void string2ByteArray(char* input, byte* output)
{
    uint8_t loop = 0;
    uint8_t i = 0;
    
    while(input[loop] != '\0')
    {
        output[i++] = input[loop++];
    }
}

// czytanie wartości z przetwornika ADC


void read_imu_data()
{
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); 
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 7*2, true);
    sensors.acc_x = Wire.read()<<8 | Wire.read();
    sensors.acc_y = Wire.read()<<8 | Wire.read();
    sensors.acc_z = Wire.read()<<8 | Wire.read();
    sensors.temp = Wire.read()<<8 | Wire.read();

    sensors.gyro_x = Wire.read()<<8 | Wire.read();
    sensors.gyro_y = Wire.read()<<8 | Wire.read();
    sensors.gyro_z = Wire.read()<<8 | Wire.read();
}
// konfiguracja I2C oraz UART
void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);


  for (int cal_int = 0; cal_int < 1000 ; cal_int ++){                  
    read_imu_data(); 
    //Add the gyro x offset to the gyro_x_cal variable                                            
    gyro_x_cal += sensors.gyro_x;
    //Add the gyro y offset to the gyro_y_cal variable                                              
    gyro_y_cal += sensors.gyro_y; 
    //Add the gyro z offset to the gyro_z_cal variable                                             
    gyro_z_cal += sensors.gyro_z; 
    //Delay 3us to have 250Hz for-loop                                             
    delay(3);                                                          
  }
  gyro_x_cal /= 1000;                                                 
  gyro_y_cal /= 1000;                                                 
  gyro_z_cal /= 1000;

 radio.begin();
 radio.openWritingPipe(pipe);

}

void loop()
{
 
  if(loop_counter%6==0)
  {
    read_imu_data();
    sensors.gyro_x -= gyro_x_cal;                                                 
    sensors.gyro_y -= gyro_y_cal;                                                 
    sensors.gyro_z -= gyro_z_cal;

    sensors.gyro_x /= 131;                                                 
    sensors.gyro_y /= 131;                                                 
    sensors.gyro_z /= 131;

    sum_x += sensors.gyro_x;
    sum_y += sensors.gyro_y;
    sum_z += sensors.gyro_z;
  }
  
  if(loop_counter%18==0)
  {
    sensors.gyro_x = sum_x;
    sensors.gyro_y = sum_y;
    sensors.gyro_z = sum_z;
    sensors.gyro_x *= 0.1;
    sensors.gyro_y *= 0.4
    
    ;    //powinno byc mniej
    sensors.gyro_z *= 0.064;

    if(sensors.gyro_x>90){
      sensors.gyro_x=90;
    }

    if(sensors.gyro_x<-90){
      sensors.gyro_x=-90;
    }
    
    if(sensors.gyro_y>90){
      sensors.gyro_y=90;
    }

    if(sensors.gyro_y<-90){
      sensors.gyro_y=-90;
    }
    if(sensors.gyro_z>90){
      sensors.gyro_z=90;
    }

    if(sensors.gyro_z<-90){
      sensors.gyro_z=-90;
    }

    // Zapisywanie wszystkich danych do jednego ciagu znaków
    sprintf(str, "%4d%4d%4d%4d%4d%4d%4d%4d", sensors.adcValue[0], sensors.adcValue[1], sensors.adcValue[2], sensors.adcValue[3], sensors.adcValue[4], sensors.gyro_x, sensors.gyro_y, sensors.gyro_z);

    sprintf(nadawane, "%3d",sensors.gyro_y);

    
    // zamiana ciągu znaków na ciąg bajtów
    string2ByteArray(str, arr);
    str_len = strlen(str);
    // liczenie sumy kontrolnej

    // wyświetlanie wartości
    Serial.println(str);
     msg[0] = (sensors.gyro_y+90);
//     Serial.println(nadawane);
  }

  loop_counter++;
  delay(10);
  

  //msg[1]=10;




 radio.write(msg, 2);
 

 
}
