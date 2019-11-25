#include <HX711.h>
#include "HX711.h"
#include <SoftwareSerial.h>






/*
 Arduino pin 2 -> HX711 SCK
 Arduino pin 3 -> HX711 DT
 Arduino pin 5V -> HX711 VCC
 Arduino pin GND -> HX711 GND 
*/


HX711 scale(3, 2);


/*Ağırlık Sensörü Değişkenleri*/
float calibration_factor = 22900;     // Kendi Ağırlık sensörünüze göre ayarlamalısınız
float units;                          // Ölçülen Ağırlık Sensörden gelir.
double enb;                           //En büyük ölçülen ağırlık değeri
int checkcount=0;                     // En büyük ölçülen ağırlıktan az ölçülen ölçümleri sayar
boolean isTrapActive=false;
/*Wifi ESP8266 Modülü Değişkenleri*/
int rxPin=4;
int txPin=5;
String agAdi = "<your_ssid>";                 //Ağımızın adını buraya yazıyoruz.    
String agSifresi = "<your_pass>";
/*HC05 Modülü Değişkenleri*/
int enablePin=8;
/*ThingSpeak Değişkenleri*/
String TsIp="184.106.153.149";
String oku;
String appMode="Alarm";




SoftwareSerial esp(rxPin, txPin);
SoftwareSerial hc05(10,9);


void setup() {
    
  Serial.begin(9600);
  hx711_kur();
  esp8266_kur();
  hc05.begin(19200);
  hc05.listen();
  
  
}

void loop() {
  kalibre_et();
  check();
  delay(100);
  
  if(enb>5 && checkcount>20 && isTrapActive && appMode=="Alarm"){
    esp.listen();
    gonder();
  }
  if(appMode=="Tarti"){
    hc05.print(units);
    delay(1000);
  }
  oku="";
  bt_kontrol();
}

void gonder(){
   
      Serial.println(String(enb)+"  "+String(checkcount));
      again_wifi: 
      esp.println("AT+CIPSTART=\"TCP\",\""+TsIp+"\",80");
      if(esp.find("Error")){                                      //Bağlantı hatası kontrolü yapıyoruz.
          Serial.println("AT+CIPSTART Error");
      }
      String veri = "GET https://api.thingspeak.com/update?api_key=<your_ts_api_key>";
      veri += "&field2=";
      veri += String(enb);                                       //Göndereceğimiz nem değişkeni
      veri += "\r\n\r\n";
      esp.print("AT+CIPSEND=");                                   //ESP'ye göndereceğimiz veri uzunluğunu veriyoruz.
      esp.println(veri.length()+2);
      delay(500);
      if(esp.find(">")){                                          //ESP8266 hazır olduğunda içindeki komutlar çalışıyor.
          esp.print(veri);                                          //Veriyi gönderiyoruz.
          Serial.println(veri);
          Serial.println("Veri gonderildi.");
          delay(100);
          Serial.println("Baglantı Kapatildi.");
          esp.println("AT+CIPCLOSE"); 
          delay(100);
      }
      else{
          Serial.print(".");
          esp.flush();
          goto again_wifi;
      }
      enb=0;
      checkcount=0;
      hc05.listen();
    
}
void esp8266_kur(){
  int timer=0;
    
  again:                                   //ESP8266 ile seri haberleşmeyi başlatıyoruz.
  esp.begin(115200);    
  delay(100);                          
  Serial.println("AT Yollandı");                              //AT komutu ile modül kontrolünü yapıyoruz.
  while(!esp.find("OK")){                                     //Modül hazır olana kadar bekliyoruz.
    if(timer>10){
      timer=0;
      Serial.println("ESP8266 Tekrar Bulunmaya Çalışılıyor");
      goto again;
    }
    esp.println("AT");
    Serial.println("ESP8266 Bulunamadı.");
    timer++;
  }
  timer=0;
  Serial.println("OK Komutu Alındı");
  esp.println("AT+CWMODE=1");                                 //ESP8266 modülünü client olarak ayarlıyoruz.
  while(!esp.find("OK")){                                     //Ayar yapılana kadar bekliyoruz.
    if(timer>10){
      timer=0;
      Serial.println("Client olarak ayarlama tekrar deneniyor.");
      goto again;
    }
    esp.println("AT+CWMODE=1");
    Serial.println("Ayar Yapılıyor....");
    timer++;
  }
  timer=0;
  Serial.println("Client olarak ayarlandı");
  connect_again:
  Serial.println("Aga Baglaniliyor...");
  Serial.println("AT+CWJAP=\""+agAdi+"\",\""+agSifresi+"\"");
  esp.println("AT+CWJAP=\""+agAdi+"\",\""+agSifresi+"\"");    //Ağımıza bağlanıyoruz.
  while(!esp.find("OK")){
    if(timer>20){
      timer=0;
      Serial.println("Aga Baglanma Tekrar Deneniyor");
      goto connect_again;
    }
    Serial.print(".");
    timer++;
    }                                     //Ağa bağlanana kadar bekliyoruz.
    timer=0;
  Serial.println("Aga Baglandi.");
  delay(1000);
}


/*Ağırlık Sensörünün Kurulumu*/
void hx711_kur(){
  Serial.println("HX711 Kalibrasyonu");
  Serial.println("Tüm Ağırlıkları Kaldırın");
  Serial.println("Okumaya Başlandıktan sonra Ağırlığını bildiğiniz Bir Cisim Koyun");
  Serial.println("Kalibrasyonu a veya + İle Arttırabilirisin");
  Serial.println("Kalibrasyonu z veya - İle Azaltabilirsin");
  scale.set_scale();
  scale.tare();  //Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
}

 void kalibre_et(){
  
  scale.set_scale(calibration_factor); //Kalibrasyon faktörünün atanması
  Serial.print("Reading: ");
  units = scale.get_units(), 10;
  if (units < 0)
  {
    units = 0.00;
  }

  Serial.print(units);
  Serial.print(" kg"); 
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.println();

  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == '+' || temp == 'a')
      calibration_factor += 10;
    else if(temp == '-' || temp == 'z')
      calibration_factor -= 10;
  }
  
}


void bt_kontrol(){
  if(hc05.available()){
    while(hc05.available()){
    oku+=char(hc05.read());  
  }
  Serial.println(oku);
  if(oku=="Etkin"){
    isTrapActive=true;
    oku="";
    enb=0;
    checkcount=0;
  }
  else if(oku=="Dur"){
    isTrapActive=false;
    oku="";
    enb=0;
    checkcount=0;
  }
  else if(oku=="Alarm"){
    isTrapActive=false;
    appMode="Alarm";
    oku="";
    enb=0;
    checkcount=0;
    
    
  }
  else if(oku=="Tarti"){
    isTrapActive=false;
    appMode="Tarti";
    oku="";
    enb=0;
    checkcount=0; 
  }
  hc05.flush();
  }
  
}


void check() {
  
  if(enb<units){
    enb=units;
    checkcount=0;
  }
  else if(enb>units){
    checkcount+=1;
  }
}

void hc05_at_mode(){
    if (hc05.available())
    Serial.write(hc05.read());
  if (Serial.available()){
    hc05.write(Serial.read());
  }
}
  